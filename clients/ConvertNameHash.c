#include <stdio.h>
#include <string.h>

#include "NameToHash.h"
#include "HashToName.h"


#define OPT_CONVERT_NAMETOHASH 1
#define OPT_CONVERT_HASHTONAME 2

int main(int argc, char *argv[]) {
	char input[500];
	char output[500];
	int flags = 0, i;

	flags = 0;
	for(i=1; i<argc; i++) {
		if(!strcmp(argv[i], "--name2hash")) {
			flags = OPT_CONVERT_NAMETOHASH;
		} else if(!strcmp(argv[i], "--hash2name")) {
			flags = OPT_CONVERT_HASHTONAME;
		}
	}

	if(flags == OPT_CONVERT_NAMETOHASH) {
		while(fgets(input, 500, stdin)) {
			input[strlen(input)-1] = 0;
			convertNameToHash(input, output, LEGACY_SEED);
			puts(output);
		}
	} else if(flags == OPT_CONVERT_HASHTONAME) {
		while(fgets(input, 500, stdin)) {
			input[strlen(input)-1] = 0;
			convertHashToName(input, output);
			puts(output);
		}
	}

	return 0;
}
