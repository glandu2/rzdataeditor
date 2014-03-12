#ifndef RAPPELZDECRYPT_H
#define RAPPELZDECRYPT_H

#include <stdlib.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

unsigned int decryptRappelzData(char *data, char *decrypted, int size, unsigned int decryptPos);
size_t cryptfread(void *ptr, size_t size, size_t count, FILE *stream);
size_t cryptfwrite(void *ptr, size_t size, size_t count, FILE *stream);

#ifdef __cplusplus
}
#endif

#endif
