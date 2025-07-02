#ifdef _WIN32
#include <nero_html.h>
#include <nero_module_file.h>
#include <nero_pages.h>
#include <windows.h>
#include <shlwapi.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

static file default_file_config = {
    .root = ".\\root\\",
    .default_document = default_documents,
    .mine_types = {extensions, mime_types}};

static const char *get_mime_type(const char *filename, const file *config)
{
    const char **exts = config->mine_types[0];
    const char **mimes = config->mine_types[1];
    const char *ext = PathFindExtensionA(filename);
    if (!ext || !*ext)
        return "application/octet-stream";
    for (int i = 0; exts[i] && mimes[i]; i++)
    {
        if (_stricmp(ext, exts[i]) == 0)
            return mimes[i];
    }
    return "application/octet-stream";
}

static char *find_default_document(const char *directory, const char **default_documents)
{
    char test_path[MAX_PATH];
    for (int i = 0; default_documents[i]; i++)
    {
        PathCombineA(test_path, directory, default_documents[i]);
        DWORD attr = GetFileAttributesA(test_path);
        if (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY))
        {
            return strdup(test_path);
        }
    }
    return NULL;
}

static char *list_dir_html(const char *path, const char *virtual_path, size_t *html_size)
{
    char search_path[MAX_PATH];
    snprintf(search_path, sizeof(search_path), "%s\\*", path);

    WIN32_FIND_DATAA fd;
    HANDLE hFind = FindFirstFileA(search_path, &fd);
    if (hFind == INVALID_HANDLE_VALUE)
        return html_error_page("Unable to list directory", html_size);

    HTML_document *doc = HTML_Create_Document();
    HTML_tag *head = HTML_Create_Tag("head", NULL, false);
    HTML_tag *body = HTML_Create_Tag("body", NULL, false);
    HTML_Add_Child(doc->html, head, HTML_ADD_END, 0, NULL);
    HTML_Add_Child(doc->html, body, HTML_ADD_END, 0, NULL);

    HTML_Add_Child(head, HTML_Create_Tag("meta", NULL, true), HTML_ADD_END, 0, NULL);
    HTML_tag *title = HTML_Create_Tag("title", virtual_path, false);
    HTML_Add_Child(head, title, HTML_ADD_END, 0, NULL);
    HTML_Add_Child(body, HTML_Create_Tag("h1", virtual_path, false), HTML_ADD_END, 0, NULL);

    HTML_tag *ul = HTML_Create_Tag("ul", NULL, false);
    HTML_Add_Child(body, ul, HTML_ADD_END, 0, NULL);

    if (strlen(virtual_path) > 1)
    {
        HTML_tag *li = HTML_Create_Tag("li", NULL, false);
        HTML_tag *a = HTML_Create_Tag("a", "..", false);
        HTML_Add_Attribute(&a->attributes, HTML_Create_Attribute("href", "../"), HTML_ADD_END, 0, NULL);
        HTML_Add_Child(li, a, HTML_ADD_END, 0, NULL);
        HTML_Add_Child(ul, li, HTML_ADD_END, 0, NULL);
    }

    do
    {
        if (!strcmp(fd.cFileName, ".") || !strcmp(fd.cFileName, ".."))
            continue;

        HTML_tag *li = HTML_Create_Tag("li", NULL, false);
        HTML_tag *span = HTML_Create_Tag("span", (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? "[DIR] " : "[FILE] ", false);
        HTML_tag *a = HTML_Create_Tag("a", fd.cFileName, false);

        char href[PATH_MAX];
        snprintf(href, sizeof(href), "%s%s%s", virtual_path, fd.cFileName,
                 (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? "/" : "");
        HTML_Add_Attribute(&a->attributes, HTML_Create_Attribute("href", href), HTML_ADD_END, 0, NULL);

        HTML_Add_Child(li, span, HTML_ADD_END, 0, NULL);
        HTML_Add_Child(li, a, HTML_ADD_END, 0, NULL);

        if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            ULONGLONG size = ((ULONGLONG)fd.nFileSizeHigh << 32) | fd.nFileSizeLow;
            char info[64];
            snprintf(info, sizeof(info), " (%llu bytes)", size);
            HTML_Add_Child(li, HTML_Create_Tag("span", info, false), HTML_ADD_END, 0, NULL);
        }

        HTML_Add_Child(ul, li, HTML_ADD_END, 0, NULL);
    } while (FindNextFileA(hFind, &fd));

    FindClose(hFind);

    size_t total_size = HTML_Document_LookupSize(doc);
    char *html = malloc(total_size);
    if (!html)
    {
        HTML_Destroy_Document(&doc);
        return NULL;
    }
    total_size = HTML_Document_Fill(doc, html, total_size);
    HTML_Destroy_Document(&doc);
    if (html_size)
        *html_size = total_size;
    return html;
}

static bool send_file(const char *path, HTTP_Connection *conn, HTTP_Header *header, file *config)
{
    HANDLE hFile = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        return false;

    LARGE_INTEGER size;
    if (!GetFileSizeEx(hFile, &size))
    {
        CloseHandle(hFile);
        return false;
    }

    const char *mime = get_mime_type(path, config);
    HTTP_Header *response = HTTP_Header_CreateServerHeader();
    if (!response)
    {
        CloseHandle(hFile);
        return false;
    }

    const char *range = HTTP_Header_GetValue(header, "range");
    bool partial = false;
    LARGE_INTEGER start = {0}, end = {0};

    if (range)
    {
        char *dup = strdup(range);
        char *first = strchr(dup, '=');
        if (first && ++first)
        {
            char *second = strchr(first, '-');
            if (second)
                *second++ = '\0';
            start.QuadPart = _strtoui64(first, NULL, 10);
            end.QuadPart = (second && *second) ? _strtoui64(second, NULL, 10) : size.QuadPart - 1;
            if (!SetFilePointerEx(hFile, start, NULL, FILE_BEGIN))
            {
                free(dup);
                CloseHandle(hFile);
                HTTP_Header_Destroy(&response);
                return false;
            }
            partial = true;
        }
        free(dup);
    }

    char temp[128];
    DWORD bytes;
    char buffer[FILE_READ_BUFFER_SIZE];

    if (partial)
    {
        LONGLONG len = end.QuadPart - start.QuadPart + 1;
        snprintf(temp, sizeof(temp), "%lld", len);
        HTTP_Header_Push(response, "Content-Length", temp, true);
        snprintf(temp, sizeof(temp), "bytes %lld-%lld/%lld", start.QuadPart, end.QuadPart, size.QuadPart);
        HTTP_Header_Push(response, "Content-Range", temp, true);
        HTTP_Header_Push(response, "Content-Type", mime, true);
        HTTP_Header_SendToClient(conn, response, 206);

        LONGLONG sent = 0;
        while (sent < len && ReadFile(hFile, buffer, (DWORD)min(FILE_READ_BUFFER_SIZE, len - sent), &bytes, NULL) && bytes > 0)
        {
            if (HTTP_Write(conn, buffer, bytes) < 0)
                break;
            sent += bytes;
        }
    }
    else
    {
        snprintf(temp, sizeof(temp), "%lld", size.QuadPart);
        HTTP_Header_Push(response, "Content-Length", temp, true);
        HTTP_Header_Push(response, "Content-Type", mime, true);
        HTTP_Header_SendToClient(conn, response, 200);

        while (ReadFile(hFile, buffer, FILE_READ_BUFFER_SIZE, &bytes, NULL) && bytes > 0)
        {
            if (HTTP_Write(conn, buffer, bytes) < 0)
                break;
        }
    }

    HTTP_Header_Destroy(&response);
    CloseHandle(hFile);
    return true;
}

static HTTP_Module_Response file_action(void *internal, HTTP_Connection *conn, HTTP_Header *header)
{
    if (!internal || !conn || !header)
        return HTTP_MODULE_FAIL;

    file *config = internal;
    char full[MAX_PATH], virtual[MAX_PATH] = "/";
    DWORD len = GetFullPathNameA(config->root, MAX_PATH, full, NULL);
    if (len == 0 || len > MAX_PATH)
        return HTTP_MODULE_FAIL;

    HTTP_Map *map = HTTP_Map_Get(header);
    for (size_t i = 0; i < map->count; i++)
    {
        PathAppendA(full, map->path[i]);
        if (*(map->path[i]))
        {
            strcat(virtual, map->path[i]);
            strcat(virtual, "/");
        }
    }

    DWORD attr = GetFileAttributesA(full);
    if (attr == INVALID_FILE_ATTRIBUTES)
    {
        size_t html_size;
        char *html = html_error_page("File or directory not found", &html_size);
        if (html)
        {
            HTTP_Header *resp = HTTP_Header_CreateServerHeader();
            char sz[32];
            snprintf(sz, sizeof(sz), "%zu", html_size);
            HTTP_Header_Push(resp, "Content-Type", "text/html", true);
            HTTP_Header_Push(resp, "Content-Length", sz, true);
            HTTP_Header_SendToClient(conn, resp, 404);
            HTTP_Write(conn, html, html_size);
            free(html);
        }
        return HTTP_MODULE_OK;
    }

    if (attr & FILE_ATTRIBUTE_DIRECTORY)
    {
        char *index = find_default_document(full, config->default_document);
        if (index)
        {
            send_file(index, conn, header, config);
            free(index);
            return HTTP_MODULE_OK;
        }
        size_t html_len;
        char *html = list_dir_html(full, virtual, &html_len);
        if (html)
        {
            char sz[32];
            snprintf(sz, sizeof(sz), "%zu", html_len);
            HTTP_Header *resp = HTTP_Header_CreateServerHeader();
            HTTP_Header_Push(resp, "Content-Type", "text/html", true);
            HTTP_Header_Push(resp, "Content-Length", sz, true);
            HTTP_Header_SendToClient(conn, resp, 200);
            HTTP_Write(conn, html, html_len);
            free(html);
            return HTTP_MODULE_OK;
        }
        return HTTP_MODULE_IGNORE;
    }

    send_file(full, conn, header, config);
    return HTTP_MODULE_OK;
}

const HTTP_Module module_file = {
    .name = "File",
    .ver = "1.1",
    .internal = &default_file_config,
    .load = NULL,
    .action = file_action,
    .destroy = NULL};
    
#endif