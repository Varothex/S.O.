#include <alloc_proiect.h>
#include <stdio.h>

/*
Memoria care nu este la finalul heap-ului nu poate fi eliberata, dar o putem marca ca fiind utilizabila (free). 
Deci trebuie sa stim dimensiunea si daca este free sau not-free. Pentru a retine asta, folosim un header pentru fiecare bloc de memorie.
Cand cerem x bytes de memorie, calculam dimensiunea totala a header+x si apelam sbrk(total_size), unde punem si headerul si blocul de memorie.
Nu stim daca blocurile de memorie sunt puse unul dupa altul, asa ca le punem intr-o lista inlantuita.
Acum punem header-ul intr-un union de 16 bytes, pentru a alinia header-ul cu adrese de memorie de 16. Blocul de memorie se va afla la
finalul header-ul, deci va fi mai usor de accesat.
*/

// Vom folosi un mutex pentru a preveni mai multe thread-uri din a accesa memoria.
pthread_mutex_t global_malloc_lock;

// Vom folosi doi pointeri pentru a urmari lista.
header_t *head = NULL, *tail = NULL;

//pentru a aloca spatiu
void *malloc(size_t size)
{
	
	size_t total_size;
	void *block;
	header_t *header;
	
	// Daca dimensiunea ceruta = 0, returnam NULL.
	if (!size)
		return NULL;
	
	// Blocam alte thread-uri.
	pthread_mutex_lock(&global_malloc_lock);
	
	// Cautam daca este loc free in memorie si il folosim pe acela, fara sa marim heap-ul.		
	header = get_free_block(size);
	if (header) 
	{
		// Ura, am gasit memorie! Marcam zona ca nemaifiind free, scoatem lock-ul si returnam pointer-ul spre noua zona de memorie.
		header->s.is_free = 0;
		pthread_mutex_unlock(&global_malloc_lock);
		
		// Mergem pe byte-ul fix de langa finalul header-ului, deoarece este primul byte al blocului de memorie.
		return (void*)(header + 1);
	}
	
	// Daca nu gasim, apelam sbrk() si cerem spatiul necesar pentru blocul de memorie+header.
	total_size = sizeof(header_t) + size;
	block = sbrk(total_size);
	
	if (block == (void*) -1) 
	{
		pthread_mutex_unlock(&global_malloc_lock);
		return NULL;
	}
	
	// Punem datele in header:
	header = block;
	header->s.size = size;
	header->s.is_free = 0;
	header->s.next = NULL;
	
	//actualizam starea listei inlantuite prin actualizarea head si tail
	if (!head)
		head = header;
	if (tail)
		tail -> s.next = header;
	tail = header;
	
	//deblocam
	pthread_mutex_unlock(&global_malloc_lock);
	
	
	// Ascundem header-ul + returnam memoria blocului
	return (void*)(header + 1);
}

//pentru a realoca spatiu
void *realloc(void *block, size_t size)
{
	header_t *header;
	void *ret;
	
	//daca nu avem ce realoca, alocam
	if (!block || !size)
		return malloc(size);
		
	// Mai intai cautam memorie free si daca este suficient de mare, o folosim pe aceia.
	header = (header_t*)block - 1;
	if (header->s.size >= size)
		return block;
	ret = malloc(size);
	if (ret) 
	{
		// Mutam continutul aici cu memcpy() in blocul cu dimensiunea necesara:
		memcpy(ret, block, header->s.size);
		
		// Vechiul bloc de memorie este acum free:
		free(block);
	}
	return ret;
}

header_t *get_free_block(size_t size)
{
	header_t *curr = head;
	while(curr) {
		// Cautam un bloc liber de dimensiunea ceruta.
		if (curr->s.is_free && curr->s.size >= size)
			return curr;
		curr = curr->s.next;
	}
	return NULL;
}

void free(void *block)
{
	header_t *header, *tmp;
	// Marcam sfarsitul process's data segment (brk):
	void *programbreak;

	if (!block)
		return;
		
	pthread_mutex_lock(&global_malloc_lock);
	
	// Mai intai luam header-ul:
	header = (header_t*)block - 1;
	
	// Folosim sbrk(0) pentru a obtine program break address:
	programbreak = sbrk(0);

	// Verificam daca blocul de eliberat este ultimul din lista (adica coincide cu programbreak). daca este, micsoram heap-ul si eliberam memoria.
	if ((char*)block + header -> s.size == programbreak) 
	{
		if (head == tail) 
		{
			head = tail = NULL;
		} 
		else 
		{
			tmp = head;
			while (tmp) 
			{
				if(tmp -> s .next == tail) 
				{
					tmp -> s.next = NULL;
					tail = tmp;
				}
				tmp = tmp -> s.next;
			}
		}
		
		// sbrk() cu argumente negative decrementeaza program break, deci eliberam memorie:
		sbrk(0 - header -> s.size - sizeof(header_t));

		pthread_mutex_unlock(&global_malloc_lock);
		return;
	}
	
	// Daca blocul de eliberat nu este ultimul din lista nu il stergem, dar il marcam ca fiind free.
	header -> s.is_free = 1;
	pthread_mutex_unlock(&global_malloc_lock);
}


/*
// functie de debug care afiseaza intreaga lista
void print_mem_list()
{
	header_t *curr = head;
	printf("head = %p, tail = %p \n", (void*)head, (void*)tail);
	while(curr) {
		printf("addr = %p, size = %zu, is_free=%u, next=%p\n",
			(void*)curr, curr->s.size, curr->s.is_free, (void*)curr->s.next);
		curr = curr->s.next;
	}
}

*/