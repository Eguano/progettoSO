# MicroPandOS su umps3

## Gruppo lso24az08

Per compilare il progetto:
```bash
make
```
Aprire `umps3` e creare una macchina, controllare per bene che i path corrispondano.

Per eliminare i file .o e temporanei:
```bash
make clean
```
(farlo prima di un push sul repo)

Per compilare i testers:
```bash
  cd testers
  make
```

Per eliminare i .o dei testers:
```bash
  cd testers
  make clean
``` 

## Dettagli implementativi
Motivazioni dietro ad alcune scelte di implementazione:
- Per la sospensione dei processi sono state usate liste di pcb anzichè singoli campi pensando che successivamente ci sarebbero potuti essere più processi in attesa dello stesso device.
- Aggiunto un service code per l'SSI, nominato ENDIO (8), per gestire il momento in cui viene generato l'interrupt che segnala la fine di un'operazione di IO, per poi sbloccare il processo che era in attesa di quell'evento. Per mandare il messaggio all'SSI contenente questo codice, è stato usato un metodo diretto non potendo usare le SYSCALL.
- I processi bloccati a causa di una receive non vengono memorizzati in una struttura dati in quanto per raggiungerli in seguito si usa il puntatore al destinatario nella send.
- Definita una funzione per copiare i valori dei registri dello state singolarmente a causa di problemi derivanti da memcpy.