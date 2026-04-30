// gestione comandi FTP
#include "common.h"
#include "connection.h"
#include "client.h"

void cmd_log(FILE *log_file, const char *cmd)
{
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    fprintf(log_file, "[%02d:%02d:%02d] %s\n", tm->tm_hour, tm->tm_min, tm->tm_sec, cmd);
    fflush(log_file); // forza la scrittura immediata su disco
}

// LIST invia la lista dei file della directory sulla data connection
void cmd_list(int client_fd, int data_fd, const char *cwd, FILE *log_file)
{
    DIR *dir;
    struct dirent *entry;
    char buffer[1024];

    ftp_reply(client_fd, log_file, 150, "Connessione aperta con successo");

    // apre la directory corrente
    dir = opendir(cwd);
    if (dir == NULL)
    {
        ftp_reply(client_fd, log_file, 550, "Directory non trovata");
        close(data_fd);
        return;
    }

    // invia ogni entry sulla data connection
    while ((entry = readdir(dir)) != NULL)
    {
        snprintf(buffer, sizeof(buffer), "%s\r\n", entry->d_name);
        send(data_fd, buffer, strlen(buffer), 0);
    }

    closedir(dir);
    close(data_fd);
    ftp_reply(client_fd, log_file, 226, "Trasferimento completo");
}

// RETR scarica un file dal server al client sulla data connection
void cmd_retr(int client_fd, int data_fd, const char *cwd, const char *filename, FILE *log_file)
{
    char path[1024];
    char buffer[1024];
    FILE *file;
    int n;

    // costruzione path assoluto con directory corrente e file
    snprintf(path, sizeof(path), "%s/%s", cwd, filename);

    // apertura in modalità binaria di lettura per supportare ogni file
    file = fopen(path, "rb");
    if (file == NULL)
    {
        ftp_reply(client_fd, log_file, 550, "File non trovato");
        close(data_fd);
        return;
    }

    ftp_reply(client_fd, log_file, 150, "Invio file");

    // legge il file a blocchi (1024 byte) e lo invia sulla data connection
    while ((n = fread(buffer, 1, sizeof(buffer), file)) > 0)
    {
        send(data_fd, buffer, n, 0);
    }

    fclose(file);
    close(data_fd);
    ftp_reply(client_fd, log_file, 226, "Trasferimento file avvenuto con successo");
}

// STOR carica un file dal client al server sulla data connection
void cmd_stor(int client_fd, int data_fd, const char *cwd, const char *filename, FILE *log_file)
{
    char path[1024];
    char buffer[1024];
    FILE *file;
    int n;

    snprintf(path, sizeof(path), "%s/%s", cwd, filename);

    ftp_reply(client_fd, log_file, 150, "Pronto alla ricezione");

    // apertura in modalità binaria di scrittura per supportare ogni file
    file = fopen(path, "wb");
    if (file == NULL)
    {
        ftp_reply(client_fd, log_file, 550, "Errore nella creazione del file di scrittura");
        close(data_fd);
        return;
    }

    // lettura dalla data connection e scrittura sul file a blocchi (1024 byte)
    while ((n = recv(data_fd, buffer, sizeof(buffer), 0)) > 0)
    {
        fwrite(buffer, 1, n, file);
    }

    fclose(file);
    close(data_fd);
    ftp_reply(client_fd, log_file, 226, "Ricezione del file con successo");
}

// CWD cambia la directory di lavoro del processo figlio
void cmd_cwd(int client_fd, char *cwd, const char *arg, FILE *log_file)
{
    char new_path[1024];

    // se il path inizia con / è assoluto, altrimenti è relativo alla directory corrente
    if (arg[0] == '/')
    {
        snprintf(new_path, sizeof(new_path), "%s", arg);
    }
    else
    {
        snprintf(new_path, sizeof(new_path), "%s/%s", cwd, arg);
    }

    // cambia la directory del processo figlio
    if (chdir(new_path) < 0)
    {
        ftp_reply(client_fd, log_file, 550, "Directory non trovata");
        return;
    }

    // aggiorna cwd con il percorso pulito per i comandi successivi
    if (getcwd(cwd, 1024) == NULL)
    {
        perror("Errore getcwd");
    }

    ftp_reply(client_fd, log_file, 250, "Cambio Directory effettuato con successo");
}

// invia risposta FTP formattata
void ftp_reply(int fd, FILE *log_file, int code, const char *msg)
{
    char buffer[BUFFER_SIZE];
    snprintf(buffer, sizeof(buffer), "%d %s\r\n", code, msg);
    cmd_log(log_file, buffer);
    send(fd, buffer, strlen(buffer), 0);
}

