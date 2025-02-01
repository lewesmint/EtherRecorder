#ifndef APP_ERROR_H
#define APP_ERROR_H

typedef enum AppError{
    APP_EXIT_SUCCESS = 0,
    APP_EXIT_FAILURE = 1,
    APP_CONFIG_ERROR = 2,
    APP_LOGGER_ERROR = 3
} AppError;

#endif // APP_ERROR_H
