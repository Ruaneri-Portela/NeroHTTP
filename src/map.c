#include <nero_http.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Extrai o mapa da requisição HTTP a partir do cabeçalho (prologue)
// Retorna um HTTP_Map com ação, segmentos do caminho e verbos (query string)
HTTP_Map *HTTP_Map_Get(HTTP_Header *header)
{
    if (!header || !header->prologue)
        return NULL;

    size_t prologue_len = strlen(header->prologue);
    if (prologue_len < 1)
        return NULL;

    // Duplica a linha prologue para manipulação segura
    char *line = malloc(prologue_len + 1);
    if (!line)
    {
        HTTP_PRINT_ERROR(stderr, "malloc failed");
        return NULL;
    }
    strcpy(line, header->prologue);

    HTTP_Map *map = calloc(1, sizeof(HTTP_Map));
    if (!map)
    {
        free(line);
        HTTP_PRINT_ERROR(stderr, "calloc failed");
        return NULL;
    }

    map->internal = line;

    // Extrai ação (método HTTP), que é a primeira palavra
    char *space1 = strchr(line, ' ');
    if (!space1)
    {
        free(line);
        free(map);
        return NULL;
    }
    *space1 = '\0';
    map->method = line;

    // Extrai URI (caminho + query), segunda palavra
    char *space2 = strchr(space1 + 1, ' ');
    if (!space2)
    {
        free(line);
        free(map);
        return NULL;
    }
    *space2 = '\0';

    // Pega o caminho da URI (parte após o primeiro espaço)
    char *uri = space1 + 1;

    // Procura os segmentos do caminho separados por '/'
    char *segment_start = strchr(uri, '/');
    if (!segment_start)
        return map; // sem caminho definido, retorna só a ação

    // Divide os segmentos do caminho e armazena em map->path
    while (segment_start)
    {
        char *next_segment = strchr(segment_start + 1, '/');

        // Aloca ou realoca o array de segmentos
        if (map->path)
        {
            map->count++;
            char **tmp = realloc(map->path, map->count * sizeof(*map->path));
            if (!tmp)
            {
                HTTP_PRINT_ERROR(stderr, "realloc failed");
                free(line);
                free(map->path);
                free(map);
                return NULL;
            }
            map->path = (const char **)tmp;
        }
        else
        {
            map->count = 1;
            map->path = malloc(sizeof(*map->path));
            if (!map->path)
            {
                HTTP_PRINT_ERROR(stderr, "malloc failed");
                free(line);
                free(map);
                return NULL;
            }
        }

        // Termina a string no '/' atual para isolar segmento e salva o segmento (sem '/')
        *segment_start = '\0';
        map->path[map->count - 1] = segment_start + 1;

        // Se não há mais segmentos, verifica verbos (query string)
        if (!next_segment)
        {
            char *query = strchr(map->path[map->count - 1], '?');
            if (query)
            {
                *query = '\0';
                map->verb = query + 1;
            }
            break;
        }

        segment_start = next_segment;
    }

    return map;
}

// Imprime o conteúdo do HTTP_Map para debug
void HTTP_Map_Print(HTTP_Map *map)
{
    if (!map)
        return;

    printf("Action: %s\n", map->method);

    if (map->count > 0)
    {
        printf("Path:");
        for (size_t i = 0; i < map->count; i++)
            printf("/%s", map->path[i]);
        printf("\n");
    }

    if (map->verb)
        printf("Verb: %s\n", map->verb);
}

// Libera memória associada ao HTTP_Map e zera o ponteiro
void HTTP_Map_Destroy(HTTP_Map **map)
{
    if (!map || !*map)
        return;

    free((*map)->path);
    free((*map)->internal);
    free(*map);
    *map = NULL;
}