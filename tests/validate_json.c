#include <cjson/cJSON.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void)
{
    size_t capacity = 4096;
    size_t length = 0;
    char *text = malloc(capacity);
    if (text == NULL) { return 1; }
    int byte;
    while ((byte = getchar()) != EOF) {
        if (length + 1 >= capacity) {
            if (capacity >= 2 * 1024 * 1024) { free(text); return 1; }
            capacity *= 2;
            char *grown = realloc(text, capacity);
            if (grown == NULL) { free(text); return 1; }
            text = grown;
        }
        text[length++] = (char)byte;
    }
    text[length] = '\0';
    if (ferror(stdin) || length < 2 || text[length - 1] != '\n' ||
        text[length - 2] == '\n' || memchr(text, '\0', length) != NULL) {
        free(text); return 1;
    }
    cJSON *root = cJSON_ParseWithLengthOpts(text, length + 1, NULL, 1);
    free(text);
    if (!cJSON_IsObject(root) || cJSON_GetArraySize(root) != 9) {
        cJSON_Delete(root); return 1;
    }
    const char *strings[] = {"city", "region", "region_code", "country", "country_code", "ip", "isp"};
    int result = 0;
    bool meaningful = false;
    for (size_t index = 0; index < sizeof(strings) / sizeof(strings[0]); ++index) {
        const cJSON *field = cJSON_GetObjectItemCaseSensitive(root, strings[index]);
        if (!cJSON_IsString(field) && !cJSON_IsNull(field)) { result = 1; }
        if (index < 5 && cJSON_IsString(field) && field->valuestring[0] != '\0') {
            meaningful = true;
        }
    }
    const char *coordinates[] = {"lat", "lon"};
    for (size_t index = 0; index < 2; ++index) {
        const cJSON *field = cJSON_GetObjectItemCaseSensitive(root, coordinates[index]);
        double bound = index == 0 ? 90 : 180;
        if (!cJSON_IsNull(field) && (!cJSON_IsNumber(field) ||
            !isfinite(field->valuedouble) || fabs(field->valuedouble) > bound)) { result = 1; }
    }
    cJSON_Delete(root);
    return meaningful ? result : 1;
}
