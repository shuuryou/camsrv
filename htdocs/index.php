<?php
require_once 'init.php';

$heatmap_enabled = intval(getSetting('webinterface.enableheatmap')) == 1;

$pagevars = array
(
    'HeatmapEnabled' => $heatmap_enabled
);

if ($heatmap_enabled) {
    $cameras = buildCameraList(); // Not escaped

    $Heatmap = array();
    foreach ($cameras as $camera) {
        $recordings = getSetting($camera .'.destination');
        $extension = pathinfo(getSetting('camsrvd.filenametpl'), PATHINFO_EXTENSION);

        $entry = array
        (
        'Camera' => fix($camera),
        'Title' => fix(getSetting($camera .'.title')),
        'Data' => createHeatmap($recordings, $extension) // Escapes itself
        );

        $Heatmap[] = $entry;
    }

    $pagevars['Heatmap'] = $Heatmap;
    $pagevars['HeatmapColumns'] = heatmapColumns();
}

template('index', 'Overview', $pagevars);

function createHeatmap($Directory, $Extension)
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

        if (!preg_match('/-MOTION-([0-9]+)/', $current, $motion)) {
            continue;
        }

        $motion = $motion[1];

        $file = $Directory .'/'. $current;

        if (!is_file($file)) {
            continue;
        }

        $mtime = filemtime($file);

        if (time() - $mtime < 10) {
            continue; // File is still being written to
        }

        $files[] = array('Modified' => $mtime, 'Motion' => $motion);
    }

    closedir($dh);

    uasort($files, 'videoArrayForHeatmapSortFunction');

    /* ---------------------------------------------------- */

    $date_format = getSetting('webinterface.dateformatheatmap');
    $time_format = getSetting('webinterface.timeformatheatmap');

    $columns = heatmapColumns(); // Gets escaped by function

    $ret = array();

    foreach ($files as &$file) {
        // Array keys are escaped here; values are escaped in a second pass
        $date = fix(strftime($date_format, $file['Modified']));
        $time = fix(strftime($time_format, round($file['Modified'] / 3600) * 3600));

        if (!isset($ret[$date])) {
            foreach ($columns as $col) {
                $ret[$date][$col] = array
                (
                    'FirstID' => null,
                    'Motion' => 0,
                    'Color' => null
                );
            }
        }

        $ret[$date][$time]['Motion'] += $file['Motion'];

        // The modification date is used as an ID of sorts
        if ($ret[$date][$time]['FirstID'] === null && $file['Modified'] > 0) {
            $ret[$date][$time]['FirstID'] = $file['Modified'];
        }
    }

    // Array keys were escaped above
    foreach ($ret as &$v) {
        foreach ($v as &$w) {
            $w['Color'] = fix(GetHeatmapColor($w['Motion']));
            $w['Motion'] = fix(number_format($w['Motion'], 0));
            $w['FirstID'] = fix($w['FirstID']);
        }
    }

    return $ret;
}

function videoArrayForHeatmapSortFunction(array $a, array $b)
{
    if ($a['Modified'] == $b['Modified']) {
        return 0;
    }
    return ($a['Modified'] < $b['Modified']) ? -1 : 1;
}

function heatmapColumns()
{
    $time_format = getSetting('webinterface.timeformatheatmap');

    $ret = array();

    for ($i = 0; $i < 24; $i++) {
        $ret[] = fix(gmstrftime($time_format, 3600 * $i));
    }

    return $ret;
}
