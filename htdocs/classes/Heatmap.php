<?php

namespace CamSrv;

if (!defined('CAMSRV')) die();

final class Heatmap
{
    private array $files = [];
    private array $data = [];

    public function __construct(
        private string $directory,
        private readonly string $extension
    ) {
        $this->directory = rtrim($this->directory, DIRECTORY_SEPARATOR);

        $this->loadFiles();
        $this->processData();
    }

    public function getData(): array
    {
        return $this->data;
    }

    /**
     * @return list<string>
     */
    public static function getColumns(): array
    {
        $columns = [];
        $format = DateFormatter::format('timeformatheatmap', 0);

        for ($hour = 0; $hour < 24; $hour++) {
            $timestamp = $hour * 3600;
            $columns[] = fix(DateFormatter::format('timeformatheatmap', $timestamp));
        }

        return $columns;
    }

    private function loadFiles(): void
    {
        $dirHandle = opendir($this->directory);

        if ($dirHandle === false) {
            throw new \RuntimeException('Failed to open directory: ' . $this->directory);
        }

        try {
            while (($filename = readdir($dirHandle)) !== false) {
                if ($filename == '.' || $filename == '..') {
                    continue;
                }

                $filepath = $this->directory . DIRECTORY_SEPARATOR . $filename;
                $realpath = realpath($filepath);
                if ($realpath === false || strpos($realpath, $this->directory . DIRECTORY_SEPARATOR) !== 0) {
                    continue; // Skip files outside our directory
                }

                $this->processFile($filename);
            }
        } finally {
            closedir($dirHandle);
        }

        // Sort by modification time
        usort($this->files, fn($a, $b) => $a['Modified'] <=> $b['Modified']);
    }

    private function processFile(string $filename): void
    {
        // Check extension
        if (strcasecmp(pathinfo($filename, PATHINFO_EXTENSION), $this->extension) !== 0) {
            return;
        }

        // Must have motion data
        if (!preg_match('/-MOTION-(\d+)/', $filename, $matches)) {
            return;
        }

        $motion = (int)$matches[1];
        $filepath = $this->directory . DIRECTORY_SEPARATOR . $filename;

        // Check if it's a regular file
        if (!is_file($filepath)) {
            return;
        }

        $mtime = filemtime($filepath);

        if ($mtime === false) {
            return;
        }

        // Skip files being written (modified in last 10 seconds)
        if (time() - $mtime < 10) {
            return;
        }

        $this->files[] = [
            'Modified' => $mtime,
            'Motion' => $motion
        ];
    }

    private function processData(): void
    {
        $columns = self::getColumns();

        // Initialize data structure
        foreach ($this->files as $file) {
            $date = (new \DateTimeImmutable('@' . $file['Modified']))->format('Y-m-d');
            $hourTimestamp = ((int)floor($file['Modified'] / 3600)) * 3600;
            $hour = DateFormatter::format('timeformatheatmap', $hourTimestamp);

            // Initialize date if not exists
            if (!isset($this->data[$date])) {
                foreach ($columns as $col) {
                    $this->data[$date][$col] = [
                        'FirstID' => null,
                        'Motion' => 0,
                        'Color' => null
                    ];
                }
            }

            // Accumulate motion data
            $this->data[$date][$hour]['Motion'] += $file['Motion'];

            // Set first ID (earliest recording in this hour)
            if ($this->data[$date][$hour]['FirstID'] === null && $file['Modified'] > 0) {
                $this->data[$date][$hour]['FirstID'] = $file['Modified'];
            }
        }

        // Format final data
        foreach ($this->data as &$dateData) {
            foreach ($dateData as &$hourData) {
                $hourData['Color'] = HeatmapColorCalculator::getColor($hourData['Motion']);
                $hourData['Motion'] = fix(number_format($hourData['Motion'], 0));
                $hourData['FirstID'] = $hourData['FirstID'] !== null
                    ? fix((string)$hourData['FirstID'])
                    : null;
            }
        }
    }
}
