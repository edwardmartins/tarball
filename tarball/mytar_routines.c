#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "mytar.h"

extern char *use;

/** Copy nBytes bytes from the origin file to the destination file.
 *
 * origin: pointer to the FILE descriptor associated with the origin file
 * destination:  pointer to the FILE descriptor associated with the destination file
 * nBytes: number of bytes to copy
 *
 * Returns the number of bytes actually copied.
 */
int copynFile(FILE * origin, FILE * destination, int nBytes)
{
	int cont = 0;
	int c;

	while(cont < nBytes && (c = getc(origin)) != EOF){
		putc((unsigned char)c,destination);
		cont++;
	}
	return cont;
}

/** Loads a string from a file.
 *
 * file: pointer to the FILE descriptor 
 * 
 * The loadstr() function must allocate memory from the heap to store 
 * the contents of the string read from the FILE. 
 * Once the string has been properly built in memory, the function returns
 * the starting address of the string (pointer returned by malloc()) 
 * 
 * Returns: !=NULL if success, NULL if error
 */
char *loadstr(FILE * file)
{
    // Count the number of bytes of the string
    int numBytes = 0;
    char c;
    while(( c = getc(file)) != '\0' && c != EOF){
        numBytes++;
    }

    if(c == EOF)
    	return NULL;

    // +1 to add byte '\0'
    numBytes++;

    // File pointer to the beginning of the file
    fseek(file,-numBytes,SEEK_CUR);

    // Reserve memory for the string
    char *str;
    if((str = malloc (sizeof(char) * numBytes)) == NULL){
    	return NULL;
    }

    // Read the string and fills str
    fread(str,sizeof(char),numBytes,file);

    return str;

}
/** Read tarball header and store it in memory.
 *
 * tarFile: pointer to the tarball's FILE descriptor 
 * nFiles: output parameter. Used to return the number 
 * of files stored in the tarball archive (first 4 bytes of the header).
 *
 * On success it returns the starting memory address of an array that stores 
 * the (name,size) pairs read from the tar file. Upon failure, the function returns NULL.
 */

stHeaderEntry* readHeader(FILE *tarFile, int *nFiles)
{

	// Read the number of files of the tar
	fread(nFiles,sizeof(int),1,tarFile);

	// Reserve memory for the header
	stHeaderEntry *header;
	if((header=  malloc(sizeof(stHeaderEntry) * (*nFiles))) == NULL){
		perror("cannot reserve memory for header");
		fclose(tarFile);
		return NULL;
	}

	int i,j;
	for( i = 0; i < *nFiles; i++){
		// Read the name of each file
		if((header[i].name = loadstr(tarFile)) == NULL){
			for(j = 0; j < *nFiles; j++){
				free(header[j].name);
			}
			free(header);
			fclose(tarFile);
			return NULL;
		}
		// Read the size of each file
		fread(&header[i].size,sizeof(unsigned int),1,tarFile);
	}

	return header;
}

/** Creates a tarball archive 
 *
 * nfiles: number of files to be stored in the tarball
 * filenames: array with the path names of the files to be included in the tarball
 * tarname: name of the tarball archive
 * 
 * On success, it returns EXIT_SUCCESS; upon error it returns EXIT_FAILURE. 
 * (macros defined in stdlib.h).
 *
 * HINTS: First reserve room in the file to store the tarball header.
 * Move the file's position indicator to the data section (skip the header)
 * and dump the contents of the source files (one by one) in the tarball archive. 
 * At the same time, build the representation of the tarball header in memory.
 * Finally, rewind the file's position indicator, write the number of files as well as 
 * the (file name,file size) pairs in the tar archive.
 *
 * Important reminder: to calculate the room needed for the header, a simple sizeof 
 * of stHeaderEntry will not work. Bear in mind that, on disk, file names found in (name,size) 
 * pairs occupy strlen(name)+1 bytes.
 *
 */
