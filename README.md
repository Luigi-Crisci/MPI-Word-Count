
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

```c
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
la dimensione *30* deriva dalla lunghezza massima delle parole nell'alfabeto italiano, che è 29 per la parola "*esofagodermatodigiunoplastica*". [^2]

Per la gestione delle parole in memoria è stata utilizzata una **hash map** implementata *ad hoc*, basa sull'algoritmo di hashing FNV su un massimo di 100000 liste.  
Questo valore deriva da una precisa considerazione: questa implementazione vuole risolvere il problema del *Word Count* su documenti *reali*, cioè scritti in un linguaggio umano e che rispettino nelle forme una sintassi ben definita. Ne consegue che il numero di parole considerabili è definibile in funzione del numero di parole presenti (ed utilizzate) in tale linguaggio. In questo caso si è scelto di prendere come riferimento la lingua italiana, la quale conta circa 50000 parole di utilizzo comune [^3], valore raddoppiato per ridurre le collisioni in caso di presenza di termini ricercati o dialettali.
Il numero di liste è comunque modificabile tramite il metodo *initialize_map*, che in questa soluzione è definito come *num_bytes / MAX_WORD_LENGHT*

```c
static unsigned hashing(char *key)
{
	for (int i = 0; i < strlen(key); i++)
		key[i] = tolower(key[i]);
	
    unsigned char *p = key;
	unsigned len = strlen(key);
    unsigned h = 2166136261;
    int i;

    for (i = 0; i < len; i++)
    {
        h = (h * 16777619) ^ p[i];
    }

    return h % num_lists < 0 ? h % num_lists * -1 : h % num_lists;
}
```
Fowler–Noll–Vo hash: la presenza del valore *-1* nel return è dovuto ai caratteri wchar, che nel prodotto forniscono valori negativi

Per la lettura delle parole nel file si fa uso di un modulo tokenizzatore, il quale legge da un file e restituisce la prossima parola delimitata da un carattere di delimitazione.  
Un **delimitatore** è un carattere speciale che identifica la fine di una parola: uno spazio, un newline, una tabulazione, della punteggiatura etc. sono considerati delimitatori. L'elenco dei delimitatori utilizzati in questa soluzione è presente nel file "delimeters.txt".  
E' possibile fornire anche un proprio file di delimitatori.

La fase di conteggio è quindi effettuata da ogni nodo, avendo come unica metrica il byte di partenza, il file di partenza ed il numero di byte da leggere.  
Questa risulta molto semplice, in quanto non bisogna far altro che leggere una parola dal file ed inserire nella hash map una coppia \<parola,1\> che, se presente, aggiornerà il valore precedente nella hash map oppure, se non presente, creerà un nuovo nodo per quella parola. Quindi viene decrementato il numero di byte da leggere e viene cambiato il file se necessario.

```c
	initialize_map(bytes_to_read / MAX_WORD_LENGHT);
	new_hash_map();
	while (bytes_to_read > 0 && current_file < num_files)
	{
		if (file == NULL)
		{
			sprintf(path, "%s/%s", basepath, files[current_file]-> d_name);
			file = fopen(path, "r");
			if (start_byte > 0) //Skip to start position
			{
				fseek(file, start_byte, SEEK_SET);
				start_byte = 0;
			}
		}

		readed_bytes = next_word(file, &word);
		bytes_to_read -= readed_bytes;
		if (word == NULL)
		{
			fclose(file);
			current_file++;
			file = NULL;
			continue;
		}

		if (bytes_to_read >= 0)
		{
			//Actually add the word to the hash map
			strcpy(word_cell.key, word);
			add_or_update(&word_cell);
		}

		free(word);
	}
```
#### 1.2.1.3. Reduce

La fase di reduce prevede il collezionamento dei risultati dei vari nodi verso il nodo 0, il quale si occuperà dell'output finale.

La struttura adottata è ad ***albero rovesciato direzionato***:
- I nodi rappresentano le foglie dell'albero
- Ogni arco rappresenta:
  - Invio dei risultati se diretto ad un nodo con etichetta diversa dal nodo di partenza
  - Ricezione dei risultati dai nodi origine degli archi entranti
- Il nodo radice è il nodo obiettivo della reduce

![Reduce](images/reduce.gif)

La scelta di tale struttura è dovuta alla volontà di massimizzare l'utilizzo dei nodi nella fase di reduce e distribuire di conseguenza il calcolo in modo intelligente tra i nodi del cluster: infatti tale scelta permette di eseguire, per $n$ nodi, al più $\lceil log_2n \rceil$ *receive* e ridurre considerevolmente il carico di lavoro che, altrimenti, sarebbe gravato su un singolo modo.  
La comunicazione in questa fase è essenzialmente *asincrona*: ogni nodo definisce dei buffer di ricezione per ogni nodo da cui dovrà ricevere dati che verranno riempiti asincronamente. Ciò ha il vantaggio di massimizzare la velocità di ricezione al costo di un incremento nell'uso di memoria RAM, evitando inoltre stalli nella fase di ricezione a causa di nodi più lenti di altri.

L'algoritmo può essere scomposto in 3 fasi:  
1. Calcolo di *receivers* e *sender*;
2. Ricezione dei dati
3. Invio dei dati

##### Calcolo *receivers* e *senders*

Ogni nodo ha necessità di calcolare a priori quali saranno i nodi da cui riceverà i dati temporanei ed a quale nodo dovrà inviare i dati da lui computati. Questo è necessario per sfruttare al meglio i sistemi di comunicazione asincrona.  
Per fare ciò ogni nodo **attraversa l'albero e calcola il suo comportamento per ogni livello:** per ogni livello, calcola il nodo da cui dovrà eventualmente ricevere ed alloca un buffer apposito ed aggiorna il nodo a cui dovrà inviare i risultati.

```c
send_proc = rank - 1; //Default behaviour
	while (cur_rank % 2 == 0 && level < ceil(log2(num_proc)))
	{
	 	//I'm the last one
		if (ceil(num_proc / pow(2, level)) == (cur_rank + 1)){
			//If I am a power of two and last one, i communicate with rank 0
			if (is_power_of_two(cur_rank))
			{
				send_proc = 0;
				break;
			}
		}
		else
		{
			if (num_requests == dim_buffer)
			{
				dim_buffer *= 2;
				buffers = realloc(buffers, sizeof(cell*) * dim_buffer);
			}

			buffers[num_requests] = calloc(MAX_WORD_NUM,sizeof(cell));
			MPI_Irecv(buffers[num_requests], MAX_WORD_NUM * sizeof(cell), CELL, (int)(rank + pow(2, level)),
					  0, MPI_COMM_WORLD, &requests[num_requests]);
			num_requests++;
		}
		cur_rank = cur_rank / 2;
		level++;
		send_proc = (int)(rank - pow(2,level));
	}
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
[^2]: https://www.focus.it/cultura/curiosita/qual-e-la-parola-piu-lunga-della-lingua-italiana
[^3]: http://www.treccani.it/magazine/lingua_italiana/domande_e_risposte/lessico/lessico_267.html
