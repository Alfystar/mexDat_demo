//
// Created by alfylinux on 27/09/18.
//

#include <stdio.h>
#include <stdlib.h>

#include "mexData.h"

conversation *c;

int main(int argc, char* argv[])
{
	if(argc >=2)
	{
		c=initConv(argv[1],atoi(argv[2]));
		printConv(c);


		/*conversation conv;
		convInfo cI;
		mex m;

		m.info.timeM=currTimeSys();
		m.info.usId=5;
		m.text=NULL;
		m.next=NULL;
		printf("print Mex\n");
		printMex(&m);

		cI.adminId=65;
		cI.nMex=2;
		cI.timeCreate=currTimeSys();
		printf("\nprint convInfo\n");
		printConvInfo(&cI);

		conv.head.timeCreate=currTimeSys();
		conv.head.nMex=0;
		conv.head.adminId=5;
		conv.mexList=NULL;
		printf("\nprint Conversation\n");
		printConv(&conv);
		 */
		return 0;

	}
	return 0;
}