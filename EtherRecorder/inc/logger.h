#ifndef LOGGER_H
#define LOGGER_H


void foo_logger(void);

// Initialize the logger
void Logger_Init(void);

// Log a message
void Logger_Log(char* str);

// Uninitialize the logger
void Logger_Uninit();

#endif // LOGGER_H