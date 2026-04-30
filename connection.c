// data connection FTP in modalità passiva PASV
#include "common.h"
#include "connection.h"

// apertura porta passiva, notifica del client e restituzione del data_fd
int pasv_open(int client_fd)
{
    int pasv_fd;
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);

    // creazione socket TCO per la data connection
    pasv_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (pasv_fd < 0)
    {
        perror("Errore creazione socket pasv_fd");
        exit(EXIT_FAILURE);
    }

    // bind su porta casuale per conflitti con più client o porte già in uso
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = 0;

    if (bind(pasv_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("Errore bind pas_fd");
        exit(EXIT_FAILURE);
    }

    // ascolto solo di 1 client
    if (listen(pasv_fd, 1) < 0)
    {
        perror("Errore listen pas_fd");
        exit(EXIT_FAILURE);
    }

    // porta scelta dal sistema operativo
    getsockname(pasv_fd, (struct sockaddr *)&addr, &addr_len);
    int port = ntohs(addr.sin_port);
    int p1 = port / 256;
    int p2 = port % 256;

    // invio notifica al client che dovrà connettersi con essi sulla data connection
    char reply[64];
    snprintf(reply, sizeof(reply), "227 Accesso modalità passiva (127,0,0,1,%d,%d)\r\n", p1, p2);
    send(client_fd, reply, strlen(reply), 0);

    // attesa connessione client
    int data_fd = accept(pasv_fd, NULL, NULL);
    close(pasv_fd);

    return data_fd;
}