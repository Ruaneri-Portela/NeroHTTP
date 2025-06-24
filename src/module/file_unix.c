#ifndef _WIN32
#include <nero_module_file.h>
#include <nero_pages.h>
#include <nero_html.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <limits.h>

static file default_file_config = {
    .root = "./root/",
    .default_document = default_documents,
    .mine_types = {extensions, mime_types}};

static const char *get_mime_type(const char *filename, const file *config)
{
    const char **exts = config->mine_types[0];
    const char **mimes = config->mine_types[1];
    const char *ext = strrchr(filename, '.');
    if (!ext || !*ext)
        return "application/octet-stream";
    for (int i = 0; exts[i] && mimes[i]; i++)
    {
        if (strcasecmp(ext, exts[i]) == 0)
            return mimes[i];
    }
    return "application/octet-stream";
}

static char *list_dir_html(const char *path, const char *virtual_path, size_t *html_size)
{
    if (!path || !virtual_path)
        return NULL;

    HTML_document *document = HTML_Create_Document();
    HTML_tag *head = HTML_Create_Tag("head", NULL, false);
    HTML_tag *body = HTML_Create_Tag("body", NULL, false);
    HTML_Add_Child(document->html, head, HTML_ADD_END, 0, NULL);
    HTML_Add_Child(document->html, body, HTML_ADD_END, 0, NULL);

    HTML_tag *meta = HTML_Create_Tag("meta", NULL, true);
    HTML_Add_Attribute(&meta->attributes, HTML_Create_Attribute("charset", "UTF-8"), HTML_ADD_END, 0, NULL);
    HTML_Add_Child(head, meta, HTML_ADD_START, 0, NULL);

    char title_buf[PATH_MAX + 32];
    snprintf(title_buf, sizeof(title_buf), "Index of %s", virtual_path);
    HTML_Add_Child(head, HTML_Create_Tag("title", title_buf, false), HTML_ADD_END, 0, NULL);
    HTML_Add_Child(body, HTML_Create_Tag("h1", title_buf, false), HTML_ADD_END, 0, NULL);

    DIR *dir = opendir(path);
    if (!dir)
    {
        HTML_Destroy_Document(&document);
        return html_error_page("The folder cannot can mapper", html_size);
    }

    HTML_tag *ul = HTML_Create_Tag("ul", NULL, false);
    HTML_Add_Child(body, ul, HTML_ADD_END, 0, NULL);

    if (strlen(virtual_path) > 1)
    {
        HTML_tag *li = HTML_Create_Tag("li", NULL, false);
        HTML_tag *span = HTML_Create_Tag("span", "[DIR] ", false);
        HTML_tag *a = HTML_Create_Tag("a", "..", false);
        HTML_Add_Attribute(&a->attributes, HTML_Create_Attribute("href", "../"), HTML_ADD_END, 0, NULL);
        HTML_Add_Child(li, span, HTML_ADD_END, 0, NULL);
        HTML_Add_Child(li, a, HTML_ADD_END, 0, NULL);
        HTML_Add_Child(ul, li, HTML_ADD_END, 0, NULL);
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL)
    {
        if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
            continue;

        char full_path[PATH_MAX];
        char full_virtual[PATH_MAX];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
        snprintf(full_virtual, sizeof(full_virtual), "%s%s%s",
                 virtual_path, entry->d_name, entry->d_type == DT_DIR ? "/" : "");

        struct stat st;
        if (stat(full_path, &st) != 0)
            continue;

        HTML_tag *li = HTML_Create_Tag("li", NULL, false);
        HTML_tag *span = HTML_Create_Tag("span", S_ISDIR(st.st_mode) ? "[DIR] " : "[FILE] ", false);
        HTML_tag *a = HTML_Create_Tag("a", entry->d_name, false);
        HTML_Add_Attribute(&a->attributes, HTML_Create_Attribute("href", full_virtual), HTML_ADD_END, 0, NULL);

        HTML_Add_Child(li, span, HTML_ADD_END, 0, NULL);
        HTML_Add_Child(li, a, HTML_ADD_END, 0, NULL);

        if (!S_ISDIR(st.st_mode))
        {
            char buffer[64];
            snprintf(buffer, sizeof(buffer), " (%lld bytes)", (long long)st.st_size);
            HTML_Add_Child(li, HTML_Create_Tag("span", buffer, false), HTML_ADD_END, 0, NULL);
        }

        HTML_Add_Child(ul, li, HTML_ADD_END, 0, NULL);
    }

    closedir(dir);
    size_t total_size = HTML_Document_LookupSize(document);
    char *html = malloc(total_size);
    total_size = HTML_Document_Fill(document, html, total_size);
    HTML_Destroy_Document(&document);
    if (html_size)
        *html_size = total_size;
    return html;
}

