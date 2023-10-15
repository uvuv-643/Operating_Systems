#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  
    int text_pipe[2];
    int size_pipe[2];

    pipe(text_pipe);
    pipe(size_pipe);

    pde_t pid = fork();

    if (pid == 0) {

        // child proccess

        uint64 message_size;
        char* message;

        // wait while parent wont send something
        while (read(size_pipe[0], &message_size, sizeof(uint64)) == 0);
        while (read(text_pipe[0], &message, message_size) == 0);
        printf("%d: got %s\n", getpid(), message);

        // writting response
        char* response_message = "pong";
        uint64 response_size = strlen(response_message);
        write(size_pipe[1], &response_size, sizeof(uint64));
        write(text_pipe[1], &response_message, response_size);

        // wait until parent proccess won't signal about finishing ping-pong
        char close_byte;
        while (read(size_pipe[0], &close_byte, 1) == 0);

        close(size_pipe[0]);
        close(size_pipe[1]);
        close(text_pipe[0]);
        close(text_pipe[1]);

        exit(0);

    } else {

        // parent proccess

        char* message = "ping";
        uint64 message_size = strlen(message);

        write(size_pipe[1], &message_size, sizeof(uint64));
        write(text_pipe[1], &message, message_size);

        char* response_message;
        uint64 response_size;

        while (read(size_pipe[0], &response_size, sizeof(uint64)) == 0);
        while (read(text_pipe[0], &response_message, response_size) == 0);
        printf("%d: got %s\n", getpid(), response_message);
    
        // write 1 byte to indicate that programm finished to child proccess
        // to prevent closing child process before printing all information
        char close_byte = 'c';
        write(size_pipe[1], &close_byte, 1);

        close(size_pipe[0]);
        close(size_pipe[1]);
        close(text_pipe[0]);
        close(text_pipe[1]);

        exit(0);

    }

}
