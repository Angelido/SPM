# TITOLO

Questa repository contiene il codice riguardante l'esame di Informatica per il corso di studio di Biotecnologie. Di seguito vediamo una descrizione del codice con anche esempi di utilizzo.

# 📦 MODULO 1

## Codice Implementato

Il codice si trova nel file:

- `graph.py` → contiene tre classi principali: `Node`, `Edge` e `Graph`. La classe centrale è `Graph`.

### Classe `Node` (ausiliaria a `Graph`)

La classe `Node` rappresenta un nodo, definito come:

1. `name` (`str`) → nome del nodo  
2. `trait` (oggetto `Trait`, fornito dal docente) → tratto associato  
3. `infected` (`bool`) → utile per il Modulo 3; indica se il nodo è già stato "infettato" (default: `False`)

**Metodi speciali:**

- `__repr__`, `__str__` → rappresentazione testuale
- `__eq__` → due nodi sono uguali se hanno lo stesso nome
- `__hash__` → hash basato sul nome (nodi con lo stesso nome hanno lo stesso hash)
- `__deepcopy__` → consente di fare copie profonde del nodo

### Classe `Edge` (ausiliaria a `Graph`)

Rappresenta un arco come una quadrupla:

1. `from_node` (`str`) → nodo di partenza  
2. `to_node` (`str`) → nodo di arrivo  
3. `weight` (`float`) → peso dell’arco  
4. `label` (`str`) → etichetta dell’arco

**Metodi speciali:**

- `__repr__`, `__str__` → rappresentazione testuale
- `__eq__` → due archi sono uguali se partenza, arrivo e label coincidono (peso ignorato)

### Classe `Graph`

Modella un grafo come:

1. `nodes` (`dict[str, Node]`) → mappa nomi-nodi  
2. `edges` (`list[Edge]`) → lista di archi

**Metodi speciali:**

- `__contains__` → permette di controllare presenza di nodi o archi:

  ```python
  if node in graph
  if node.name in graph
  if edge in graph
  ```
- `__getitem__` → accede al `Trait` di un nodo dato il suo nome o l’oggetto `Node`:

  ```python
  trait = graph[nodo1]
  trait = graph[nodo1.name]
  ```
- `__repr__`, `__str__` → rappresentazioni testuali
- `__len__` → numero di nodi
- `__deepcopy__` → copia profonda dell'intero grafo
- `__iter__` → abilita il `for` sugli archi:

  ```python
  for edge in graph
  ```

**Metodi classici:**

- `add_node(node)` → aggiunge un nodo (solo se il nome non è già presente)
- `add_edge(edge)` → aggiunge un arco (solo se i nodi esistono)
- `print_nodes()` / `print_edges()` → stampa nodi o archi
- `get_nodes()` / `get_edges()` → restituisce le rispettive liste


## 🚀 QuickStart Modulo 1

Esempio pratico per creare un grafo:

```python
# Inizializzo il grafo vuoto
grafo = Graph()

# Creo dei Trait
red = Trait('blue', dominance=0.1, coding_sequences={"AA", "AA"})
green = Trait('green', dominance=0.1, coding_sequences={"CG", "GG"})
brown = Trait('brown', dominance=0.8, coding_sequences={"AG", "GA"})

# Definisco nomi dei nodi
name1 = "blue"
name2 = "green"
name3 = "brown"

# Creo i nodi
nodo1 = Node(name1, red)
nodo2 = Node(name2, green)
nodo3 = Node(name3, brown)

# Aggiungo i nodi al grafo
grafo.add_node(nodo1)
grafo.add_node(nodo2)
grafo.add_node(nodo3)

# Creo e aggiungo archi
Edge1 = Edge(name1, name2, 0.2, "edge1")
Edge2 = Edge(name2, name3, 0.7, "edge2")

grafo.add_edge(Edge1)
grafo.add_edge(Edge2)
```

Esempio pratico di stampa.

```python
print(grafo)
```
```shell
NODES: 
Node blue: Trait blue with dominance 0.1 and coding sequences {'AA'} 
Node green: Trait brown with dominance 0.8 and coding sequences {'GA', 'AG'} 
Node brown: Trait brown with dominance 0.8 and coding sequences {'GA', 'AG'} 
EDGES: 
Edge blue -> green, weight: 0.5, label: (edge1) 
Edge green -> brown, weight: 0.5, label: (edge2)
```

