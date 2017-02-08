<?php
/*
 * Web Interface for Camera Recordings
 *
 */

error_reporting(E_ALL);
set_error_handler('ErrorCallback');
set_exception_handler('ExceptionCallback');
assert_options(ASSERT_ACTIVE, 1);
assert_options(ASSERT_WARNING, 0);
assert_options(ASSERT_BAIL, 0);
assert_options(ASSERT_QUIET_EVAL, 0);
assert_options(ASSERT_CALLBACK, 'AssertCallback');

if (!empty($_SERVER['REQUEST_METHOD']) && strtoupper($_SERVER['REQUEST_METHOD']) == 'HEAD')
	exit();

mb_internal_encoding('UTF-8');
setlocale(LC_ALL, GetSetting('webinterface.locale'));
ob_start();

function AssertCallback($File, $Line /*, $expression*/)
{
	printf('Assertion failed in %s on line %d.', $File, $Line);
	exit(1);
}

function ErrorCallback($Severity, $Message, $File, $Line)
{
	if (error_reporting() == 0) return;		
	throw new ErrorException($Message, 0, $Severity, $File, $Line);
}

function ExceptionCallback(Exception $Exception)
{
	printf('Unhandled %s ("%s") in %s on line %d. Trace: %s',
			get_class($Exception),
			$Exception->getMessage(),
			$Exception->getFile(),
			$Exception->getLine(),
			$Exception->getTraceAsString());
	exit(1);
}

function GetSetting($setting)
{
	global $___CONFIGURATION_CACHE;
	
	if (!isset($___CONFIGURATION_CACHE))
		$___CONFIGURATION_CACHE = parse_ini_file('/etc/camsrv.ini', true, INI_SCANNER_RAW);

	if (empty($setting))
		throw new Exception('Empty setting name not supported.');

	$setting = explode('.', $setting);

	$tmp = &$___CONFIGURATION_CACHE;
	
	foreach ($setting as $key)
	{
		if (!array_key_exists($key, $tmp))
			throw new Exception(sprintf('Setting "%s" not found.', $key));
		
		$tmp = &$tmp[$key];

		if (is_array($tmp))
			continue;

		if (!empty($tmp))
			return $tmp;
		else
			return '';
	}
	
	assert(FALSE); // Should never reach here
}

function GetHeatmapColor($value, $failsafe = 'FFFFFF')
{
	if (!is_numeric($value))
		return '#'. $failsafe;

	$value = intval($value);
	
	$colors = GetHeatmapColors();

	if (empty($colors))
		return '#'. $failsafe;

	$previous_threshold = 0;
	$previous_color = 'FFFFFF';

	foreach ($colors as $threshold => $color)
	{
		if ($threshold == $value)
			return '#'. $color;

		if ($threshold > $value)
		{
			$p = ($value - $previous_threshold) / ((float)$threshold - (float)$previous_threshold);

			$previous_rgb = HexColorToRGB($previous_color);
			$current_rgb = HexColorToRGB($color);

			$ret_r = Interpolate($previous_rgb['r'], $current_rgb['r'], $p);
			$ret_g = Interpolate($previous_rgb['g'], $current_rgb['g'], $p);
			$ret_b = Interpolate($previous_rgb['b'], $current_rgb['b'], $p);

			return sprintf('#%02X%02X%02X', $ret_r, $ret_g, $ret_b);
		}

		$previous_threshold = $threshold;
		$previous_color = $color;
	}

	// Return color for highest threshold if the above loop ends and we get here
	return '#'. $previous_color;
}

function GetHeatmapColors()
{
	global $___COLOR_CACHE;

	if (isset($___COLOR_CACHE))
		return $___COLOR_CACHE;

	$setting = GetSetting('webinterface.heatmapcolors');
	$setting = explode('|', $setting);

	$___COLOR_CACHE = array();

	foreach ($setting as $v)
	{
		$v = explode(':', $v);

		$bad = false;

		if (count($v) != 2)
			$bad = true;

		if (!$bad && !is_numeric($v[0]))
			$bad = true;

		if (!$bad && !preg_match('/^[A-F0-9]+$/i', $v[1]))
			$bad = true;

		if ($bad)
			throw new Exception('Invalid heatmap colors. Illegal value "'. $v .'".');

		$___COLOR_CACHE[$v[0]] = strtoupper($v[1]);
	}

	arsort($___COLOR_CACHE);

	return $___COLOR_CACHE;
}

function HexColorToRGB($color)
{
	$ret = array();

	if (strlen($color) == 6)
	{
		$colorint = hexdec($color);
		$ret['r'] = 0xFF & ($colorint >> 0x10);
		$ret['g'] = 0xFF & ($colorint >> 0x8);
		$ret['b'] = 0xFF & $colorint;

		return $ret;
	}

	if (strlen($color) == 3)
	{
		$ret['r'] = hexdec(str_repeat(substr($color, 0, 1), 2));
		$ret['g'] = hexdec(str_repeat(substr($color, 1, 1), 2));
		$ret['b'] = hexdec(str_repeat(substr($color, 2, 1), 2));

		return $ret;
	}

	assert(FALSE);
}

function Interpolate($a, $b, $p)
{
	$a = floatval($a);
	$b = floatval($b);
	$p = floatval($p);

	return ($a * (1 - $p) + $b * $p);
}

function UnFix($String)
{
	return html_entity_decode($String, ENT_QUOTES, 'UTF-8');
}

function Fix($String)
{
	return htmlentities($String, ENT_COMPAT, 'UTF-8');
}

function Template($Page, $PageTitle, array $Variables = array())
{
	$Page = strtolower($Page);
	
	$file = sprintf('template/%s.php', $Page);
	
	if (!file_exists($file))
		throw new Exception('Template file "'. $Page .'" not found.');
	
	define ('CAMSRV', TRUE);

	// These curly braces prevent variable scope pollution so that
	// temporary loop variables are not accessible from other parts.

	{
		$SiteName = Fix(GetSetting('webinterface.title'));
		$PageTitle = Fix($PageTitle);
		$Menu = BuildCameraMenu();
		
		require_once('template/header.php');
	}
	
	{
		foreach ($Variables as $k => $v)
			$$k = $v;
		require_once($file);
	}
	
	{
		require_once('template/footer.php');
	}
	
	ob_end_flush();
	exit();
}

function BuildCameraList()
{
	$cameras = GetSetting('webinterface.cameras');
	return explode(',', $cameras);
}

function BuildCameraMenu()
{
	$cameras = BuildCameraList(); // Not escaped
	
	$ret = array();
	
	$selectedcamera = null;
	
	if (!empty($_GET['camera']) && in_array($_GET['camera'], $cameras))
		$selectedcamera = $_GET['camera'];
		
	foreach ($cameras as $camera)
	{
		$title = GetSetting($camera .'.title');

		$ret[] = array
		(
			'ID'		=> Fix($camera),
			'Title'		=> Fix($title)
		);
	}
	
	return array
	(
		'SelectedCamera'	=> Fix($selectedcamera),
		'Cameras'			=> $ret
	);
}
