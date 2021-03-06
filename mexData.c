//
// Created by alfylinux on 27/09/18.
//

#include "mexData.h"

///Funzioni di interfaccia

conversation *initConv(char *path,int adminId)
{
	conversation c;
	c.stream= openConfStream(path);
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
//todo da testare
conversation *openConf(char * convPath)
{
	FILE *f=openConfStream(convPath);
	if (f == NULL) {
		perror("tab open error:");
		return NULL;
	}
	return loadConvF(f);
}


FILE *openConfStream(char *path)
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

convRam* copyConv (conversation *c)
{
	convRam *cR = calloc(1, sizeof(convRam));
	if (!cR)
	{
		return NULL;
	}
	memcpy(&cR->head, &c->head, sizeof(convInfo));
	cR->mexList = calloc(c->head.nMex, sizeof(mex *));
	if (!cR->mexList)
	{
		free(cR);
		return NULL;
	}
	mex *mexNode;

	for(int i=0;i<cR->head.nMex;i++)
	{
		///genero un nuovo nodo dei messaggi in ram
		mexNode = malloc(sizeof(mex));
		if (!mexNode)
		{

			for(int j=0; j<i; j++)
			{
				freeMex(cR->mexList[j]);
			}
			free(cR->mexList);
			free(cR);

			return NULL;
		}
		///Copio i metadati
		memcpy(mexNode,&c->mexList[i]->info, sizeof(mexInfo));

		///Crea in ram la stringa di dim arbitraria e mex la punta
		mexNode->text = malloc(strlen(c->mexList[i]->text)+1);
		if (!mexNode->text)
		{
			for(int j=0; j<i; j++)
			{
				freeMex(cR->mexList[j]);
			}
			free(cR->mexList);
			free(cR);
			return NULL;
		}
		strcpy(mexNode->text,c->mexList[i]->text);

		cR->mexList[i] = mexNode;   //salvo il puntatore nell'array
	}
	return cR;
}

int addMex(conversation *c, mex *m)
{
	if(saveNewMexF(m, c->stream))
	{
		perror("error during write :");
		return -1;
	}
	c->head.nMex++;
	c->mexList=reallocarray(c->mexList,c->head.nMex, sizeof(mex *));
	if(overrideHeadF(&c->head,c->stream))
	{
		return -1;
	}
	return 0;
}

mex *makeMex(char *text,int usId)
{
	/// text su un buf temporaneo
	mex *m=malloc(sizeof(mex));
	if (m==NULL)
	{
		return NULL;
	}
	m->info.usId=usId;
	m->info.timeM=currTimeSys();
	m->text=malloc(strlen(text)+1);
	strcpy(m->text,text);
	return m;
}

int freeConv(conversation *c)
{

	//libero tutti i messaggi
	for(int i=0; i<c->head.nMex; i++)
	{
		freeMex(c->mexList[i]);
	}
	fclose(c->stream);
	free(c);    //dopo aver liberato e chiuso tutto libero la memoria
	return 0;
}

///Funzioni verso File

int setUpConvF(int adminId,FILE *stream)
{
	struct stat streamInfo;
	fstat(fileno(stream), &streamInfo);
	if(streamInfo.st_size!=0)     //il file era già esistente e contiene dei dati
	{
		errno=EEXIST; //file descriptor non valido, perchè il file contiene già qualcosa
		return -1;
	}
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

int saveNewMexF(mex *m, FILE *stream)
{
	/// Scrive in maniera atomica rispetto al Processo
	size_t lenText =strlen(m->text)+1;

	flockfile(stream);
	fseek(stream,0,SEEK_END); //mi porto alla fine per aggiungere

	if(fWriteF(stream, sizeof(m->info),1,&m->info))
	{
		funlockfile(stream);
		return -1;
	}
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
		printf("File con solo testa\n");
		return conv;
	}
	conv->mexList=calloc(conv->head.nMex, sizeof(mex *));   //creo un array di puntatori a mex
	mex *mexNode;
	size_t len;
	for(int i=0;i<conv->head.nMex;i++)
	{
		///genero un nuovo nodo dei messaggi in ram
		mexNode=malloc(sizeof(mex));

		///Copio i metadati
		memcpy(mexNode,dataPoint, sizeof(mexInfo));
		dataPoint +=sizeof(mexInfo);
		///Crea in ram la stringa di dim arbitraria e mex la punta
		len=strlen(dataPoint)+1;
		mexNode->text=malloc(len);
		strcpy(mexNode->text,dataPoint);
		dataPoint+=len;

		conv->mexList[i]=mexNode;   //salvo il puntatore nell'array
		/*
		printf("\nil nuovo messaggio creato è:\n");
		printMex(mexNode);
		*/
	}
	return conv;
}

///Funzioni di supporto

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
		//printf("prima fwrite; dat=%p\n",dat);
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

int freeMex(mex *m)
{
	free(m->text);
	free(m);
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

///Funzioni di visualizzazione

void printConv(conversation *c)
{
	printf("-------------------------------------------------------------\n");
	printf("\tLa Conversazione ha salvati i seguenti messaggi:\n");
	printf("\tsizeof(mex)=%ld\tsizeof(mexInfo)=%ld\tsizeof(convInfo)=%ld\n",sizeof(mex),sizeof(mexInfo),sizeof(convInfo));
	printf("FILE stream pointer\t-> %p\n",c->stream);
	printf("\n\t[][]La Conversazione è:[][]\n\n");
	printConvInfo(&c->head);

	printf("##########\n\n");

	for(int i=0; i<c->head.nMex; i++)
	{
		printf("--->Mex[%d]:\n",i);
		printMex(c->mexList[i]);
		printf("**********\n");
	}
	printf("-------------------------------------------------------------\n");
	return;
}

void printConvRam(convRam *c)
{
	printf("-------------------------------------------------------------\n");
	printf("\tLa Conversazione ha salvati i seguenti messaggi:\n");
	printf("\tsizeof(mex)=%ld\tsizeof(mexInfo)=%ld\tsizeof(convInfo)=%ld\n",sizeof(mex),sizeof(mexInfo),sizeof(convInfo));
	printf("FILE stream pointer\t-> NULL (only ram version)\n");
	printf("\n\t[][]La Conversazione è:[][]\n\n");
	printConvInfo(&c->head);

	printf("##########\n\n");

	for(int i=0; i<c->head.nMex; i++)
	{
		printf("--->Mex[%d]:\n",i);
		printMex(c->mexList[i]);
		printf("**********\n");
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
	printf("Mex data Store locate in=%p:\n",m);
	printf("info.usId\t-> %d\n",m->info.usId);
	printf("time Message\t-> %s",timeString(m->info.timeM));
	if(m->text!=NULL)
	{
		printf("Text:\n-->  %s\n",m->text);
	} else{
		printf("Text: ##Non Presente##\n");
	}
}

void printConvInfo(convInfo *cI)
{
	/*
	cI->nMex
	cI->adminId
	cI->timeCreate
	*/
	printf("#1\tConversation info data Store:\n");
	printf("nMess\t\t-> %d\n",cI->nMex);
	printf("adminId\t\t-> %d\n",cI->adminId);
	printf("Time Creat\t-> %s\n",timeString(cI->timeCreate));
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