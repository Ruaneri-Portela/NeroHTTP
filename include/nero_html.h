#ifndef NERO_HTML_H
#define NERO_HTML_H
#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>

// Representa um atributo HTML, como class="container"
typedef struct
{
    char *name;    // nome do atributo (ex: "class")
    char *content; // valor do atributo (ex: "container")
} HTML_attribute;

// Representa uma tag HTML genérica
typedef struct HTML_tag
{
    char *name;                  // nome da tag (ex: "div", "p", "body")
    HTML_attribute **attributes; // lista de atributos (NULL-terminated)
    struct HTML_tag **children;  // lista de tags filhas (NULL-terminated)
    char *text_content;          // conteúdo de texto interno, se houver (opcional)
    bool void_element;
} HTML_tag;

// Representa um documento HTML completo
typedef struct
{
    const char *doctype; // ex: "html"
    HTML_tag *html;      // raiz do documento: <html>...</html>
} HTML_document;

typedef enum
{
    // Flags para adicionar filhos
    HTML_ADD_START = 1 << 1,    // adiciona no início
    HTML_ADD_END = 1 << 2,      // adiciona no fim
    HTML_ADD_BY_INDEX = 1 << 3, // adiciona numa posição específica (index) ou após tag nomeada
    HTML_ADD_BY_TAG = 1 << 4,   // adiciona após um elemento específico (por nome)

    // Flag para substituir filhos
    HTML_REPLACE = 1 << 5, // substitui conteúdo

    // Flags para remover filhos
    HTML_REMOVE_FIRST = 1 << 6,
    HTML_REMOVE_LAST = 1 << 7,
    HTML_REMOVE_INDEX = 1 << 8,
    HTML_REMOVE_NAME = 1 << 9,
    HTML_REMOVE_DIRECT = 1 << 10
} HTML_Flags;

size_t HTML_Document_Fill(const HTML_document *document, char *buffer, size_t max_len);

void HTML_Tag_Fill(const HTML_tag *tag, char *buffer, size_t *written, size_t max_len);

void HTML_Attribute_Fill(const HTML_attribute *attribute, char *buffer, size_t *written, size_t max_len);

// Calcula o tamanho necessário para serializar a tag HTML e seus filhos
size_t HTML_Document_LookupSize(const HTML_document *document);

size_t HTML_Tag_LookupSize(const HTML_tag *tag);

size_t HTML_Attribute_LookupSize(const HTML_attribute *attribute);

// Cria uma nova tag
HTML_tag *HTML_Create_Tag(const char *name, const char *text_content, bool void_tag);

// Adiciona um filho a um elemento pai, com flags de controle
// by_index: posição onde inserir se aplicável
// by_tag: nome da tag após a qual inserir, se aplicável (pode ser NULL)
bool HTML_Add_Child(HTML_tag *parent, HTML_tag *child, HTML_Flags flags, size_t by_index, const char *by_tag);

// Remove filhos do elemento pai conforme flags e critérios
bool HTML_Remove_Child(HTML_tag *parent, HTML_Flags flags, HTML_tag *by_direct, const char *by_tag_name, int by_index);

// Destrói totalmente a tag
void HTML_Destroy_Tag(HTML_tag **tag);

// Cria um novo atributo
HTML_attribute *HTML_Create_Attribute(const char *name, const char *value);

// Adiciona um filho ao vector por realocação
// by_index: posição onde inserir se aplicável
// by_attribute: nome do atributo após a qual inserir, se aplicável (pode ser NULL)
bool HTML_Add_Attribute(HTML_attribute ***attribute_vector, HTML_attribute *attribute, HTML_Flags flags, size_t by_index, const char *by_attribute);

// Remove filhos do elemento pai conforme flags e critérios
bool HTML_Remove_Attribute(HTML_attribute ***attribute_vector, HTML_Flags flags, HTML_attribute *by_direct, const char *by_attribute_name, int by_index);

// Destrói totalmente a tag
void HTML_Destroy_Attribute(HTML_attribute ***attribute_vector);

// Cria e inicializa um documento HTML básico
HTML_document *HTML_Create_Document(void);

// Destrói o documento e libera toda a memória alocada
void HTML_Destroy_Document(HTML_document **document);

#endif