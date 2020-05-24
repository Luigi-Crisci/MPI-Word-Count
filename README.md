
# 1. Word Count with MPI
An MPI implementation of Word Count problem


## 1.1. Introduzione
Word Count consiste nel determinare in un testo quante occorrenze ci sono di ogni parola presente. Generalmente si presenta in applicazioni in cui è necessario mantenere le parole inserite in limiti definiti (es. nella narrativa, il numero di parole definisce la categoria dello scritto).  



<p align="center">
<img align="center" src="./images/over-the-word-count.jpg" width="400" style="margin: 10px auto 20px; display: block;">
</p>

Benchè estremamente semplice nella sua definizione, Word Count rappresenta una sfida nel campo della programmazione distribuita a causa dell'enorme taglia dei problemi che possono presentarsi: pensiamo infatti a situazioni in cui i documenti da esaminare hanno dimensioni arrivani anche a 100 TB, in cui un'esecuzione sequenziale arriva a richiedere mesi se non anni di computazione.  

E' naturale quindi pensare a Word Count come un problema di programmazione distribuita, in cui la distrbuzione dell'input tra i nodi di un cluser può significativamente diminuire il tempo necessario, passando da anni al tempo per un caffè :coffee:


## 1.2. Soluzione proposta

Vieni quindi proposta una soluzione al problema *Word Count* in ambiente distribuito, dove più nodi hanno accesso agli stessi dati. L'input è un insieme di file, e l'output è la lista delle parole contenute nei file associate al numero di loro occorrenze.

La soluzione proposta vuole massimizzare i vantaggi derivanti dalla distribuzione del calcolo e dalla *data locality*, massimizzando il tempo di computazione dei nodi e minimizzando quello di comunicazione. 
Il protocollo di comunicazione utilizzato per la comunicazione e distribuzione del calcolo è Message Passing Interface (MPI) [^0], il quale rappresenta lo standard *de facto* nel campo della comunicazione tra nodi in un cluster, di cui è stata utlizzata l'implementazione OpenMPI [^1]

<figure align="center">
  <img src="./images/open-mpi-logo.png" style="margin: 10px auto 20px; display: block;">
  <figcaption> OpenMPI logo </figcaption>
</figure>

La soluzione è stata scritta in linguaggio C, e ne viene in seguito presentata e analizzata l'architettura che la compone.

### 1.2.1. Architettura 

La soluzione segue il paradigma ***Map-Reduce***, in cui la computazione è composta da una fase di *map*, in cui ogni nodo processa una pate dell'input, e da una fase di *reduce*, in cui i risultati dei singoli nodi vengono messi insieme. 

L'input è fornito a tutti i nodi ed è composto da una path in cui sono presenti i file da esaminare. **Si assume che tutti i nodi abbiano accesso ai file alla path di input**  
Il processo 0 provvederà a dividere il *workload* tra i nodi del cluster ed inviare loro i dati necessari (incluso se stesso), quindi ogni nodo processa la sua porzione di input. I dati vengono raccolti dal processo 0, che si occuperà di fornirli in output.

La soluzione presentata è pertanto divisibile in 3 fasi:
1. Partizionamento e distribuzione dell'input ai nodi;
2. *Word Count* della porzione assegnata (***Map***);
3. Collezionamento dei risultati (***Reduce***).

#### 1.2.1.1. Partizionamento e distribuzione dell'input

In questa fase è solo il proesso 0 ad eseguirla, in quanto è propedeutica alla fase di calcolo distribuita.  
Lo scopo di questa fase è quello di definire un partizionamento equilibrato del calcolo tra i vari nodi, al fine di minimizzare i tempi di *idle* derivanti da una distribuzione sbilanciata.  
L'approccio scelto è ***byte level***: ad ogni nodo è assegnata una porzione del file di una taglia fissata di byte, uguale per ogni nodo. Le motivazioni dietro questa scelta sono chiare:
- Distribuzione equa: i nodi processeranno un quantitativo di byte uguale tra loro, uniformando il numero di accessi in lettura ai file necessari;
- Disaccoppiamento tra i nodi: ogni nodo riceve un numero di byte da processare ed il file da cui partire, e computa finchè ha da leggere senza interessarsi dello stato degli altri nodi.
Ogni nodo riceve quindi numero di byte da leggere ed il file di partenza: questo perchè i byte potrebbero attraversare più file. E' imperativo quindi che questi siano ordinati in egual modo per ogni nodo

