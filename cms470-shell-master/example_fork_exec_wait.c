// Combination of fork(), exec(), and wait()
// DSM, 2016
// modified VHS, 2019

// Use wait to make the parent process suspend execution until its child
// finishes. This can be used to make an interactive shell.

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(void) {
    
    int rc = fork();
    printf("rc from fork: %d\n", rc);
  
    //rc from fork is 0 which means that this is the child process
    // Child executes another program (sleep) via execvp
    if (rc == 0) {
        printf("Child about to sleep for 10 seconds...\n");
      
        // Call sleep to make the process nap for 10 seconds
        char *args[3];
        args[0] = "sleep";
        args[1] = "10";
        args[2] = NULL;
        
        execvp(args[0], args);
        
        // We should never reach here
        perror("execvp");
        return -1;
    }
    
    // rc isn't 0 which means this is the parent process.
    // Parent waits for child to finish
    else if (rc > 0) {
        printf("Parent will wait for child to terminate...\n");
        
        // The basic wait system call 
        // passing NULL as argument suspends the calling process 
        // until one of its children terminates
        int rc = wait(NULL);
        if (rc < 0) {
            perror("wait");
            return -1;
        }
        
        printf("Parent is done waiting!\n");
    }
    
    else {
        perror("fork");
    }
    
    return 0;
}
