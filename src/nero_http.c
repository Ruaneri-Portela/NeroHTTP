#include <nero_http.h>
#include <nero_module.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>

// --- Módulos registrados ---
extern const HTTP_Module module_hello_world;
extern const HTTP_Module module_file;

const HTTP_Module *defaults_all_modules[] = {
    &module_file,
    &module_hello_world,
    NULL,
};

// --- Controle de execução ---
static bool run = true;

static void handle_close(int sig)
{
    printf("Received signal %d, shutting down...\n", sig);
    run = false;
}

int main()
{
    // --- Tratador de sinal para encerramento gracioso ---
    signal(SIGINT, handle_close);

    // --- Inicialização de rede (Windows) ---
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        HTTP_PRINT_ERROR(stderr, "WSAStartup failed");
        return 1;
    }
#endif

    // --- Inicialização OpenSSL ---
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();

    SSL_CTX *ctx = SSL_CTX_new(TLS_server_method());
    if (!ctx)
    {
        HTTP_PRINT_SSL_ERROR(stderr, "SSL_CTX_new failed");
        return 1;
    }

    if (!SSL_CTX_use_certificate_file(ctx, "cert.pem", SSL_FILETYPE_PEM) ||
        !SSL_CTX_use_PrivateKey_file(ctx, "key.pem", SSL_FILETYPE_PEM))
    {
        HTTP_PRINT_SSL_ERROR(stderr, "Erro carregando cert.pem/key.pem");
        SSL_CTX_free(ctx);
        return 1;
    }

    // --- Configuração do gerenciador de conexões ---
    HTTP_Connection_Manager manager = {0};
    manager.ssl_ctx = ctx;
    manager.modules = (void **)defaults_all_modules;

    // --- Criação do socket ---
    manager.server = socket(AF_INET, SOCK_STREAM, 0);
    if (manager.server < 0)
    {
        HTTP_PRINT_ERROR(stderr, "socket");
        SSL_CTX_free(ctx);
        return 1;
    }

    struct sockaddr_in server_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(PORT),
        .sin_addr.s_addr = INADDR_ANY,
    };

    if (bind(manager.server, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        HTTP_PRINT_ERROR(stderr, "bind");
        close_socket(manager.server);
        SSL_CTX_free(ctx);
        return 1;
    }

    if (listen(manager.server, 1) < 0)
    {
        HTTP_PRINT_ERROR(stderr, "listen");
        close_socket(manager.server);
        SSL_CTX_free(ctx);
        return 1;
    }

    // --- Loop principal ---
    printf("Servidor ouvindo na porta %d...\n", PORT);
    manager.run = run;
    while (run)
    {
        socket_poll_fd fds[] = {
            {.fd = manager.server, .events = POLLIN}};

        int poll_ret = poll_socket(fds, 1, 100); // espera 100ms
        if (poll_ret < 0)
        {
            HTTP_PRINT_ERROR(stderr, "poll");
            break;
        }

        if (poll_ret == 0)
        {
            HTTP_Manager_Connections(&manager);
            continue;
        }

        if (fds[0].revents & POLLIN)
        {
            HTTP_Connection *conn = HTTP_Connection_Get(&manager);
            if (!conn)
                continue;

            conn->modules = manager.modules;

            if (!HTTP_Manager_Push_Connection(&manager, conn))
                HTTP_Connection_Destroy(&conn);
        }
    }

    // --- Encerramento ---
    manager.run = false;
    close_socket(manager.server);
    SSL_CTX_free(ctx);

#ifdef _WIN32
    WSACleanup();
#endif

    return 0;
}
