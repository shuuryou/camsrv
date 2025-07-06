<?php

declare(strict_types=1);

if (!defined('CAMSRV')) {
	die();
}

// Type hints for template variables
/** @var bool $HeatmapEnabled */
/** @var array<array{Camera: string, Title: string, Data: array}> $Heatmap */
/** @var array<string> $HeatmapColumns */
?>

<?php if (!$HeatmapEnabled): ?>
	<div class="empty-state">
		<p class="center">Please select a camera.</p>
	</div>
<?php else: ?>
	<div class="heatmap-container">
		<?php foreach ($Heatmap as $entry): ?>
			<section class="camera-heatmap" data-camera="<?= $entry['Camera'] ?>">
				<h3><?= $entry['Title'] ?></h3>

				<?php if (empty($entry['Data'])): ?>
					<p class="no-data">No recordings with a motion index available for this camera.</p>
				<?php else: ?>
					<div class="overflow-auto">
						<table class="striped">
							<thead>
								<tr>
									<th scope="col">Date</th>
									<?php foreach ($HeatmapColumns as $column): ?>
										<th scope="col"><?= $column ?></th>
									<?php endforeach; ?>
								</tr>
							</thead>
							<tbody>
								<?php foreach ($entry['Data'] as $date => $hours): ?>
									<tr>
										<th scope="row"><?= $date ?></th>
										<?php foreach ($hours as $hour): ?>
											<td
												<?php if (!empty($hour['FirstID'])): ?>
												role="button"
												tabindex="0"
												data-camera="<?= $entry['Camera'] ?>"
												data-recording="<?= $hour['FirstID'] ?>"
												aria-label="View recording from <?= $date ?> at this hour with <?= $hour['Motion'] ?> motion events"
												<?php endif; ?>
												title="<?= $hour['Motion'] ?> motion events"
												style="background-color: <?= $hour['Color'] ?>">
												<span class="visually-hidden"><?= $hour['Motion'] ?></span>
											</td>
										<?php endforeach; ?>
									</tr>
								<?php endforeach; ?>
							</tbody>
						</table>
					</div>
				<?php endif; ?>
			</section>
		<?php endforeach; ?>
	</div>

	<script>
		// Modern event handling for heatmap cells
		document.addEventListener('DOMContentLoaded', () => {
			const heatmapCells = document.querySelectorAll('.heatmap-cell.clickable');

			heatmapCells.forEach(cell => {
				// Click handler
				cell.addEventListener('click', (e) => {
					const camera = e.currentTarget.dataset.camera;
					const recording = e.currentTarget.dataset.recording;

					if (camera && recording) {
						cameraViewer.navigateToRecording(camera, recording);
					}
				});

				// Keyboard handler for accessibility
				cell.addEventListener('keydown', (e) => {
					if (e.key === 'Enter' || e.key === ' ') {
						e.preventDefault();
						const camera = e.currentTarget.dataset.camera;
						const recording = e.currentTarget.dataset.recording;

						if (camera && recording) {
							cameraViewer.navigateToRecording(camera, recording);
						}
					}
				});
			});
		});
	</script>
<?php endif; ?>