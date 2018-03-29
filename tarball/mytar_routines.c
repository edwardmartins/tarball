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
 * Returns the number of bytes actually copied or -1 if an error occured.
 */
int
copynFile(FILE * origin, FILE * destination, int nBytes)
{
	int contador = 0;
	int c;

	while(contador < nBytes && (c = getc(origin)) != EOF){
		putc((unsigned char)c,destination);
		contador++;
	}
	return contador;
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
char
*loadstr(FILE * file)
{
    // contamos el numero de bytes del string
    int numBytes = 0;
    char c;
    while(( c = getc(file)) != '\0' && c != EOF){
        numBytes++;
    }

    if(c == EOF)
    	return NULL;

    // +1 para añadir el byte '\0'
    numBytes++;

    // volvemos al inicio del fichero
    fseek(file,-numBytes,SEEK_CUR);

    // reservamos memoria para el string
    char *str;
    if((str = malloc (sizeof(char) * numBytes)) == NULL){
    	return NULL;
    }

    // cargamos el string
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

	// leemos el numero de archivos que tiene el tar
	fread(nFiles,sizeof(int),1,tarFile);

	// reservamos memoria para la cabecera
	stHeaderEntry *cabecera;
	if((cabecera =  malloc(sizeof(stHeaderEntry) * (*nFiles))) == NULL){
		perror("error al reservar memoria en la cabecera");
		fclose(tarFile);
		return NULL;
	}

	int i,j;
	for( i = 0; i < *nFiles; i++){
		// cargamos el nombre de cada archivo en la cabecera
		if((cabecera[i].name = loadstr(tarFile)) == NULL){
			// liberamos memoria ante un error
			for(j = 0; j < *nFiles; j++){
				free(cabecera[j].name);
			}
			free(cabecera);
			fclose(tarFile);
			return NULL;
		}
		// cargamos el tamaño de cada archivo en la cabecera
		fread(&cabecera[i].size,sizeof(unsigned int),1,tarFile);
	}

	return cabecera;
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
int
createTar(int nFiles, char *fileNames[], char tarName[])
{
	FILE *tar;
	stHeaderEntry *cabecera;
	int i,j;

	// Abrimos tarfile
	if((tar = fopen(tarName,"w")) == NULL){
		perror("error al abrir el tarFile");
		return EXIT_FAILURE;
	}

	// Reservamos espacio para la cabecera
	if((cabecera = malloc(sizeof(stHeaderEntry)* nFiles)) == NULL){
		perror("error al reservar memoria de la cabecera");
		return EXIT_FAILURE;
	}

	// Calculamos el tamaño de la cabecera para poder posicionarnos en la region de datos
	int tamNombres = sizeof(int); // primer entero que guarda el numero de ficheros
	int tamCabecera;

	// Suma de las longitudes de los nombres
	for(i = 0; i < nFiles; i++){
		tamNombres += strlen(fileNames[i]) + 1;
		cabecera[i].name = malloc(sizeof(strlen(fileNames[i]) + 1));
	}
	// Tamaño de los nombres + entero que guarda el tamaño de cada fichero
	tamCabecera = tamNombres + (nFiles * sizeof(int));

	// Nos posicionamos en el byte del fichero donde comienza la region de datos
	fseek(tar,tamCabecera,SEEK_SET);

	// Copiamos el contenido de cada fichero(inputFile) en el tar
	FILE *inputFile;
	for(j = 0; j < nFiles; j++){

		if((inputFile = fopen(fileNames[j],"r")) == NULL){
			// liberamos memoria ante un error
			for(j = 0; j < nFiles; j++){
				free(cabecera[j].name);
			}
			free(cabecera);
			fclose(tar);
			perror("error de apertura del fichero");
			return EXIT_FAILURE;
		}
		// Copia el fichero y devuelve el numero de bytes copiados
		int numBytes = copynFile(inputFile,tar,INT_MAX);

		// Cargamos la cabecera
		cabecera[j].size = numBytes; // copio el tamaño del fichero en la cabecera
		strcpy(cabecera[j].name, fileNames[j]); // copio el nombre en la cabecera
		fclose(inputFile); // cierro el fichero
	}

	// mueve el indicador de posición del fichero al inicio del tarball
	rewind(tar);

	// Escribimos el numero de ficheros
	fwrite(&nFiles,sizeof(int),1,tar);

	int z;
	for(z = 0; z < nFiles; z++){

		// Volcamos los nombres al tar
		fwrite(cabecera[z].name,sizeof(char),strlen(cabecera[z].name) + 1 ,tar);

		// Volcamos los tamaños de los archivos al tar
		fwrite(&cabecera[z].size,sizeof(unsigned int),1,tar);

	}

	// liberamos memoria
	for(j = 0; j < nFiles; j++){
		free(cabecera[j].name);
	}
	free(cabecera);

	// cerramos
	fclose(tar);

	printf("Tar creado con exito\n");
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
int
extractTar(char tarName[])
{
	FILE *tar;
	FILE *outFile;
	int i, j;

	// Abrimos el tar
	if((tar = fopen(tarName,"r")) == NULL){
		perror("No se puede abrir el fichero tar");
		return EXIT_FAILURE;
	}

	// Cargamos la cabecera
	int numFiles;
	stHeaderEntry *cabecera;
	if((cabecera = readHeader(tar, &numFiles)) == NULL){
		return EXIT_FAILURE;
	}


	// Extraemos los ficheros
	for(i = 0; i < numFiles; i++){
		if((outFile = fopen(cabecera[i].name, "w")) == NULL){
			// liberamos memoria ante un error
			for(j = 0; j < numFiles; j++){
				free(cabecera[j].name);
			}
			free(cabecera);
			fclose(tar);
			return EXIT_FAILURE;
		}
		copynFile(tar,outFile,cabecera[i].size);
		fclose(outFile);
	}

	// Liberamos memoria
	for(j = 0; j < numFiles; j++){
		free(cabecera[j].name);
	}
	free(cabecera);

	// cerramos
	fclose(tar);

	printf("Tar extraido con exito\n");
	return EXIT_SUCCESS;
}
