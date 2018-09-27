//
// Created by alfylinux on 27/09/18.
//

#include "mexData.h"

//Funzioni di interfaccia

conversation *initConv(char *path,int adminId)
{
	conversation c;
	c.stream=openConf(path);
	if(setUpConvF(adminId,c.stream))
	{
		char buf[128]; // buff per creare la tringa di errore dinamicamente
		switch (errno)
		{
			case EEXIST:
				//tutto ok, posso andare avanti, è una semplice apertura
				break;
			default:

				sprintf(buf,"open file %s, take error:",path);
				perror(buf);
				return NULL;
				break;
		}
	}

	return loadConvF(c.stream);
}

FILE *openConf(char* path)
{
	int confFd=open(path,O_RDWR|O_CREAT,0666);
	if(confFd==-1)
	{
		perror("open FD for Tab take error:");
		return NULL;
	}
	FILE *f= fdopen(confFd,"r+");
	if(f==NULL)
	{
		perror("tab open error:");
		return NULL;
	}
	return f;

}

int setUpConvF(int adminId,FILE *stream)
{
	conversation newCon;
	newCon.head.adminId=adminId;
	newCon.head.nMex=0;
	newCon.head.timeCreate=currTimeSys();
	overrideHeadF(&newCon.head,stream);
	return 0;
}

int overrideHeadF(convInfo *cI, FILE *stream)
{
	flockfile(stream);
	rewind(stream);
	if(fWriteF(stream, sizeof(convInfo),1,cI))
	{
		funlockfile(stream);
		perror("fwrite fail in overrideHeadF");
		return -1;
	}

	funlockfile(stream);

	return 0;
}

int saveMexF(mex *m, FILE *stream)
{
	/// Scrive in maniera atomica rispetto al Processo
	int lenText =strlen(m->text)+1;
	//char tag[4]={1,2,3,4};

	/*
	 * -1: in ascii indica "Start Of Heading"
	 * -2: in ascii indica "Start Of Text"
	 * -3: in ascii indica "End Of Text"
	 */
	flockfile(stream);
	fseek(stream,0,SEEK_END); //mi porto alla fine per aggiungere
	/*
	///start Header tag
	if(fWriteF(stream, 1,1,&tag[0]))
	{
		funlockfile(stream);
		return -1;
	}
	*/
	if(fWriteF(stream, sizeof(m->info.usId),1,m->info.usId))
	{
		funlockfile(stream);
		return -1;
	}

	if(fWriteF(stream, sizeof(m->info.timeM),1,m->info.timeM))
	{
		funlockfile(stream);
		return -1;
	}
	/*
	/// start mesage tag
	if(fWriteF(stream, 1,1,&tag[1]))
	{
		funlockfile(stream);
		return -1;
	}
	 */
	if(fWriteF(stream, lenText,1,m->text))
	{
		funlockfile(stream);
		return -1;
	}
	funlockfile(stream);
	return 0;
}

