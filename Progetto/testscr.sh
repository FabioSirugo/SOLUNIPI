#!/bin/bash

exec &> testout.log


for i in {1..50} 
do
    ./client Utente$i 1 &
done

wait

echo "Fine Stores"

wait



for j in {31..50}
do
    ./client Utente$j 3 &
done

wait
for j in {1..30}
do
    ./client Utente$j 2 &
done

wait
echo "Fine"

wait