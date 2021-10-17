/**
 * @file main.c
 * @brief Description
 * @date 2018-1-1
 * @author name of author
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "debug.h"
#include "memory.h"
#include "args.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <signal.h>
#include <time.h>

#define MAX_LINE_CHARS		256
typedef struct {
	int G_ANALYZED;
	int G_OK;
	int G_ERROR;
	int G_MISMATCH;
} SUMMARY_PARAMS;

typedef struct {
	int MODE;
	char FILE[MAX_LINE_CHARS];
	int NUMBER_FILE;
} BATCH_MODE;
BATCH_MODE batch_params = {0,"",0};

void extension_manager(char *file);
void directory_manager(char *directory);
void file_manager(char **files, int file_count);
void batch_manager(char *file);
void signal_manager(int signal);

/**
 * given (vezes que é metido o argumento) / arg (é o proprio argumento) **/

int main(int argc, char *argv[]) {
	
	struct gengetopt_args_info args;
	if(cmdline_parser(argc, argv, &args) ){
		ERROR(1,"Erro");
	}

	//set variables to optimise commands
	char *directory = args.dir_arg;
	int file_count = args.file_given;
	// Multiple filenames
	char **fileName = args.file_orig;
	char *file_batch = args.batch_orig;

	// VERIFY THE GENGETOPT COMMAND ARGS
	if(args.nohelp_given){
		execlp("./prog","prog", "--help", NULL);
	}else if(!args.file_given && !args.batch_given && !args.dir_given){
		printf("Error you need to set arguments \n");
		exit(0);
	}else if(args.file_given && !args.batch_given && !args.dir_given){
		file_manager(fileName, file_count); 
	}else if(!args.file_given && args.batch_given && !args.dir_given){
		batch_manager(file_batch);
	}else if(!args.file_given && !args.batch_given && args.dir_given){
		directory_manager(directory);
	}else{
		printf("Error too many arguments\n");
		exit(0);
	}

	cmdline_parser_free(&args);
    return 0;
}

void file_manager(char **files, int file_count){
	pid_t pid;
	FILE *fileopener;
	//SET SUMMARY VARIABLES TO 0
	SUMMARY_PARAMS params = {0, 0, 0, 0};

	//SIGNAL STRUCTER
	struct sigaction act;
		
	act.sa_handler = signal_manager;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;           	
	act.sa_flags |= SA_RESTART;

	for(int i = 0; i < file_count; i++){
		params.G_ANALYZED++;
		// BATCH MODE VARIABLES
		strcpy(batch_params.FILE, files[i]);
		batch_params.NUMBER_FILE = i;

		// CHECK IF FILE OPEN
		if((fileopener = fopen(files[i], "r"))== NULL){
			printf("[ERROR] cannot open file '%s' -- No such file or directory \n", files[i]);
			params.G_ERROR++;
			continue;
		}


		// CHECK IF FILE CLOSE
		if(fclose(fileopener) != 0){
			printf("[ERROR] cannot close file '%s' -- No such file or directory \n", files[i]);
			params.G_ERROR++;
			continue;
		}
		//IF EVERTHINGS IS GOOD, RUN EXTENSION_MANAGER
		pid = fork();
		switch(pid){
			case -1:
				ERROR(1, "Error executing fork()");
				break;
			case 0: // SON
				//check if file open
				extension_manager(files[i]);
				
				break;
			default: // FATHER
				// WAIT FOR SON
				for(int i = 0; i < file_count; i++){
					if(sigaction(SIGQUIT, &act, NULL) < 0){
						ERROR(3, "sigaction (sa_handler) - SIGQUIT");
					}
					if(sigaction(SIGINT, &act, NULL) <0){
						ERROR(3, "sigaction (sa_handler) - SIGINT ");
					}
					wait(NULL);
				}
				break;
		}
		
		//OPEN A FILE 
		FILE *fileReader;
		if ((fileReader = fopen("saveFile.txt", "rw")) == NULL){
			ERROR(5, "fopen() - não foi possível abrir o ficheiro");
		}
		// READ NEXT LINE OF FILEl (\N IS INCLUDED)		
		char file_line[MAX_LINE_CHARS];
		if (fgets(file_line, MAX_LINE_CHARS, fileReader) == NULL){
			ERROR(7, "fgets() - não foi possível ler uma linha do ficheiro");
		}

		// VERIFY IF ANY CONST IS IN THE FILE FILE
			char *ext = strrchr(files[i], '.')+1;
			const char *pdf = "PDF";
			const char *gif = "GIF";
			const char *png = "PNG";
			const char *jpeg = "JPEG";
			const char *zip = "7-zip";
			const char *mp4 = "MP4";
			const char *html = "HTML";


			kill(getpid(), SIGQUIT);


			//IF YES IS A OK TYPE
			if(strstr(file_line, pdf) != NULL || 
				strstr(file_line, gif) != NULL || 
				strstr(file_line, png) != NULL || 
				strstr(file_line, jpeg) != NULL || 
				strstr(file_line, zip) != NULL ||
				strstr(file_line, mp4) != NULL ||
				strstr(file_line, html) != NULL){
				params.G_OK++;
				printf("[OK] '%s': extension '%s' matches file type '%s'\n", files[i], ext, ext);
			}else{
				//ELSE IS A MISMATCH TYPE
				char delim[] = ";";
				char *ptr = strtok(file_line, delim);
				char *file_line_with_ptr = strrchr(ptr, ':')+2;
				char *extension_compare = strrchr(ptr, '/')+1;
				if(strstr(extension_compare,"pdf") != NULL ||
					strstr(extension_compare,"gif") != NULL ||
					strstr(extension_compare,"png") != NULL ||
					strstr(extension_compare,"jpeg") != NULL ||
					strstr(extension_compare,"7z") != NULL ||
					strstr(extension_compare,"mp4") != NULL ||
					strstr(extension_compare,"html") != NULL)
				{
					
					params.G_MISMATCH++;
					printf("[MISMATCH] '%s': extension is '%s', file type is '%s'\n", files[i],ext,extension_compare);
				}else{
					// ELSE IS A INFO TYPE
					printf("[INFO] '%s': type '%s' is not supported by checkFile \n", files[i], file_line_with_ptr);
				}
			}
		
	}
	// SUMMARY OF THE RESULST OF THE EXECUTATION
	printf("[SUMMARY] files analyzed: %d; files OK: %d; Mismatch: %d; errors: %d \n", params.G_ANALYZED, params.G_OK, params.G_MISMATCH, params.G_ERROR);
	
	// REMOVE FILE (WHERE IS WRITING THE LINE)
	remove("saveFile.txt");
}
             
