<?php

declare(strict_types=1);

if (!defined('CAMSRV')) {
	die();
}

// Type hints for template variables
/** @var string $Camera */
/** @var array $Recordings */
/** @var ?int $Recording */
/** @var ?string $RecordingURL */
/** @var bool $EnableStream */
/** @var bool $EnableHeatmap */

// Helper function to check if we should show video
$showVideo = $EnableStream || (!empty($Recording) && !empty($RecordingURL));
$isLiveView = $EnableStream && empty($Recording);
?>

<fieldset role="group">
	<button id="previous" aria-label="Previous recording" title="Previous recording">
		<span aria-hidden="true">&lt;&lt;</span>
		<span class="visually-hidden">Previous</span>
	</button>

	<select id="date"
		class="recording-selector"
		aria-label="Select recording">
		<option value="" <?= !$EnableStream ? 'disabled' : '' ?> <?= empty($Recording) ? 'selected' : '' ?>>
			Live View
		</option>

		<?php foreach ($Recordings as $date => $entries): ?>
			<optgroup label="<?= $date ?>">
				<?php foreach ($entries as $time => $entry): ?>
					<option value="<?= $entry['ID'] ?>"
						<?= $EnableHeatmap ? 'style="background-color: ' . $entry['Color'] . '"' : '' ?>
						<?= $entry['ID'] === $Recording ? 'selected' : '' ?>>
						<?= $time ?>
					</option>
				<?php endforeach; ?>
			</optgroup>
		<?php endforeach; ?>
	</select>

	<button id="next" aria-label="Next recording" title="Next recording">
		<span aria-hidden="true">&gt;&gt;</span>
		<span class="visually-hidden">Next</span>
	</button>
</fieldset>

<?php if ($showVideo): ?>
	<div class="video-container">
		<video id="video"
			class="video-js vjs-default-skin vjs-big-play-centered vjs-16-9"
			data-setup='<?= json_encode([
							'controls' => !$isLiveView,
							'responsive' => true,
							'autoplay' => true,
							'preload' => 'auto',
							'techOrder' => ['html5'],
							'liveui' => $isLiveView
						], JSON_THROW_ON_ERROR) ?>'>

			<?php if (!empty($Recording) && !empty($RecordingURL)): ?>
				<source src="<?= $RecordingURL ?>" type="video/mp4">
			<?php elseif ($EnableStream): ?>
				<source src="live.php?camera=<?= $Camera ?>" type="video/mp4">
			<?php endif; ?>

			<p class="vjs-no-js">
				To view the recordings, you must enable JavaScript and use a
				browser that supports HTML5 video. To view the live image,
				you need the very latest version of your browser.
			</p>
		</video>
	</div>

	<!-- Load video.js only when needed -->
	<script src="resource/video.min.js"></script>
	<script>
		// Initialize video player with error handling
		document.addEventListener('DOMContentLoaded', () => {
			const player = videojs('video');

			player.on('error', () => {
				console.error('Video playback error:', player.error());
			});

			// Add keyboard shortcuts
			player.on('keydown', (e) => {
				if (e.which === 37) { // Left arrow
					player.currentTime(player.currentTime() - 10);
				} else if (e.which === 39) { // Right arrow
					player.currentTime(player.currentTime() + 10);
				}
			});
		});
	</script>
<?php else: ?>
	<div class="no-video-message">
		<p>No video available.</p>
	</div>
<?php endif; ?>