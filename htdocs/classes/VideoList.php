<?php

namespace CamSrv;

use Directory;

if (!defined('CAMSRV')) die();

final class VideoList
{
    private array $files = [];
    private array $formattedList = [];

    public function __construct(
        private string $directory,
        private readonly string $extension,
        private readonly string $baseUrl
    ) {
        $this->directory = rtrim($this->directory, DIRECTORY_SEPARATOR);

        $this->loadFiles();
        $this->formatList();
    }

    public function getFormattedList(): array
    {
        return $this->formattedList;
    }

    public function getFirstRecordingId(): ?int
    {
        if (empty($this->formattedList)) {
            return null;
        }

        $firstDate = array_key_first($this->formattedList);
        $firstTime = array_key_first($this->formattedList[$firstDate]);

        return $this->formattedList[$firstDate][$firstTime]['ID'];
    }

    public function findRecordingUrl(int $recordingId): ?string
    {
        foreach ($this->formattedList as $dateGroup) {
            foreach ($dateGroup as $recording) {
                if ($recording['ID'] === $recordingId) {
                    return $recording['URL'];
                }
            }
        }

        return null;
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

        // Extract motion value
        $motion = 0;
        if (preg_match('/-MOTION-(\d+)/', $filename, $matches)) {
            $motion = (int)$matches[1];
        }

        $this->files[] = [
            'URL' => $this->baseUrl . DIRECTORY_SEPARATOR . $filename,
            'Modified' => $mtime,
            'Motion' => $motion
        ];
    }

    private function formatList(): void
    {
        $dateFormat = DateFormatter::format('dateformatcombobox', 0);
        $timeFormat = DateFormatter::format('timeformatcombobox', 0);

        $previousModified = null;

        foreach ($this->files as $file) {
            $date = fix(DateFormatter::format('dateformatcombobox', $file['Modified']));

            if ($previousModified !== null) {
                $time = fix(sprintf(
                    '%s - %s',
                    DateFormatter::format('timeformatcombobox', $previousModified),
                    DateFormatter::format('timeformatcombobox', $file['Modified'])
                ));
            } else {
                $time = fix(DateFormatter::format('timeformatcombobox', $file['Modified']));
            }

            $previousModified = $file['Modified'];

            $this->formattedList[$date][$time] = [
                'ID' => $file['Modified'],
                'URL' => fix($file['URL']),
                'Color' => HeatmapColorCalculator::getColor($file['Motion'])
            ];
        }

        // Reverse order (newest first)
        $this->formattedList = array_reverse($this->formattedList, true);

        foreach ($this->formattedList as &$dateGroup) {
            $dateGroup = array_reverse($dateGroup, true);
        }
    }
}
