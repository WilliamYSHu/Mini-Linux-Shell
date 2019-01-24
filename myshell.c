 #include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#define MAX_INPUT 512
#define MAX_LINE 2048

void myPrint(char *msg)
{
    write(STDOUT_FILENO, msg, strlen(msg));
}

void myErr(){
  char error_message[30] = "An error has occurred\n";
  write(STDOUT_FILENO, error_message, strlen(error_message));
}

void evalProg(char* myargv[]){
  pid_t pid;
  int status = 0;
  if((pid = fork()) == 0){
    execvp(myargv[0],myargv);
    myErr();
    exit(-1);
  } else {
    if(waitpid(pid,&status,0) < 0) {
      myErr();
    }
    return;
  }
}

int checkBuildin(char* input){
  for (char* iterB = input; *iterB != '\0'; iterB++){
    if(*iterB == 'c' && *(iterB + 1) == 'd')
      return 1;
    else if (*(iterB + 1) != '\0' && *(iterB + 2) != '\0' && *(iterB + 3) != '\0' && *iterB == 'e' && *(iterB + 1) == 'x'
        && *(iterB + 2) == 'i' && *(iterB + 3) == 't')
        return 1;
    else if (*(iterB + 1) != '\0' && *(iterB + 2) != '\0'  && *iterB == 'p' && *(iterB + 1) == 'w'
        && *(iterB + 2) == 'd')
        return 1;
  }
  return 0;
}

int checkEmptyLine(char* input) {
  for(char* iter = input; *iter != '\0';iter++){
    int c = (int)(*iter);
    if (c != 32 && c != 9 && c!= 10) {
      return 1;
    }
  }
  return 0;
}

int checkColons(char* input) {
  for(char* iter = input; *iter != '\0';iter++){
    int c = (int)(*iter);
    if (c != 32 && c != 59 && c != 9 && c!= 10) {
      return 1;
    }
  }
  return 0;
}

int checkArrow(char* input) {
  for(char* iter = input; *iter != '\0';iter++){
    int c = (int)(*iter);
    if (c != 32 && c != 62 && c != 9) {
      return 1;
    }
  }
  return 0;
}

int checkSpaceTab(char* input) {
  for(char* iter = input; *iter != '\0';iter++){
    int c = (int)(*iter);
    if (c != 32 && c != 9) {
      return 1;
    }
  }
  return 0;
}

