#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

void http_server_init(void);

void web_update_data(
    int n1,
    char operador,
    int n2,
    int acertos_atual,
    const char *digitado_atual,
    const char *msg,
    const char *evento,
    int premio_ativo,
    int tempo_premio
);

#endif