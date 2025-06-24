#include <nero_http.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// --- Envio de Cabeçalhos HTTP ---
bool HTTP_Header_SendToClient(HTTP_Connection *conn, HTTP_Header *header, int status_code)
{
    if (!conn || !header)
        return false;

    char buffer[1024];
    snprintf(buffer, sizeof(buffer), "HTTP/1.1 %d OK\r\n", status_code);

    int bytes_written = HTTP_Write(conn, buffer, strlen(buffer));
    if (bytes_written < 0)
        goto fail;

    header->prologue = strdup(buffer);
    char *remove_break = strchr(header->prologue, '\r');
    if (remove_break)
        *remove_break = '\0';

    for (HTTP_Header_Value *value = header->values; value; value = value->next)
    {
        bytes_written = HTTP_Write(conn, value->name, strlen(value->name));
        if (bytes_written < 0)
            goto fail;

        bytes_written = HTTP_Write(conn, ": ", 2);
        if (bytes_written < 0)
            goto fail;

        if (value->value)
        {
            bytes_written = HTTP_Write(conn, value->value, strlen(value->value));
            if (bytes_written < 0)
                goto fail;
        }

        bytes_written = HTTP_Write(conn, "\r\n", 2);
        if (bytes_written < 0)
            goto fail;
    }

    HTTP_Write(conn, "\r\n", 2);
    return true;

fail:
    HTTP_PRINT_ERROR(stderr, "failed to send header");
    return false;
}

// --- Adiciona ou atualiza um cabeçalho ---
bool HTTP_Header_Push(HTTP_Header *header, const char *name, const char *value, bool replace)
{
    if (!header || !name)
        return false;

    HTTP_Header_Value *last = NULL;

    for (HTTP_Header_Value *current = header->values; current; current = current->next)
    {
        if (strcasecmp(current->name, name) == 0)
        {
            free(current->value);
            current->value = value ? strdup(value) : NULL;

            if (replace)
                return true; // Atualiza valor existente
        }
        last = current;
    }

    HTTP_Header_Value *new_value = malloc(sizeof(HTTP_Header_Value));
    if (!new_value)
    {
        HTTP_PRINT_ERROR(stderr, "malloc");
        return false;
    }

    new_value->next = NULL;
    new_value->name = strdup(name);
    new_value->value = value ? strdup(value) : NULL;

    if (!new_value->name || (value && !new_value->value))
    {
        HTTP_PRINT_ERROR(stderr, "strdup");
        free(new_value->name);
        free(new_value->value);
        free(new_value);
        return false;
    }

    if (last)
        last->next = new_value;
    else
        header->values = new_value;

    header->count++;
    return true;
}

// --- Remove cabeçalho ---
bool HTTP_Header_RemoveObject(HTTP_Header *header, const char *name, bool firstFind)
{
    if (!header || !name)
        return false;

    HTTP_Header_Value *current = header->values;
    HTTP_Header_Value *prev = NULL;

    while (current)
    {
        if (strcasecmp(current->name, name) == 0)
        {
            if (prev)
                prev->next = current->next;
            else
                header->values = current->next;

            free(current->name);
            free(current->value);
            free(current);
            header->count--;

            if (firstFind)
                return true;

            current = prev ? prev->next : header->values;
            continue;
        }

        prev = current;
        current = current->next;
    }

    return false;
}

// --- Libera memória dos cabeçalhos ---
void HTTP_Header_Destroy(HTTP_Header **header)
{
    if (!header || !(*header))
        return;

    HTTP_Header_Value *current = (*header)->values;

    while (current)
    {
        HTTP_Header_Value *next = current->next;
        free(current->name);
        free(current->value);
        free(current);
        current = next;
    }

    free((*header)->prologue);
    free(*header);
    *header = NULL;
}

// --- Cria um novo cabeçalho padrão de servidor ---
HTTP_Header *HTTP_Header_CreateServerHeader()
{
    HTTP_Header *header = malloc(sizeof(HTTP_Header));
    if (!header)
    {
        HTTP_PRINT_ERROR(stderr, "malloc");
        return NULL;
    }

    header->values = NULL;
    header->count = 0;

    HTTP_Header_Push(header, "Server", "NeroServer/0.1", false);

    char buffer[30];
    time_t now = time(NULL);
    struct tm *tm_info = gmtime(&now);
    strftime(buffer, sizeof(buffer), "%a, %d %b %Y %H:%M:%S GMT", tm_info);
    HTTP_Header_Push(header, "Date", buffer, false);

    return header;
}

