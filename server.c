// server FTP concorrente - accept, loop, fork
#include "common.h"
#include "client.h"

int *active_clients;   // clienti attivi nella memoria condivisa
#define MAX_CLIENTS 20  // limite massimo di connessioni simultanee

int main()
{
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    sem_t *sem;     // semaforo per proteggere active_clients da race condition

    // creazione del socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("Creazione del socket fallita");
        exit(EXIT_FAILURE);
    }

    // permette il riavvio immediato del server senza aspettare che la porta di liberi
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // alloca active_clients nella memoria condivisa con mmap
    active_clients = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    *active_clients = 0;

    // alloca il semafono nella memoria condivisa per la sincronizzazione tra processi
    sem = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    sem_init(sem, 1, 1);
    
    // configurazione dell'indirizzo
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // binding del socket
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // in ascolto per connessioni in entrata
    if (listen(server_fd, 50) < 0)
    {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // ignora SIGCHLD, il kernel pulisce automaticamente i figli terminati evitando la creazione di processi zombie
    signal(SIGCHLD, SIG_IGN);
    while (1)
    {
        /// accettazione di una nuova connessione
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
        {
            perror("accept");
            close(server_fd);
            continue;
        }

        // lettura protetta dal semaforo per evitare race condition
        sem_wait(sem);
        int clients = *active_clients;
        sem_post(sem);

        // se il limite delle connessioni viene superato, la connessione viene rifiutata con il codice FTP 421
        if (clients >= MAX_CLIENTS)
        {
            send(new_socket, "421 Massimo clients consentiti\r\n", 32, 0);
            close(new_socket);
            continue;
        }

        // fork crea un processo figlio per la gestione del client in parallelo
        pid_t pid = fork();
        if (pid < 0)
        {
            perror("fork");
            close(new_socket);
            continue;
        }

        if (pid == 0)
        {
            // processo figlio
            close(server_fd); // il figlio non accetta nuove connessioni

            sem_wait(sem); // acquisisce il lock
            (*active_clients)++; // incrementa il contatore 
            sem_post(sem); // rilascia il lock

            handle_session(new_socket); // il figlio gestisce la sessione FTP del client

            sem_wait(sem); // acquisisce il lock
            (*active_clients)--; // decrementa il contatore
            sem_post(sem); // rilascia il lock

            exit(EXIT_SUCCESS);
        }
        // processo padre
        close(new_socket);
    }

    return 0;
}