```python
print(repr(grafo))
```
```shell
Graph:: 3 nodes, 2 edges
```

```python
grafo.print_nodes()
```
```shell
Node blue: Trait blue with dominance 0.1 and coding sequences {'AA'} 
Node green: Trait brown with dominance 0.8 and coding sequences {'GA', 'AG'} 
Node brown: Trait brown with dominance 0.8 and coding sequences {'GA', 'AG'}
```

```python
grafo.print_edges()
```
```shell
Edge blue -> green, weight: 0.5, label: (edge1) 
Edge green -> brown, weight: 0.5, label: (edge2)
```



## ✅ Conclusioni Modulo 1

La classe `Graph` implementa correttamente tutte le specifiche richieste:

1. Restituisce nodi e archi (con `get_*` e `print_*`)
2. Supporta controllo di appartenenza (`in`)
3. Permette iterazione sugli archi (`for`)
4. Ogni nodo ha nome univoco
5. Gli archi sono direzionati, con label e peso



# 📦 MODULO 2



## Codice Implementato

Il codice è distribuito nei seguenti file:

- `graph.py` → aggiunti due nuovi metodi alla classe `Graph`.
- `graph_path.py` → contiene funzioni per trovare tutti i cammini da un nodo in un grafo.
- `jaccard_similarity.py` → contiene funzioni per calcolare la similarità di Jaccard tra insiemi.



### Estensioni alla classe `Graph` (`graph.py`)

Abbiamo aggiunto i seguenti metodi:

1. `alphabetical_node_order()`  
   Restituisce un dizionario `dict[str, int]` che associa a ogni nodo un intero secondo l’ordine lessicografico del nome del nodo. Utile per costruire la matrice di adiacenza.  
   ```python
   node_order = grafo.alphabetical_node_order()
   print(node_order)
   # Output: {'blue': 1, 'brown': 2, 'green': 3}
   ```

2. `adjacency_matrix()`  
   Costruisce e restituisce la matrice di adiacenza (`np.ndarray`) del grafo, seguendo l’ordinamento fornito dal metodo precedente.  
  



### File `graph_path.py`

Tutte le funzioni supportano `find_all_paths()`:

1. `get_key_by_value(diz, val)`  
   Dato un dizionario e un intero, restituisce la chiave (nome del nodo) associata a quel valore.  
   ```python
   name = get_key_by_value(node_order, 3)
   print("The name of the node is", name)
   # Output: The name of the node is green
   ```

2. `find_all_paths(graph, start)`  
   Restituisce tutti i cammini (`list[list[str]]`) a partire dal nodo indicato.

3. `recursive_visit(...)`  
   Visita ricorsiva in profondità per generare tutti i cammini. Funzione interna usata da `find_all_paths()`.

4. `get_adjacent_nodes(matrix, node_order, current)`  
   Restituisce una lista dei nodi adiacenti a `current`, usando la matrice di adiacenza.  
   ```python
   adj_nodes = get_adjacent_nodes(matrix, node_order, 'blue')
   print("I nodi adiacenti a blue sono:")
   for node in adj_nodes:
       print(node)
   # Output:
   # I nodi adiacenti a blue sono:
   # green
   ```

5. `print_all_paths(paths)`  
   Stampa ogni cammino in formato leggibile (es. `A -> B -> C`).



###  File `jaccard_similarity.py`

1. `Jaccard_similarity(set1, set2)`  
   Calcola la similarità di Jaccard tra due insiemi:
   $$
   \text{Jaccard}(A, B) = \frac{|A \cap B|}{|A \cup B|}
   $$

2. `Jaccard_similarity_multiple_sets(all_sets)`  
   Restituisce un dizionario che associa a ogni coppia di insiemi la loro similarità di Jaccard.

3. `Jaccard_similarity_multiple_sets_mean(all_sets)`  
   Calcola la media della similarità di Jaccard su tutte le coppie possibili.



## 🚀 QuickStart Modulo 2

Creazione della matrice di adiacenza

