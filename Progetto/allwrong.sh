#!/bin/bash

exec &> testout.log


for j in {1..30}
do
    ./client Utente$j 2 &
done

wait

for j in {31..50}
do
    ./client Utente$j 3 &
done

wait

echo "Fine"

wait