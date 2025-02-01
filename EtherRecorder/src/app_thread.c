#include "app_thread.h"

#include <semaphore.h>
#include <string.h>

#include "config.h"
#include "platform_threads.h"

THREAD_LOCAL static const char *thread_label = NULL;

// static sem_t logger_semaphore;

void* app_thread(app_thread_args_t* thread_args) {
    if (thread_args->init_func)
        thread_args->init_func(thread_args);
    thread_args->func(thread_args);
    if (thread_args->exit_func)
        thread_args->exit_func(thread_args);
    
    return NULL;
}

void create_app_thread(app_thread_args_t *thread) {
    if (thread->pre_create_func)
        thread->pre_create_func(thread);
    platform_thread_create(&thread->thread_id, (thread_func_t)app_thread, thread);
    if (thread->post_create_func)
        thread->post_create_func(thread);
}

void set_thread_label(const char *label) {
    thread_label = label;
}

const char* get_thread_label() {
    return thread_label;
}

void start_threads(app_thread_args_t *threads, int num_threads) {
    // // Initialize the logger semaphore
    // sem_init(&logger_semaphore, 0, 0);

    for (int i = 0; i < num_threads; i++) {
        create_app_thread(&threads[i]);
    }
}
        // launch_thread_function, thread.func, 
        
        
//         thread.arg, thread.name, thread.post_init_func);
//     }

//     platform_thread_t thread_handles

//     for (int i = 0; i < num_threads; i++) {
//         if (strcmp(threads[i].name, "log") == 0) {
//             // Start the logger thread and wait for it to signal readiness
//             platform_thread_create(&thread_handles[i], prelaunch_thread_function, threads[i].func, threads[i].name);
//             sem_wait(&logger_semaphore);
//         } else {
//             // Start other threads
//             platform_thread_create(&thread_handles[i], prelaunch_thread_function, threads[i].func, threads[i].name);
//         }
//     }

//     // Wait for all threads to finish
//     for (int i = 0; i < num_threads; i++) {
//         platform_thread_join(thread_handles[i], NULL);
//     }

//     // Destroy the logger semaphore
//     sem_destroy(&logger_semaphore);
// }
