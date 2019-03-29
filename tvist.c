#include "tvist.h"

int main(int argc, char *argv[])
{
  FILE *inStream, *outStream;
  uint8_t *fileHeader, *restHeader, *pixelData;
  uint32_t fileSize, pixelOffset;
  int remaining;
  char *fBuffer;
  int inLen = 0;
  int outLen = 0;

  if(argc != 3)
  {
    printf("Usage: tvist \"inputfile\" \"outputfile\".\n");
    return -1;
  }

  while(argv[1][inLen] != '\0')
    inLen++;

  while(argv[2][outLen] != '\0')
    outLen++;

  fBuffer = malloc(sizeof(char)*(72+inLen)); /*72 is length of below string*/
  snprintf(fBuffer, 72+inLen, "ffmpeg -y -hide_banner -loglevel panic -i %s -f image2pipe -vcodec bmp - ", argv[1]);

  if((inStream = popen(fBuffer, "r")) == NULL)
  {
    perror("Failed to start ffmpeg process.");
    return -1;
  }

  fBuffer = realloc(fBuffer, sizeof(char)*(56+outLen)); /*56 is length of below string*/
  snprintf(fBuffer, 79+outLen, "ffmpeg -y -hide_banner -loglevel panic -f image2pipe -i - %s", argv[2]);

  if((outStream = popen(fBuffer, "w")) == NULL)
  {
    pclose(inStream);
    perror("Failed to start ffmpeg process.");
    return -1;
  }

  free(fBuffer);

  fileHeader = malloc(sizeof(uint8_t)*14);
  remaining = read(fileno(inStream), (void*)fileHeader, 14);

  while(remaining != 0)
  { /*load one bmp and do stuff with it*/
    if(remaining != 14)
    {
      pclose(inStream);
      pclose(outStream);
      perror("Incomplete header recieved from ffmpeg.");
      return -1;
    }

    memcpy(&fileSize, fileHeader+2, 4); /*get fileSize and pixel matrix offset*/
    memcpy(&pixelOffset, fileHeader+10, 4);

    if(fileSize <= 0 || pixelOffset <= 0)
    {
      pclose(inStream);
      pclose(outStream);
      printf("Invalid header data. filesize: %d, pixel matrix offset: %d\n", fileSize, pixelOffset);
      perror("");
      return -1;
    }

    restHeader = malloc(sizeof(uint8_t)*(pixelOffset-14)); /*grab the other header stuff we dont want to break*/
    readBytes(fileno(inStream), restHeader, pixelOffset-14);

    pixelData = malloc(sizeof(uint8_t)*(fileSize-pixelOffset)); /*grab the pixel matrix so we can mess with it*/
    readBytes(fileno(inStream), pixelData, fileSize-pixelOffset);

    mutate(pixelData, fileSize-pixelOffset); /*change each frame with sox*/

    write(fileno(outStream), (void*)fileHeader, 14); /*write to ffmpeg*/
    write(fileno(outStream), (void*)restHeader, pixelOffset-14);
    write(fileno(outStream), (void*)pixelData, fileSize-pixelOffset);

    free(fileHeader);
    free(restHeader);
    free(pixelData);

    fileHeader = malloc(sizeof(uint8_t)*14);
    remaining = read(fileno(inStream), (void*)fileHeader, 14);
  }

  pclose(inStream);
  pclose(outStream);
  free(fileHeader);

  return 0;
}

void readBytes(int fileNo, uint8_t *buffer, unsigned int bytes)
{ /*reads until number of bytes read in or pipe closed*/
  unsigned int lastRead;
  unsigned int totalRead = 0;
  unsigned int totalLeft = bytes;
  uint8_t *ptrBuffer = buffer;

  while(totalLeft > 0)
  {
    lastRead = read(fileNo, (void*)ptrBuffer, totalLeft);

    totalRead += lastRead;
    totalLeft -= lastRead;
    ptrBuffer += lastRead;
  }
}

void *writer(void *in)
{
  rwData *inf = (rwData*)in;
  unsigned int bytesLeft = inf->length;
  while(bytesLeft != 0)
  {
    bytesLeft -= write(inf->fd, (void*)(inf->data + inf->length-bytesLeft), bytesLeft);
  }

  close(inf->fd);
  return NULL;
}

void *reader(void *in)
{
  rwData *inf = (rwData*)in;
  unsigned int bytesRead;
  unsigned int totalRead = 0;

  do
  {
    bytesRead = read(inf->fd, (void*)(inf->data + totalRead), inf->length-totalRead);
    totalRead += bytesRead;
  }
  while(bytesRead != 0);

  close(inf->fd);
  return NULL;
}

void mutate(uint8_t *pixelData, unsigned int length)
{ /*consider loading file of sequential commands to run against pixeldata?*/
  char *cmd[] = {"sox", "-e", "a-law", "-r", "16k", "-c", "1", "-t", "raw", "-", "-e", "a-law", "-r", "16k", "-c", "1", "-t", "raw", "-", "highpass", "5", NULL};
  uint8_t *temp;
  pthread_t r, w;
  rwData *writerData, *readerData;
  int in[2];
  int out[2];

  pipe(in);
  pipe(out);

  if(fork())
  { /*parent process*/
    close(in[0]);
    close(out[1]);

    temp = malloc(sizeof(uint8_t)*length);
    writerData = malloc(sizeof(rwData));
    readerData = malloc(sizeof(rwData));

    writerData->fd = in[1];
    readerData->fd = out[0];

    writerData->data = pixelData;
    readerData->data = temp;

    writerData->length = length;
    readerData->length = length;

    pthread_create(&w, NULL, writer, writerData);
    pthread_create(&r, NULL, reader, readerData);

    pthread_join(w, NULL);
    pthread_join(r, NULL);

    wait(NULL); /*should probably add error-handling for sox now*/

    memcpy(pixelData, temp, length);
    free(temp);
    free(writerData);
    free(readerData);
  }
  else
  { /*child process*/
    close(in[1]);
    close(out[0]);

    dup2(in[0], 0); /*bind stdin&out to pipe with parent*/
    dup2(out[1], 1);

    close(in[0]);
    close(out[1]);

    prctl(PR_SET_PDEATHSIG, SIGHUP); /*may be redundant*/
    execvp(cmd[0], cmd);
  }
}
