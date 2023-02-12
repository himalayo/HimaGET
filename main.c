#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
#include <string.h>
#include <dirent.h>

#include "hash_table.h"
#include "spawn.h"
#include "context.h"
#include "file_utils.h"

#define HEADER_POP 1
#define HEADER_DONE 1<<2

void
main (int argc, char** argv)
{	
	setup_mime_types();
	char temp_buff[50];
	write(STDOUT_FILENO,temp_buff,snprintf(temp_buff,sizeof(temp_buff),"HimaGET\nScanning directory... "));
	hash_table* files = scan_files("./html/");
	write(STDOUT_FILENO,temp_buff,snprintf(temp_buff,sizeof(temp_buff),"OK\nSpawning MicroENG... "));
	
	ipc_pipe microeng = spawn_microeng("./microeng");;
	ipc_context context;
	read(microeng.read,temp_buff,sizeof("MicroENG\n"));
	if (strncmp("MicroENG\n",temp_buff,sizeof("MicroENG")) == 0)
	{
		write(STDOUT_FILENO,temp_buff,snprintf(temp_buff,sizeof(temp_buff),"OK\nCreating context... "));
		context = create_context(microeng,"test",*(int*)&"GET ");
		
		if ( context.read.buffer == NULL || context.write.buffer == NULL )
		{
			write(STDOUT_FILENO,temp_buff,snprintf(temp_buff,sizeof(temp_buff),"FAILED\n"));
			wait(NULL);
			exit(EXIT_FAILURE);
		}
		
		write(STDOUT_FILENO,temp_buff,snprintf(temp_buff,sizeof(temp_buff),"OK\n"));
	}
	else
	{
		write(STDOUT_FILENO,temp_buff,snprintf(temp_buff,sizeof(temp_buff),"FAILED\n"));
		wait(NULL);
		exit(EXIT_FAILURE);
	}
	

	write(STDOUT_FILENO,temp_buff,snprintf(temp_buff,sizeof(temp_buff),"Spinning read buffer... "));
	if (!spin_read(&microeng))
	{
		write(STDOUT_FILENO,temp_buff,snprintf(temp_buff,sizeof(temp_buff),"FAILED\n"));
		wait(NULL);
		exit(EXIT_FAILURE);
	}
	write(microeng.write,"spin_wrb\n",sizeof("spin_wrb\n"));
	write(STDOUT_FILENO,temp_buff,snprintf(temp_buff,sizeof(temp_buff),"OK\nListening\n"));
		
	for(;;)
	{
		context.read.header->opcode = HEADER_POP;
		while ( !( context.read.header->opcode == HEADER_DONE ) )
		{
			sleep(0);
		}
		
		//Get path
		char path[255];
		for (int i=0; i<sizeof(path); i++)
		{
			char curr_char = *(char*)(context.read.buffer+strlen("GET /")+i);
			if (curr_char != ' ' && curr_char != '\n' && curr_char != '\0')
			{
				path[i] = curr_char;
				continue;
			}
			path[i] = '\0';
			break;
		}
		

		file* requested_file = hash_table_get(files,path);
		if (requested_file == NULL)
		{
			continue;
		}
		char buf[256];
		time_t rawtime = time(NULL);
  	 	struct tm *ptm = localtime(&rawtime);
    		strftime(buf, sizeof(buf), "%c", ptm);
		size_t len = sprintf(context.write.buffer,"HTTP/1.1 200 OK\nDate: %s\nContent-Type: %s\nConnection: close\nContent-Length: %d\n\n",buf,requested_file->mime,requested_file->data.size);
		memcpy(context.write.buffer+len,requested_file->data.bytes,requested_file->data.size);
		context.write.header->len = len+requested_file->data.size;
		context.write.header->argument = context.read.header->argument;
		context.write.header->opcode = 1;
	}

	wait(NULL);	
	exit(EXIT_SUCCESS);
}
