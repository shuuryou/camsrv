<?php

namespace CamSrv;

if (!defined('CAMSRV')) die();

final class DateFormatter
{
    private static array $formatCache = [];

    public static function format(string $formatType, int $timestamp): string
    {
        $format = self::getFormat($formatType);

        // Convert strftime format to date format
        $dateFormat = self::convertStrftimeToDate($format);

        return (new \DateTime('@' . $timestamp))
            ->setTimezone(new \DateTimeZone(date_default_timezone_get()))
            ->format($dateFormat);
    }

    private static function getFormat(string $type): string
    {
        if (!isset(self::$formatCache[$type])) {
            self::$formatCache[$type] = Config::get('webinterface.' . $type);
        }

        return self::$formatCache[$type];
    }

    private static function convertStrftimeToDate(string $format): string
    {
        // Basic conversion map - extend as needed
        $map = [
            '%Y' => 'Y',
            '%y' => 'y',
            '%m' => 'm',
            '%d' => 'd',
            '%H' => 'H',
            '%I' => 'h',
            '%M' => 'i',
            '%S' => 's',
            '%p' => 'A',
            '%P' => 'a',
            '%B' => 'F',
            '%b' => 'M',
            '%A' => 'l',
            '%a' => 'D',
            '%e' => 'j',
            '%u' => 'N',
            '%w' => 'w',
            '%j' => 'z',
            '%U' => 'W',
            '%V' => 'W',
            '%G' => 'o',
            '%g' => 'o',
            '%C' => 'y',
            '%x' => 'm/d/Y',
            '%X' => 'H:i:s',
            '%c' => 'D M j H:i:s Y',
            '%r' => 'h:i:s A',
            '%R' => 'H:i',
            '%T' => 'H:i:s',
            '%D' => 'm/d/y',
            '%F' => 'Y-m-d',
            '%n' => "\n",
            '%t' => "\t",
            '%%' => '%'
        ];

        return strtr($format, $map);
    }
}
