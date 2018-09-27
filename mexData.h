//
// Created by alfylinux on 27/09/18.
//

#ifndef MEXDAT_DEMO_MEXDATA_H
#define MEXDAT_DEMO_MEXDATA_H

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <string.h>

typedef struct mex_{
	int UsId;
	time_t timeM;
	char* text;
}mex;

typedef struct conversation_{
	int nMex;
	mex *conv
}conversation;


///Prototipi
time_t currTimeSys();
char * timeString(time_t *t);
int saveMexF(mex *m, FILE *stream);
int fWriteMex(FILE *stream,size_t sizeElem, int nelem,void *data);





#endif //MEXDAT_DEMO_MEXDATA_H

/***************************************************************/
/*Funzioni per riconoscere/creare/rimuovere pacchetti messaggio*/

/*
 * La struttura del pacchetto è del tipo:
 *
 * (i numeri sono i valori decimali dell'equivalente ascii)
 *
 * /---|----|-----|---|-----------|----|---\
 * | 1 | Id | Ora | 2 |   TEXT    |'\0'| 3 |
 * \---|----|-----|---|-----------|----|---/
 *
 * -1: in ascii indica "Start Of Heading"
 * -2: in ascii indica "Start Of Text"
 * -3: in ascii indica "End Of Text"
 *
 * (Opzionale)
 * -4: in ascii indica "End of Trasmission"
 */