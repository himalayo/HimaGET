#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include "hash_table.h"
#include "file_utils.h"

char *get_filename_ext(char *filename) {
   char *dot = strrchr(filename, '.');
    if(!dot || dot == filename) return "";
    return dot + 1;
}

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
setup_mime_types()
{
	mime_types = hash_table_init(0);
	hash_table_set(mime_types,"html","text/html",sizeof("text/html"));
	hash_table_set(mime_types,"htm","text/html",sizeof("text/html"));
	hash_table_set(mime_types,"js","text/javascript",sizeof("text/javascript"));
	hash_table_set(mime_types,"css","text/css",sizeof("text/css"));
}

char*
get_mime(char* filename)
{
	char* mime = hash_table_get(mime_types,get_filename_ext(filename));
	if (mime == NULL)
		return mime_unknown;
	return mime;
}
buffer
str_to_buff(char* str)
{
	buffer out = {.size=strlen(str)};
	out.bytes = malloc(out.size);
	strncpy(out.bytes,str,out.size);
	return out;
}
hash_table*
scan_files(char* directory)
{
	struct dirent** entries;
	int n = scandir(directory,&entries,NULL,alphasort);
	
	hash_table* files = hash_table_init(0);
	while(n--)
	{
		if (entries[n]->d_type != DT_REG)
		{
			continue;
		}
		char path[255];
		strcpy(path,directory);
		strcat(path,entries[n]->d_name);
	
		file curr_file = {.name=str_to_buff(entries[n]->d_name),.data=read_file(open(path,O_RDONLY)),.mime=get_mime(entries[n]->d_name)};
		hash_table_set(files,entries[n]->d_name,&curr_file,sizeof(file));
		free(entries[n]);
	}

	free(entries);
	return files;
}
