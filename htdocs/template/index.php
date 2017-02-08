<?php if (!defined('CAMSRV')) die(); ?>

<?php if (!$HeatmapEnabled) { ?>
	<p class="center">Bitte wählen Sie eine Überwachungskamera aus der obigen Liste aus.</p>
<?php } else { ?>
	<?php foreach ($Heatmap as $Entry) { ?>
		<h3><?php echo $Entry['Title']; ?></h3>
		<table class="heatmap pure-table pure-table-bordered">
			<thead>
				<tr>
					<td></td>
					<?php foreach ($HeatmapColumns as $Column) { ?>
						<td><?php echo $Column; ?></td>
					<?php } ?>
				</tr>
			</thead>
			<tbody>
				<?php foreach ($Entry['Data'] as $Date => $Hours) { ?>
					<tr>
						<td><?php echo $Date; ?></td>
						<?php foreach ($Hours as $Hour) { ?>
							<td <?php if (!empty($Hour['FirstID'])) { ?> class="link" onclick="Go2('<?php echo $Entry['Camera']; ?>', '<?php echo $Hour['FirstID']; ?>');" <?php } ?> title="<?php echo $Hour['Motion']; ?>" style="background: <?php echo $Hour['Color']; ?>"></td>
						<?php } ?>
					</tr>
				<?php } ?>
			</tbody>
		</table>
	<?php } ?>
<?php } ?>