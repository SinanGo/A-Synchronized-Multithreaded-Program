#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>

#define MAX_LINE_LENGTH 128
#define ARRAY_SIZE 64

char lines[ARRAY_SIZE][MAX_LINE_LENGTH]; // array of string
char* fileName; // global variable for storing the file name
char fileSize; // number of lines in the file

//variables storing the number of threads for each kind
int numReadThreads;
int numUpperThreads;
int numReplaceThreads;
int numWriteThreads;

//index variables for each kind
int readIndex;
int upperIndex;
int replaceIndex;
int writeIndex;

pthread_mutex_t systemLock; // used for synchronization

//function headers
int checkFileSize();
void* readFunc(void* thread_id);
void* upperFunc(void* thread_id);
void* replaceFunc(void* thread_id);
void* writeFunc(void* thread_id);

int main(int argc, char** argv) {
    
    if(argc != 8){
    printf("You should enter the arguments as follows:\n"
	"./ProgramName -d TxtFileName -n numberOfReadThreads"
	" numberOfUpperThreads numberOfReplaceThreads numberOfWriteThreads");
	return 1;
    }
    
    fileName = argv[2];
    numReadThreads = atoi(argv[4]);
    numUpperThreads = atoi(argv[5]);
    numReplaceThreads = atoi(argv[6]);
    numWriteThreads = atoi(argv[7]);
    
    checkFileSize(); // for assigning number of lines to global fileSize variable
    
    // Creation of all threads
    pthread_t readThreads[numReadThreads];
    pthread_t upperThreads[numUpperThreads];
    pthread_t replaceThreads[numReplaceThreads];
    pthread_t writeThreads[numWriteThreads];
    
    for (long i = 0; i < numReadThreads; i++) {
        if (pthread_create(&readThreads[i], NULL, readFunc, (void*)i) != 0) {
            fprintf(stderr, "Error: Could not create a read thread.\n");
            return 2;
        }
    }

    for (long i = 0; i < numUpperThreads; i++) {
        if (pthread_create(&upperThreads[i], NULL, upperFunc, (void*)i) != 0) {
            fprintf(stderr, "Error: Could not create a upper thread.\n");
            return 3;
        }
    }

    for (long i = 0; i < numReplaceThreads; i++) {
        if (pthread_create(&replaceThreads[i], NULL, replaceFunc, (void*)i) != 0) {
            fprintf(stderr, "Error: Could not create a replace thread.\n");
            return 4;
        }
    }	
    for (long i = 0; i < numWriteThreads; i++) {
        if (pthread_create(&writeThreads[i], NULL, writeFunc, (void*)i) != 0) {
            fprintf(stderr, "Error: Could not create write thread.\n");
            return 5;
        }
    }
	
    // join statements for Main function to wait for all the threads
    for (int i = 0; i < numReadThreads; i++) {
        pthread_join(readThreads[i], NULL);
    }
    
    for (int i = 0; i < numUpperThreads; i++) {
        pthread_join(upperThreads[i], NULL);
    }
    
    for (int i = 0; i < numReplaceThreads; i++) {
        pthread_join(replaceThreads[i], NULL);
    }
    
    for (int i = 0; i < numWriteThreads; i++) {
        pthread_join(writeThreads[i], NULL);
    }

return 0; // end of main
}
//**********************************************************************

int checkFileSize(){  // this function assigns the size of the file to the global
					  // fileSize variable by counting '\n' characters in the file
	FILE* fp;
	fp = fopen(fileName,"r");
	
	if(fp == NULL){
		printf("Error: File could not be opened in checkFileSize Function\n");
		return 6;
	}
	
	char ch = getc(fp);	// fileSize is 0 initially. If the character got from the
					    // file is '\n', we increment global fileSize variable by 1
	while(ch != EOF){

		if(ch == '\n')
			fileSize++;
	
		ch = getc(fp);
	}
	
	fclose(fp);
} // end of checkFileSize
//**********************************************************************

void* readFunc(void* thread_id) {	// function of the read thread
    
	int tid = (long)thread_id;	// we get an id for recognizing the thread 
    
    while(1){				
    	
    	pthread_mutex_lock(&systemLock);	// we first lock the system so that only one read thread
    										// can do a file reading operation at a time
    	if(readIndex==fileSize){
    		pthread_mutex_unlock(&systemLock);	// if readIndex is equal to the fileSize, it means
    		break;								// we have read all the lines. We break the while
    	}										// loop and exit
    	
    	FILE* fp = fopen(fileName, "r");
    										
    	if(fp == NULL){						// specialized error exception for each thread
			printf("Error: File could not be opened in Read_%d!\n",tid);
			exit(7);
		}
    	
    	char line[MAX_LINE_LENGTH]; 			
    	int lineToRead = 0;
													// we read each line one by one from the file until
    	while (fgets(line, MAX_LINE_LENGTH, fp)) {	// the line we want. Then, we store this line in our
													// global array named 'lines' and break the while loop
        	if (lineToRead == readIndex) {					
    			printf("Read_%d read the line %d which is : %s", tid, lineToRead, line); 
    			strcpy(lines[readIndex], line);				 
    			readIndex++;			                                 
    			break;				// we inform the user about the operation we did
			}
 			
			lineToRead++;
		}

		fclose(fp);				// when we are done with the file and read operation, we close
								// the file and unlock the system
		pthread_mutex_unlock (&systemLock);
	}

	printf("Read_%d is exiting\n",tid);	// we inform the user when we are exiting
	pthread_exit(NULL);
}