// gestisce la sessione FTP comleta di un singolo client e viene chiamata dal processo figlio dopo il fork()
void handle_session(int client_fd)
{
    char buffer[BUFFER_SIZE];
    int logged_in = 0;             // flag per l'autenticazione del client
    char cwd[1024] = "./ftp_root"; // directory di lavoro iniziale
    int data_fd = -1;              // fd sulla data connection
    FILE *log_file = fopen("/tmp/server.log", "a");

    ftp_reply(client_fd, log_file, 220, "Benvenuto al server FTP di Aurora Fabio");

    while (1)
    {
        // select() con timeout di 30 secondi: evita che i client inattivi occupino risorse in modo indefinito
        fd_set read_fds;
        struct timeval timeout;
        FD_ZERO(&read_fds);
        FD_SET(client_fd, &read_fds);
        timeout.tv_sec = 30;
        timeout.tv_usec = 0;
        int ready = select(client_fd + 1, &read_fds, NULL, NULL, &timeout);
        if (ready == 0)
        {
            // timeout scaduto e disconnessione del client
            ftp_reply(client_fd, log_file, 421, "Client disconnesso per timeout della connessione");
            break;
        }
        if (ready < 0)
        {
            perror("Errore select");
            break;
        }

        // lettura comando inviato dal client
        memset(buffer, 0, sizeof(buffer));
        if (recv(client_fd, buffer, sizeof(buffer) - 1, 0) <= 0)
        {
            break;
        }

        // rimozione di \r\n per confronto con il comando
        buffer[strcspn(buffer, "\r\n")] = '\0';

        // gestione comandi FTP
        if (strncmp(buffer, "USER", 4) == 0)
        {
            // accettazione di qualsiasi utente (anonimo)
            cmd_log(log_file, buffer);
            ftp_reply(client_fd, log_file, 331, "Password richiesta");
        }
        else if (strncmp(buffer, "PASS", 4) == 0)
        {
            // accetta qualsiasi password (anonimo)
            logged_in = 1;
            cmd_log(log_file, buffer);
            ftp_reply(client_fd, log_file, 230, "Login effettuato");
        }
        else if (strncmp(buffer, "PASV", 4) == 0)
        {
            // apertura della data connection in modalità passiva: il server apre una porta casuale e attende il client
            cmd_log(log_file, buffer);
            data_fd = pasv_open(client_fd);
        }

        else if (strncmp(buffer, "QUIT", 4) == 0)
        {
            // uscita del client
            cmd_log(log_file, buffer);
            ftp_reply(client_fd, log_file, 221, "Arrivederci");
            break;
        }
        else if (!logged_in)
        {
            // blocco di ogni comando se il client non ha effettuato l'accesso
            cmd_log(log_file, buffer);
            ftp_reply(client_fd, log_file, 530, "Effettua l'accesso");
        }

        else if (strncmp(buffer, "CWD", 3) == 0)
        {
            cmd_log(log_file, buffer);
            cmd_cwd(client_fd, cwd, buffer + 4, log_file);
        }
        else if (strncmp(buffer, "LIST", 4) == 0)
        {
            cmd_log(log_file, buffer);
            cmd_list(client_fd, data_fd, cwd, log_file);
            // reset, per il prossimo comando bisogna applicare un nuovo PASV
            data_fd = -1;
        }
        else if (strncmp(buffer, "RETR", 4) == 0)
        {
            cmd_log(log_file, buffer);
            cmd_retr(client_fd, data_fd, cwd, buffer + 5, log_file);
            data_fd = -1;
        }
        else if (strncmp(buffer, "STOR", 4) == 0)
        {
            cmd_log(log_file, buffer);
            cmd_stor(client_fd, data_fd, cwd, buffer + 5, log_file);
            data_fd = -1;
        }
        else if (strncmp(buffer, "PWD", 3) == 0)
        {
            cmd_log(log_file, buffer);
            char pwd_reply[1026];
            snprintf(pwd_reply, sizeof(pwd_reply), "\"%s\"", cwd);
            ftp_reply(client_fd, log_file, 257, pwd_reply);
        }

        else if (strncmp(buffer, "TYPE", 4) == 0)
        {
            // accettazione di valori binari e ASCII
            cmd_log(log_file, buffer);
            ftp_reply(client_fd, log_file, 200, "Ok");
        }
        else if (strncmp(buffer, "SIZE", 4) == 0)
        {
            // per non bloccare il client
            cmd_log(log_file, buffer);
            ftp_reply(client_fd, log_file, 502, "Ok");
        }
        else
        {
            cmd_log(log_file, buffer);
            ftp_reply(client_fd, log_file, 502, "Comando sconosciuto!");
        }
    }

    fclose(log_file);
    close(client_fd);
}

