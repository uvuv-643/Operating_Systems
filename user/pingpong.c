#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#define PING_PONG_MAX_BUFFER_SIZE 1024

int
main(int argc, char *argv[])
{
  
    int main_pipe[2];
    pipe(main_pipe);

    pde_t pid = fork();

    if (pid == 0) {

        // child proccess

        char* message;

        // wait while parent wont send something
        while (read(main_pipe[0], &message, PING_PONG_MAX_BUFFER_SIZE) == 0);
        printf("%d: got %s\n", getpid(), message);

        // writting response
        char* response_message = "pong";
        write(main_pipe[1], &response_message, 4);

        // wait until parent proccess won't signal about finishing ping-pong
        char close_byte;
        while (read(main_pipe[0], &close_byte, 1) == 0);

        close(main_pipe[0]);
        close(main_pipe[1]);

        exit(0);

    } else {

        // parent proccess

        char* message = "ping";
        write(main_pipe[1], &message, PING_PONG_MAX_BUFFER_SIZE);

        char* response_message;
        while (read(main_pipe[0], &response_message, PING_PONG_MAX_BUFFER_SIZE) == 0);
        printf("%d: got %s\n", getpid(), response_message);
    
        // write 1 byte to indicate that programm finished to child proccess
        // to prevent closing child process before printing all information
        char close_byte = 'c';
        write(main_pipe[1], &close_byte, 1);

        close(main_pipe[0]);
        close(main_pipe[1]);

        exit(0);

    }

}
