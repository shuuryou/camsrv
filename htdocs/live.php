<?php

declare(strict_types=1);

namespace CamSrv;

require_once 'init.php';

try {
    // Check if streaming is enabled
    if ((int)Config::get('webinterface.enablestream') !== 1) {
        http_response_code(403);
        throw new \RuntimeException('Live streaming is disabled.');
    }

    // Validate camera parameter
    $camera = filter_input(INPUT_GET, 'camera', FILTER_SANITIZE_SPECIAL_CHARS);

    if ($camera === null || $camera === false || $camera === '') {
        http_response_code(400);
        throw new \InvalidArgumentException('No camera specified.');
    }

    Camera::validateCamera($camera);

    // Get stream URL
    // Historical note: The 'livestream' setting was introduced for cameras with
    // fixed high/low quality streams. The idea was that the old camsrvd would
    // instruct ffmpeg to grab the the high-quality 'stream' URL while the web
    // interface could use a lower-quality 'livestream' URL to reduce bandwidth
    // and server load. camsrvd is dead, but we still support the distinction
    // for compatibility purposes.
    $stream = Config::get($camera . '.livestream');

    if (empty($stream)) {
        $stream = Config::get($camera . '.stream');
    }

    if (empty($stream)) {
        http_response_code(500);
        throw new \RuntimeException('H264 stream for camera is missing.');
    }

    // Get stream command
    $command = Config::get('webinterface.streamcommand');

    if (empty($command)) {
        http_response_code(500);
        throw new \RuntimeException('Grabber command for web interface is missing.');
    }

    // Build command with proper escaping
    $command = str_replace('{STREAM}', escapeshellarg($stream), $command);

    // Disable all output buffering for streaming
    ini_set('zlib.output_compression', 'Off');

    while (ob_get_level() > 0) {
        ob_end_clean();
    }

    // Set streaming headers
    header('Connection: close');
    header('Content-Type: video/mp4');
    header('Cache-Control: no-cache, no-store, must-revalidate');
    header('Pragma: no-cache');
    header('Expires: 0');

    // Stream the video
    passthru($command, $exitCode);

    if ($exitCode !== 0) {
        error_log("Stream command failed with exit code: $exitCode");
    }
} catch (\Throwable $e) {
    error_log($e->getMessage());

    if (!headers_sent()) {
        echo json_encode(['error' => $e->getMessage()]);
    }

    exit(1);
}
