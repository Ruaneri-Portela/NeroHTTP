#ifndef NERO_MODULE_PAGES_H
#define NERO_MODULE_PAGES_H
#include <nero_html.h>

char *html_error_page(const char *message, size_t *html_size);

char *html_server_error_page(const char *message, size_t *html_size);

char *html_error_custom_page(int code, const char *title, const char *description, size_t *html_size);

#endif