<?php if (!defined('CAMSRV')) die(); ?>
<!DOCTYPE html>
<html lang="de">
<head>
	<meta charset="utf-8">
	<title><?php echo $SiteName; ?> - <?php echo $PageTitle; ?></title>
	<link rel="stylesheet" href="resource/pure.css">
	<link rel="stylesheet" href="resource/style.css">
	<link rel="stylesheet" href="resource/video-js.css">
	<script src="resource/script.js"></script>
</head>
<body>
	<div id="main">
		<div class="header">
			<h1><?php echo $SiteName; ?></h1>
			<h2><?php echo $PageTitle; ?></h2>
		</div>
		<div class="cameramenu pure-menu pure-menu-horizontal">
			<ul class="pure-menu-list">
				<li class="pure-menu-item <?php if (empty($Menu['SelectedCamera'])) { ?>pure-menu-selected<?php } ?>">
					<a href="index.php" class="pure-menu-link">Übersicht</a>
				</li>
				<?php foreach ($Menu['Cameras'] as $Camera) { ?>
					<li class="pure-menu-item <?php if ($Camera['ID'] === $Menu['SelectedCamera']) { ?>pure-menu-selected<?php } ?>">
						<a href="view.php?camera=<?php echo $Camera['ID'] ?>" class="pure-menu-link">
							<?php echo $Camera['Title']; ?>
						</a>
					</li>
				<?php } ?>
			</ul>
		</div>

		<div class="content">
