#include <nero_html.h>
#include <nero_http.h>
#include <limits.h>

static const char *html_doctype = "<!DOCTYPE html>";

size_t HTML_Document_Fill(const HTML_document *document, char *buffer, size_t max_len)
{
    if (!document || !buffer)
        return 0;
    size_t local_written = strlen(document->doctype);
    snprintf(buffer, max_len, "%s", document->doctype);
    HTML_Tag_Fill(document->html, buffer, &local_written, max_len);

    *(buffer + local_written + 1) = '\0';
    return local_written;
}

void HTML_Tag_Fill(const HTML_tag *tag, char *buffer, size_t *written, size_t max_len)
{
    if (!tag || !buffer || !written)
        return;

    *(buffer + *written) = '<';
    *written += 1;

    snprintf(buffer + *written, max_len, "%s", tag->name);
    *written += strlen(tag->name);

    if (tag->attributes)
        for (size_t i = 0; tag->attributes[i] != NULL; i++)
        {
            HTML_Attribute_Fill(tag->attributes[i], buffer, written, max_len);
        };

    *(buffer + *written) = '>';
    *written += 1;

    if (!tag->void_element)
    {
        if (tag->children)
            for (size_t i = 0; tag->children[i] != NULL; i++)
            {
                HTML_Tag_Fill(tag->children[i], buffer, written, max_len);
            }

        if (tag->text_content)
        {
            snprintf(buffer + *written, max_len, "%s", tag->text_content);
            *written += strlen(tag->text_content);
        }

        snprintf(buffer + *written, max_len, "</%s>", tag->name);
        *written += 3 + strlen(tag->name);
    }
}

void HTML_Attribute_Fill(const HTML_attribute *attribute, char *buffer, size_t *written, size_t max_len)
{
    if (!attribute || !buffer || !written)
        return;
    snprintf(buffer + *written, max_len, " %s=\"%s\"", attribute->name, attribute->content);
    *written += 4 + strlen(attribute->name) + strlen(attribute->content);
}

size_t HTML_Document_LookupSize(const HTML_document *document)
{
    if (!document)
        return 0;

    size_t size = strlen(document->doctype);
    return size += HTML_Tag_LookupSize(document->html) + 1;
}

size_t HTML_Tag_LookupSize(const HTML_tag *tag)
{
    if (!tag)
        return 0;
    size_t size = 0;

    // < >
    if (tag->void_element)
    {
        size += 3 + strlen(tag->name);
    }
    else
    {
        //< ></>
        if (tag->text_content)
            size += strlen(tag->text_content);
        size += 6 + (strlen(tag->name) * 2);
    }
    if (tag->children)
        for (size_t i = 0; tag->children[i] != NULL; i++)
        {
            size += HTML_Tag_LookupSize(tag->children[i]);
        }
    if (tag->attributes)
        for (size_t i = 0; tag->attributes[i] != NULL; i++)
        {
            size += HTML_Attribute_LookupSize(tag->attributes[i]);
        }
    return size;
}

size_t HTML_Attribute_LookupSize(const HTML_attribute *attribute)
{
    if (!attribute)
        return 0;
    // " ="
    return strlen(attribute->content) + strlen(attribute->name) + 4;
}

HTML_document *HTML_Create_Document(void)
{
    HTML_document *new = calloc(1, sizeof(HTML_document));
    if (!new)
    {
        HTTP_PRINT_ERROR(stderr, "calloc");
        return NULL;
    }
    new->html = HTML_Create_Tag("html", NULL, false);
    new->doctype = html_doctype;
    return new;
}

HTML_tag *HTML_Create_Tag(const char *name, const char *text_content, bool void_tag)
{
    if (!name)
        return NULL;
    HTML_tag *tag = calloc(1, sizeof(HTML_tag));
    if (!tag)
    {
        HTTP_PRINT_ERROR(stderr, "malloc");
        return NULL;
    }

    tag->void_element = void_tag;

    tag->name = strdup(name);
    tag->text_content = text_content ? strdup(text_content) : NULL;
    if (text_content && !tag->text_content)
    {
        free(tag->name);
        free(tag);
        return NULL;
    }

    if (!tag->name)
    {
        free(tag);
        return NULL;
    }
    return tag;
}

