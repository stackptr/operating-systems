# Operating Systems

Programming assignments written for COP4610, taught by Matt Porter and Chris Lacher in Spring 2013.

The original assignment requirements are in the base folder while each project folder contains the deliverable code.

* **Assignment 1** - Create a shell similar to tcsh, supporting three built-in commands (cd, echo, exit), external command execution, file redirection, and environment variables.

* **Assignment 2** - Take the shell from assignment 1 and add background process support. This requires supporting the & operator a jobs command to list the current processes with job ID and process ID.

* **Assignment 3** - Perform matrix multiplication using multithreading with pthreads. Implement the producer-consumer project from the text using pthreads and mutex.

* **Assignment 4** - Create a clone utility that will copy the contents of a given directory to another directory. The utility will preserve all permissions and other metadata of the files and will only use low-level reading and writing functions (no use of external utilities such as cp). It should perform recursive copying without "copying the copy" if the destination directory exists in the source directory.