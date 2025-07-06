<?php

declare(strict_types=1);

if (!defined('CAMSRV')) {
	die();
}

// Type hints for template variables
/** @var string $siteName */
/** @var string $pageTitle */
/** @var array{SelectedCamera: string, Cameras: array<array{ID: string, Title: string}>} $menu */
?>
<!DOCTYPE html>
<html lang="de">

<head>
	<meta charset="utf-8">
	<meta name="viewport" content="width=device-width, initial-scale=1.0">
	<title><?= $siteName ?> - <?= $pageTitle ?></title>

	<link rel="stylesheet" href="resource/pico.min.css">
	<link rel="stylesheet" href="resource/style.css">
	<link rel="stylesheet" href="resource/video-js.min.css">
</head>

<body>
	<main class="container">

		<nav>
			<ul>
				<li><strong><?= $siteName ?></strong></li>
			</ul>
			<ul>
				<li>
					<a href="index.php" <?= empty($menu['SelectedCamera']) ? 'aria-current="page"' : '' ?>>
						Overview
					</a>
				</li>
				<?php foreach ($menu['Cameras'] as $camera): ?>
					<li>
						<a href="view.php?camera=<?= $camera['ID'] ?>"
							<?= $camera['ID'] === $menu['SelectedCamera'] ? 'aria-current="page"' : '' ?>>
							<?= $camera['Title'] ?>
						</a>
					</li>
				<?php endforeach; ?>
			</ul>
		</nav>

		<article>
			<header><?= $pageTitle ?></header>