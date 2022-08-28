static char* mime_unknown = "application/octet-stream";
static hash_table* mime_types;

typedef struct buffer {
	size_t size;
	char* bytes;
} buffer;


typedef struct file {
	buffer name;
	buffer data;
	char* mime;
} file;

void setup_mime_types();
hash_table* scan_files(char*);
