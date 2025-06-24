#ifndef NERO_MODULE_FILE_H
#define NERO_MODULE_FILE_H
#include <nero_module.h>

typedef struct
{
    const char *root;
    const char **default_document;
    const char **mine_types[2]; // [0] extensões, [1] tipos MIME correspondentes
} file;

static const char *default_documents[] = {
    "index.html", "index.htm", NULL};

static const char *extensions[] = {
    ".html", ".htm", ".css", ".js", ".png", ".jpg", ".jpeg", ".gif", ".txt", ".json", NULL};

static const char *mime_types[] = {
    "text/html", "text/html", "text/css", "application/javascript",
    "image/png", "image/jpeg", "image/jpeg", "image/gif",
    "text/plain", "application/json", NULL};
    
#define FILE_READ_BUFFER_SIZE 65536
#endif