#include <nero_http.h>
#include <nero_module.h>
#include <nero_pages.h>

// --- Escrita HTTP ---
int HTTP_Write(HTTP_Connection *conn, const char *data, size_t length)
{
    if (conn->ssl)
    {
        int bytes_written = SSL_write(conn->ssl, data, (int)length);
        if (bytes_written <= 0)
        {
            HTTP_PRINT_SSL_ERROR(stderr, "SSL write error");
            return -1;
        }
        return bytes_written;
    }
    else
    {
        return send(conn->client, data, (int)length, 0);
    }
}

// --- Leitura HTTP ---
int HTTP_Read(HTTP_Connection *conn, char *buffer, size_t length)
{
    if (conn->ssl)
    {
        int bytes_read = SSL_read(conn->ssl, buffer, (int)length);
        if (bytes_read <= 0)
        {
            HTTP_PRINT_SSL_ERROR(stderr, "SSL read error");
            return -1;
        }
        return bytes_read;
    }
    else
    {
        return recv(conn->client, buffer, (int)length, 0);
    }
}

static void HTTP_HandleServerError(HTTP_Connection *conn)
{
    HTTP_Header *response = HTTP_Header_CreateServerHeader();
    if (response)
    {
        HTTP_Header_Push(response, "Content-Type", "text/html", true);
        size_t msg_len;
        const char *msg = html_server_error_page("No modules runend", &msg_len);
        char length_str[32];
        snprintf(length_str, sizeof(length_str), "%zu", msg_len);
        HTTP_Header_Push(response, "Content-Length", length_str, true);
        HTTP_Header_SendToClient(conn, response, 500);
        HTTP_Write(conn, msg, strlen(msg));
        HTTP_Header_Destroy(&response);
    }
    conn->ended = true;
}

// --- Manipula uma conexão HTTP ---
void *HTTP_HandleConnection(HTTP_Connection *conn)
{
    if (!conn || !conn->run)
        return NULL;

    bool keep_connection = false;

    do
    {
        keep_connection = false;

        HTTP_Header *receive_header = HTTP_Header_GetFromClient(conn);
        if (!receive_header)
            break;

        bool handled = false; // Flag para saber se algum módulo processou com sucesso

        // Processa cada módulo registrado
        for (HTTP_Module **module = (HTTP_Module **)conn->modules; *module != NULL; module++)
        {
            HTTP_Module_Response res = (*module)->action((*module)->internal, conn, receive_header);

            switch (res)
            {
            case HTTP_MODULE_OK:
                handled = true;
                // Sucesso, termina processamento dos módulos
                goto end_modules;

            case HTTP_MODULE_OK_HOLD:
                handled = true;
                // Mantém conexão ativa para próxima requisição
                keep_connection = true;
                goto end_modules;

            case HTTP_MODULE_IGNORE:
                // Continua para próximo módulo
                break;

            case HTTP_MODULE_FAIL:
                HTTP_PRINT_ERROR(stderr, "module failed: %s\n", (*module)->name);
                HTTP_HandleServerError(conn);
                HTTP_Header_Destroy(&receive_header);
                conn->ended = true;
                return NULL;

            case HTTP_MODULE_FATAL:
                HTTP_PRINT_ERROR(stderr, "module forced exit: %s\n", (*module)->name);
                exit(EXIT_FAILURE);
            }
        }

        // Se nenhum módulo processou com sucesso, envia erro 500
        if (!handled)
        {
            HTTP_HandleServerError(conn);
            HTTP_Header_Destroy(&receive_header);
            return NULL;
        }

    end_modules:

        HTTP_Header_Destroy(&receive_header);

    } while (keep_connection && *(conn->run));

    conn->ended = true;
    return NULL;
}
