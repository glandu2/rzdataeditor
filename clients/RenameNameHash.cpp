#include <stdio.h>
#include <string.h>

#include "NameToHash.h"
#include "HashToName.h"
#include <vector>
#include <string>

#include <sys/types.h>
#ifndef _MSC_VER
#include <dirent.h>
#endif

int patternMatch(char *buffer, char *pattern) {
	char *p1, *p2;

	p1 = buffer;
	p2 = pattern;

	while(1) {
		if(*p2 == '*') {
			if(*(p2+1) == *p1) {
				if(*p1 == 0) return 1;
				else {
					p2+=2;
					p1++;
				}
			} else if(*p1 == 0) return 0;
			else p1++;
		} else {
			if(*p1 != *p2) return 0;
			else if(*p1 == 0) return 1;
			else {
				p1++;
				p2++;
			}
		}
	}
}

#define OPT_CONVERT_NAMETOHASH 1
#define OPT_CONVERT_HASHTONAME 2

int main(int argc, char *argv[]) {
	/*
	std::vector<std::string> filesToRename;
	std::vector<char*> args;
	std::string targetFolder = "./";
	int flags = 0, i;

	flags = 0;
	for(i=1; i<argc; i++) {
		if(!strcmp(argv[i], "--n2h")) {
			flags = OPT_CONVERT_NAMETOHASH;
		} else if(!strcmp(argv[i], "--h2n")) {
			flags = OPT_CONVERT_HASHTONAME;
		} else {
			args.push_back(argv[i]);
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
*/
	return 0;
}
