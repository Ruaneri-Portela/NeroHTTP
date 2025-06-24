#include <nero_module.h>
#include <nero_html.h>

static HTTP_Module_Response hello_world_action(void *internal, HTTP_Connection *conn, HTTP_Header *header)
{
    (void)internal;
    (void)header;

    const char *connection = HTTP_Header_GetValue(header, "Connection");
    bool hold = connection ? (strcasecmp(connection, "keep-alive") == 0) : false;

    HTML_document *doc = HTML_Create_Document();
    HTML_tag *head = HTML_Create_Tag("head", NULL, false);
    HTML_tag *body = HTML_Create_Tag("body", NULL, false);

    HTML_Add_Child(doc->html, head, HTML_ADD_END, 0, NULL);
    HTML_Add_Child(doc->html, body, HTML_ADD_END, 0, NULL);

    HTML_tag *meta = HTML_Create_Tag("meta", NULL, true);
    HTML_Add_Child(body, meta, HTML_ADD_END, 0, NULL);

    HTML_Add_Attribute(&meta->attributes, HTML_Create_Attribute("charset", "UTF-8"), HTML_ADD_END, 0, NULL);

    HTML_tag *h1 = HTML_Create_Tag("h1", "Hello World!", false);
    HTML_Add_Child(body, h1, HTML_ADD_END, 0, NULL);

    HTTP_Header *response = HTTP_Header_CreateServerHeader();
    if (!response)
    {
        HTTP_PRINT_ERROR(stderr, "failed to create response header\n");
        return HTTP_MODULE_FAIL;
    }

    size_t html_len = HTML_Document_LookupSize(doc);

    char *html = malloc(html_len);

    html_len = HTML_Document_Fill(doc, html, html_len);

    char length_str[32];
    snprintf(length_str, sizeof(length_str), "%zu", html_len);

    HTTP_Header_Push(response, "Content-Type", "text/html", true);
    HTTP_Header_Push(response, "Content-Length", length_str, true);

    HTML_Destroy_Document(&doc);
    if (!HTTP_Header_SendToClient(conn, response, 200))
    {
        HTTP_PRINT_ERROR(stderr, "failed to send HTTP header\n");
        HTTP_Header_Destroy(&response);
        free(html);
        return HTTP_MODULE_FAIL;
    }

    int sent = HTTP_Write(conn, html, html_len);

    free(html);
    HTTP_Header_Destroy(&response);

    if (sent < 0)
    {
        HTTP_PRINT_ERROR(stderr, "failed to send response body\n");
        return HTTP_MODULE_FAIL;
    }

    return hold ? HTTP_MODULE_OK_HOLD : HTTP_MODULE_OK;
}

const HTTP_Module module_hello_world = {
    .name = "Hello World",
    .ver = "1.0",
    .internal = NULL,
    .load = NULL,
    .action = hello_world_action,
    .destroy = NULL};