// --- Leitura do cabeçalho HTTP do cliente ---
HTTP_Header *HTTP_Header_GetFromClient(HTTP_Connection *conn)
{
    bool slash_r = false;
    int count = 0;

    char *buffer = malloc(1024);
    size_t length = 1024;
    size_t total_read = 0;

    if (!buffer)
    {
        HTTP_PRINT_ERROR(stderr, "malloc");
        return NULL;
    }

    do
    {
        if (total_read >= length)
        {
            length += 1024;
            char *new_buffer = realloc(buffer, length);
            if (!new_buffer)
            {
                HTTP_PRINT_ERROR(stderr, "realloc");
                free(buffer);
                return NULL;
            }
            buffer = new_buffer;
        }

        char *lastPointer = buffer + total_read;
        int bytes_read = HTTP_Read(conn, lastPointer, 4);
        if (bytes_read <= 0)
        {
            HTTP_PRINT_ERROR(stderr, "failed to read header");
            free(buffer);
            return NULL;
        }

        for (char *p = lastPointer; p < lastPointer + bytes_read; p++)
        {
            if (*p == '\r')
                slash_r = true;
            else if (*p == '\n')
            {
                if (slash_r)
                {
                    slash_r = false;
                    count++;
                }
                else
                {
                    slash_r = false;
                    count = 0;
                }

                if (count == 2)
                {
                    *p = '\0'; // finaliza string do header
                    break;
                }
            }
            else
            {
                slash_r = false;
                count = 0;
            }
        }

        total_read += bytes_read;

    } while (count < 2);

    if (total_read == 0)
    {
        free(buffer);
        return NULL;
    }

    HTTP_Header *header = calloc(1, sizeof(HTTP_Header));
    if (!header)
    {
        HTTP_PRINT_ERROR(stderr, "calloc");
        free(buffer);
        return NULL;
    }

    char *prologue_end = strstr(buffer, "\r\n");
    if (prologue_end)
    {
        size_t prologue_length = prologue_end - buffer;
        header->prologue = malloc(prologue_length + 1);
        if (!header->prologue)
        {
            HTTP_PRINT_ERROR(stderr, "malloc");
            free(buffer);
            free(header);
            return NULL;
        }
        memcpy(header->prologue, buffer, prologue_length);
        header->prologue[prologue_length] = '\0';
    }
    else
    {
        header->prologue = strdup(buffer);
    }

    // Processa os campos de cabeçalho
    char *buffer_header = prologue_end ? prologue_end + 2 : buffer;
    char *next_colon = NULL;
    char *next_end_line = NULL;

    do
    {
        next_colon = strchr(buffer_header, ':');
        next_end_line = strstr(buffer_header, "\r\n");

        if (!next_colon || !next_end_line || next_colon > next_end_line)
            break;

        size_t name_length = next_colon - buffer_header;
        char *name = malloc(name_length + 1);
        if (!name)
        {
            HTTP_PRINT_ERROR(stderr, "malloc");
            HTTP_Header_Destroy(&header);
            free(buffer);
            return NULL;
        }
        memcpy(name, buffer_header, name_length);
        name[name_length] = '\0';

        // Pula ':' e espaço
        char *value_start = next_colon + 1;
        while (*value_start == ' ')
            value_start++;

        size_t value_length = next_end_line - value_start;
        char *value = malloc(value_length + 1);
        if (!value)
        {
            HTTP_PRINT_ERROR(stderr, "malloc");
            free(name);
            HTTP_Header_Destroy(&header);
            free(buffer);
            return NULL;
        }
        memcpy(value, value_start, value_length);
        value[value_length] = '\0';

        HTTP_Header_Push(header, name, value, false);

        free(name);
        free(value);

        buffer_header = next_end_line + 2;

    } while (1);

    free(buffer);
    return header;
}

// --- Busca valor de um cabeçalho pelo nome ---
const char *HTTP_Header_GetValue(HTTP_Header *header, const char *object)
{
    if (!header || !object)
        return NULL;

    for (HTTP_Header_Value *value = header->values; value; value = value->next)
    {
        if (strcasecmp(value->name, object) == 0)
            return value->value;
    }
    return NULL;
}

// --- Imprime cabeçalho para debug ---
void HTTP_Header_Print(HTTP_Header *header)
{
    if (!header)
        return;

    printf("Prologue: %s\n", header->prologue);

    for (HTTP_Header_Value *value = header->values; value; value = value->next)
    {
        printf("%s: %s\n", value->name, value->value ? value->value : "(null)");
    }
}
