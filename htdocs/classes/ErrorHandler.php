<?php

namespace CamSrv;

if (!defined('CAMSRV')) die();

final class ErrorHandler
{
    public static function handleError(int $severity, string $message, string $file, int $line): ?bool
    {
        if (error_reporting() === 0) {
            return null;
        }

        $severityName = match ($severity) {
            E_ERROR => 'Error',
            E_WARNING => 'Warning',
            E_PARSE => 'Parse Error',
            E_NOTICE => 'Notice',
            E_CORE_ERROR => 'Core Error',
            E_CORE_WARNING => 'Core Warning',
            E_COMPILE_ERROR => 'Compile Error',
            E_COMPILE_WARNING => 'Compile Warning',
            E_USER_ERROR => 'User Error',
            E_USER_WARNING => 'User Warning',
            E_USER_NOTICE => 'User Notice',
            E_RECOVERABLE_ERROR => 'Recoverable Error',
            E_DEPRECATED => 'Deprecated',
            E_USER_DEPRECATED => 'User Deprecated',
            default => 'Unknown Error Type'
        };

        error_log(sprintf(
            'Unhandled %s ("%s") in %s on line %d.',
            $severityName,
            $message,
            $file,
            $line
        ));

        http_response_code(500);
        exit(1);
    }

    public static function handleException(\Throwable $exception): never
    {
        error_log(sprintf(
            'Unhandled %s ("%s") in %s on line %d. Trace: %s',
            $exception::class,
            $exception->getMessage(),
            $exception->getFile(),
            $exception->getLine(),
            $exception->getTraceAsString()
        ));

        http_response_code(500);
        exit(1);
    }
}