int createTar(int nFiles, char *fileNames[], char tarName[])
{
	FILE *tar;
	stHeaderEntry *header;
	int i,j;

	// Open the tarball
	if((tar = fopen(tarName,"w")) == NULL){
		perror("Cannot open the tarFile");
		return EXIT_FAILURE;
	}

	// Reserve memory for the header
	if((header = malloc(sizeof(stHeaderEntry)* nFiles)) == NULL){
		perror("Cannot reserve memory for the header");
		return EXIT_FAILURE;
	}

	// Calculate the size of the header
	// First we add an int(represents the number of files in the tarball)
	int sizeOfNames = sizeof(int);
	int sizeOfHeader;

	// Sum of name's size
	for(i = 0; i < nFiles; i++){
		sizeOfNames += strlen(fileNames[i]) + 1;
		header[i].name = malloc(sizeof(strlen(fileNames[i]) + 1));
	}
	
	// Header = sizeNames + space to save the size of each file
	sizeOfHeader = sizeOfNames + (nFiles * sizeof(int));

	// File position to the data region
	fseek(tar,sizeOfHeader,SEEK_SET);

	// Copy the content of each file into the tar
	FILE *inputFile;
	for(j = 0; j < nFiles; j++){
		// Open each file
		if((inputFile = fopen(fileNames[j],"r")) == NULL){
			for(j = 0; j < nFiles; j++){
				free(header[j].name);
			}
			free(header);
			fclose(tar);
			perror("Cannot open inputFile");
			return EXIT_FAILURE;
		}
		// Copy the content into the tar
		int numBytes = copynFile(inputFile,tar,INT_MAX);

		// Save the header
		header[j].size = numBytes;
		strcpy(header[j].name, fileNames[j]);
		fclose(inputFile); 
	}

	// File position to the beginning of the tarball
	rewind(tar);

	// Write the number of files at the top of the tarball
	fwrite(&nFiles,sizeof(int),1,tar);

	int z;
	for(z = 0; z < nFiles; z++){

		// Write the name of each file into the tarball
		fwrite(header[z].name,sizeof(char),strlen(header[z].name) + 1 ,tar);

		// Write the size of each file into the tarball
		fwrite(&header[z].size,sizeof(unsigned int),1,tar);

	}

	// Free memory
	for(j = 0; j < nFiles; j++){
		free(header[j].name);
	}
	free(header);

	// Close
	fclose(tar);

	printf("Tarball created successfully\n");
	return EXIT_SUCCESS;
}

/** Extract files stored in a tarball archive
 *
 * tarName: tarball's pathname
 *
 * On success, it returns EXIT_SUCCESS; upon error it returns EXIT_FAILURE. 
 * (macros defined in stdlib.h).
 *
 * HINTS: First load the tarball's header into memory.
 * After reading the header, the file position indicator will be located at the 
 * tarball's data section. By using information from the 
 * header --number of files and (file name, file size) pairs--, extract files 
 * stored in the data section of the tarball.
 *
 */
int extractTar(char tarName[])
{
	FILE *tar;
	FILE *outFile;
	int i, j;

	// Open the tarball
	if((tar = fopen(tarName,"r")) == NULL){
		perror("Cannot open the tarball");
		return EXIT_FAILURE;
	}

	// Loads the header
	int numFiles;
	stHeaderEntry *header;
	if((header = readHeader(tar, &numFiles)) == NULL){
		return EXIT_FAILURE;
	}


	// Extracts the files
	for(i = 0; i < numFiles; i++){
		// Open each file
		if((outFile = fopen(header[i].name, "w")) == NULL){
			for(j = 0; j < numFiles; j++){
				free(header[j].name);
			}
			free(header);
			fclose(tar);
			return EXIT_FAILURE;
		}
		// Copy the data
		copynFile(tar,outFile,header[i].size);
		fclose(outFile);
	}

	// Free memory
	for(j = 0; j < numFiles; j++){
		free(header[j].name);
	}
	free(header);

	// Close
	fclose(tar);

	printf("Tarball extracted successfully\n");
	return EXIT_SUCCESS;
}
