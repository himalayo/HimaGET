#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
#include <string.h>
#include <dirent.h>
#include <sched.h>

#include "hash_table.h"
#include "spawn.h"
#include "context.h"

#define HEADER_POP 1
#define HEADER_DONE 1<<2

typedef struct buffer {
	size_t size;
	char* bytes;
} buffer;


typedef struct file {
	buffer name;
	buffer data;
} file;

int
is_html(const struct dirent* entry)
{
	if (strncmp(".html",entry->d_name+strlen(entry->d_name)-strlen(".html"),strlen(".html")))
	{
		return 0;
	}
	return 1;
}

buffer
str_to_buff(char* str)
{
	buffer out = {.size=strlen(str)};
	out.bytes = malloc(out.size);
	strncpy(out.bytes,str,out.size);
	return out;
}

buffer
read_file(int fd)
{
	size_t curr_size = 64;
	int curr_index = 0;
	char* output = malloc(64);
	while(1)
	{
		curr_size *= 2;
		output = realloc(output,curr_size);
		ssize_t bytes_read = read(fd,output,curr_size-curr_index);
		curr_index += bytes_read;
		
		if (bytes_read <= 0)
		{
			curr_size = curr_index;
			output = realloc(output,curr_size);
			break;
		}

	}
	close(fd);
	return (buffer) {.size=curr_size,.bytes=output};
}

void
main (int argc, char** argv)
{	
	char temp_buff[50];
	struct dirent** entries;
	
	write(STDOUT_FILENO,temp_buff,snprintf(temp_buff,sizeof(temp_buff),"HimaGET\nScanning directory... "));
	int n = scandir("./html",&entries,is_html,alphasort);
	
	hash_table* files = hash_table_init(0);
	while(n--)
	{
		if (entries[n]->d_type != DT_REG)
		{
			continue;
		}
		char path[255] = "./html/";
		strcat(path,entries[n]->d_name);
		file curr_file = {.name=str_to_buff(entries[n]->d_name),.data=read_file(open(path,O_RDONLY))};
		hash_table_set(files,entries[n]->d_name,&curr_file,sizeof(file));
		free(entries[n]);
	}
	free(entries);

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
			sched_yield();
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
		size_t len = sprintf(context.write.buffer,"HTTP/1.1 200 OK\nLocation: http://localhost:2096/\nDate: %s\nContent-Type: text/html\nContent-Length: %d\n\n%s",buf,requested_file->data.size,requested_file->data.bytes);
		context.write.header->len = len;
		context.write.header->argument = context.read.header->argument;
		context.write.header->opcode = 1;
	}

	wait(NULL);	
	exit(EXIT_SUCCESS);
}
