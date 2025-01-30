#ifndef CONFIG_H
#define CONFIG_H

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

#include "platform_utils.h"

#define MAX_LINE_LENGTH 256
#define MAX_SECTION_LENGTH 50
#define MAX_KEY_LENGTH 50
#define MAX_VALUE_LENGTH 200

typedef struct ConfigEntry {
    char section[MAX_SECTION_LENGTH];
    char key[MAX_KEY_LENGTH];
    char value[MAX_VALUE_LENGTH];
    struct ConfigEntry *next;
} ConfigEntry;

static ConfigEntry *config_entries = NULL;

static void trim_whitespace(char *str) {
    char *end;

    // Trim leading space
    while (isspace((unsigned char)*str)) str++;

    if (*str == 0) return;

    // Trim trailing space
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;

    end[1] = '\0';
}

/**
 * @copydoc load_config
 */
int load_config(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        stream_print(stderr, "Failed to load configuration file: %s\n Default settings will be used\n", filename);
        return 1; // Return 1 to indicate using default settings, we don't fail if the file is not found
    }

    char line[MAX_LINE_LENGTH];
    char current_section[MAX_SECTION_LENGTH] = "";

    while (fgets(line, sizeof(line), file)) {
        trim_whitespace(line);

        if (line[0] == '\0') continue; // Skip empty lines
        if (line[0] == ';' || line[0] == '#') continue; // Skip comments
        if (line[0] == '[') {
            char *end = strchr(line, ']');
            if (end) {
                *end = '\0';
                strncpy(current_section, line + 1, sizeof(current_section) - 1);
                current_section[sizeof(current_section) - 1] = '\0';
            }
        } else {
            char *equals = strchr(line, '=');
            if (equals) {
                *equals = '\0';
                char *key = line;
                char *value = equals + 1;
                trim_whitespace(key);
                trim_whitespace(value);

                ConfigEntry *entry = (ConfigEntry *)malloc(sizeof(ConfigEntry));
                strncpy(entry->section, current_section, sizeof(entry->section) - 1);
                strncpy(entry->key, key, sizeof(entry->key) - 1);
                strncpy(entry->value, value, sizeof(entry->value) - 1);
                entry->section[sizeof(entry->section) - 1] = '\0';
                entry->key[sizeof(entry->key) - 1] = '\0';
                entry->value[sizeof(entry->value) - 1] = '\0';
                entry->next = config_entries;
                config_entries = entry;
            }
        }
    }

    fclose(file);
    return 1; // Return 1 to indicate success
}

/**
 * @copydoc get_config_string
 */
const char* get_config_string(const char *section, const char *key, const char *default_value) {
    for (ConfigEntry *entry = config_entries; entry; entry = entry->next) {
        if (strcmp(entry->section, section) == 0 && strcmp(entry->key, key) == 0) {
            return entry->value;
        }
    }
    return default_value;
}

/**
 * @copydoc get_config_int
 */
int get_config_int(const char *section, const char *key, int default_value) {
    const char *value = get_config_string(section, key, NULL);
    return value ? atoi(value) : default_value;
}

/**
 * @copydoc get_config_bool
 */
int get_config_bool(const char *section, const char *key, int default_value) {
    const char *value = get_config_string(section, key, NULL);
    if (!value) return default_value;
    return (strcasecmp(value, "true") == 0 || strcmp(value, "1") == 0);
}

/**
 * @copydoc get_config_float
 */
double get_config_float(const char *section, const char *key, double default_value) {
    const char *value = get_config_string(section, key, NULL);
    return value ? atof(value) : default_value;
}

/**
 * @copydoc get_config_hex
 */
uint64_t get_config_hex(const char *section, const char *key, uint64_t default_value) {
    const char *value = get_config_string(section, key, NULL);
    if (!value) return default_value;
    return strtoull(value, NULL, 16);
}

/**
 * @copydoc free_config
 */
void free_config() {
    ConfigEntry *entry = config_entries;
    while (entry) {
        ConfigEntry *next = entry->next;
        free(entry);
        entry = next;
    }
    config_entries = NULL;
}

#endif // CONFIG_H