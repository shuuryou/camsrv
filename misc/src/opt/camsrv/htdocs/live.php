<?php
require_once('/opt/camsrv/config.php');

$SelectedCamera = NULL;

if (!empty($_GET['camera']) && array_key_exists($_GET['camera'], $Cameras))
	$SelectedCamera = $_GET['camera'];

if ($SelectedCamera === NULL)
{
	http_response_code(404);
	echo 'Camera does not exist.';
	return;
}

header('Transfer-Encoding: chunked');
header('Connection: close');
header('Content-Type: video/x-flv');

$cmd = sprintf('/usr/bin/ffmpeg -v error -threads auto -i %s -c:v copy -an -f flv -',
	escapeshellarg($Cameras[$SelectedCamera]['LiveURL']));

passthru($cmd);