conversation *loadConvF(FILE *stream)
{
	conversation *conv=malloc(sizeof(conversation));
	conv->stream=stream;
	///accedo al file e lo copio in un buffer in blocco
	flockfile(stream);
	fflush(stream);
	struct stat streamInfo;
	fstat(fileno(stream), &streamInfo);
	if(streamInfo.st_size==0)
	{
		funlockfile(stream);
		conv->head.nMex=0;
		conv->mexList=NULL;
		return conv;
	}
	char buf[streamInfo.st_size];
	rewind(stream);
	fReadF(stream,1,streamInfo.st_size,buf);
	funlockfile(stream);
	///accesso eseguito inizio il compattamento dati

	void *dataPoint;

	///copio i dati di testa

	memcpy(&conv->head,buf, sizeof(conv->head));
	dataPoint=buf+sizeof(conv->head);
	if(streamInfo.st_size== sizeof(conv->head))
	{
		//non sono presenti messaggi e ho una conversazione vuota
		return conv;
	}

	size_t len;
	mex *mexNode=malloc(sizeof(mex));
	mex *mexNodeOld;
	conv->mexList=mexNode;

	///Copio i metadati
	memcpy(mexNode,dataPoint, sizeof(mexInfo));
	dataPoint +=sizeof(mexInfo);
	///Crea in ram la stringa di dim arbitraria e mex la punta
	len=strlen(dataPoint)+1;
	mexNode->text=malloc(len);
	strcpy(mexNode->text,dataPoint);
	dataPoint+=len;
	mexNodeOld=mexNode;

	for(int i=1;i<conv->head.nMex;i++)
	{
		///genero un nuovo nodo della lista
		mexNode=malloc(sizeof(mex));

		///Copio i metadati
		memcpy(mexNode,dataPoint, sizeof(mexInfo));
		dataPoint +=sizeof(mexInfo);
		///Crea in ram la stringa di dim arbitraria e mex la punta
		len=strlen(dataPoint)+1;
		mexNode->text=malloc(len);
		strcpy(mexNode->text,dataPoint);
		dataPoint+=len;
		//la vecchia punta la nuova
		mexNodeOld->next=mexNode;
		//la nuova diventa vecchia
		mexNodeOld=mexNode;
	}
	mexNode->next=0;
	return conv;

}

//Funzioni di supporto

int fWriteF(FILE *f,size_t sizeElem, int nelem,void *dat)
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
		cont += fwrite(dat+cont, 1, sizeElem*nelem-cont, f);
	}
	return 0;
}

int fReadF(FILE *f,size_t sizeElem, int nelem,void *save)
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
		cont += fread(save+cont, 1, sizeElem*nelem-cont, f);
	}
	return 0;
}

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

//Funzioni di visualizzazione

void printConv(conversation *c)
{
	printf("-------------------------------------------------------------\n");
	printf("\tLa Conversazione ha salvati i seguenti messaggi:\n");
	printf("\tsizeof(mex)=%d\tsizeof(mexInfo)=%d\tsizeof(convInfo)=%d\n",sizeof(mex),sizeof(mexInfo),sizeof(convInfo));
	printf("\n\t[][]La Conversazione è:[][]\n\n");
	printConvInfo(&c->head);

	mex *currMex=c->mexList;
	printf("##########\n\n");

	for(int i=0; currMex!=NULL; currMex=currMex->next)
	{
		printf("--->Mex[%d]:",i);
		printMex(currMex);
		printf("**********\n");
		i++;
	}
	printf("-------------------------------------------------------------\n");
	return;
}

void printMex(mex *m)
{
	/*
	m->text
	m->info.usId
	m->info.timeM
	m->next
	 */
	printf("Mex data Store:\n");
	printf("info.usId\t\t-> %d\n",m->info.usId);
	printf("timeM\t\t-> %s",timeString(m->info.timeM));
	printf("Next\t\t-> %p\n",m->next);
	if(m->text!=NULL)
	{
		printf("Text:\n%s\n",m->text);
	} else{
		printf("Text: ##Non Presente##\n");
	}


	return;
}

void printConvInfo(convInfo *cI)
{
	/*
	cI->nMex
	cI->adminId
	cI->timeCreate
	*/
	printf("#1\tConversation info data Store:\nnMess\t\t-> %d\nadminId\t\t-> %d\nTime Creat\t-> %s\n",cI->nMex,cI->adminId,timeString(cI->timeCreate));
	return;
}


char * timeString(time_t t)
{
	char * c_time_string;
/* Convert to local time format. */
	//printf("ctime before\n");
	c_time_string = ctime(&t);   /// è thread safe, ha una memoria interna
	//printf("ctime after\n");
	if (c_time_string == NULL) {
		fprintf(stderr, "Failure to convert the current time.\n");
	}
	return c_time_string;

}