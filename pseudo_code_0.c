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
        //this child will run the code for the wc command, we will start the last commands first for      simplicity reasons
        changefd(p.read, standard_in) //wc will now read from the pipe rather than the standard in
        run("wc")
}
child {
        //ls child
        changefd(p.write, standard_out) //ls will write to the pipe that wc reads from. We have 'tricked  it' into thinking that the pipe is the standard output. Calls to write(1, *, *) now write to the pipe p
        run("ls")
}