Andiamo ad analizzare alcuni passi critici nell'esecuzione:

Per ogni nodo, il nodo 0 calcola le informazioni necessarie: nella struttura *info* è presenta il numero di byte da leggere, il file di partenza ed il byte di partenza. Il nodo assegna numero di byte da leggere uguale o di 1 unità maggiore a quello degli altri nodi (l'unità in più è necessaria per la gestione delle dimensioni che non dividono il numero di nodi) ed il file a lui assegnato.  
Per calcolare il file a cui il nodo i arriva al termine della sua computazione (il quale sarà l'input per il nodo i+1) somma al byte di partenza del nodo i-esimo il numero dii byte da lui letti e gli sottrae iterativamente le dimensioni del file attuale e successivi fino a che non trova il file di arrivo (in cui **start_byte** è più piccolo della sua dimensione). Se però **start_byte** è esattamente uguale alla dimensione del file corrente, allora significa che il nodo i legge esattamente fino alla sua fine e pertanto il nodo i+1 inizia a leggere dal successivo.

```c
for (int i = 0; i < num_proc; i++)
		{
			partitioning[i] = overflow && overflow_value-- > 0 ? total_dim / num_proc + 1 : total_dim / num_proc;
			infos[i].num_file = current_file;
			infos[i].num_byte = start_byte;

			if (i != (num_proc - 1))
			{
				start_byte += partitioning[i];
				for (; current_file < num_files; current_file++)
				{
					if (start_byte <= dimensions[current_file])
					{
						if (start_byte == dimensions[current_file])
						{
							start_byte = 0;
							current_file++;
						}
						break;
					}
					start_byte -= dimensions[current_file];
				}
```
A questo punto è necessario calcolare il byte di partenza del nodo i+1: a questo punto **start_byte** si trova all'ultimo byte letto dal nodo i, ma questo potrebbe trovarsi nel mezzo di una parola. L'algoritmo sceglie quindi di portare **start_byte** alla posizione della prima parola che il nodo i+1 dovrà leggere, che non è altro che la parola successiva a quella tagliata.  
Per farlo saltiamo al byte **start_byte-1** (solo se non partiamo dall'inizio) e consumiamo ogni carattere che non sia un delimitatore di parola. Dato che potremmo terminare il file nel caso in cui la parola tagliata sia l'ultima, tale condizione viene ulteriormente verificata.

```
				//Need to check if the current byte cuts a word
				sprintf(path, "%s/%s", basepath, files[current_file]->d_name);
				f = fopen(path, "r");

				if(start_byte != 0){
					fseek(f, start_byte - 1, SEEK_SET);
					while (!is_delimeter(c = fgetc(f)))
					{
						start_byte++;
						partitioning[i]++;
					}

					//The cutted word could be the last one
					if (feof(f))
					{
						start_byte = 0;
						current_file++;
					}
				}
			}

			infos[i].counting = partitioning[i];
		}
```

Il lettore potrebbe domandarsi il perchè non sia stato utilizzato un approccio *word level* o *line level* che eliminerebbe molta della complessità presenti, ed il motivo è presto spiegato ed è figlio del tipo di input a disposizione: entrambi gli approcci, seppur equilibrando al meglio il grado di distribuzione dell'input tra i nodi (ma non gli accessi al file) necessitano di una fase di *pre-processing* iniziale in cui contare il numero di parole o linee presenti nei file. Tale fase può essere estremamente lunga per file di dimensioni considerevoli, e pertanto poco performante.

#### 1.2.1.2. Word count 

Per il conteggio delle parole si utilizza una struttura **cell** composta da chiave-valore così definita:

```c
typedef struct c_cell{
	char key[30];
	long int value;
} cell;

```

## 1.3. Benchmark

### 1.3.1. Architettura di testing

### 1.3.2. Esperimenti 

#### 1.3.2.1. Weak Scalability

#### 1.3.2.2. Strong Scalability

### 1.3.3. Risultati

## 1.4. Conclusioni

## 1.5. Bibliografia

[^0]: https://www.mpi-forum.org/docs/mpi-3.1/mpi31-report.pdf
[^1]: https://www.open-mpi.org/
