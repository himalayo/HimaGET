typedef struct ipc_pipe {
	int write;
	int read;
} ipc_pipe;

ipc_pipe spawn_microeng(char*);
bool spin_read(ipc_pipe*);
