#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#ifdef _WIN32
#include <io.h>
#endif
#include "RappelzDecrypt.h"

#define BUFFER_SIZE 65536
int main(int argc, char *argv[])
{
    FILE *fin, *fout;
    char *buffer;
    int nbRead;
	int decryptPtr;

	buffer = (char*) malloc(BUFFER_SIZE);
	if(!buffer) {
		fprintf(stderr, "Not enough memory\n");
		return 1;
	}

	if(argc < 2) {
		fin = stdin;
		fout = stdout;
	} else if(argc < 3) {
		fin = fopen(argv[1], "rb");
		fout = stdout;
	} else {
		fin = fopen(argv[1], "rb");
		fout = fopen(argv[2], "wb");
	}

#ifdef _WIN32
  setmode(fileno(stdout), O_BINARY);
  setmode(fileno(stdin), O_BINARY);
#endif

	decryptPtr = 0;
    while((nbRead = fread(buffer, 1, BUFFER_SIZE, fin)) > 0) {
		decryptPtr = decryptRappelzData(buffer, buffer, nbRead, decryptPtr);
		fwrite(buffer, 1, nbRead, fout);
    }

    if(fin != stdin) fclose(fin);
    if(fout != stdout) fclose(fout);
	free(buffer);
    return 0;
}
