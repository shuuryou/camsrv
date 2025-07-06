<?php

namespace CamSrv;

if (!defined('CAMSRV')) die();

final class HeatmapColorCalculator
{
    private static ?array $colorCache = null;

    public static function getColor(int|string $value, string $failsafe = 'FFFFFF'): string
    {
        if (!is_numeric($value)) {
            return '#' . $failsafe;
        }

        $value = (int)$value;
        $colors = self::getColors();

        if (empty($colors)) {
            return '#' . $failsafe;
        }

        $previousThreshold = 0;
        $previousColor = 'FFFFFF';

        foreach ($colors as $threshold => $color) {
            if ($threshold === $value) {
                return '#' . $color;
            }

            if ($threshold > $value) {
                $p = ($value - $previousThreshold) /
                    ((float)$threshold - (float)$previousThreshold);

                $previousRgb = self::hexToRgb($previousColor);
                $currentRgb = self::hexToRgb($color);

                $r = (int)self::interpolate($previousRgb['r'], $currentRgb['r'], $p);
                $g = (int)self::interpolate($previousRgb['g'], $currentRgb['g'], $p);
                $b = (int)self::interpolate($previousRgb['b'], $currentRgb['b'], $p);

                return sprintf('#%02X%02X%02X', $r, $g, $b);
            }

            $previousThreshold = $threshold;
            $previousColor = $color;
        }

        return '#' . $previousColor;
    }

    /**
     * @return array<int, string>
     */
    private static function getColors(): array
    {
        if (self::$colorCache !== null) {
            return self::$colorCache;
        }

        $setting = Config::get('webinterface.heatmapcolors');
        $colors = explode('|', $setting);

        self::$colorCache = [];

        foreach ($colors as $colorDef) {
            $parts = explode(':', $colorDef);

            if (count($parts) !== 2) {
                throw new \Exception('Invalid heatmap colors. Illegal value "' . $colorDef . '".');
            }

            if (!is_numeric($parts[0])) {
                throw new \Exception('Invalid heatmap colors. Threshold must be numeric.');
            }

            if (!preg_match('/^[A-F0-9]+$/i', $parts[1])) {
                throw new \Exception('Invalid heatmap colors. Color must be hex.');
            }

            self::$colorCache[(int)$parts[0]] = strtoupper($parts[1]);
        }

        arsort(self::$colorCache);

        return self::$colorCache;
    }

    /**
     * @return array{r: int, g: int, b: int}
     */
    private static function hexToRgb(string $color): array
    {
        return match (strlen($color)) {
            6 => [
                'r' => 0xFF & (hexdec($color) >> 0x10),
                'g' => 0xFF & (hexdec($color) >> 0x8),
                'b' => 0xFF & hexdec($color)
            ],
            3 => [
                'r' => hexdec(str_repeat($color[0], 2)),
                'g' => hexdec(str_repeat($color[1], 2)),
                'b' => hexdec(str_repeat($color[2], 2))
            ],
            default => throw new \Exception('Invalid color format: ' . $color)
        };
    }

    private static function interpolate(float $a, float $b, float $p): float
    {
        return $a * (1 - $p) + $b * $p;
    }

    public static function reset(): void
    {
        self::$colorCache = null;
    }
}