void extension_manager(char *file){
	//CHECK EXTENSION OF THE FILE
	char *ext = strrchr(file, '.')+1;
	
	//OPEN A FILE AND GET THE EXECLP OUTPUT
	int fileOpen;
	if((fileOpen = open("saveFile.txt", O_RDWR | O_CREAT))==-1){ /*open the file */
	  perror("open");
	}
	dup2(fileOpen,STDOUT_FILENO);
	dup2(fileOpen,STDERR_FILENO);
	close(fileOpen);
	if(strstr(ext,"pdf") != NULL ||
		strstr(ext,"gif") != NULL ||
		strstr(ext,"png") != NULL ||
		strstr(ext,"jpeg") != NULL ||
		strstr(ext,"7z") != NULL ||
		strstr(ext,"mp4") != NULL ||
		strstr(ext,"html") != NULL 
	){
		execlp("file","file", file, NULL);
	}else{
		execlp("file","file", file,"-i","-s", NULL);
	}
	
	
	
}
void directory_manager(char *directory){
	struct dirent *	de;
	DIR *dir = opendir(directory);
	char *file[MAX_LINE_CHARS];
	// file = (0x9, 0)
	// 0x9 (0, 0, 0, 0)
	// 0x20 = (O,L,A,\0)
	// 0x12 = (T,E,S,T,E)
	int file_count = 0;
	
	if (dir == NULL)  // opendir returns NULL if couldn't open directory
    {
        printf("[ERROR] cannot open dir '%s' --  No such file or directory \n", directory);
        return;
    }
	while ((de = readdir(dir)) != NULL){
		
		
		if(strcmp(de->d_name, ".") != 0 && strcmp(de->d_name, "..") != 0){
			char concat[MAX_LINE_CHARS];
			// IF DIRECTORY DONT HAVE / ADD
			if(strstr(directory,"/") == NULL)
			{
				snprintf(concat, sizeof(concat), "%s%s", directory, "/");
				strcpy(directory, concat);
			}
			snprintf(concat, sizeof(concat), "%s%s", directory, de->d_name);
			// CALCULATE STRING SIZE
			// CREATE DINAMYC ARRAY
			file[file_count] = (char *) malloc(strlen(concat));	
			strcpy(file[file_count], concat);
			file_count++;
		}
	}
	closedir(dir);
	// EXECUTES THE FILE_MANAGER
	file_manager(file, file_count);
    
}
void batch_manager(char *file){
	char *file_final[MAX_LINE_CHARS];
	batch_params.MODE = 1;
	int file_count = 0;
	FILE * fp;
    char * line = NULL;
    size_t len = 0;
    ssize_t read;

    fp = fopen(file, "r");
    if (fp == NULL)
        exit(EXIT_FAILURE);

    while ((read = getline(&line, &len, fp)) != -1) {
		// remove newline \n
		char *pos;
		if ((pos=strchr(line, '\n')) != NULL){
			*pos = '\0';
		}
		// VERIFY IF STRING AS /  IF NOT ADD DIRECTORY 
		if(strstr(line,"/") == NULL){
			char delim[] = "/";
			char *ptr = strtok(file, delim);
			char concat[MAX_LINE_CHARS];
			snprintf(concat, sizeof(concat), "%s%s%s", ptr, "/", line);
			strcpy(line, concat);
		}
		file_final[file_count] = (char *) malloc(strlen(line));	
		strcpy(file_final[file_count], line);
		file_count++;
    }
	file_manager(file_final, file_count);
	fclose(fp);
    if (line){ 
        free(line);
	}
	
}
void signal_manager(int signal){
	int aux; // axiliar variable
	aux = errno;
	if (signal == SIGQUIT){ 
        printf("\nCaptured SIGQUIT signal (sent by PID: %d)\n", getpid());
		pause();
	}
	if(batch_params.MODE == 1){
		if(signal == SIGINT){
			time_t t = time(NULL);
			struct tm tm = *localtime(&t);
			printf("%d.%02d.%02d_%02dh%02d:%02d\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
			printf("nº %d/%s\n", batch_params.NUMBER_FILE, batch_params.FILE);
			exit(0);	
		}
	}
	errno = aux; 
} 