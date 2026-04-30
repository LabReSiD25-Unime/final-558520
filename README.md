# Server FTP Concorrente in C

Server FTP multiprocesso implementato in C per il Laboratorio di Reti e Sistemi Distribuiti.

## Descrizione

Questo progetto implementa un **server FTP concorrente** che supporta le seguenti operazioni:
- `LIST` - elenca i file nella directory corrente
- `CWD` - cambia directory
- 'PWD' - per directory corrente
- `RETR` - scarica un file dal server
- `STOR` - carica un file sul server
- `QUIT` - chiude la connessione

Il server gestisce **molteplici client contemporaneamente** usando i processi.

## Requisiti

- GCC (GNU Compiler Collection)
- Linux/Unix 
- make

## Installazione

```bash
git clone https://github.com/LabReSiD25-Unime/final-558520.git
cd final-558520
```

## Compilazione

```bash
make
```

Questo genera l'eseguibile `server`.

## Utilizzo

### Avviare il server

```bash
sudo ./server
```

Per avviare il server si ha il bisogno dei permessi dell'amministratore perché ascolta sulla porta 21.

### Connettersi dal client

```bash
ftp -p -P 21 127.0.0.1
```


## Comandi disponibili

- `ls` - elenca i file
- `cd <directory>` - cambia directory
- `get <file>` - scarica un file
- `put <file>` - carica un file
- `quit` - esce

## Pulire i file compilati

```bash
make clean
```

## Struttura del progetto

```
.
├── server.c           # Main server
├── client.c           # client
├── connection.c       # gestione connessioni
├── server.h
├── client.h
├── connection.h
├── common.h           # header comuni
├── Makefile           # compilazione
├── ftp_root/          # root directory FTP
├── test_carico.sh     # script test carico
└── README.md          
```

## Autore

Aurora Fabio

## Note

Il progetto include uno script di test (`test_carico.sh`) per verificare il comportamento con carico concorrente.

In un altro terminale
```bash
tail -f server.log
```
per vedere il log in tempo reale mentre il server gira 