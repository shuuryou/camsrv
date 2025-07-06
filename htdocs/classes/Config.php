<?php

namespace CamSrv;

if (!defined('CAMSRV')) die();

final class Config
{
    private static ?array $cache = null;
    private static string $configFile = '/etc/camsrv.ini';

    public static function get(string $setting): string
    {
        if (self::$cache === null) {
            self::loadConfig();
        }

        if (empty($setting)) {
            throw new \Exception('Empty setting name not supported.');
        }

        $keys = explode('.', $setting);
        $current = self::$cache;

        foreach ($keys as $key) {
            if (!is_array($current) || !array_key_exists($key, $current)) {
                throw new \Exception(sprintf('Setting "%s" not found.', $setting));
            }

            $current = $current[$key];
        }

        if (is_array($current)) {
            throw new \Exception(sprintf('Setting "%s" is an array.', $setting));
        }

        return (string)$current;
    }

    public static function setConfigFile(string $file): void
    {
        self::$configFile = $file;
        self::$cache = null;
    }

    public static function reset(): void
    {
        self::$cache = null;
    }

    private static function loadConfig(): void
    {
        $config = parse_ini_file(self::$configFile, true, INI_SCANNER_RAW);

        if ($config === false) {
            throw new \Exception('Failed to parse configuration file: ' . self::$configFile);
        }

        self::$cache = $config;
    }
}
