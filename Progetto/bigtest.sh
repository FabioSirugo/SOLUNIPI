#!/bin/bash

exec &> testout.log

#Avvio 1500 client

for i in {1..500} 
do
    ./client Utente$i 1 &
done
wait
echo "Fine bigStore"
wait

for i in {1..500} 
do
    ./client Utente$i 2 &
done

wait
echo "Fine bigRetrieve"
wait

for i in {1..500} 
do
    ./client Utente$i 3 &
done

wait
echo "Fine bigDelete"
wait
