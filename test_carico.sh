#!/bin/bash

# test di carico per lanciare N client simultaneamente e verificare la corretta gestione del server che riguarda la concorrenza

# directory radice per get e put
cd ~
echo "test" > test.txt

N=50 # numero di client simultanei
for i in $(seq 1 $N); do
    (
        # lancio di ogni client in background con & e <<EOF per lanciare i comandi automaticamente senza interazioni
        ftp -p -P 21 127.0.0.1 <<EOF
anonymous
password
ls
get test.txt
put test.txt
bye
EOF
    ) &
done

wait # aspetta che tutti i processi in background terminino
echo "Test completato con $N client simultanei"