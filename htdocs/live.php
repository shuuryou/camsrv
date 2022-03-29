<?php
require_once 'init.php';

$Enabled = intval(getSetting('webinterface.enablestream'));

if ($Enabled !== 1) {
    http_response_code(403);
    echo 'Live streaming is disabled.';
    return;
}

$cameras = buildCameraList(); // Not escaped

if (!empty($_GET['camera'])) {
    $camera = $_GET['camera'];
}

if (!in_array($_GET['camera'], $cameras)) {
    http_response_code(404);
    echo 'Camera does not exist.';
    return;
}

$stream = getSetting($camera .'.livestream');

if (empty($stream)) {
    $stream = getSetting($camera .'.stream');
}

if (empty($stream)) {
    http_response_code(500);
    echo 'H264 stream for camera is missing.';
    return;
}

$command = getSetting('webinterface.streamcommand');

if (empty($command)) {
    http_response_code(500);
    echo 'Grabber command for web interface is missing.';
    return;
}

$command = str_replace('{STREAM}', escapeshellarg($stream), $command);

// Turn off all output buffering or the command will never output anything
ini_set('zlib.output_compression', 'Off');
while (ob_get_level() > 0 && ob_end_clean());

header('Transfer-Encoding: chunked');
header('Connection: close');
header('Content-Type: video/mp4');

passthru($command);
