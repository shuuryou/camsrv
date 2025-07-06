<?php
error_reporting(E_ALL);
set_error_handler('errorCallback');
set_exception_handler('exceptionCallback');
assert_options(ASSERT_ACTIVE, 1);
assert_options(ASSERT_WARNING, 0);
assert_options(ASSERT_BAIL, 0);
assert_options(ASSERT_QUIET_EVAL, 0);
assert_options(ASSERT_CALLBACK, 'assertCallback');

if (!empty($_SERVER['REQUEST_METHOD'])) {
    // Happy now, phpcs?
    if (strtoupper($_SERVER['REQUEST_METHOD']) == 'HEAD') {
         exit();
    }
}

mb_internal_encoding('UTF-8');
setlocale(LC_ALL, getSetting('webinterface.locale'));
ob_start();

function assertCallback($File, $Line /*, $expression*/)
{
    printf('Assertion failed in %s on line %d.', $File, $Line);
    exit(1);
}

function errorCallback($Severity, $Message, $File, $Line)
{
    if (error_reporting() == 0) {
        return;
    }
    printf(
        'Unhandled %s ("%s") in %s on line %d.',
        $Severity, $Message, $File, $Line
    );
    exit(1);
}

function exceptionCallback(Throwable $Exception)
{
    printf(
        'Unhandled %s ("%s") in %s on line %d. Trace: %s',
        get_class($Exception),
        $Exception->getMessage(),
        $Exception->getFile(),
        $Exception->getLine(),
        $Exception->getTraceAsString()
    );
    exit(1);
}

function getSetting($setting)
{
    global $___CONFIGURATION_CACHE;

    if (!isset($___CONFIGURATION_CACHE)) {
        $___CONFIGURATION_CACHE = parse_ini_file(
            '/etc/camsrv.ini',
            true, INI_SCANNER_RAW
        );
    }

    if (empty($setting)) {
        throw new Exception('Empty setting name not supported.');
    }

    $setting = explode('.', $setting);

    $tmp = &$___CONFIGURATION_CACHE;

    foreach ($setting as $key) {
        if (!array_key_exists($key, $tmp)) {
            throw new Exception(sprintf('Setting "%s" not found.', $key));
        }

        $tmp = &$tmp[$key];

        if (is_array($tmp)) {
            continue;
        }

        if (!empty($tmp)) {
            return $tmp;
        } else {
            return '';
        }
    }

    assert(false); // Should never reach here
}

function getHeatmapColor($value, $failsafe = 'FFFFFF')
{
    if (!is_numeric($value)) {
        return '#'. $failsafe;
    }

    $value = intval($value);

    $colors = getHeatmapColors();

    if (empty($colors)) {
        return '#'. $failsafe;
    }

    $previous_threshold = 0;
    $previous_color = 'FFFFFF';

    foreach ($colors as $threshold => $color) {
        if ($threshold == $value) {
            return '#'. $color;
        }

        if ($threshold > $value) {
            $p = ($value - $previous_threshold) /
                ((float)$threshold - (float)$previous_threshold);

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

function getHeatmapColors()
{
    global $___COLOR_CACHE;

    if (isset($___COLOR_CACHE)) {
        return $___COLOR_CACHE;
    }

    $setting = getSetting('webinterface.heatmapcolors');
    $setting = explode('|', $setting);

    $___COLOR_CACHE = array();

    foreach ($setting as $v) {
        $v = explode(':', $v);

        $bad = false;

        if (count($v) != 2) {
            $bad = true;
        }

        if (!$bad && !is_numeric($v[0])) {
            $bad = true;
        }

        if (!$bad && !preg_match('/^[A-F0-9]+$/i', $v[1])) {
            $bad = true;
        }

        if ($bad) {
            throw new Exception('Invalid heatmap colors. Illegal value "'. $v .'".');
        }

        $___COLOR_CACHE[$v[0]] = strtoupper($v[1]);
    }

    arsort($___COLOR_CACHE);

    return $___COLOR_CACHE;
}

function hexColorToRGB($color)
{
    $ret = array();

    if (strlen($color) == 6) {
        $colorint = hexdec($color);
        $ret['r'] = 0xFF & ($colorint >> 0x10);
        $ret['g'] = 0xFF & ($colorint >> 0x8);
        $ret['b'] = 0xFF & $colorint;

        return $ret;
    }

    if (strlen($color) == 3) {
        $ret['r'] = hexdec(str_repeat(substr($color, 0, 1), 2));
        $ret['g'] = hexdec(str_repeat(substr($color, 1, 1), 2));
        $ret['b'] = hexdec(str_repeat(substr($color, 2, 1), 2));

        return $ret;
    }

    assert(false);
}

function interpolate($a, $b, $p)
{
    $a = floatval($a);
    $b = floatval($b);
    $p = floatval($p);

    return ($a * (1 - $p) + $b * $p);
}

function unfix($String)
{
    return html_entity_decode($String, ENT_QUOTES, 'UTF-8');
}

function fix($String)
{
    return htmlentities($String, ENT_COMPAT, 'UTF-8');
}

function template($Page, $PageTitle, array $Variables = array())
{
    $Page = strtolower($Page);

    $file = sprintf('template/%s.php', $Page);

    if (!file_exists($file)) {
        throw new Exception('Template file "'. $Page .'" not found.');
    }

    define('CAMSRV', true);

    // These curly braces prevent variable scope pollution so that
    // temporary loop variables are not accessible from other parts.

    {
        $SiteName = fix(getSetting('webinterface.title'));
        $PageTitle = fix($PageTitle);
        $Menu = BuildCameraMenu();

        include_once 'template/header.php';
    }

    {
    foreach ($Variables as $k => $v) {
         $$k = $v;
    }
        include_once $file;
    }

    {
        include_once 'template/footer.php';
    }

    ob_end_flush();
    exit();
}

function buildCameraList()
{
    $cameras = getSetting('webinterface.cameras');
    return explode(',', $cameras);
}

function buildCameraMenu()
{
    $cameras = BuildCameraList(); // Not escaped

    $ret = array();

    $selectedcamera = null;

    if (!empty($_GET['camera']) && in_array($_GET['camera'], $cameras)) {
        $selectedcamera = $_GET['camera'];
    }

    foreach ($cameras as $camera) {
        $title = getSetting($camera .'.title');

        $ret[] = array
        (
            'ID' => fix($camera),
            'Title' => fix($title)
        );
    }

    return array
    (
        'SelectedCamera' => fix($selectedcamera),
        'Cameras' => $ret
    );
}