```python
# Aggiungiamo un nuovo arco
Edge3 = Edge("blue", "brown", 0.5, "edge3")
grafo.add_edge(Edge3)

# Otteniamo la matrice di adiacenza
matrix = grafo.adjacency_matrix()
print(matrix)
```
```shell
# Output:
[[0 1 1]
 [0 0 0]
 [0 1 0]]
```

Cammini a partire da un nodo

```python
# Troviamo tutti i cammini da "blue"
paths = find_all_paths(grafo, "blue")

# Stampiamo i risultati
print("Cammini trovati partendo da blue:", len(paths))
print_all_paths(paths)
```
```shell
# Output:
Cammini trovati partendo da blue: 2
blue -> brown
blue -> green -> brown
```

Calcolo della Jaccard Similarity

```python
# Due insiemi di Trait
set1 = [red, green]
set2 = [green, brown]

# Similarità tra due insiemi
jac = Jaccard_similarity(set1, set2)
print("La Jaccard similarity tra set1 e set2 è:", jac)
```
```shell
# Output:
La Jaccard similarity tra set1 e set2 è: 0.333333333333
```

```python
# Terzo insieme
set3 = [red, green, brown]
all_sets = [set1, set2, set3]

# Similarità media
jac_mean = Jaccard_similarity_multiple_sets_mean(all_sets)
print("La Jaccard similarity media tra tutti i set è:", jac_mean)
```
```shell
# Output:
La Jaccard similarity media tra tutti i set è: 0.555555555555
```



## ✅ Conclusioni Modulo 2

Abbiamo completato con successo tutte le funzionalità richieste:

1. Metodo `adjacency_matrix` per ottenere la matrice di adiacenza del grafo.
2. Serie di funzioni per trovare tutti i cammini da un nodo iniziale (`graph_path.py`).
3. Calcolo della similarità di Jaccard tra due insiemi (`Jaccard_similarity`).
4. Estensione alla similarità media su più insiemi (`Jaccard_similarity_multiple_sets_mean`).



# 📦 MODULO 3 



## Codice Implementato

Il codice relativo a questo modulo si trova nel file:

- `diffusion.py`: contiene la definizione della classe `Message`, responsabile della diffusione di un messaggio attraverso un grafo.



### Classe `Message`

Questa classe modella un messaggio che può propagarsi all'interno di un grafo, secondo determinate regole. È caratterizzata da:

- `name` (str) → nome identificativo del messaggio.
- `trait` (oggetto di tipo `Trait`) → rappresenta la caratteristica genetica associata al messaggio.
- `diffusion_power` (int) → quantità iniziale di potenza disponibile per la diffusione.

**Metodi principali**:

- `__str__` → ridefinisce il comportamento di stampa del messaggio, utile per debugging e visualizzazione.

- `reducing_power(nodo)` → definisce la regola di riduzione della potenza nel momento in cui un messaggio tenta di propagarsi a un nuovo nodo. Se il tratto del nodo corrisponde a quello del messaggio, la potenza diminuisce di 1; altrimenti di 2.

- `reducing_power_with_Jaccard(nodo)` → variante che utilizza la similarità di Jaccard (Modulo 2) tra i tratti del messaggio e del nodo per calcolare il costo della diffusione.

- `control_power(nodo)` → verifica se c’è potenza sufficiente per diffondere il messaggio al nodo dato, sulla base della funzione di riduzione scelta.

- `message_diffusion(grafo, nodi_di_partenza)` → gestisce il processo di diffusione a partire dai nodi iniziali. Restituisce un dizionario che associa a ciascun passo (intero) l’elenco dei nodi infettati in quel momento.

- `diffusion_process(...)` → funzione ricorsiva interna che implementa la logica di propagazione.

- `print_message_diffusion(diffusione)` → stampa in modo leggibile il risultato della diffusione ottenuto da `message_diffusion`.



## 🚀 QuickStart Modulo 3

Consideriamo il seguente grafo di esempio:

![Grafo](grafo.png)

Esempio 1: Messaggio con tratto non presente nel grafo, dove quindi il costo di diffusione sarà maggiore.

