// Using file descriptors to redirect output
// DSM, 2016
// updated, VHS 2019

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

int main(void) {
    
    int rc = fork();

    // Child runs execvp
    if (rc == 0) {
        
        // Close stdout, freeing its filedescriptor
        // Recall: stdin has descriptor 0, stdout has 1, stderr has 2
        close(1);
        
        // Open another file, which gets the lowest numbered file descriptor
        // available: the one just freed by closing stdout
        open("output.txt", O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
        
        // Now, any output that would go to stdout by default will go to
        // the file instead!
        
        // Call ls
        char *args[3];
        args[0] = "ls";
        args[1] = "-l";
        args[2] = NULL;
        
        execvp(args[0], args);
        
        // We should never reach here
        perror("execvp");
        return -1;
    }
    
    // Parent waits for child to finish
    else if (rc > 0) {
        // The basic wait system call suspends the calling process until one
        // of its children terminates
        int rc = wait(NULL);
        if (rc < 0) {
            perror("wait");
            return -1;
        }
    }
    
    else {
        perror("fork");
    }
    
    return 0;
}
