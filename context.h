typedef struct ipc_header {
	char opcode;
	int argument;
	size_t len;
} ipc_header;

typedef struct ipc_buffer {
	ipc_header* header;
	void* buffer;
} ipc_buffer;

typedef struct ipc_context {
	ipc_buffer read;
	ipc_buffer write;
	int prefix;
	char* name;
} ipc_context;

ipc_context create_context(ipc_pipe,char*,int);