HTML_attribute *HTML_Create_Attribute(const char *name, const char *value)
{
    if (!name || !value)
        return NULL;

    HTML_attribute *attribute = malloc(sizeof(HTML_attribute));
    if (!attribute)
    {
        HTTP_PRINT_ERROR(stderr, "malloc");
        return NULL;
    }

    attribute->name = strdup(name);
    attribute->content = strdup(value);
    if (!attribute->name || !attribute->content)
    {
        free(attribute);
        return NULL;
    }
    return attribute;
}

void HTML_Destroy_Document(HTML_document **document)
{
    if (!document || !(*document))
        return;
    HTML_Destroy_Tag(&(*document)->html);
    free(*document);
    *document = NULL;
}

void HTML_Destroy_Tag(HTML_tag **tag)
{
    if (!tag || !(*tag))
        return;

    free((*tag)->name);
    free((*tag)->text_content);

    if ((*tag)->attributes)
        HTML_Destroy_Attribute(&(*tag)->attributes);

    if ((*tag)->children)
    {
        HTML_tag **vec = (*tag)->children;
        for (size_t i = 0; vec[i] != NULL; i++)
        {
            HTML_Destroy_Tag(&vec[i]);
        }
        free(vec);
    }

    free(*tag);
    *tag = NULL;
}

void HTML_Destroy_Attribute(HTML_attribute ***attribute_vector)
{
    if (!attribute_vector || !(*attribute_vector))
        return;

    HTML_attribute **vec = *attribute_vector;

    for (size_t i = 0; vec[i] != NULL; i++)
    {
        free(vec[i]->name);
        free(vec[i]->content);
        free(vec[i]);
    }

    free(vec);
    *attribute_vector = NULL;
}

bool HTML_Add_Child(HTML_tag *parent, HTML_tag *child, HTML_Flags flags, size_t by_index, const char *by_tag)
{
    if (!parent || !child)
        return false;

    size_t vector_size = 0;
    while (parent->children && parent->children[vector_size])
        vector_size++;

    size_t insert_pos = SIZE_MAX;
    HTML_tag *to_replace = NULL;

    // Verifica flags para encontrar posição de inserção ou substituição
    if (flags & HTML_REPLACE)
    {
        if ((flags & HTML_ADD_BY_INDEX) && by_index < vector_size)
        {
            to_replace = parent->children[by_index];
            insert_pos = by_index;
        }
        else if ((flags & HTML_ADD_BY_TAG) && by_tag)
        {
            for (size_t i = 0; i < vector_size; i++)
            {
                if (strcasecmp(parent->children[i]->name, by_tag) == 0)
                {
                    to_replace = parent->children[i];
                    insert_pos = i;
                    break;
                }
            }
        }
        else if (flags & HTML_ADD_START && vector_size > 0)
        {
            to_replace = parent->children[0];
            insert_pos = 0;
        }
        else if (flags & HTML_ADD_END && vector_size > 0)
        {
            to_replace = parent->children[vector_size - 1];
            insert_pos = vector_size - 1;
        }
    }
    else
    {
        if (flags & HTML_ADD_START)
            insert_pos = 0;
        else if (flags & HTML_ADD_END)
            insert_pos = vector_size;
        else if ((flags & HTML_ADD_BY_INDEX) && by_index <= vector_size)
            insert_pos = by_index;
        else if ((flags & HTML_ADD_BY_TAG) && by_tag)
        {
            for (size_t i = 0; i < vector_size; i++)
            {
                if (strcasecmp(parent->children[i]->name, by_tag) == 0)
                {

                    if (flags & HTML_ADD_START)
                    {
                        insert_pos = i;
                        break;
                    }
                    else if (flags & HTML_ADD_END)
                    {
                        insert_pos = i;
                        continue;
                    }
                    insert_pos = i;
                }
            }
        }
    }

    if (insert_pos == SIZE_MAX)
        return false;

    // REPLACE direto
    if (to_replace)
    {
        parent->children[insert_pos] = child;
        HTML_Destroy_Tag(&to_replace);
        return true;
    }

    // Alocar novo vetor
    HTML_tag **new_vector = calloc(vector_size + 2, sizeof(HTML_tag *));
    if (!new_vector)
    {
        HTTP_PRINT_ERROR(stderr, "calloc");
        return false;
    }

    // Copia antes do ponto de inserção
    if (insert_pos > 0)
        memcpy(new_vector, parent->children, sizeof(HTML_tag *) * insert_pos);

    // Insere novo filho
    new_vector[insert_pos] = child;

    // Copia restante
    if (insert_pos < vector_size)
        memcpy(new_vector + insert_pos + 1, parent->children + insert_pos, sizeof(HTML_tag *) * (vector_size - insert_pos));

    // Finaliza
    new_vector[vector_size + 1] = NULL;
    free(parent->children);
    parent->children = new_vector;

    return true;
}

