<?php

declare(strict_types=1);

namespace CamSrv;

use DateTimeImmutable;

require_once 'init.php';

try {
    $heatmapEnabled = (int)Config::get('webinterface.enableheatmap') === 1;

    $pageVars = [
        'HeatmapEnabled' => $heatmapEnabled
    ];

    if ($heatmapEnabled) {
        $heatmapData = [];

        foreach (Camera::getList() as $camera) {
            $destination = Config::get($camera . '.destination');
            $extension = pathinfo(Config::get('camsrvd.filenametpl'), PATHINFO_EXTENSION);

            $heatmap = new Heatmap($destination, $extension);

            $heatmapData[] = [
                'Camera' => fix($camera),
                'Title' => fix(Config::get($camera . '.title')),
                'Data' => $heatmap->getData()
            ];
        }

        $pageVars['Heatmap'] = $heatmapData;
        $pageVars['HeatmapColumns'] = Heatmap::getColumns();
    }

    template('index', 'Overview', $pageVars);
} catch (\Throwable $e) {
    error_log($e->getMessage());
    http_response_code(500);
    echo json_encode(['error' => $e->getMessage()]);
    exit(1);
}
