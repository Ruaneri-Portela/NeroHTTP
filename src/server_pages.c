#include <nero_pages.h>
#include <string.h>
#include <stdio.h>

static char *html_build_error_page(int code, const char *title_text, const char *message, size_t *html_size)
{
    char *full_title = malloc(strlen(title_text) + 64);
    if (!full_title)
        return NULL;

    snprintf(full_title, sizeof(full_title), "%d %s", code, title_text);

    if (!message)
        message = "An unexpected error occurred.";

    HTML_document *doc = HTML_Create_Document();

    HTML_tag *head = HTML_Create_Tag("head", NULL, false);
    HTML_tag *meta = HTML_Create_Tag("meta", NULL, true);
    HTML_Add_Attribute(&meta->attributes, HTML_Create_Attribute("charset", "UTF-8"), HTML_ADD_END, 0, NULL);

    HTML_tag *style = HTML_Create_Tag("style",
                                      "body { font-family: sans-serif; background: #f9f9f9; color: #333; padding: 40px; text-align: center; }"
                                      "h1 { font-size: 2.5em; margin-bottom: 0.5em; }"
                                      "p  { font-size: 1.2em; }",
                                      false);

    HTML_tag *title = HTML_Create_Tag("title", full_title, false);

    HTML_Add_Child(head, meta, HTML_ADD_END, 0, NULL);
    HTML_Add_Child(head, title, HTML_ADD_END, 0, NULL);
    HTML_Add_Child(head, style, HTML_ADD_END, 0, NULL);

    HTML_tag *body = HTML_Create_Tag("body", NULL, false);
    HTML_tag *h1 = HTML_Create_Tag("h1", full_title, false);
    HTML_tag *p = HTML_Create_Tag("p", message, false);

    HTML_Add_Child(body, h1, HTML_ADD_END, 0, NULL);
    HTML_Add_Child(body, p, HTML_ADD_END, 0, NULL);

    HTML_Add_Child(doc->html, head, HTML_ADD_END, 0, NULL);
    HTML_Add_Child(doc->html, body, HTML_ADD_END, 0, NULL);

    size_t total_size = HTML_Document_LookupSize(doc);
    char *html = malloc(total_size);
    if (html)
        total_size = HTML_Document_Fill(doc, html, total_size);

    HTML_Destroy_Document(&doc);
    if (html_size)
        *html_size = total_size;
    return html;
}

char *html_error_page(const char *message, size_t *html_size)
{
    return html_build_error_page(404, "Not Found", message ? message : "The requested resource was not found.", html_size);
}

char *html_server_error_page(const char *message, size_t *html_size)
{
    return html_build_error_page(500, "Internal Server Error", message ? message : "The server encountered an unexpected condition.", html_size);
}

char *html_error_custom_page(int code, const char *title, const char *description, size_t *html_size)
{
    return html_build_error_page(code, title ? title : "Error", description ? description : "An unknown error occurred.", html_size);
}
