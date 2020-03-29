#!/bin/bash

# Rimuovo eventuali file di log
# da esecuzioni precedenti.
rm -f log/*

# Se allo script è passato il flag per eseguire il debug...
if [ $# = 1 ] && [ $1 = "--debug" ]; then

	for i in $(seq 20); do
		valgrind bin/client 5 8 20 >& log/client${i}.txt &
	done

	valgrind bin/supervisor 8 >& log/supervisor.txt &

	sleep 80

	pkill -INT memcheck-amd64-
	pkill -INT memcheck-amd64-
	
# Altrimenti si procede eseguendo un ciclo
# completo di test, come richiesto dalla 
# specifica del progetto.
else

	echo Lanciando il supervisor.
	bin/supervisor 8 >& log/supervisor.txt &

	sleep 2; echo Lanciando 20 clients.

	for((i=0; i<20; i+=2)); do
		bin/client 5 8 20 >& log/client${i}.txt &
		bin/client 5 8 20 >& log/client$(($i+1)).txt &
		sleep 1
	done

	for i in $(seq 6); do
		echo Inviando un SIGINT al supervisor.
		pkill -INT supervisor
		sleep 10
	done

	echo Inviando due SIGINT al supervisor.
	pkill -INT supervisor; pkill -INT supervisor

	sleep 2; echo Stampa delle stime.; ./misura.sh 20
fi
