#include <stdio.h>
#include <stdlib.h>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>

#define N 13

extern char **environ;
char uName[20];

char *allowed[N] = {"cp","touch","mkdir","ls","pwd","cat","grep","chmod","diff","cd","exit","help","sendmsg"};

struct message {
	char source[50];
	char target[50]; 
	char msg[200];
};

void terminate(int sig) {
        printf("Exiting....\n");
        fflush(stdout);
        exit(0);
}

void sendmsg (char *user, char *target, char *msg) {
	// TODO:
	// Send a request to the server to send the message (msg) to the target user (target)
	// by creating the message structure and writing it to server's FIFO

	int server_fd;
	struct message req;

	//fill the message structure with (Source is user, Target is target, message is msg)
	strncpy(req.source, user, sizeof(req.source) - 1);
	req.source[sizeof(req.source) - 1] = '\0';
	strncpy(req.target, target, sizeof(req.target) - 1);
	req.target[sizeof(req.target) - 1] = '\0';
	strncpy(req.msg, msg, sizeof(req.msg) - 1);
	req.msg[sizeof(req.msg) - 1] = '\0';


	//open the server FIFO (O_WRONLY)
	server_fd = open("serverFIFO", O_WRONLY);
	if (server_fd == -1) {
		perror("rsh: Failed to open serverFIFO");
		return;
	}
	//write the message structure to the server's FIFO
	if (write(server_fd, &req, sizeof(struct message)) == -1) {
		perror("rsh: Failed to write request to serverFIFO");
	}

	//close the server FIFO
	close(server_fd);
}

void* messageListener(void *arg) {
	// TODO:
	// Read user's own FIFO in an infinite loop for incoming messages
	// The logic is similar to a server listening to requests
	// print the incoming message to the standard output in the
	// following format
	// Incoming message from [source]: [message]
	// put an end of line at the end of the message



	int user_fifo;
	struct message incoming_msg;

	//open the user FIFO
	user_fifo = open(uName, O_RDONLY);
	if (user_fifo == -1) {
		perror("rsh: Failed to open user's own FIFO for reading");
		pthread_exit((void*)-1);
	}

	//open another file descriptor (dummyfd)
	
	int dummyfd = open(uName, O_WRONLY);
	if (dummyfd == -1) {
		perror("rsh: Failed to open user's own FIFO for writing (dummy)");
		close(user_fifo);
		pthread_exit((void*)-1);
	}
	
	while (1) {
		//read the message from the user FIFO
		if (read(user_fifo, &incoming_msg, sizeof(struct message)) == sizeof(struct message)) {
			//print the message


			printf("Incoming message from %s: %s\n", incoming_msg.source, incoming_msg.msg);
			fprintf(stderr,"rsh>"); //reprint the prompt to make it look better

			fflush(stdout);
			fflush(stderr);
		} else {
			//handle read error


			continue;
		}
	}


	close(user_fifo);
	close(dummyfd);

	pthread_exit((void*)0);


}

int isAllowed(const char*cmd) {
	int i;
	for (i=0;i<N;i++) {
		if (strcmp(cmd,allowed[i])==0) {
			return 1;
		}
	}
	return 0;
}

