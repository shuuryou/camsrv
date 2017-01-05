<?php


$SelectedCamera = NULL;
$SelectedRecording = NULL;
$SelectedRecordingFile = NULL;
$Recordings = NULL;

if (!empty($_GET['camera']) && array_key_exists($_GET['camera'], $Cameras))
	$SelectedCamera = $_GET['camera'];

if (!empty($_GET['recording']) && is_numeric($_GET['recording']))
	$SelectedRecording = intval($_GET['recording']);
	
if ($SelectedCamera !== NULL)
{
	$Recordings = DirListing($Cameras[$SelectedCamera]['Files'], $Cameras[$SelectedCamera]['URL']);

	if ($SelectedRecording !== NULL)
	{
		$Found = FALSE;
		
		foreach ($Recordings as $v)
			foreach ($v as $w)
				if ($w['ID'] == $SelectedRecording)
				{
					$Found = TRUE;
					$SelectedRecordingFile = $w['File'];
					break 2;
				}
				
		if (!$Found)
		{
			http_response_code(500);
			echo 'Selected recording not found.';
			return;
		}
	}
}

function DirListing($Directory, $URL)
{
	$dh = opendir($Directory);

	$files = array();

	for (;;)
	{
		$current = readdir($dh);
		
		if ($current === FALSE)
			break;
			
		$file = $Directory .'/'. $current;
		
		if (!is_file($file))
			continue;
		
		$extension = pathinfo($current, PATHINFO_EXTENSION);
		
		if (strtolower($extension) !== 'mp4')
			continue;

		$mtime = filemtime($file);
		
		if (time() - $mtime < 5)
			continue; // File is still being written to
		
		$files[$current] = $mtime;
	}

	closedir($dh);
	uasort($files, 'CacheCompare');
	
	$ret = array();
	$last = NULL;
	foreach ($files as $file => $mtime)
	{
		$Date = strftime(DATESTR, $mtime);
		$Time = strftime(TIMESTR, $mtime);
		
		if ($last !== NULL)
		{
			$LastTime = strftime(TIMESTR, $last);
			$DispTime =  $LastTime .' - '. $Time;
		}
		else
			$DispTime = $Time;
		
		$ret[$Date][$DispTime] = array('ID' => $mtime, 'File' => $URL .'/'. $file);
		
		$last = $mtime;
	}
	
	unset($files);
	
	$ret = array_reverse($ret);
	foreach ($ret as &$v)
		$v = array_reverse($v);
	
	return $ret;
}

function CacheCompare($a, $b)
{
	if ($a == $b) return 0;
	return ($a < $b) ? -1 : 1;
}
?>
<!DOCTYPE html>
<html lang="de">
<head>
	<meta charset="utf-8">
	<?php if ($SelectedCamera !== NULL) { ?>
		<title><?php echo htmlspecialchars(TITLE); ?> - <?php echo htmlspecialchars($SelectedCamera); ?></title>
	<?php } else { ?>
		<title><?php echo htmlspecialchars(TITLE); ?> - Überwachungskameras</title>
	<?php } ?>
	<link rel="stylesheet" href="res/pure.css">
	<link rel="stylesheet" href="res/style.css">
	<link rel="stylesheet" href="res/video-js.css">
	<script src="res/navigation.js"></script>
	<script>var camera = <?php echo json_encode($SelectedCamera); ?>;</script>
</head>
<body>
	<div id="main">
		<div class="header">
			<h1><?php echo htmlspecialchars(TITLE); ?></h1>
			<?php if ($SelectedCamera !== NULL) { ?>
				<h2><?php echo htmlspecialchars($SelectedCamera); ?></h2>
			<?php } else { ?>
				<h2>Überwachungskameras</h2>
			<?php } ?>
		</div>
		<div class="cameramenu pure-menu pure-menu-horizontal">
			<ul class="pure-menu-list">
				<?php foreach ($Cameras as $k => $v) { ?>
					<?php if ($SelectedCamera == $k) { ?>
						<li class="pure-menu-item pure-menu-selected"><?php echo htmlspecialchars($k); ?></li>
					<?php } else { ?>
						<li class="pure-menu-item"><a href="/?camera=<?php echo rawurlencode($k); ?>" class="pure-menu-link"><?php echo htmlspecialchars($k); ?></a></li>
					<?php } ?>
				<?php } ?>
			</ul>
		</div>
		
		<?php if ($SelectedCamera !== NULL) { ?>
			<div class="content">
				<div class="chooser pure-g">
					<div class="pure-u-1-5 left"><button id="previous" onclick="Previous();" class="pure-button pure-button-primary">&lt;&lt;</button></div>
					<div class="pure-u-3-5 center pure-form ">
						<select id="date" onchange="Go(this);">
							<option value="">Live-Bild</option>
							<?php foreach ($Recordings as $Date => $Entries) { ?>
								<optgroup label="<?php echo htmlspecialchars($Date); ?>">
									<?php foreach($Entries as $Time => $Entry) { ?>
										<option <?php if ($Entry['ID'] == $SelectedRecording) { ?>selected<?php } ?> value="<?php echo htmlspecialchars($Entry['ID']); ?>"><?php echo htmlspecialchars($Time); ?></option>
									<?php } ?>
								</optgroup>
							<?php } ?>
						</select>
						<?php if ($SelectedRecording !== NULL && $SelectedRecordingFile !== NULL) { ?>
							<button onclick="Download('<?php echo htmlspecialchars($SelectedRecordingFile); ?>');" class="pure-button">Download</button>
						<?php } else { ?>
							<button disabled class="pure-button">Download</button>
						<?php } ?>
					</div>
					<div class="pure-u-1-5 right"><button id="next" onclick="Next();" class="pure-button pure-button-primary">&gt;&gt;</button></div>
				</div>

				<video id="video" class="video-js vjs-default-skin vjs-big-play-centered vjs-16-9" data-setup='{"controls": true, "responsive": true, "autoplay": true, "preload": "auto", "techOrder": ["html5", "flash"]}'>
					<?php if ($SelectedRecording !== NULL && $SelectedRecordingFile !== NULL) { ?>
						<source src="<?php echo htmlspecialchars($SelectedRecordingFile); ?>" type="video/mp4">
					<?php } else { ?>
						<source src="/live.php?camera=<?php echo htmlspecialchars($SelectedCamera); ?>" type="video/x-flv">
					<?php } ?>
					<p class="vjs-no-js">
						Um die Aufzeichnungen anzusehen, müssen Sie JavaScript aktivieren
						und einen Browser verwenden, der HTML5 oder Adobe Flash Player
						unterstützt. Für die Anzeige des Live-Bildes wird Adobe Flash
						Player benötigt.
					</p>
				</video>
			</div>
		<?php } else { ?>
			<p class="center">Bitte wählen Sie eine Überwachungskameras aus der obigen Liste aus.</p>
		<?php } ?>
	</div>
	<?php if ($SelectedCamera !== NULL) { ?>
		<script src="res/video-js.js"></script>
	<?php } ?>
</body>
</html>