```python
# Inizializzo il messaggio
grey=Trait('grey_eyes', dominance=0.02, coding_sequences={"TA", "AT"})
mex = Message("Grey_eyes", trait=grey, diffusion_power=6)

# Diffondo il messaggio nel grafo di sopra a partire dai nodi 1 e 7
diffusion=mex.message_diffusion(grafo, ["persona1", "persona7"])

# PASSO 0 --> diffusion_power = 6
# L'infezione del nodo 1 e 7 è gratuita.
# PASSO 1 --> diffusion_power = 6
# Dal nodo 1 partiranno 3 archi diretti ai nodi 2, 3, 5: costo 2.
# Dal nodo 7 non esce nessuno arco: costo 0.
# PASSO 2 --> diffusion_power = 4
# Dal nodo 2 partiranno 3 archi diretti ai nodi 4, 8, 9: costo 2.
# Dal nodo 3 non esce nessun arco (nodi già visitati): costo 0.
# Dal nodo 5 parte un arco diretto al nodo 6: costo 2.
# PASSO 3 --> diffusion_power = 0
# IL PROCESSO FINISCE PERCHÈ NON C'È PIÙ POTENZA DI DIFFUSIONE

# Stampo il risultato di diffusione
mex.print_message_diffusion(diffusion)
```
```bash
Il Messaggio Grey_eyes con il seguente tratto: (Trait grey_eyes with dominance 0.02 and coding sequences {'AT', 'TA'}) si è diffuso nel grafo nel seguente ordine: 

I nodi ['persona1', 'persona7'] sono stati visitati al passaggio 0. 
I nodi ['persona2', 'persona3', 'persona5'] sono stati visitati al passaggio 1. 
I nodi ['persona4', 'persona8', 'persona9', 'persona6'] sono stati visitati al passaggio 2.
```

Esempio 2: Messaggio che incontra tratti compatibili (il costo sarà dunque minore certe volte).

```python
# Inizializzo il messaggio
brown=Trait('brown_eyes', dominance=0.3, coding_sequences={"CG", "GG"})
mex=Message('Brown_eyes', trait=brown, diffusion_power=4)

# Diffondo il messaggio nel grafo di sopra a partire dai nodi 1 e 7
diffusion=mex.message_diffusion(grafo, ["persona1", "persona7"])

# PASSO 0 --> diffusion_power = 4
# L'infezione del nodo 1 e 7 è gratuita.
# PASSO 1 --> diffusion_power = 4
# Dal nodo 1 partiranno 3 archi diretti ai nodi 2, 3, 5: costo 2.
# Dal nodo 7 non esce nessuno arco: costo 0.
# PASSO 2 --> diffusion_power = 2
# Dal nodo 2 partiranno 3 archi diretti ai nodi 4, 8, 9: costo 1.
# Dal nodo 3 non esce nessun arco: non c'è abbastanza potenza.
# Dal nodo 5 non esce nessun arco: non c'è abbastanza potenza.
# PASSO 3 --> diffusion_power = 1
# Dal nodo 4 parte un arco diretto al nodo 6: costo 1.
# IL PROCESSO FINISCE PERCHÈ NON C'È PIÙ POTENZA DI DIFFUSIONE

# Stampo il risultato di diffusione
mex.print_message_diffusion(diffusion)
```
```bash
Il Messaggio Brown_eyes con il seguente tratto: (Trait brown_eyes with dominance 0.3 and coding sequences {'CG', 'GG'}) si è diffuso nel grafo nel seguente ordine: 

I nodi ['persona1', 'persona7'] sono stati visitati al passaggio 0. 
I nodi ['persona2', 'persona3', 'persona5'] sono stati visitati al passaggio 1. 
I nodi ['persona4', 'persona8', 'persona9'] sono stati visitati al passaggio 2. 
Il nodo ['persona6'] è stato visitato al passaggio 3.
```



## ✅ Conclusioni Modulo 3

Questo modulo ha introdotto un meccanismo completo di diffusione di messaggi in un grafo, che tiene conto delle caratteristiche genetiche dei nodi:

1. La classe `Message` consente di modellare un messaggio associato a un tratto genetico.
2. Il metodo `message_diffusion` implementa la logica di propagazione del messaggio, considerando costi variabili.
3. La stampa dei risultati con `print_message_diffusion` fornisce un resoconto chiaro delle fasi di infezione.
