/*	
	Alex Ryse
	Title Case Counter
	MPI program that determines the frequencies of title-cased words in a text file
*/

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<ctype.h>
#include<mpi.h>

#define FILE_NAME_LEN 100 /* Max length for name of text file */

/* Helper function countUpper to count the number of uppercase letters */
int countUpper(const char* buffer, int size) {
	int counter = 0;
	for (int i = 0; i < size; i++) {
		if (isupper(buffer[i])) {
			counter++;
		}
	}
	return counter;
}

int main(int argc, char* argv[]) {

	/* Global variables here */
	double elapsed_time;			/* Parallel execution time */
	int    id;							/* Process ID number */
	int    p;							/* Number of processes */
	MPI_File	file;					/* File pointer */
	MPI_Info	info = MPI_INFO_NULL;	/* Info */
	MPI_Comm	comm = MPI_COMM_WORLD;	/* Communicator */
	int	amode = MPI_MODE_RDONLY;		/* Read only */
	char filename[FILE_NAME_LEN];		/* Store name of text file */

	MPI_Init(&argc, &argv);
	
	/* Starting the timer here */
	MPI_Comm_rank(MPI_COMM_WORLD, &id);
	MPI_Comm_size(MPI_COMM_WORLD, &p);
	MPI_Barrier(MPI_COMM_WORLD);
	elapsed_time = -MPI_Wtime();

	/* Checking the command line arguments here */
	if (argc != 2) {
		if (id == 0) {
			printf("Command line: %s <filename>\n", argv[0]);
			MPI_Finalize();
			return 1;
		}
	}

	/* Copying the command line text file to filename */
	strncpy_s(filename, FILE_NAME_LEN, argv[1], FILE_NAME_LEN - 1);
	filename[FILE_NAME_LEN - 1] = '\0'; /* Adding the null terminator */

	/* Opening the text file */
	int fileOpen= MPI_File_open(comm, filename, amode, info, &file);

	/* Checking the file open status here */
	if (fileOpen != MPI_SUCCESS) {
		if (id == 0) {
			printf("Error opening the file: %s\n", filename);
		}
		MPI_Abort(comm, fileOpen);
	}

	/* Getting the size of the text file here */
	MPI_Offset fileSize;
	MPI_File_get_size(file, &fileSize);

	/* Calculating how much of text file that each process will read */
	int sectionSize = fileSize / p; /* Each section is n / p (where n is size of text file) */
	int remaining = fileSize % p; /* Remainder from n / p calculation */

	/* Start and end indicate the starting and ending byte offset in the file for the current process */
	int start = id * sectionSize + (id < remaining ? id : remaining);
	int end = start + sectionSize + (id < remaining ? 1 : 0);

	/* Calculating the buffer to read the current section of the text file */ 
	char* buffer = (char*)malloc((end - start) * sizeof(char));

	/* Reading the current section of the file */
	MPI_File_read_at(file, start, buffer, end - start, MPI_CHAR, MPI_STATUS_IGNORE);

	/* Counting the number of uppercase in the buffer */
	int currCount = countUpper(buffer, end - start);

	/* Reducing all counts to the root process (process 0) */
	int totalCount;
	MPI_Reduce(&currCount, &totalCount, 1, MPI_INT, MPI_SUM, 0, comm);

	/* Stopping the timer here */
	elapsed_time += MPI_Wtime();

	/* Diplaying the total count and execution time here */
	if (id == 0) {
		printf("Total count: %d\n", totalCount);
		printf("Total execution time: %f seconds\n", elapsed_time);
	}

	free(buffer); /* Deallocate the buffer */
	MPI_File_close(&file); /* Close the file pointer */
	MPI_Finalize(); /* Ending the MPI communications */

	return 0;
}
