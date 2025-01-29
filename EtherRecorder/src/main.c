#include <stdio.h>

#include "logger.h"
#include "config.h"
#include "platform_utils.h"
#include "platform_sockets.h"


int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;
    foo_config();
    foo_platform_utils();
    foo_platform_sockets();
    foo_logger();
    // Initialize the logger
    Logger_Init();
    // Log a message
    Logger_Log("Hello, World!");
    // Uninitialize the loggerb
    Logger_Uninit();
}