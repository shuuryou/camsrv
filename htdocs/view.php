<?php

declare(strict_types=1);

namespace CamSrv;

require_once 'init.php';

try {
    $heatmapEnabled = (int)Config::get('webinterface.enableheatmap') === 1;
    $streamEnabled = (int)Config::get('webinterface.enablestream') === 1;

    // Validate camera parameter
    $camera = filter_input(INPUT_GET, 'camera', FILTER_SANITIZE_SPECIAL_CHARS);

    if ($camera === null || $camera === false || $camera === '') {
        http_response_code(400);
        throw new \InvalidArgumentException('No camera specified.');
    }

    Camera::validateCamera($camera);

    // Get camera configuration
    $destination = Config::get($camera . '.destination');
    $localUrl = Config::get($camera . '.localurl');
    $extension = pathinfo(Config::get('camsrvd.filenametpl'), PATHINFO_EXTENSION);

    // Get recording parameter
    $recordingParam = filter_input(INPUT_GET, 'recording', FILTER_VALIDATE_INT);
    $recording = $recordingParam !== false ? $recordingParam : null;
    $recordingUrl = null;

    // Create video list
    $videoList = new VideoList($destination, $extension, $localUrl);
    $recordings = $videoList->getFormattedList();

    // Auto-select first recording if no stream and no recording selected
    if (!$streamEnabled && $recording === null && !empty($recordings)) {
        $recording = $videoList->getFirstRecordingId();
    }

    // Find selected recording
    if ($recording !== null) {
        $recordingUrl = $videoList->findRecordingUrl($recording);

        if ($recordingUrl === null) {
            http_response_code(404);
            throw new \RuntimeException('Selected recording not found.');
        }
    }

    // Render template
    template('view', Config::get($camera . '.title'), [
        'EnableHeatmap' => $heatmapEnabled,
        'EnableStream' => $streamEnabled,
        'Camera' => fix($camera),
        'Recordings' => $recordings,
        'Recording' => $recording,
        'RecordingURL' => $recordingUrl
    ]);
} catch (\Throwable $e) {
    error_log($e->getMessage());
    http_response_code($e instanceof \InvalidArgumentException ? 400 : 500);
    echo json_encode(['error' => $e->getMessage()]);
    exit(1);
}
