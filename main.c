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
#define MAX_LINE_CHARS		256
int G_ANALYZED=0;
int G_OK=0;
int G_ERROR=0;
int G_MISMATCH=0;

void extension_manager(char *file);
void directory_manager(char *directory);
void file_manager(char **file, int file_count);

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
	char **fileName = args.file_orig;

	
	
	if(args.nohelp_given){
		execlp("./prog","prog", "--help", NULL);
	}else if(!args.file_given && !args.batch_given && !args.dir_given){
		printf("Error you need to set arguments \n");
		exit(0);
	}else if(args.file_given && !args.batch_given && !args.dir_given){
		file_manager(fileName, file_count);
	}else if(!args.file_given && args.batch_given && !args.dir_given){
	
	}else if(!args.file_given && !args.batch_given && args.dir_given){
		directory_manager(directory);
	}else{
		printf("Error too many arguments\n");
		exit(0);
	}
	cmdline_parser_free(&args);
    return 0;
}

void file_manager(char **file, int file_count){
	pid_t pid;
	FILE *fileopener;
	for(int i = 0; i < file_count; i++){
		printf("%s \n", file[i]);
		G_ANALYZED++;
		if((fileopener = fopen(file[i], "r"))== NULL){
			printf("[ERROR] cannot open file '%s' -- No such file or directory \n", file[i]);
			G_ERROR++;
			continue;
		}
		//check if file close
		if(fclose(fileopener) != 0){
			printf("[ERROR] cannot close file '%s' -- No such file or directory \n", file[i]);
			G_ERROR++;
			continue;
		}
		//everythings god, run extension_manager
		pid = fork();
		switch(pid){
			case -1:
				ERROR(1, "Error executing fork()");
				break;
			case 0: //son
				//check if file open
				extension_manager(file[i]);
				
				break;
			default: // father
				for(int i = 0; i < file_count; i++){
					wait(NULL);
				}
				break;
		}
		
		FILE *fileReader;
		if ((fileReader = fopen("saveFile.txt", "r")) == NULL){
			ERROR(5, "fopen() - não foi possível abrir o ficheiro");
		}
		// ler a próxima linha do ficheiro (\n vem incluído na string)		
		char file_line[MAX_LINE_CHARS];
		if (fgets(file_line, MAX_LINE_CHARS, fileReader) == NULL){
			ERROR(7, "fgets() - não foi possível ler uma linha do ficheiro");
		}
			char *ext = strrchr(file[i], '.')+1;
			const char *pdf = "PDF";
			const char *gif = "GIF";
			const char *png = "PNG";
			const char *jpeg = "JPEG";
			const char *zip = "7-zip";
			const char *mp4 = "MP4";
			const char *html = "HTML";
			
			if(strstr(file_line, pdf) != NULL || 
				strstr(file_line, gif) != NULL || 
				strstr(file_line, png) != NULL || 
				strstr(file_line, jpeg) != NULL || 
				strstr(file_line, zip) != NULL ||
				strstr(file_line, mp4) != NULL ||
				strstr(file_line, html) != NULL){
				// char *pointOne = strrchr(file_line, ':')+2;
				// char *pointTwo = strrchr(pointOne, ',');
				// printf("%s \n", pointTwo);
				G_OK++;
				printf("[OK] '%s': extension '%s' matches file type '%s'\n", file[i], ext, ext);
			}else{
				
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
					G_MISMATCH++;
					printf("[MISMATCH] '%s': extension is '%s', file type is '%s'\n", file[i],ext,extension_compare);
				}else{
					printf("[INFO] '%s': type '%s' is not supported by checkFile \n", file[i], file_line_with_ptr);
				}
			}
		
	}
	printf("[SUMMARY] files analyzed: %d; files OK: %d; Mismatch: %d; errors: %d \n", G_ANALYZED, G_OK, G_MISMATCH, G_ERROR);
}
             
void extension_manager(char *file){
	//printf("%s\n",file);
	char *ext = strrchr(file, '.')+1;
	
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
	DIR *dr = opendir(directory);
	char *file[MAX_LINE_CHARS];
	//char **file;
	int file_count = 0;
	
	if (dr == NULL)  // opendir returns NULL if couldn't open directory
    {
        printf("[ERROR] cannot open dir '%s' --  No such file or directory \n", directory);
        return;
    }
	while ((de = readdir(dr)) != NULL){
		
		
		if(strcmp(de->d_name, ".") != 0 && strcmp(de->d_name, "..") != 0){
			// lets go now
			char concat[MAX_LINE_CHARS];
			snprintf(concat, sizeof(concat), "%s%s", directory, de->d_name);
			//printf("%s \n", concat);
			file[file_count] = concat;
			printf("%s \n", file[file_count]);
			file_count++;
		}
	}
	closedir(dr);
	//FILE *file[file_count];
	for (int i = 0; i < file_count; i++)
	{
		printf("%s \n", file[i]);
	}
	
	
	//file_manager(file, file_count);
    
}
