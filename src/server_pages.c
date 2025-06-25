#include <nero_pages.h>
#include <string.h>
#include <stdio.h>

static char *html_build_error_page(int code, const char *title_text, const char *message, size_t *html_size)
{
    size_t len = strlen(title_text) + 128;
    char *full_title = malloc(len);
    if (!full_title)
        return NULL;

    snprintf(full_title, len, "%d %s", code, title_text);

    if (!message)
        message = "An unexpected error occurred.";

    HTML_document *doc = HTML_Create_Document();

    HTML_tag *head = HTML_Create_Tag("head", NULL, false);
    HTML_tag *meta = HTML_Create_Tag("meta", NULL, true);
    HTML_Add_Attribute(&meta->attributes, HTML_Create_Attribute("charset", "UTF-8"), HTML_ADD_END, 0, NULL);

    HTML_tag *style = HTML_Create_Tag("style",
                                      "body {\n"
                                      "  font-family: sans-serif;\n"
                                      "  padding: 40px;\n"
                                      "  text-align: center;\n"
                                      "  transition: background 0.3s, color 0.3s;\n"
                                      "}\n"
                                      "button.theme-toggle {\n"
                                      "  position: absolute;\n"
                                      "  top: 10px;\n"
                                      "  right: 10px;\n"
                                      "  padding: 8px 12px;\n"
                                      "  border: none;\n"
                                      "  border-radius: 5px;\n"
                                      "  background-color: #ccc;\n"
                                      "  cursor: pointer;\n"
                                      "  font-size: 0.9em;\n"
                                      "}\n"
                                      "body[data-theme='dark'] {\n"
                                      "  background: #121212;\n"
                                      "  color: #eee;\n"
                                      "}\n"
                                      "body[data-theme='dark'] h1 {\n"
                                      "  color: #fff;\n"
                                      "}\n"
                                      "body[data-theme='dark'] p {\n"
                                      "  color: #ccc;\n"
                                      "}\n"
                                      "body[data-theme='light'] {\n"
                                      "  background: #f9f9f9;\n"
                                      "  color: #333;\n"
                                      "}\n"
                                      "body[data-theme='light'] h1 {\n"
                                      "  color: #222;\n"
                                      "}\n"
                                      "body[data-theme='light'] p {\n"
                                      "  color: #444;\n"
                                      "}\n"
                                      "@media (prefers-color-scheme: dark) {\n"
                                      "  body:not([data-theme]) {\n"
                                      "    background: #121212;\n"
                                      "    color: #eee;\n"
                                      "  }\n"
                                      "}\n"
                                      "@media (prefers-color-scheme: light) {\n"
                                      "  body:not([data-theme]) {\n"
                                      "    background: #f9f9f9;\n"
                                      "    color: #333;\n"
                                      "  }\n"
                                      "}",
                                      false);

    HTML_tag *script = HTML_Create_Tag("script",
                                       "function toggleTheme() {\n"
                                       "  const body = document.body;\n"
                                       "  const current = body.getAttribute('data-theme');\n"
                                       "  const newTheme = current === 'dark' ? 'light' : 'dark';\n"
                                       "  body.setAttribute('data-theme', newTheme);\n"
                                       "}",
                                       false);

    HTML_tag *title = HTML_Create_Tag("title", full_title, false);

    HTML_Add_Child(head, meta, HTML_ADD_END, 0, NULL);
    HTML_Add_Child(head, title, HTML_ADD_END, 0, NULL);
    HTML_Add_Child(head, style, HTML_ADD_END, 0, NULL);
    HTML_Add_Child(head, script, HTML_ADD_END, 0, NULL);

    HTML_tag *body = HTML_Create_Tag("body", NULL, false);
    HTML_tag *h1 = HTML_Create_Tag("h1", full_title, false);
    HTML_tag *p = HTML_Create_Tag("p", message, false);

    HTML_tag *button = HTML_Create_Tag("button", "Toggle Theme", false);
    HTML_Add_Attribute(&button->attributes, HTML_Create_Attribute("class", "theme-toggle"), HTML_ADD_END, 0, NULL);
    HTML_Add_Attribute(&button->attributes, HTML_Create_Attribute("onclick", "toggleTheme()"), HTML_ADD_END, 0, NULL);

    HTML_Add_Child(body, h1, HTML_ADD_END, 0, NULL);
    HTML_Add_Child(body, p, HTML_ADD_END, 0, NULL);
    HTML_Add_Child(body, button, HTML_ADD_END, 0, NULL);

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
