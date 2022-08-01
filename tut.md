Piping Tutorial For Pipex / Minishell



If you've tried writing a shell before you might have noticed a pretty complex feature : piping commands. In this tutorial I'll try to take you from zero to hero (or close to that) and walk you through the process of using fork, the exec family of functions, pipe, and dup/dup2.



For this tutorial to not be too long I'll continue with the assumption that you know what a file descriptor, a process, and a pid (process id) are, and that can write code in C. You can find most of this by running `man 2 read 2 write 2 fork` and I will be using these functions `man 2 fork 2 dup2 2 close 2 execlp 3 perror`.



The idea behind setting up pipes is in it's essence quite simple : a pipe is just two file descriptors, a 'read end' and a 'write end', with the 'read end' reading what's written to the 'write end'. By setting up a pipe for each pair of executables that should work together and making these programs replace standard in / standard out with pipe outputs / inputs we'll have piped them together.
Here's some pseudo-code for making a program that runs `ls | wc` that we will gradually transform into valid C code
```C
/*
functions :
-createpipe -> creates a pipe with a read and write end
-child -> runs the following bracketed code inside a child process
-run -> executes the command given to it and completely stops the ongoing process
-changefd new_fd std_stream -> makes reading/writing from std_strean read/write from newfd

variables :
-p -> the pipe between ls and wc
*/

p = createpipe()
child {
	//this child will run the code for the wc command, we will start the last commands first for simplicity reasons
	changefd(p.read, standard_in) //wc will now read from the pipe rather than the standard in
	run("wc")
}
child {
	//ls child
	changefd(p.write, standard_out) //ls will write to the pipe that wc reads from. We have 'tricked it' into thinking that the pipe is the standard output. Calls to write(1, *, *) now write to the pipe p
	run("ls")
}
```
the program creates 2 child processes, makes each of them interract with the pipe and then run a command.



Let's start using `fork` ! The `fork` function will duplicate the current process (including stack and heap memory) and make both processes continue from the same point
```C
//fork
#include <sys/types.h>
#include <unistd.h>

//printf
#include <stdio.h>

int main() {
	int n = 9;
	int childpid;

	childpid = fork();
	printf("9 + 1 = %d | childpid = %d\n", ++n, childpid);
	//because the memory is duplicated, modifying n in both processes isn't a data race situation
}
```
In this simple demo program you'll see that the line after `fork` is ran twice, that is one per process. However it's output is slightly different. That is because `fork` returns a different value on both processes : the child (new) process is returned 0 whereas the parent (original) process gets the pid of the created process. We can therefore think of the pseudo-code child function as the following C macro.
```C
define child if (fork()==0)
```
If `fork` fails it will return -1, with all this new knowledge the pseudo-code looks like this now :
```C
/*
functions :
-createpipe -> creates a pipe with a read and write end
-run -> executes the command given to it and completely stops the ongoing process
-changefd new_fd std_stream -> makes reading/writing from std_strean read/write from newfd

variables :
-p -> the pipe between ls and wc
-childpid -> the result of the fork() function
*/

int childpid;
p = createpipe()

childpid = fork();
if (childpid == -1) {
	perror("fork error");
} else if (childpid == 0) {
	//wc child
	changefd(p.read, standard_in)
	run("wc")
}

childpid = fork();
if (childpid == -1) {
	perror("fork error");
} else if (childpid == 0) {
	//ls child
	changefd(p.write, standard_out)
	run("ls")
}
```



That was the hardest function, let's do something easier now ^^. The `pipe` functions take an array of 2 integers 'p' as it's single argument then sets `p[0]` to a read file descriptor and `p[1]` to a write file descriptor. It returns 0 on success and -1 if it fails. For convenience we'll now use the following macros :
```C
#define STDIN 0
#define STDOUT 1
#define PREAD 0
#define PWRITE 1
```
Our pseudo-code with safety precautions now looks something like this :
```C
/*
functions :
-run -> executes the command given to it and completely stops the ongoing process
-changefd new_fd std_stream -> makes reading/writing from std_strean read/write from newfd

variables :
-p -> the pipe between ls and wc
-childpid -> the result of the fork() function
*/

#define STDIN 0
#define STDOUT 1
#define PREAD 0
#define PWRITE 1

int main() {
	int childpid;
	int p[2];

	if (pipe(p))
		perror("pipe error")
	childpid = fork();
	if (childpid == -1) {
		perror("fork error");
	} else if (childpid == 0) {
		//wc child
		changefd(p[PREAD], STDIN)
		run("wc")
	}

	childpid = fork();
	if (childpid == -1) {
		perror("fork error");
	} else if (childpid == 0) {
		//ls child
		changefd(p[PWRITE], STDOUT)
		run("ls")
	}
}
```

//todo : CLOSE, SAFE CLOSES, DUP2, SAFE DUP2, EXEC, SAFE EXEC
+CLOSE PIPE IN MAIN






FINAL SINGLE PIPE CODE
```C
//perror
#include <stdio.h>
//pipe, close, dup2, execlp
#include <unistd.h>
//fork
#include <sys/types.h>
#include <unistd.h>

#define STDIN 0
#define STDOUT 1
#define PREAD 0
#define PWRITE 1

int main() {
	int childpid;
	int p[2];

	if (pipe(p))
		perror("pipe error");
	childpid = fork();
	if (childpid == -1) {
		perror("fork error");
	} else if (childpid == 0) {
		//wc child
		if (dup2(p[PREAD], STDIN)== -1)
			perror("dup2 error");
		if (close(p[PWRITE])
			|| close(p[PREAD]))
			perror("close error");
		execlp("/bin/wc", "wc", NULL);
		perror("exec error");
	}

	childpid = fork();
	if (childpid == -1) {
		perror("fork error");
	} else if (childpid == 0) {
		//ls child
		if (dup2(p[PWRITE], STDOUT)== -1)
			perror("dup2 error");
		if (close(p[PREAD])
			|| close(p[PWRITE]))
			perror("close error");
		execlp("/bin/ls", "ls", NULL);
		perror("exec error");
	}
	close(p[0]);
	close(p[1]);
}
```