void* upperFunc(void* thread_id) {	// the function of the upper thread
    
	int tid = (long)thread_id;	// we get an id for recognizing the thread 
    
    while(1){
    	
    	pthread_mutex_lock(&systemLock);	// we first lock the system
    	
    	int shouldStop=1;			// this is the variable for stopping the whole operation
    	
    	if(readIndex==fileSize){ 		// if we have read all the files or upperIndex
    		shouldStop=0;				// is still smaller than readIndex, we continue	    
    	}
    	else if(upperIndex < readIndex){
    		shouldStop=0;
    	}					
							
    	if(shouldStop){			
    		pthread_mutex_unlock(&systemLock);// if we are not allowed to do an 'upper' operation,
    		continue;			  			  // we unlock the system and wait for an appropriate situation
    	}
    	
    	if(upperIndex == fileSize){				// if we have made all the lines in upper case letters,
    		pthread_mutex_unlock(&systemLock);	// we break and exit
    		break;
    	}
    	
    	char oldLine[MAX_LINE_LENGTH];	// these arrays are storing the old and updated lines so that
  		char newLine[MAX_LINE_LENGTH];	// we can see the changes
    	int lineToExecute = upperIndex++;
  
    	strcpy(oldLine,lines[lineToExecute]);	// we copy the old line to the 'oldLine' array
    	
    	for (int i = 0; i < strlen(oldLine); i++) { // we update the line and store it in the 'newLine' array
            newLine[i] = toupper(oldLine[i]);
        }
        
		printf("Upper_%d read index %d and converted : %s to : %s", tid, lineToExecute, oldLine, newLine);
        
		strcpy(lines[lineToExecute],newLine);	
        					// we store 'newLine' in the 'lines' array and unlock the system
		pthread_mutex_unlock (&systemLock);
    }
     
    printf("Upper_%d is exiting\n",tid); // when we are exiting, we inform the user
    pthread_exit(NULL);
}

void* replaceFunc(void* thread_id) {	
    
	int tid = (long)thread_id;
    
    while(1){
    	
    	pthread_mutex_lock(&systemLock);
    	
    	int shouldStop=1;			// funcionality of this function is similar to upperFunc
    	
    	if(readIndex==fileSize){ 		
    		shouldStop=0;					    
    	}
    	else if(replaceIndex < readIndex){
    		shouldStop=0;
    	}
    	
    	if(shouldStop){
    		pthread_mutex_unlock(&systemLock);
    		continue;
    	}
    	
    	if(replaceIndex == fileSize){
    		pthread_mutex_unlock(&systemLock);
    		break;
    	}
    	
    	char oldLine[MAX_LINE_LENGTH];
  		char newLine[MAX_LINE_LENGTH];
    	int lineToReplace = replaceIndex++;
  
    	strcpy(oldLine,lines[lineToReplace]);
    	strcpy(newLine,lines[lineToReplace]);
    	
		for (int i = 0; i < strlen(oldLine); i++) {
            if (oldLine[i] == ' ') {
                newLine[i] = '_';
            }
        }

        printf("Replace_%d read index %d and converted : %s to : %s", tid, lineToReplace, oldLine, newLine);
        strcpy(lines[lineToReplace],newLine);
        
		pthread_mutex_unlock (&systemLock);
    }
     
    printf("Replace_%d is exiting\n",tid);
    pthread_exit(NULL);
}

void* writeFunc(void* thread_id) {	
    
	int tid = (long)thread_id;
    
    while(1){
    	
    	pthread_mutex_lock(&systemLock); // for write operation, we should be sure that related index is done
    									 // with read, replace and write operations. After that, we can continue
    	int shouldStop=1;

    	if(readIndex==fileSize && upperIndex==fileSize && replaceIndex==fileSize){
    		shouldStop = 0;	
    	}
    	else if(writeIndex < readIndex && writeIndex < replaceIndex && writeIndex < upperIndex){
    		shouldStop = 0;
    	}
    	
    	if(shouldStop){
    		pthread_mutex_unlock(&systemLock);
    		continue;
    	}
    	
    	if(writeIndex == fileSize){
    		pthread_mutex_unlock(&systemLock);
    		break;
    	}

    	FILE* fp = fopen(fileName,"r+"); //we open the file in both read and write modes
    	
    	if(fp == NULL){				
			printf("Error: File could not be opened in Write_%d!\n",tid);
			exit(8);
		}
    	
    	char dummy[MAX_LINE_LENGTH]; // dummy array for reading the lines
    	int lineToWrite = writeIndex++;

		if(lineToWrite==0){
    		printf("Write_%d write line %d back which is : %s", tid, lineToWrite, lines[lineToWrite]);
    		fprintf(fp,"%s",lines[lineToWrite]);
    	}    	    	
					// if the index is 0, we write directly, if it is not, we read the lines 
    	else{		// for shifting the file pointer. Then, we write
			int i = 1;

			while (fgets(dummy, MAX_LINE_LENGTH, fp)) {

				if (i == lineToWrite) {	
					printf("Write_%d write line %d back which is : %s", tid, lineToWrite, lines[lineToWrite]);
					fprintf(fp,"%s",lines[lineToWrite]);
					break;
				}

				i++;
			}
		}
				// after doing the operation, we close the file and unlock the system.
		fclose(fp);
  	
  		pthread_mutex_unlock (&systemLock);
    }
     
    printf("Write_%d is exiting\n",tid);
    pthread_exit(NULL);
}