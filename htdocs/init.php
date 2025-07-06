<?php

declare(strict_types=1);

namespace CamSrv;

use DateTime;
use DateTimeImmutable;
use DateTimeZone;
use Exception;
use Throwable;

define('CAMSRV', true);

spl_autoload_register(function ($class) {
    // Convert namespace to file path
    $prefix = 'CamSrv\\';
    if (strncmp($prefix, $class, strlen($prefix)) !== 0) {
        return;
    }

    $relativeClass = substr($class, strlen($prefix));
    $file = __DIR__ . DIRECTORY_SEPARATOR . 'classes' . DIRECTORY_SEPARATOR . str_replace('\\', '/', $relativeClass) . '.php';

    require_once $file;
});

error_reporting(E_ALL);
set_error_handler(ErrorHandler::handleError(...));
set_exception_handler(ErrorHandler::handleException(...));

if (!empty($_SERVER['REQUEST_METHOD']) && strtoupper($_SERVER['REQUEST_METHOD']) === 'HEAD') {
    exit();
}

mb_internal_encoding('UTF-8');
setlocale(LC_ALL, Config::get('webinterface.locale'));
ob_start();

function template(string $page, string $pageTitle, array $variables = []): never
{
    $page = strtolower($page);
    $file = sprintf('template/%s.php', $page);

    if (!file_exists($file)) {
        throw new Exception('Template file "' . $page . '" not found.');
    }

    // Header variables
    $siteName = fix(Config::get('webinterface.title'));
    $pageTitle = fix($pageTitle);
    $menu = Camera::buildMenu();

    include_once 'template/header.php';

    // Page content with isolated scope
    (static function () use ($file, $variables): void {
        extract($variables, EXTR_SKIP);
        include $file;
    })();

    // Footer
    include_once 'template/footer.php';

    ob_end_flush();
    exit();
}

function fix(string $string): string
{
    return htmlspecialchars($string, ENT_QUOTES | ENT_HTML5, 'UTF-8');
}

function unfix(string $string): string
{
    return html_entity_decode($string, ENT_QUOTES | ENT_HTML5, 'UTF-8');
}
