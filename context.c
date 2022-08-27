#include <fcntl.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>

#include "spawn.h"
#include "context.h"

#define PACK_SIZE 5*1024*1024

ipc_buffer
get_shared_mem(char* path)
{
	int fd = shm_open(path,O_RDWR,0);
	void* mem = mmap(NULL,
			PACK_SIZE,
			PROT_READ|PROT_WRITE,
			MAP_SHARED,
			fd,
			0);
	close(fd);
	return (ipc_buffer) {.header=(ipc_header*)mem,.buffer=mem+sizeof(ipc_header)};
}

ipc_context
create_context(ipc_pipe microeng,char* name, int prefix)
{
	char buff[128];
	char write_name[50];
	char read_name[50];
	
	snprintf(write_name,sizeof(write_name),"/%smicroengwrites",name);
	snprintf(read_name,sizeof(read_name),"/%smicroengreads",name);

	snprintf(buff,sizeof(buff),"strt_ctx\nname_ctx %s\npfix_ctx %d\nlnch_ctx\n",name,prefix);
	write(microeng.write,buff,sizeof(buff));
	read(microeng.read,buff,sizeof("done\n"));
	ipc_context out = {.read=get_shared_mem(read_name),.write=get_shared_mem(write_name),.name=malloc(strlen(name)),.prefix=prefix};
	strcpy(out.name,name);
	return out;
}
