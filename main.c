//
// Created by alfylinux on 27/09/18.
//

#include <stdio.h>
#include <stdlib.h>

#include "mexData.h"

conversation *c;
void help ();

int main(int argc, char* argv[])
{
	if(argc >=2)
	{
		c = initConv(argv[1], atoi(argv[2]));
		if(c==NULL)
		{
			printf("Errore nell'aprire la conversazione\n");
			exit(-1);
		}
	}

	if(argc>=3) //seleziona comando
	{
		if (strcmp(argv[2], "p") == 0)
		{
			printConv(c);
			return 0;
		}
		if (strcmp(argv[2], "cp") == 0)
		{
			convRam *cp = copyConv(c);
			printConvRam(cp);
			printf("[][][][][][][][][][][][][][][][][][][][][][][][][][][]");
			printConv(c);
			return 0;
		}

	}
	if(argc>=4)
	{
		/*
		if (strcmp(argv[2], "s") == 0)
		{
			printf("name Search {%s} is at index: %d\n",argv[3],searchFirstEntry(t,argv[3]));
			return 0;
		}
		 */
	}
	if(argc>=5)
	{
		if(strcmp(argv[2],"a")==0)
		{
			mex *m=makeMex(argv[3],atoi(argv[4]));
			if(m==NULL)
			{
				perror("Error make Mex:");
				exit(-1);
			}
			if(addMex(c, m))
			{
				perror("add entry take error:");
				return -1;
			}
			printf("%s [%d] succesfull add and write\n",argv[3],atoi(argv[4]));

			return 0;
		}
	}

	help();

	fclose(c->stream);
	freeConv(c);
	return 0;
}

void help()
{
	printf("Comandi disponibili:\n");
	printf("-> [fileName] [parameter] [argv] ...\n");
	printf("Parameter List:");
	printf("\n(0)arg\n");
	printf("\tp\t\tPrinta il file indicato\n");
	printf("\tcp \tCopia e Printa il file indicato\n");

	/*
	printf("(1)arg\n");
	printf("\td [index]\tElimina nel file la entry indicata\n");
	printf("\ts [Name]\tCerca a quale entry si trova il nome cercato\n");
	*/
	printf("(2)arg\n");
	printf("\ta [Text Message] [usId]\tAggiunge una entry con questi parametri alla Tabella\n");

}