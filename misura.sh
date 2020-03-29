#!/bin/bash

CORRECT=0; ERR=0; NUMCLIENT=$1

rm -f log/supervisor_2.txt log/tot_client.txt

awk '/SUPERVISOR ESTIMATE/ {print $3 " " $5}' log/supervisor.txt | tail -$NUMCLIENT > log/supervisor_2.txt
cat log/client* | awk '/SECRET/ {print $4 " " $2}' > log/tot_client.txt

exec 3<"log/supervisor_2.txt"

while read -u 3 line; do

	ESTIM=$(echo $line | cut -d' ' -f1)
	ID=$(echo $line | cut -d' ' -f2)
	CLIENT_SECRET=$(( $(grep $ID log/tot_client.txt | cut -d' ' -f1) + 0 ))

	echo ID=$ID SECRET=$CLIENT_SECRET Stima=$ESTIM

	DIFF=$(( $ESTIM - $CLIENT_SECRET ))
	
	if [ $DIFF -lt 0 ]; then
		DIFF=$(( 0 - $DIFF ))
	fi

	if [ $DIFF -lt 25 ] && [ $DIFF -gt -25 ]; then
		(( CORRECT++ ))
	fi

	ERR=$(( $ERR + $DIFF ))
done

perc=$(echo "scale=2; ($CORRECT/$NUMCLIENT)*100" | bc)
errore=$(echo "scale=2; $ERR/$NUMCLIENT" | bc)

echo "============== Statistiche ========="
echo "Stime corrette: $CORRECT ($perc%)"
echo "Errore medio: $errore ms"
echo "===================================="

rm -f log/supervisor_2.txt log/tot_client.txt
