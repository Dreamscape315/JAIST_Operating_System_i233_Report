#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "banggen.inc"

#define	MAXBANG	4096
#define MAXTHREADS 128
#define dprintf(x) printf x
struct fentry {
	int	blkno;	// First block for this file
	int	nblks;	// # of blocks for this file
} files[MAXTHREADS][MAXBANG];

int nfile[MAXTHREADS];

typedef int (*fp_alloc_t)(int);
typedef int (*fp_free_t)(int, int);
typedef void (*fp_vinit_t)(void);
typedef void (*fp_vref_t)(int, int);
typedef void (*fp_vchk_t)(void);

struct {
	int nbang;
	fp_alloc_t fp_alloc;
	fp_free_t fp_free;
	fp_vinit_t fp_vinit;
	fp_vref_t fp_vref;
	fp_vchk_t fp_vchk;
} ssbv;

void
_ssbanger_init(int nbang, fp_alloc_t fp_alloc, fp_free_t fp_free,
		fp_vinit_t fp_vinit, fp_vref_t fp_vref, fp_vchk_t fp_vchk)
{
	int th, fe;

	ssbv.nbang = nbang;
	ssbv.fp_alloc = fp_alloc;
	ssbv.fp_free = fp_free;
	ssbv.fp_vinit = fp_vinit;
	ssbv.fp_vref = fp_vref;
	ssbv.fp_vchk = fp_vchk;

	for (th = 0 ; th < MAXTHREADS ; th++) {
		for (fe = 0 ; fe < MAXBANG ; fe++) {
			files[th][fe].blkno = -1;
		}
		nfile[th] = 0;
	}
}

void
_ssbanger(int th)
{
	int i;
	int fe;
	uint32_t nb;
	int nallocreq = 0;
	int nallocok = 0;
	int nallocng = 0;

	srand(0);

	for (i = 0 ; i < ssbv.nbang ; i++) {
		int aorf;
		aorf = rand();
		aorf = ((aorf >> 24) & 0xff)
			^ ((aorf >> 16) & 0xff)
			^ ((aorf >> 8) & 0xff)
			^ ((aorf >> 0) & 0xff);

		if (aorf >= 64) {
			// Alloc
			if (nfile[th] == MAXBANG)
				continue;

			nallocreq++;

			// Find an empty slot in files[];
			for (fe = 0 ; fe < MAXBANG ; fe++) {
				if (files[th][fe].blkno < 0)
					break;
			}
			if (fe == MAXBANG) {
				// Panic; nfile[th] != MAXBANG but no free in files[].
				fprintf(stderr, "ssbanger: panic: no free files[] entry\n");
				exit(1);
			}

			// Calculate request size
			do {
				nb = banggen(0.0, 0.5, 4);
			} while (nb == 0);

			dprintf(("ssbanger(%d): a %d\n", nallocreq, nb));

			if ((files[th][fe].blkno = (*ssbv.fp_alloc)(nb)) >= 0) { 
				files[th][fe].nblks = nb;
				printf("ssbanger: A %d %d\n", files[th][fe].blkno, files[th][fe].nblks);
				nfile[th]++;
				nallocok++;
			} else {
				printf("ssbanger: A failed for %d\n", nb);
				nallocng++;
			}
		} else {
			// Free
			if (nfile[th] == 0)
				continue;

			// Select an entry to free
			int re;
			re = rand() % nfile[th];

			// printf("ssbanger: re %d\n", re);

			for (fe = 0 ; fe < MAXBANG ; fe++) {
				if (files[th][fe].blkno >= 0) {
					if (--re < 0) {
						// This is the entry to free
						dprintf(("ssbanger: f %d %d\n", files[th][fe].blkno, files[th][fe].nblks));
						(*ssbv.fp_free)(files[th][fe].blkno, files[th][fe].nblks);
						printf("ssbanger: F %d %d\n", files[th][fe].blkno, files[th][fe].nblks);
						nfile[th]--;
						files[th][fe].blkno = -1;
						break;
					}
				}
			}

			if (fe == MAXBANG) {
				fprintf(stderr, "ssbanger: panic: failed to select a release entry\n");
				exit(1);
			}
		}
	}

	printf("ssbanger: total %d allocreq %d success %d fail %d\n", ssbv.nbang, nallocreq, nallocok, nallocng);
}

void
_ssbanger_finalize(int nthreads)
{
	int th, fe;

#if 1
	// Debuggin'

	printf("ssbanger: dumping\n");

	for (th = 0 ; th < nthreads ; th++) {
		for (fe = 0 ; fe < MAXBANG ; fe++) {
			if (files[th][fe].blkno < 0)
				continue;
			printf("[%d] %d %d\n", fe, files[th][fe].blkno, files[th][fe].nblks);
		}
	}
#endif

	if (ssbv.fp_vinit && ssbv.fp_vref && ssbv.fp_vchk) {
		(*ssbv.fp_vinit)();
		for (th = 0 ; th < nthreads ; th++) {
			for (fe = 0 ; fe < MAXBANG ; fe++) {
				if (files[th][fe].blkno >= 0)
					(*ssbv.fp_vref)(files[th][fe].blkno, files[th][fe].nblks);
			}
		}
		(*ssbv.fp_vchk)();
	}
}

void
ssbanger(int nbang, fp_alloc_t fp_alloc, fp_free_t fp_free,
		fp_vinit_t fp_vinit, fp_vref_t fp_vref, fp_vchk_t fp_vchk)
{
	_ssbanger_init(nbang, fp_alloc, fp_free, fp_vinit, fp_vref, fp_vchk);
	_ssbanger(0);
	_ssbanger_finalize(1);
}

struct tentry {
	int tnum;
	pthread_t tid;
} threads[MAXTHREADS];























void *
_ssbanger_mt(void *args)
{
	struct tentry *thread = args;
	_ssbanger(thread->tnum);
	return NULL;
}





















void
ssbanger_mt(int nbang, int nthreads, fp_alloc_t fp_alloc, fp_free_t fp_free,
		fp_vinit_t fp_vinit, fp_vref_t fp_vref, fp_vchk_t fp_vchk)
{
	int th;

	_ssbanger_init(nbang / nthreads, fp_alloc, fp_free, fp_vinit, fp_vref, fp_vchk);
	for (th = 0; th < nthreads; th++) {
		threads[th].tnum = th;
		pthread_create(&threads[th].tid, NULL, _ssbanger_mt, &threads[th]);
	}

	for (th = 0; th < nthreads; th++) {
		pthread_join(threads[th].tid, NULL);
	}

	_ssbanger_finalize(nthreads);
}
