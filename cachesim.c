#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#define maxchar 5
#define addressbits 48

struct Block {
	long tag;
	int v;
};

int main(int argc, char *argv[]) {
	if (argc != 7) {
		printf("Wrong Number of Arguments\n");
	}

	int i, j;

	int cacheSize = atoi(argv[1]);
	//512 direct p0 fifo 8 trace1.txt

	// argv[2] = associativity, argv[3] = prefetch policy
	// argv[4] = replacement (fifo), argv[5] = block size
	int blockSize = atoi(argv[5]);

	FILE *stream = fopen(argv[6], "r");
	if (stream == NULL) {
		printf("Error: Opening File");
		return 0;
	}

	int memr = 0, memw = 0, cacheh = 0, cachem = 0, assoc, sets;
	long a;
	char rw;

	sscanf(argv[2], "%*[^0123456789]%d", &assoc);

	int *arr_setassocindex = NULL;

	if (strcmp(argv[2], "direct") == 0) {
		sets = cacheSize / blockSize;
		assoc = 1;
	}		
	else if (strcmp(argv[2], "assoc") == 0) {
		sets = 1;
		assoc = cacheSize / blockSize;
	} else {
		sets = cacheSize / (blockSize * assoc);
		arr_setassocindex = (int*)malloc(sets * sizeof(int));
		for (i = 0; i < sets; i++) {
			arr_setassocindex[i] = 0;
		}
	}

	struct Block **cache = (struct Block**) malloc(sets * sizeof(struct Block*));
	for (i = 0; i < sets; i++) {
		cache[i] = (struct Block*) malloc(assoc * sizeof(struct Block));
	}

	int offsetnum = (int)log2((double)blockSize);
	int indexnum = (int)log2((double)sets);
	int iassoc = 0;
	long indexbits, tagbits;

	while (fscanf(stream, "%*x: %c", &rw) > 0) {
		fscanf(stream, "%lx", &a);
		// to test

		if (strcmp(argv[2], "direct") == 0) {
			indexbits = a >> offsetnum & ((1 << indexnum) - 1);
			tagbits = a >> (offsetnum + indexnum);

			// printf("Cache: %lu | Tag: %lu\n", cache[indexbits][0].tag, tagbits);

			if (cache[indexbits][0].tag == tagbits && cache[indexbits][0].v == 1) {
				// Hit
				cacheh++;
				if (rw == 'W') {
					memw++;
				}
			}
			else {
				memr++;
				if (rw == 'W') {
					memw++;
				}
				//Miss
				cachem++;
				cache[indexbits][0].tag = tagbits;

				if (cache[indexbits][0].v != 1) {
					cache[indexbits][0].v = 1;
				}

				if (strcmp(argv[3], "p1") == 0) {
					long c = a + (long) blockSize;
					indexbits = c >> offsetnum & ((1 << indexnum) - 1);
					tagbits = c >> (offsetnum + indexnum);

					if (cache[indexbits][0].tag == tagbits && cache[indexbits][0].v == 1) {
						// Do Nothing
					
					}
					else {
						cache[indexbits][0].tag = tagbits;
						if (cache[indexbits][0].v != 1) {
							cache[indexbits][0].v = 1;
						}

						memr++;
	
					
					}
				}
			}
		}
		else if (strcmp(argv[2], "assoc") == 0) {
			tagbits = a >> offsetnum;
			for (i = 0; i < assoc; i++) {

				if (tagbits == cache[0][i].tag && cache[0][i].v == 1) {
					//Hit
					cacheh++;

					if (rw == 'W') {
						memw++;
					}
					break;
				}

				if (iassoc == assoc) {	// Resets to 1 if 0 to satisfy fifo
					iassoc = 0;
				}

				if (i == assoc - 1) {	// Miss
					memr++;

					if (rw == 'W') {
						memw++;
					}

					cachem++;
					cache[0][iassoc].tag = tagbits;
					if (cache[0][iassoc].v != 1) {
						cache[0][iassoc].v = 1;
					}

					iassoc++;

					if (strcmp(argv[3], "p1") == 0) {
						long c = a + (long)blockSize;
						tagbits = c >> offsetnum;

						for (j = 0; j < assoc; j++) {
							if (tagbits == cache[0][j].tag && cache[0][j].v == 1) {
								//Hit
								break;
							}

							if (iassoc == assoc) {	// Resets to 1 if 0 to satisfy fifo
								iassoc = 0;
							}

							if (j == assoc - 1) {	// Miss
								memr++;
								cache[0][iassoc].tag = tagbits;
								if (cache[0][iassoc].v != 1) {
									cache[0][iassoc].v = 1;
								}
								iassoc++;
							}
						}
					}
				}
			}
		}
		else {
			indexbits = a >> offsetnum & ((1 << indexnum) - 1);
			tagbits = a >> (offsetnum + indexnum);

			for (i = 0; i < assoc; i++) {
				if (tagbits == cache[indexbits][i].tag && cache[indexbits][i].v == 1) {	// Hit
					cacheh++;

					if (rw == 'W') {
						memw++;
					}
					break;
				}
				// Miss
				if (arr_setassocindex[indexbits] == assoc) {
					arr_setassocindex[indexbits] = 0;
				}

				if (i == (assoc - 1)) {	// Miss
					memr++;
					if (rw == 'W') {
						memw++;
					}

					cachem++;
					cache[indexbits][arr_setassocindex[indexbits]].tag = tagbits;
					if (cache[indexbits][arr_setassocindex[indexbits]].v != 1) {
						cache[indexbits][arr_setassocindex[indexbits]].v = 1;
					}

					arr_setassocindex[indexbits]++;

					if (strcmp(argv[3], "p1") == 0) {
						long c = a + (long)blockSize;
						indexbits = c >> offsetnum & ((1 << indexnum) - 1);
						tagbits = c >> (offsetnum + indexnum);

						for (j = 0; j < assoc; j++) {
							if (tagbits == cache[indexbits][j].tag && cache[indexbits][j].v == 1) {
								//Hit
								break;
							}

							if (j == (assoc - 1)) {	// Miss
								memr++;

								if (arr_setassocindex[indexbits] == assoc) {
									arr_setassocindex[indexbits] = 0;
								}

								cache[indexbits][arr_setassocindex[indexbits]].tag = tagbits;
								if (cache[indexbits][arr_setassocindex[indexbits]].v != 1) {
									cache[indexbits][arr_setassocindex[indexbits]].v = 1;
								}

								arr_setassocindex[indexbits]++;
							}
						}
					}
				}
			}

		}

		/*printf("Address: %lu \n", a);
		printf("Index bits: %lu\n", indexbits);
		printf("Tag bits: %lu\n", tagbits);
		printf("\n"); */
	}

	printf("Memory reads: %d\n", memr);
	printf("Memory writes: %d\n", memw);
	printf("Cache hits: %d\n", cacheh);
	printf("Cache misses: %d\n", cachem);

	for (i = 0; i < sets; i++) {
		free(cache[i]);
	}
	free(cache);
	free(arr_setassocindex);

	return 0;
}