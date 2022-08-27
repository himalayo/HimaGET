#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>

#include "spawn.h"

ipc_pipe
spawn_microeng(char* filename)
{
	int read_from[2]; //we will close [1]
	int write_into[2]; //we will close [0]

	pipe(read_from);
	pipe(write_into);

	pid_t child = fork();
	if ( child == 0 )
	{
		close(read_from[0]);
		close(write_into[1]);
		
		char in[255];
		char out[255];

		ssize_t size_in = snprintf(in,sizeof(in),"%d\n",write_into[0]);
		ssize_t size_out = snprintf(out,sizeof(out),"%d\n",read_from[1]);

		execl(filename,filename,in,out);

		_exit(EXIT_SUCCESS);
	}

	close(read_from[1]);
	close(write_into[0]);
	
	return (ipc_pipe) {.write=write_into[1],.read=read_from[0]};
}

bool
spin_read(ipc_pipe* pipe)
{
	write(pipe->write,"spin_rdb\nget_spin\n",sizeof("spin_rdb\nget_spin\n"));
	char response[8];
	read(pipe->read,response,sizeof(response));
	return strncmp("no\0\n",response,4);
}
