#ifndef NERO_MODULE_H
#define NERO_MODULE_H
#include <nero_http.h>
/// Resultado de execução de um módulo HTTP
typedef enum
{
    HTTP_MODULE_OK,      // O módulo executou com sucesso e encerra o request
    HTTP_MODULE_OK_HOLD, // O mesmo de cima e preservando a conexão
    HTTP_MODULE_IGNORE,  // O módulo ignorou o request; continua para o próximo
    HTTP_MODULE_FAIL,    // O módulo falhou; deve ser reportado
    HTTP_MODULE_FATAL    // Falha crítica; o servidor deve ser reiniciado
} HTTP_Module_Response;

/// Estrutura base de um módulo HTTP plugável
typedef struct
{
    const char *name; // Nome do módulo
    const char *ver;  // Versão do módulo
    void *internal;   // Dados internos específicos do módulo

    void *(*load)(void);                                                                        // Função de inicialização
    HTTP_Module_Response (*action)(void *internal, HTTP_Connection *conn, HTTP_Header *header); // Manipulador principal
    void (*destroy)(void **internal);                                                           // Função de limpeza
} HTTP_Module;
#endif
