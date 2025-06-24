#include <nero_http.h>

// --- Aceita nova conexão, cria e inicia thread para lidar com ela ---
HTTP_Connection *HTTP_Connection_Get(HTTP_Connection_Manager *context)
{
    socket_fd clientfd = accept(context->server, NULL, NULL);
    if (clientfd < 0)
    {
        HTTP_PRINT_ERROR(stderr, "accept");
        return NULL;
    }

    SSL *ssl = NULL;
    if (context->ssl_ctx)
    {
        ssl = SSL_new(context->ssl_ctx);
        SSL_set_fd(ssl, clientfd);

        if (SSL_accept(ssl) <= 0)
        {
            HTTP_PRINT_SSL_ERROR(stderr, "ssl accept");
            SSL_free(ssl);
            close_socket(clientfd);
            return NULL;
        }
    }

    HTTP_Connection *conn = calloc(1, sizeof(HTTP_Connection));
    if (!conn)
    {
        HTTP_PRINT_ERROR(stderr, "calloc");
        if (ssl)
            SSL_free(ssl);
        close_socket(clientfd);
        return NULL;
    }

    conn->client = clientfd;
    conn->ssl = ssl;
    conn->run = &context->run;

    if (pthread_create(&conn->thread, NULL, (void *(*)(void *))HTTP_HandleConnection, (void *)conn) != 0)
    {
        HTTP_PRINT_ERROR(stderr, "pthread create");
        free(conn);
        if (ssl)
            SSL_free(ssl);
        close_socket(clientfd);
        return NULL;
    }

    return conn;
}

// --- Libera conexão e aguarda thread finalizar ---
void HTTP_Connection_Destroy(HTTP_Connection **conn)
{
    if (!conn || !(*conn))
        return;

    if ((*conn)->ssl)
    {
        SSL_shutdown((*conn)->ssl);
        SSL_free((*conn)->ssl);
    }

    if ((*conn)->client >= 0)
    {
        close_socket((*conn)->client);
    }

    pthread_join((*conn)->thread, NULL);
    free(*conn);
    *conn = NULL;
}

// --- Adiciona conexão na lista do gerenciador ---
bool HTTP_Manager_Push_Connection(HTTP_Connection_Manager *context, HTTP_Connection *push)
{
    if (!context || !push)
        return false;

    if (context->limit > 0 && context->count >= (size_t)context->limit)
        return false;

    push->next = NULL;
    push->prev = NULL;

    if (!context->base && !context->head)
    {
        context->base = context->head = push;
    }
    else
    {
        context->head->next = push;
        push->prev = context->head;
        context->head = push;
    }

    context->count++;
    return true;
}

// --- Remove conexão da lista e destrói ---
bool HTTP_Manager_Remove_Connection(HTTP_Connection_Manager *context, HTTP_Connection *toRemove, int position, HTTP_Connection *startOver)
{
    if (!context || (position >= 0 && context->count < 1))
        return false;

    size_t count = 0;
    for (HTTP_Connection *conn = (startOver ? startOver : context->base); conn != NULL; conn = conn->next, count++)
    {
        if ((toRemove != NULL && toRemove == conn) || (position >= 0 && count == (size_t)position))
        {
            if (conn == context->base)
                context->base = conn->next;
            else if (conn->prev)
                conn->prev->next = conn->next;

            if (conn == context->head)
                context->head = conn->prev;
            else if (conn->next)
                conn->next->prev = conn->prev;

            context->count--;
            HTTP_Connection_Destroy(&conn);
            return true;
        }
    }

    return false;
}

// --- Varre conexões e remove as que já terminaram ---
void HTTP_Manager_Connections(HTTP_Connection_Manager *context)
{
    HTTP_Connection *conn = context->base;

    while (conn != NULL)
    {
        HTTP_Connection *next = conn->next;

        if (conn->ended)
        {
            HTTP_Manager_Remove_Connection(context, conn, -1, conn);
        }

        conn = next;
    }
}