void executeCMD(char * CMD){
  if(checkSpaceTab(CMD) == 0) return;
  if(checkArrow(CMD) == 0) {
    myErr();
    return;
  }

  /*Test redirection*/
  int arrowCount = 0;
  int redirectFlag = 0;
  char* firstPart = CMD;
  char* filePartOrigin;

  for(char* iterA = CMD; *iterA != '\0'; iterA ++){
    if (*iterA == '>' && *(iterA + 1) == '+') {
      arrowCount++;
      redirectFlag = 2; // rdirection plus
      *iterA = '\0';
      if(*(iterA + 2) == '\0') arrowCount ++;
      filePartOrigin = iterA + 2;
      iterA ++;
    }
    else if (*iterA == '>') {
      arrowCount++;
      redirectFlag = 1; // redirection
      *iterA = '\0';
      if(*(iterA + 1) == '\0') arrowCount ++;
      filePartOrigin = iterA + 1;
    }
  }

  if(arrowCount > 1 || (arrowCount == 1 && checkBuildin(firstPart) == 1)) {
    myErr();
    return;
  }

  char * filePart;
  // count number of files in filePart
  if (redirectFlag == 1 || redirectFlag == 2) {
    int fileargc = 0;
    if (*filePartOrigin != ' ' && *filePartOrigin != '\t') {
      fileargc = 1;
      filePart = filePartOrigin;
    }
    for (char * iterRF = filePartOrigin; *iterRF != '\0'; iterRF ++) {
      if ((*iterRF == ' ' || *iterRF == '\t') && (*(iterRF + 1) != ' ' && *(iterRF + 1) != '\t' && *(iterRF + 1) != '\0' ) ){
        fileargc ++;
        filePart = iterRF + 1;
      }
      if (*iterRF == ' ' || *iterRF == '\t'){
        *iterRF = '\0';
      }
    }
    if(fileargc != 1) {
      myErr();
      return;
    }
  }

  if (redirectFlag == 1 ){
    pid_t pid;
    if ((pid = fork()) == 0) {
      int fd = open(filePart, O_RDWR | O_CREAT | O_EXCL, 0640);
      if (fd < 0) {
        myErr();
        exit(-1);
      }
      dup2(fd,1);
      executeCMD(firstPart);
      close(fd);
      exit(0);
    } else {
      if(waitpid(pid,NULL,0) < 0) {
        myErr();
      }
      return;
    }
  }
  else if(redirectFlag == 2) {
    // redirectPlus
    pid_t pid;
    if ((pid = fork()) == 0) {
      //child
      int fd = open(filePart, O_RDWR | O_CREAT | O_EXCL,0640);
      if (fd < 0) {
        //File already exist
        int fd2 = open(filePart, O_RDWR ,0640);
        if (fd2 < 0) {
          myErr();
          exit(-1);
        }
        // create a file, copy the file
        int tmpStoreFile = open("tmp_redirect_file", O_RDWR | O_CREAT, 0640);
        if (tmpStoreFile < 0) {
          myErr();
          exit(-1);
        }
        int readedbyte;
        char buff[256];
        while((readedbyte = read(fd2,buff,256))!= 0){
          write(tmpStoreFile,buff,readedbyte);
        }
        // finish copy need change to begining
        if(lseek(tmpStoreFile,0,SEEK_SET) != 0){
          myErr();
          exit(-1);
        }
        // rewrite the original file
        close(fd2);
        int fdfinal = open(filePart, O_RDWR | O_TRUNC | O_APPEND,0640);
        if (fdfinal < 0) {
          myErr();
          exit(-1);
        }
        dup2(fdfinal,1);
        executeCMD(firstPart);

        // finish write, now add the original file
        while((readedbyte = read(tmpStoreFile,buff,256))!= 0){
          write(fdfinal,buff,readedbyte);
        }
        close(tmpStoreFile);
        if(remove("tmp_redirect_file") != 0){
          myErr();
          exit(-1);
        }
        close(fdfinal);
        exit(0);

      } else {
        dup2(fd,1);
        executeCMD(firstPart);
        close(fd);
        exit(0);
      }
    } else {
      // parent
      if(waitpid(pid,NULL,0) < 0) {
        myErr();
      }
      return;
    }
  }

  /*parse the arguments*/

  int myargc = 0;
  if (*CMD != ' ' && *CMD != '\t') {
    myargc = 1;
  }
  for (char * iter = CMD; *iter != '\0'; iter ++) {
    if ((*iter == ' ' || *iter == '\t') && (*(iter + 1) != ' ' && *(iter + 1) != '\t' && *(iter + 1) != '\0' ) )
      myargc ++;
    if (*iter == ' ' || *iter == '\t'){
      *iter = '\0';
    }
  }

  char* myargv[myargc+1];
  int i = 0;
  if (*CMD != '\0') {
    myargv[0] = CMD;
    i = 1;
  }
  char* tmpIter = CMD;
  while (i < myargc){
    if (*tmpIter == '\0' && *(tmpIter + 1) != '\0'){
      myargv[i] = tmpIter + 1;
      i ++;
    }
    tmpIter++;
  }
  myargv[myargc] = NULL;

  /*Finish Parsing*/

  /* Built in: exit*/
  if(strcmp(myargv[0],"exit") == 0) {
    if (myargc > 1) {
      myErr();
      return;
    }
    else exit(0);
  }
  /* Built in： pwd*/
  else if (strcmp(myargv[0],"pwd") == 0) {
    if (myargc > 1) {
      myErr();
      return;
    }
    char buff[1024];
    if(getcwd(buff,sizeof(buff)) == NULL){
      myErr();
      return;
    }
    else {
      myPrint(buff);
      myPrint("\n");
    }
    return;
  }
  /* Built in: cd*/
  else if (strcmp(myargv[0],"cd") == 0) {
    if (myargc == 1){
      chdir(getenv("HOME"));
      return;
    }
    else if (myargc == 2){
      int result = chdir(myargv[1]);
      if(result != 0) myErr();
      return;
    } else {
      myErr();
      return;
    }
  }

  /*execute program*/
  else {
    evalProg(myargv);
  }
}

int main(int argc, char *argv[])
{
    char cmd_buff[MAX_INPUT + 2];
    char *pinput;

    if(argc == 1){
      while (1) {
          myPrint("myshell> ");
          pinput = fgets(cmd_buff, MAX_INPUT + 3, stdin);
          if (strlen(cmd_buff) > MAX_INPUT + 1) {
            myErr();
            fflush(stdin);
            continue;
          }
          if (!pinput) {
              exit(0);
          }

          /* Parse the colons*/
          char oneLineCMD[MAX_INPUT + 1];
          strcpy(oneLineCMD,strtok(cmd_buff,";\n"));
          executeCMD(oneLineCMD);
          char *tempP;
          while((tempP = strtok(NULL,";\n"))){
            strcpy(oneLineCMD,tempP);
            executeCMD(oneLineCMD);
          }
      }
    }
    else if(argc == 2){
      char* batchFilename = argv[1];
      char batchBuff[MAX_LINE];
      FILE* batch;
      if ((batch = fopen(batchFilename,"r")) == NULL) {
        myErr();
        return -1;
      }
      while (fgets(batchBuff,MAX_LINE,batch)!= NULL) {
        if (checkEmptyLine(batchBuff) == 0) {
          continue;
        }
        myPrint(batchBuff);
        if (strlen(batchBuff) > MAX_INPUT + 1) {
          myErr();
          continue;
        }
        strcpy(cmd_buff,batchBuff);
        if (checkColons(cmd_buff) == 0) continue;
        char oneLineCMD[MAX_INPUT + 1];
        char* temptok = strtok(cmd_buff,";\n");
        strcpy(oneLineCMD,temptok);
        executeCMD(oneLineCMD);
        char *tempP;
        while((tempP = strtok(NULL,";\n")) != NULL){
          strcpy(oneLineCMD,tempP);
          executeCMD(oneLineCMD);
        }
      }
      fclose(batch);
    }
    else{
        myErr();
        return -1;
    }
}
