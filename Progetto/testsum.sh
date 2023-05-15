#!/usr/bin/env bash

wait

#numero totale di Test
let tot=$(grep -c "Test" testout.log);
let disc=$(grep -c "Disconnessione con successo" testout.log)
let no_disc=$(grep -c "Disconnessione fallita" testout.log)
let ok_store=$(grep -c "STORE OK" testout.log)
let ko_store=$(grep -c "KO STORE" testout.log)
let errore_comando=$(grep -c "comando non conosciuto" testout.log)
let ok_retrieve=$(grep -c "integro" testout.log)
let ko_retrieve=$(grep -c "KO apertura file RETRIEVE" testout.log)
let ok_delete=$(grep -c "DELETE OK" testout.log)
let ko_delete=$(grep -c "DELETE KO" testout.log)
let nome_non_valido=$(grep -c "Nome non valido MAX_SIZE_FILE" testout.log)
let errore_readn=$(grep -c "readn" testout.log)
printf "\n\n\nTotale test lanciati: $tot\nNumero connessioni fallite a causa del nome nullo o troppo lungo : $nome_non_valido\nNumero readn errate : $errore_readn\nDisconnessi correttamente : $disc\nDisconnessione fallita per : $no_disc\nNumero di comandi non riconosciuti : $errore_comando\nNumero store eseguite con successo : $ok_store\nNumero store fallite : $ko_store\nNumero retrieve eseguite con successo : $ok_retrieve\nNumero retrieve fallite : $ko_retrieve\nNumero delete eseguite con successo : $ok_delete\nNumero delete fallite : $ko_delete\n"

wait

for i in {1..3}
do
    #Conto quanti testo di ogni tipo sono stati effettuati
    let n=$(grep -c "Test$i" testout.log);

    #Quanti a buon fine
    let ok=$(grep -c "Failure Test$i: 0 su 20" testout.log);

    #E quanti falliti
    let ko=$(grep -c "Test$i: Failure" testout.log);
    
    echo "Test $i lanciati $n [Passati: $ok su $n]";

done

wait 

printf "\n"
sleep 1
#per trovare il pid del server ed assegnarlo a server_p
server_pid=$(ps -C server -o pid=)

kill -USR1 $server_pid 
kill -TERM $server_pid 
echo "CHIUSURA SERVER"