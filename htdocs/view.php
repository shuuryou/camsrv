<?php
require_once 'init.php';

$heatmap_enabled = intval(getSetting('webinterface.enableheatmap')) == 1;
$stream_enabled = intval(getSetting('webinterface.enablestream')) == 1;

$cameras = buildCameraList(); // Not escaped

if (!empty($_GET['camera'])) {
    $camera = $_GET['camera'];
}

if (!in_array($_GET['camera'], $cameras)) {
    http_response_code(404);
    echo 'Camera does not exist.';
    return;
}

$destination = getSetting($camera .'.destination');
$local_url = getSetting($camera .'.localurl');
$extension = pathinfo(getSetting('camsrvd.filenametpl'), PATHINFO_EXTENSION);

$recording = null;
$recording_url = null;

if (!empty($_GET['recording']) && is_numeric($_GET['recording'])) {
    $recording = intval($_GET['recording']);
}

$recordings = createVideoList($destination, $extension, $local_url);

if (!$stream_enabled && $recording === null && count($recordings) > 0) {
    // Automatically "select" the first recording if live streams
    // are disabled and no recording was selected.

    reset($recordings);
    $tmp1 = key($recordings);
    reset($recordings[$tmp1]);
    $tmp2 = key($recordings[$tmp1]);
    $tmp3 = $recordings[$tmp1][$tmp2];

    $recording = $tmp3['ID'];

    unset($tmp1);
    unset($tmp2);
    unset($tmp3);
}

if ($recording !== null) {
    $found = false;

    foreach ($recordings as &$date) {
        foreach ($date as &$time) {
            if ($time['ID'] == $recording) {
                $recording_url = $time['URL'];
                $found = true;
                break 2;
            }
        }
    }

    if (!$found) {
        http_response_code(404);
        echo 'Selected recording not found.';
        return;
    }
}

$pagetitle = getSetting($camera .'.title');

$pagevars = array
(
    'EnableHeatmap' => $heatmap_enabled,
    'EnableStream' => $stream_enabled,
    'Camera' => fix($camera),
    'Recordings' => $recordings,
    'Recording' => $recording,
    'RecordingURL' => $recording_url
);


template('view', $pagetitle, $pagevars);

function createVideoList($Directory, $Extension, $URL)
{
    $Extension = strtolower($Extension);

    $dh = opendir($Directory);

    $files = array();

    for (;;) {
        $current = readdir($dh);

        if ($current === false) {
            break;
        }

        $fileext = pathinfo($current, PATHINFO_EXTENSION);

        if (strtolower($fileext) !== $Extension) {
            continue;
        }

        $file = $Directory .'/'. $current;

        if (!is_file($file)) {
            continue;
        }

        $mtime = filemtime($file);

        if (time() - $mtime < 10) {
            continue; // File is still being written to
        }

        if (!preg_match('/-MOTION-([0-9]+)/', $current, $motion)) {
            $motion = 0;
        } else {
            $motion = $motion[1];
        }

        // The modification date is used as an ID of sorts
        $files[] = array
        (
            'URL' => $URL .'/'. $current,
            'Modified' => $mtime,
            'Motion' => $motion
        );
    }

    closedir($dh);

    uasort($files, 'createVideoListSortFunction');

    /* ---------------------------------------------------- */

    $date_format = getSetting('webinterface.dateformatcombobox');
    $time_format = getSetting('webinterface.timeformatcombobox');

    $ret = array();
    $prev_modified = null;

    foreach ($files as $file) {
        // Array keys are escaped here; values are escaped in a second pass
        $date = fix(strftime($date_format, $file['Modified']));

        if ($prev_modified !== null) {
            $time = fix(
                sprintf(
                    '%s - %s',
                    strftime($time_format, $prev_modified),
                    strftime($time_format, $file['Modified'])
                )
            );
        } else {
            $time = strftime($time_format, $file['Modified']);
        }

        $prev_modified = $file['Modified'];

        $ret[$date][$time] = array
        (
            'ID' => $file['Modified'],
            'URL' => fix($file['URL']),
            'Color' => fix(getHeatmapColor($file['Motion']))
        );
        $last = $mtime;
    }

    $ret = array_reverse($ret);

    foreach ($ret as &$v) {
        $v = array_reverse($v);
    }

    return $ret;
}

function createVideoListSortFunction($a, $b)
{
    if ($a == $b) {
        return 0;
    }
    return ($a < $b) ? -1 : 1;
}
