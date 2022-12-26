/*******************************************************************
* AUTHOR: Kade McGarraghy                                          *
* LAST MODIFIED: 10/5/20                                           *
* FILE NAME: file_reader.c                                         *
* PURPOSE: To read file of lift request and store them in an array *
*******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lift_sim.h"
#include "file_reader.h"

int readInputFile (Request* requests, int* n)
{
	FILE* inputFile;
    int success = TRUE;
    int from;
    int to;

	inputFile = fopen("sim_input.txt", "r");

	
    if (inputFile == NULL)
    {
        perror("\nError: Could not open your sim input file");
		success = FALSE;
    }
    else
    {		
		while(fscanf(inputFile, "%d %d", &from, &to) == 2)
		{
			if ((from >= 1) && (from <= 20) && (to >= 1) && (to <= 20))
			{
				requests[*n].from = from;
				requests[*n].to = to;
				*n = *n + 1;
			}
		}
		
        if (*n < 50)
        {
            printf("\nError: 50 or more simulated requests required.\n");
            success = FALSE;
        }
        else if (*n > 100)
        {
            printf("\nError: There is a maximum of 100 simulated requests.\n");
            success = FALSE;
        }
		
		if (ferror(inputFile))
        {
            perror("\nError reading from file");
            success = FALSE;
        }

        fclose(inputFile);
	}
	return success;
}
