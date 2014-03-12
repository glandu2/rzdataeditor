#ifndef HASHTONAME_H
#define HASHTONAME_H

#ifdef __cplusplus
extern "C" {
#endif

int getAssociatedDataFileNumber(char *hash);
int convertHashToName(char *hash, char *name);

#ifdef __cplusplus
}
#endif

#endif