static bool send_file(const char *path, HTTP_Connection *conn, HTTP_Header *header, file *config)
{
    int fd = open(path, O_RDONLY);
    if (fd < 0)
    {
        HTTP_PRINT_ERROR(stderr, "failed to open file: %s\n", strerror(errno));
        return false;
    }

    struct stat st;
    if (fstat(fd, &st) < 0)
    {
        HTTP_PRINT_ERROR(stderr, "failed to get file size: %s\n", strerror(errno));
        close(fd);
        return false;
    }

    const char *mime_type = get_mime_type(path, config);
    HTTP_Header *response = HTTP_Header_CreateServerHeader();
    if (!response)
    {
        close(fd);
        return false;
    }

    const char *range = HTTP_Header_GetValue(header, "range");
    bool parcial = false;
    off_t start = 0, end = 0;

    if (range)
    {
        char *dup = strdup(range);
        char *first = strchr(dup, '=');
        if (first && ++first)
        {
            char *second = strchr(first, '-');
            if (second)
            {
                *second = '\0';
                second++;
            }
            start = atoll(first);
            end = second && *second ? atoll(second) : 0;

            if (start > 0)
            {
                if (lseek(fd, start, SEEK_SET) < 0)
                {
                    free(dup);
                    HTTP_Header_Destroy(&response);
                    close(fd);
                    return false;
                }
                parcial = true;
            }
        }
        free(dup);
    }

    char buffer[FILE_READ_BUFFER_SIZE];
    ssize_t bytesRead;

    if (parcial)
    {
        if (end == 0 || end >= st.st_size)
            end = st.st_size - 1;

        off_t range_len = end - start + 1;
        if (range_len <= 0)
        {
            HTTP_Header_Destroy(&response);
            close(fd);
            return false;
        }

        char temp[128];
        snprintf(temp, sizeof(temp), "%lld", (long long)range_len);
        HTTP_Header_Push(response, "Content-Length", temp, true);

        snprintf(temp, sizeof(temp), "bytes %lld-%lld/%lld",
                 (long long)start, (long long)end, (long long)st.st_size);
        HTTP_Header_Push(response, "Content-Range", temp, true);
        HTTP_Header_Push(response, "Content-Type", mime_type, true);

        if (!HTTP_Header_SendToClient(conn, response, 206))
        {
            HTTP_Header_Destroy(&response);
            close(fd);
            return false;
        }

        off_t sent = 0;
        while (sent < range_len &&
               (bytesRead = read(fd, buffer, (size_t)MIN(FILE_READ_BUFFER_SIZE, range_len - sent))) > 0)
        {
            int written = HTTP_Write(conn, buffer, bytesRead);
            if (written < 0)
            {
                HTTP_Header_Destroy(&response);
                close(fd);
                return false;
            }
            sent += bytesRead;
        }
    }
    else
    {
        char size_str[64];
        snprintf(size_str, sizeof(size_str), "%lld", (long long)st.st_size);
        HTTP_Header_Push(response, "Content-Length", size_str, true);
        HTTP_Header_Push(response, "Content-Type", mime_type, true);

        if (!HTTP_Header_SendToClient(conn, response, 200))
        {
            HTTP_Header_Destroy(&response);
            close(fd);
            return false;
        }

        while ((bytesRead = read(fd, buffer, FILE_READ_BUFFER_SIZE)) > 0)
        {
            if (HTTP_Write(conn, buffer, bytesRead) < 0)
            {
                HTTP_Header_Destroy(&response);
                close(fd);
                return false;
            }
        }
    }

    HTTP_Header_Destroy(&response);
    close(fd);
    return true;
}

