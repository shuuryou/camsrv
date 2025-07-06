<?php

namespace CamSrv;

if (!defined('CAMSRV')) die();

final class Camera
{
    /**
     * @return list<string>
     */
    public static function getList(): array
    {
        $cameras = Config::get('webinterface.cameras');
        return array_values(array_filter(explode(',', $cameras)));
    }

    /**
     * @return array{SelectedCamera: string, Cameras: list<array{ID: string, Title: string}>}
     */
    public static function buildMenu(): array
    {
        $cameras = self::getList();
        $selectedCamera = self::getSelectedCamera($cameras);
        $cameraData = [];

        foreach ($cameras as $camera) {
            $title = Config::get($camera . '.title');
            $cameraData[] = [
                'ID' => fix($camera),
                'Title' => fix($title)
            ];
        }

        return [
            'SelectedCamera' => fix($selectedCamera),
            'Cameras' => $cameraData
        ];
    }

    private static function getSelectedCamera(array $cameras): string
    {
        $camera = filter_input(INPUT_GET, 'camera', FILTER_SANITIZE_SPECIAL_CHARS);

        if ($camera !== null && $camera !== false && in_array($camera, $cameras, true)) {
            return $camera;
        }

        return '';
    }

    public static function validateCamera(string $camera): void
    {
        if (!in_array($camera, self::getList(), true)) {
            http_response_code(404);
            throw new \Exception('Camera does not exist.');
        }
    }
}
