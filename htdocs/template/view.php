<?php if (!defined('CAMSRV')) die(); ?>
<div class="chooser pure-g">
	<div class="pure-u-1-5 left"><button id="previous" onclick="Previous('<?php echo $Camera; ?>');" class="pure-button pure-button-primary">&lt;&lt;</button></div>
	<div class="pure-u-3-5 center pure-form ">
		<select id="date" onchange="Go('<?php echo $Camera; ?>', this);">
				<option <?php if (!$EnableStream) { ?>disabled<?php } ?> value="">Live-Bild</option>
			<?php foreach ($Recordings as $Date => &$Entries) { ?>
				<optgroup label="<?php echo $Date; ?>">
					<?php foreach($Entries as $Time => &$Entry) { ?>
						<option <?php if ($EnableHeatmap) { ?>style="background: <?php echo $Entry['Color']; ?>"<?php } ?> <?php if ($Entry['ID'] == $Recording) { ?>selected<?php } ?> value="<?php echo $Entry['ID']; ?>"><?php echo $Time; ?></option>
					<?php } ?>
				</optgroup>
			<?php } ?>
		</select>
		<?php if (!empty($Recording) && !empty($RecordingURL)) { ?>
			<a class="pure-button" download href="<?php echo $RecordingURL; ?>">Download</a>
		<?php } else { ?>
			<button disabled class="pure-button">Download</button>
		<?php } ?>
	</div>
	<div class="pure-u-1-5 right"><button id="next" onclick="Next('<?php echo $Camera; ?>');" class="pure-button pure-button-primary">&gt;&gt;</button></div>
</div>

<?php if ($EnableStream || (!empty($Recording) && !empty($RecordingURL))) { ?>
	<video id="video" class="video-js vjs-default-skin vjs-big-play-centered vjs-16-9"
		data-setup='{"controls": <?php if ($EnableStream && empty($Recording)) { ?>false<?php } else { ?>true<?php } ?>, "responsive": true, "autoplay": true, "preload": "auto", "techOrder": ["html5"]}'>
		<?php if (!empty($Recording) && !empty($RecordingURL)) { ?>
			<source src="<?php echo $RecordingURL; ?>" type="video/mp4">
		<?php } elseif ($EnableStream) { ?>
			<source src="live.php?camera=<?php echo $Camera; ?>" type="video/mp4">
		<?php } ?>
		<p class="vjs-no-js">
			To view the recordings, you must enable JavaScript and use a
			browser that supports HTML5 video. To view the live image,
			you need the very latest version of your browser.
			Microsoft Internet Explorer is not supported.
		</p>
	</video>
	<script src="resource/video-js.js"></script>
<?php } ?>