int main(int argc, char **argv) {
    pid_t pid;
    char **cargv; 
    char *path;
    char line[256];
    int status;
    posix_spawnattr_t attr;

    if (argc!=2) {
	printf("Usage: ./rsh <username>\n");
	exit(1);
    }
    signal(SIGINT,terminate);

    strcpy(uName,argv[1]);

    // TODO:
    // create the message listener thread

    pthread_t listener_thread;

    //detach the thread
    pthread_detach(listener_thread); 



    while (1) {

	fprintf(stderr,"rsh>");

	if (fgets(line,256,stdin)==NULL) continue;

	if (strcmp(line,"\n")==0) continue;

	line[strlen(line)-1]='\0';

	char cmd[256];
	char line2[256];
	strcpy(line2,line);
	strcpy(cmd,strtok(line," "));

	if (!isAllowed(cmd)) {
		printf("NOT ALLOWED!\n");
		continue;
	}

	if (strcmp(cmd,"sendmsg")==0) {
		// TODO: Create the target user and
		// the message string and call the sendmsg function

		// NOTE: The message itself can contain spaces
		// If the user types: "sendmsg user1 hello there"
		// target should be "user1" 
		// and the message should be "hello there"

		// if no argument is specified, you should print the following
		// printf("sendmsg: you have to specify target user\n");
		// if no message is specified, you should print the followingA
 		// printf("sendmsg: you have to enter a message\n");
		char *line_ptr = line2 + strlen("sendmsg");
		//skips the initial spaces
		while (*line_ptr == ' ' || *line_ptr == '\t') {
			line_ptr++;
		}

		//find the end of the target username

		char *target_end = strchr(line_ptr, ' ');
		
		//no arguments are provided

		if (!*line_ptr) {
			printf("sendmsg: you have to specify target user\n");
			continue;
		}

		//get the target username
		char target_user[50];

		//
		if (target_end == NULL) {
			strncpy(target_user, line_ptr, sizeof(target_user) - 1);
			target_user[sizeof(target_user) - 1] = '\0';
			printf("sendmsg: you have to enter a message\n");
			continue;
		} else {
			//get the length of target name
			size_t target_len = target_end - line_ptr;
			strncpy(target_user, line_ptr, target_len);
			target_user[target_len] = '\0';

			//move past the  target name and spaces and find start of message
			char *msg_start = target_end;
			while (*msg_start == ' ' || *msg_start == '\t') {
				msg_start++;
			}


			//case when target user is provided but not the message

			if (!*msg_start) {


				printf("sendmsg: you have to enter a message\n");
				continue;
			}

			//message is the rest of the string

			sendmsg(uName, target_user, msg_start);
		}


		continue;

	}

	if (strcmp(cmd,"exit")==0) break;

	if (strcmp(cmd,"cd")==0) {
		char *targetDir=strtok(NULL," ");
		if (strtok(NULL," ")!=NULL) {
			printf("-rsh: cd: too many arguments\n");
		}
		else {
			chdir(targetDir);
		}
		continue;
	}

	if (strcmp(cmd,"help")==0) {
		printf("The allowed commands are:\n");
		for (int i=0;i<N;i++) {
			printf("%d: %s\n",i+1,allowed[i]);
		}
		continue;
	}

	cargv = (char**)malloc(sizeof(char*));
	cargv[0] = (char *)malloc(strlen(cmd)+1);
	path = (char *)malloc(9+strlen(cmd)+1);
	strcpy(path,cmd);
	strcpy(cargv[0],cmd);

	char *attrToken = strtok(line2," "); /* skip cargv[0] which is completed already */
	attrToken = strtok(NULL, " ");
	int n = 1;
	while (attrToken!=NULL) {
		n++;
		cargv = (char**)realloc(cargv,sizeof(char*)*n);
		cargv[n-1] = (char *)malloc(strlen(attrToken)+1);
		strcpy(cargv[n-1],attrToken);
		attrToken = strtok(NULL, " ");
	}
	cargv = (char**)realloc(cargv,sizeof(char*)*(n+1));
	cargv[n] = NULL;

	// Initialize spawn attributes
	posix_spawnattr_init(&attr);

	// Spawn a new process
	if (posix_spawnp(&pid, path, NULL, &attr, cargv, environ) != 0) {
		perror("spawn failed");
		exit(EXIT_FAILURE);
	}

	// Wait for the spawned process to terminate
	if (waitpid(pid, &status, 0) == -1) {
		perror("waitpid failed");
		exit(EXIT_FAILURE);
	}

	// Destroy spawn attributes
	posix_spawnattr_destroy(&attr);

    }
    return 0;
}
