<?php
error_reporting(E_ALL);
set_error_handler('ErrorCallback');
set_exception_handler('ExceptionCallback');
assert_options(ASSERT_ACTIVE, 1);
assert_options(ASSERT_WARNING, 0);
assert_options(ASSERT_BAIL, 0);
assert_options(ASSERT_QUIET_EVAL, 0);
assert_options(ASSERT_CALLBACK, 'AssertCallback');

$___CONFIGURATION = parse_ini_file('/etc/camsrv.ini', true, INI_SCANNER_RAW);

$locale = Setting('camsrv.locale');
setlocale(LC_ALL, $locale);

$locale = Setting('camsrv.localetime');
setlocale(LC_TIME, $locale);

unset($locale);

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

function Setting($setting, $default = NULL)
{
	global $___CONFIGURATION;

	if (empty($setting))
		throw new Exception('Empty setting name not supported.');

	$setting = explode('.', $setting);

	$tmp = &$___CONFIGURATION;
	
	foreach ($setting as $key)
	{
		if (!array_key_exists($key, $tmp))
			throw new Exception(sprintf('Setting "%s" not found.', $key));
		
		$tmp = &$tmp[$key];

		if (is_array($tmp))
			continue;

		if (!empty($tmp))
			return $tmp;

		if ($default === NULL)
			throw new Exception(sprintf('Setting "%s" found but empty.', $setting));
		else
			return $default;
	}
	
	assert(FALSE); // Should never reach here
}