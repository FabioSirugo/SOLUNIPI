#!/bin/bash

exec &> testout.log

for i in {1..10} 
do
    ./client UtenteUtenteUtenteUtenteUtenteUtenteUtenteUtenteUtenteUtenteUtenteUtenteUtenteUtenteUtenteUtenteUtenteUtenteUtenteUtenteUtenteUtenteUtenteUtenteUtenteUtenteUtenteUtenteUtenteUtenteUtenteUtenteUtenteUtenteUtenteUtenteUtenteUtenteUtenteUtenteUtenteUtenteUtente$j 1 &
done

wait

echo "Fine"

wait