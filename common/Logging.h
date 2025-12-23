#ifndef LOGGING_H_
#define LOGGING_H_

#define COLOR_RESET   "\x1b[0m"
#define COLOR_BLUE    "\x1b[34m"
#define COLOR_GREEN   "\x1b[32m"
#define COLOR_YELLOW  "\x1b[33m"
#define COLOR_RED     "\x1b[31m"

#define LOG_LEVEL_DEBUG   0
#define LOG_LEVEL_INFO    1
#define LOG_LEVEL_WARNING 2
#define LOG_LEVEL_ERROR   3
#define LOG_LEVEL_NONE    4  // nothing gets printed

// Define log level here
#define LOG_LEVEL LOG_LEVEL_INFO

#if LOG_LEVEL <= LOG_LEVEL_DEBUG
#define LOG_DEBUG(fmt, ...) \
    printf(COLOR_BLUE "DEBUG - %s:%d - " fmt COLOR_RESET "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define LOG_DEBUG(fmt, ...) ((void)0)
#endif

#if LOG_LEVEL <= LOG_LEVEL_INFO
#define LOG_INFO(fmt, ...) \
    printf(COLOR_GREEN "INFO - %s:%d - " fmt COLOR_RESET "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define LOG_INFO(fmt, ...) ((void)0)
#endif

#if LOG_LEVEL <= LOG_LEVEL_WARNING
#define LOG_WARNING(fmt, ...) \
    printf(COLOR_YELLOW "WARNING - %s:%d - " fmt COLOR_RESET "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define LOG_WARNING(fmt, ...) ((void)0)
#endif

#if LOG_LEVEL <= LOG_LEVEL_ERROR
#define LOG_ERROR(fmt, ...) \
    printf(COLOR_RED "ERROR - %s:%d - " fmt COLOR_RESET "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define LOG_ERROR(fmt, ...) ((void)0)
#endif

#endif