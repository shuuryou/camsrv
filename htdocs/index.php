<?php
/*
 * Web Interface for Camera Recordings
 *
 */

require_once('init.php');

$heatmap_enabled = intval(GetSetting('webinterface.enableheatmap')) == 1;

$pagevars = array
(
	'HeatmapEnabled' => $heatmap_enabled
);

if ($heatmap_enabled)
{
	$cameras = BuildCameraList(); // Not escaped

	$Heatmap = array();
	foreach ($cameras as $camera)
	{
		$recordings = GetSetting($camera .'.destination');
		$extension = pathinfo(GetSetting('camsrvd.filenametpl'), PATHINFO_EXTENSION);

		$entry = array
		(
			'Camera'	=> Fix($camera),
			'Title'		=> Fix(GetSetting($camera .'.title')),
			'Data'		=> CreateHeatmap($recordings, $extension) // Escapes itself
		);

		$Heatmap[] = $entry;
	}

	$pagevars['Heatmap'] = $Heatmap;
	$pagevars['HeatmapColumns'] = HeatmapColumns();
}

Template('index', 'Übersicht', $pagevars);

function CreateHeatmap($Directory, $Extension)
{
	$Extension = strtolower($Extension);

	$dh = opendir($Directory);

	$files = array();

	for (;;)
	{
		$current = readdir($dh);

		if ($current === FALSE)
			break;

		$fileext = pathinfo($current, PATHINFO_EXTENSION);

		if (strtolower($fileext) !== $Extension)
			continue;

		if (!preg_match('/-MOTION-([0-9]+)/', $current, $motion))
			continue;

		$motion = $motion[1];

		$file = $Directory .'/'. $current;

		if (!is_file($file))
			continue;

		$mtime = filemtime($file);

		if (time() - $mtime < 10)
			continue; // File is still being written to

		$files[] = array('Modified' => $mtime, 'Motion' => $motion);
	}

	closedir($dh);

	uasort($files, 'VideoArrayForHeatmapSortFunction');

	/* ---------------------------------------------------- */

	$date_format = GetSetting('webinterface.dateformatheatmap');
	$time_format = GetSetting('webinterface.timeformatheatmap');

	$columns = HeatmapColumns(); // Gets escaped by function

	$ret = array();

	foreach ($files as &$file)
	{
		// Array keys are escaped here; values are escaped in a second pass
		$date = Fix(strftime($date_format, $file['Modified']));
		$time = Fix(strftime($time_format, round($file['Modified'] / 3600) * 3600));
		
		if (!isset($ret[$date]))
		{
			foreach ($columns as $col)
				$ret[$date][$col] = array
				(
					'FirstID'	=> NULL,
					'Motion'	=> 0,
					'Color'		=> NULL
				);
		}

		$ret[$date][$time]['Motion'] += $file['Motion'];
		
		// The modification date is used as an ID of sorts
		if ($ret[$date][$time]['FirstID'] === NULL && $file['Modified'] > 0)
			$ret[$date][$time]['FirstID'] = $file['Modified'];
	}
	
	// Array keys were escaped above
	foreach ($ret as &$v)
		foreach ($v as &$w)
		{
			$w['Color'] = Fix(GetHeatmapColor($w['Motion']));
			$w['Motion'] = Fix(number_format($w['Motion'], 0));
			$w['FirstID'] = Fix($w['FirstID']);
		}
		
	return $ret;
}

function VideoArrayForHeatmapSortFunction(array $a, array $b)
{
	if ($a['Modified'] == $b['Modified']) return 0;
	return ($a['Modified'] < $b['Modified']) ? -1 : 1;
}

function HeatmapColumns()
{
	$time_format = GetSetting('webinterface.timeformatheatmap');

	$ret = array();

	for ($i = 0; $i < 24; $i++)
		$ret[] = Fix(gmstrftime($time_format, 3600 * $i));
	
	return $ret;
}