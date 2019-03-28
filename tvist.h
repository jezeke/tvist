#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/prctl.h>
#include <signal.h>
#include <stdint.h>
#include <unistd.h>
#include <pthread.h>

typedef struct rwData
{
  int fd;
  uint8_t *data;
  unsigned int length;
} rwData;

void mutate(uint8_t *pixelData, unsigned int length);

void readBytes(int fileNo, uint8_t *buffer, unsigned int bytes);

void *writer(void *in);

void *reader(void *in);
