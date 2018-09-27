//
// Created by alfylinux on 27/09/18.
//

#include "mexData.h"

time_t currTimeSys()
{
	time_t current_time;

/* Obtain current time. */
	current_time = time(NULL);

	if (current_time == ((time_t) -1)) {
		fprintf(stderr, "Failure to obtain the current time.\n");
	}
	return current_time;
}

char * timeString(time_t *t)
{
	char * c_time_string;
/* Convert to local time format. */
	c_time_string = ctime(t);

	if (c_time_string == NULL) {
		fprintf(stderr, "Failure to convert the current time.\n");
	}
	return c_time_string;

}

int saveMexF(mex *m, FILE *stream)
{
	/// Scrive in maniera atomica rispetto al Processo
	int lenText =strlen(m->text)+1;
	char tag[3]={1,2,3};

	/*
	 * -1: in ascii indica "Start Of Heading"
	 * -2: in ascii indica "Start Of Text"
	 * -3: in ascii indica "End Of Text"
	 */
	flockfile(stream);
	fseek(stream,0,SEEK_END); //mi porto alla fine per aggiungere
	///
	if(fWriteMex(stream, 1,1,&tag[0]))
	{
		funlockfile(stream);
		return -1;
	}
	if(fWriteMex(stream, sizeof(m->UsId),1,m->UsId))
	{
		funlockfile(stream);
		return -1;
	}
	///
	if(fWriteMex(stream, 1,1,&tag[1]))
	{
		funlockfile(stream);
		return -1;
	}
	if(fWriteMex(stream, sizeof(m->timeM),1,m->timeM))
	{
		funlockfile(stream);
		return -1;
	}
	///
	if(fWriteMex(stream, 1,1,&tag[2]))
	{
		funlockfile(stream);
		return -1;
	}
	if(fWriteMex(stream, lenText,1,m->text))
	{
		funlockfile(stream);
		return -1;
	}
	funlockfile(stream);
	return 0;
}

conversation *loadConvF(FILE *stream)
{
	flockfile(stream);


}

int fWriteMex(FILE *f,size_t sizeElem, int nelem,void *dat)
{
	fflush(f);   /// NECESSARIO SE I USA LA MODALITA +, serve a garantire la sincronia tra R/W
	size_t cont=0;
	while(cont != sizeElem*nelem)
	{
		if(ferror(f)!=0)    // testo solo per errori perchè in scrittura l'endOfFile Cresce
		{
			// è presente un errore in scrittura
			errno=EBADFD;   //file descriptor in bad state
			return -1;
		}
		cont += fwrite(dat, 1, sizeElem*nelem, f);
	}
	return 0;
}

