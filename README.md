# Problema
L'obiettivo di word count è quello di contare il numero di parole che sono presenti in uno o più file al fine di raggiungere vari obiettivi come per esempio la raccolta di dati a fini statistici o la risoluzione di problemi legati al mondo dell'editoria o del giornalismo. Spesso i file di cui si vogliono contare le parole sono molto grandi, basti pensare per esempio ad un libro il quale può contenere anche milioni di parole, ragion per cui eseguire le operazioni di conta delle parole con un solo processore (o thread o processo) possono richiedere molto tempo. Si necessita quindi di un approccio distribuito che riesca a fornire delle prestazioni ben più elevate. Word count utilizza MPI per eseguire queste operazioni e contare le parole nel minor tempo possibile.

# Soluzione
L'idea della soluzione è molto semplice: dividere il carico di lavoro equamente tra tutti i processori coinvolti nell'elaborazione così che non ci siano processori che debbano eseguire carichi di lavoro più elevati mentre altri invece sono in idle perché hanno terminato il loro lavoro, e quindi velocizzare l'elaborazione.
A tal proposito le strade percorribili sono di due tipi:
* Distribuire lo stesso numero di parole tra tutti i processori;
* Distribuire lo stesso numero di bytes tra tutti i processori.

## Distribuire lo stesso numero di parole tra tutti i processori
Questo approccio è stato subito scartato in quanto non era un operazione semplice determinare quante parole fossero presenti nel file e distribuirle tra tutti i processori. Anche in questo caso erano possibili due soluzioni:
* Analisi statistica sul numero di byte;
* Conteggio delle parole da parte del master.

Nel primo caso si vuole provare a calcolare il numero di parole presenti nel documento facendo un'analisi statistica sul numero di bytes presenti nello stesso, ma questo metodo forniva risultati troppo aleatori innanzitutto perché la lunghezza media delle parole varia in base alla lingua del testo (6 caratteri per le parole Italiane e 5 per le parole Inglesi) e in secondo luogo perché questo calcolo era solamente una stima e non avrebbe portato alla distribuzione completamente equa del lavoro.

La seconda soluzione avrebbe permesso di poter asegnare lo stesso numero di parole a tutti i processori, ma altri problemi sorgono per questa soluzione: il processo master avrebbe dovuto leggere tutti i file per contare le parole impiegando tanto tempo e non era inoltre garantito che il lavoro fosse equamente distribuito in quanto le parole possono avere lunghezze molto diverse tra di loro.

## Distribuire lo stesso numero di bytes tra tutti i processori
La soluzione è stata quella di dividere il numero di bytes equamente tra tutti i processi. Il numero di bytes di un file è facilmente leggibile e dal momento che un byte corrisponde ad un carattere (fatta eccezione per il carattere di carriage return) non sono state necessarie particolari conversioni.
Ad ogni processo vengono inviate tre informazioni:
* Nome del file da leggere;
* Offset di partenza da dove iniziare a leggere i byte;
* Offset di fine lettura

Questi tre elementi sono stati racchiusi un una struttura dati chiamata SubTask ed ogni processo riceve una lista di SubTask dal processo master e sequenzialmente legge quella lista di file partendo dall'offset iniziale fino all'offset finale.

```c
typedef struct {
    char fileName[MAX_FILE_NAME_LENGTH];
    long startFromBytes;
    long endToBytes;
} SubTask;
```

È stato necessario rispondere ad alcune domande come:
* Che succede se l'offset di fine non corrisponde con la fine di una parola?
* Che succede se l'offset di inizio non corrisponde con l'inizio di una parola?

Se l'offset di fine non corrisponde con la fine di una parola il processo che sta leggendo continua fino a quando non trova un carattere di carriage return.
Se l'offet di inizio non corrisponde con l'inizio di una parola il processo che sta leggendo va avanti nella lettura e scarta tutto fino a quando non legge un carattere di carriage return. Scarta tutto perché ci sarà il processo precedente che avrà già letto quella parola.

# Correttezza dell'algoritmo

# Dettagli implementativi

# Benchmarks
## Scalabilità forte

## Scalabilità debole

# Conclusioni