static char *find_default_document(const char *directory, const char **default_documents)
{
    static char path[PATH_MAX];
    for (int i = 0; default_documents[i]; i++)
    {
        snprintf(path, sizeof(path), "%s/%s", directory, default_documents[i]);
        struct stat st;
        if (stat(path, &st) == 0 && S_ISREG(st.st_mode))
            return strdup(path);
    }
    return NULL;
}

static HTTP_Module_Response file_action(void *internal, HTTP_Connection *conn, HTTP_Header *header)
{
    if (!internal || !conn || !header)
        return HTTP_MODULE_FAIL;

    const char *connection = HTTP_Header_GetValue(header, "Connection");
    HTTP_Module_Response res = (connection && strcasecmp(connection, "keep-alive") == 0)
                                   ? HTTP_MODULE_OK_HOLD
                                   : HTTP_MODULE_OK;

    file *config = internal;
    char full[PATH_MAX];
    char virtual[PATH_MAX] = "/";
    realpath(config->root, full);

    HTTP_Map *map = HTTP_Map_Get(header);
    for (size_t i = 0; i < map->count; i++)
    {
        strncat(full, "/", sizeof(full) - strlen(full) - 1);
        strncat(full, map->path[i], sizeof(full) - strlen(full) - 1);

        if (*(map->path[i]))
        {
            strncat(virtual, map->path[i], sizeof(virtual) - strlen(virtual) - 1);
            strncat(virtual, "/", sizeof(virtual) - strlen(virtual) - 1);
        }
    }

    struct stat st;
    if (stat(full, &st) < 0)
    {
        size_t html_len;
        char *html = html_error_page("The requested file or directory was not found.", &html_len);
        if (html)
        {
            HTTP_Header *response = HTTP_Header_CreateServerHeader();
            char len_str[32];
            snprintf(len_str, sizeof(len_str), "%zu", html_len);
            HTTP_Header_Push(response, "Content-Type", "text/html", true);
            HTTP_Header_Push(response, "Content-Length", len_str, true);
            HTTP_Header_SendToClient(conn, response, 404);
            HTTP_Write(conn, html, html_len);
            free(html);
        }
        return res;
    }

    if (S_ISDIR(st.st_mode))
    {
        char *index = find_default_document(full, config->default_document);
        if (index)
        {
            send_file(index, conn, header, config);
            free(index);
            return res;
        }
        size_t html_len;
        char *html = list_dir_html(full, virtual, &html_len);
        if (html)
        {
            HTTP_Header *response = HTTP_Header_CreateServerHeader();
            char len_str[32];
            snprintf(len_str, sizeof(len_str), "%zu", html_len);
            HTTP_Header_Push(response, "Content-Type", "text/html", false);
            HTTP_Header_Push(response, "Content-Length", len_str, false);
            HTTP_Header_SendToClient(conn, response, 200);
            HTTP_Write(conn, html, html_len);
            free(html);
            return res;
        }
        return HTTP_MODULE_IGNORE;
    }
    else
    {
        send_file(full, conn, header, config);
        return res;
    }

    return HTTP_MODULE_IGNORE;
}

const HTTP_Module module_file = {
    .name = "File",
    .ver = "1.0",
    .internal = &default_file_config,
    .load = NULL,
    .action = file_action,
    .destroy = NULL};
#endif