bool HTML_Add_Attribute(HTML_attribute ***attribute_vector, HTML_attribute *attribute, HTML_Flags flags, size_t by_index, const char *by_attribute)
{
    if (!attribute_vector || !attribute)
        return false;

    size_t vector_size = 0;
    while (*attribute_vector && (*attribute_vector)[vector_size])
        vector_size++;

    size_t insert_pos = SIZE_MAX;
    HTML_attribute *to_replace = NULL;

    // Verifica flags para substituição
    if (flags & HTML_REPLACE)
    {
        if ((flags & HTML_ADD_BY_INDEX) && by_index < vector_size)
        {
            to_replace = (*attribute_vector)[by_index];
            insert_pos = by_index;
        }
        else if ((flags & HTML_ADD_BY_TAG) && by_attribute)
        {
            for (size_t i = 0; i < vector_size; i++)
            {
                if (strcasecmp((*attribute_vector)[i]->name, by_attribute) == 0)
                {
                    to_replace = (*attribute_vector)[i];
                    insert_pos = i;
                    break;
                }
            }
        }
        else if (flags & HTML_ADD_START && vector_size > 0)
        {
            to_replace = (*attribute_vector)[0];
            insert_pos = 0;
        }
        else if (flags & HTML_ADD_END && vector_size > 0)
        {
            to_replace = (*attribute_vector)[vector_size - 1];
            insert_pos = vector_size - 1;
        }
    }
    else
    {
        if (flags & HTML_ADD_START)
            insert_pos = 0;
        else if (flags & HTML_ADD_END)
            insert_pos = vector_size;
        else if ((flags & HTML_ADD_BY_INDEX) && by_index <= vector_size)
            insert_pos = by_index;
        else if ((flags & HTML_ADD_BY_TAG) && by_attribute)
        {
            for (size_t i = 0; i < vector_size; i++)
            {
                if (strcasecmp((*attribute_vector)[i]->name, by_attribute) == 0)
                {
                    if (flags & HTML_ADD_START)
                    {
                        insert_pos = i;
                        break;
                    }
                    else if (flags & HTML_ADD_END)
                    {
                        insert_pos = i + 1;
                        continue;
                    }
                    insert_pos = i;
                }
            }
        }
    }

    if (insert_pos == SIZE_MAX)
        return false;

    // Substituição direta
    if (to_replace)
    {
        (*attribute_vector)[insert_pos] = attribute;
        free(to_replace->content);
        free(to_replace->name);
        free(to_replace);
        return true;
    }

    // Alocar novo vetor
    HTML_attribute **new_vector = calloc(vector_size + 2, sizeof(HTML_attribute *));
    if (!new_vector)
    {
        HTTP_PRINT_ERROR(stderr, "calloc");
        return false;
    }

    // Copia antes do ponto de inserção
    if (insert_pos > 0)
        memcpy(new_vector, *attribute_vector, sizeof(HTML_attribute *) * insert_pos);

    // Insere novo atributo
    new_vector[insert_pos] = attribute;

    // Copia restante
    if (insert_pos < vector_size)
        memcpy(new_vector + insert_pos + 1, *attribute_vector + insert_pos, sizeof(HTML_attribute *) * (vector_size - insert_pos));

    // Finaliza
    new_vector[vector_size + 1] = NULL;
    free(*attribute_vector);
    *attribute_vector = new_vector;

    return true;
}
