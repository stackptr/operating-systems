/* Project 4: Clone Utility
 * Corey Johns
 * COP4610 Spring 2013
 * Deadline: 4/21/13
 *
 * Utility that recursively copies the contents of a source directory to a
 * destination directory using the UNIX API. All files that already exist in
 * the destination directory but that are not found in the source will be
 * removed.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>

#define BUF_LEN 80
#define BLOCKSIZE 4096

int set_perms(const char*, const char*);
int clone(char*, char*);
int clone_recursive(char*, char*,  char*);
int copy(char*, char*);
int remove_files(char*, char*);

int main(int argc, char **argv){
  const char* src = argv[1];  // Source directory, relative or absolute
  const char* dst = argv[2];  // Destination directory
  char src_abs[BUF_LEN];      // Absolute source path
  char dst_abs[BUF_LEN];      // Absolute destination path
  char init_abs[BUF_LEN];     // Absolute path of execution directory
  int mktarget = 0;           // Indicates if the destination dir is created

  if (argc != 3){
    printf("clone.x <source> <dest>\n");
    exit(1);
  }

  if(!strncmp(src, dst, BUF_LEN))
    exit(0); // Can this be handled somewhere else?

  getcwd(init_abs, BUF_LEN); // Record absolute path of initial directory

  // First check if source directory exists
  if (chdir(src)){
    printf("%s is not a valid source directory.\n", src);
    exit(1);
  }
  getcwd(src_abs, BUF_LEN); // Record absolute path for source

  // Return to previous directory and check for destination directory
  chdir(init_abs);
  if(chdir(dst)){ // Destination does not exist
    if(mkdir(dst, S_IRWXU | S_IRWXG | S_IRWXO)){ // Create new directory
      printf("ERROR: failed to create destination directory!\n");
      exit(1);
    }
    mktarget = 1;
    chdir(dst);
    getcwd(dst_abs, BUF_LEN); // Record absolute path for destination
    chdir(init_abs);

    printf("Creating directory %s\n", dst_abs); // Already done, so print msg.
  }
  else { // Destination exists, so record absolute path
    getcwd(dst_abs, BUF_LEN);
    chdir(init_abs);
  }

  // Set permissions of destination directory
  if (set_perms(src_abs, dst_abs)){
    printf("ERROR: failed to set permissions!");
    exit(1);
  }

  // Start recursive clone by supplying root source and destination
  if(clone(src_abs, dst_abs)){
    printf("ERROR: clone failed!\n");
    exit(1);
  }

  // Recursively remove existing files from target
  if(mktarget == 0) // Only necessariy if directory did not need to be created
    remove_files(src_abs, dst_abs);

  exit(0);
}

/* Function to copy the attributes of the a source file (arg 1) to a target
 * file (arg 2).
 *
 * Returns 0 when successful, 1 for failure.
 */
int set_perms(const char* s, const char* dst){
  struct stat src;  // Holds source attributes
  stat(s, &src);    // Retrieves source attributes.

  printf("Setting permissions for %s: %o\n", dst, src.st_mode & 07777);
  if (chmod(dst, src.st_mode))
    return 1;

  printf("Setting user and group for %s: %u, %u\n", dst,
      (unsigned int)src.st_uid, (unsigned int)src.st_gid);
  if (chown(dst, src.st_uid, src.st_gid))
    return 1;

  return 0;
}

/* Wrapper function for recursive clone function. Since it is necessary
 * to keep track of the original destination directory in order to avoid
 * "copying the copy", the true recursive function takes a third argument
 * --the original destination path--that will remain constant throughout
 * recursions. Since the third argument will always be a copy of the second,
 * it is convenient to have this function.
 */
int clone(char* src, char* dst){
  return clone_recursive(src, dst, dst);
}

/* Function to recursively copy files from a source directory (arg 1) to
 * a target directory (arg 2). Provides a check against copying the copy
 * through an abort path (arg 3), representing the original target path.
 *
 * Returns 0 on success, 1 on failure.
 */
int clone_recursive(char* src, char* dst, char* abort){
  DIR *dirp;                // Represents a directory stream
  struct dirent *entry;     // Holds a directory entity: d_ino and d_name
  char src_file[BUF_LEN+1]; // Holds source file path
  char dst_file[BUF_LEN+1]; // Holds destination file path

  if(!strncmp(src, dst, BUF_LEN))
    return 0; // Base case of recursive function

  // Open directory stream
  dirp = opendir(src);
  if (dirp == NULL){
    printf("ERROR: Could not open %s\n", src);
    return 1;
  }

  // Iterate through stream
  for (;;){
    entry = readdir(dirp);
    if(entry == NULL)
      break;
    if (!strncmp(entry->d_name, "..", 2) || !strncmp(entry->d_name, ".", 1))
      continue; // Skip ".." and "."

    // Construct absolute path for source and destination
    snprintf(src_file, BUF_LEN, "%s/%s", src, entry->d_name);
    snprintf(dst_file, BUF_LEN, "%s/%s", dst, entry->d_name);
    
    if (!strncmp(src_file, abort, BUF_LEN))
      continue;  // Skip if directory is source

    if (entry->d_type & DT_DIR){
      // Entry is a directory, so recurse
      printf("Creating directory %s\n", dst_file);
      // Check to see if directory already exists in destination
      if(chdir(dst_file)){
        if(mkdir(dst_file, 0)){
          printf("ERROR: failed to create directory!\n");
          return 1;
        }
      }

      set_perms(src_file, dst_file);

      // Clone new directory
      if(clone_recursive(src_file, dst_file, abort))
        return 1;
    }
    else {
      // Entry is a file, so copy it to destination
      printf("Copying %s to %s\n", src_file, dst_file);
      if(copy(src_file, dst_file)){
        printf("ERROR: file copy failed!\n");
        return 1;
      }
      if(set_perms(src_file, dst_file)){
        printf("ERROR: failed to set permissions!");
        return 1;
      }
    }
  }

  // Close directory stream
  closedir(dirp);
  return 0;

}


// Function to copy a file from src to dst.
// Returns 0 on successful copy, 1 on failure
int copy(char* src, char* dst){
  int src_fd, dst_fd;   // Holds src/dst file descriptors
  char buf[BLOCKSIZE];  // Buffer for data transfer
  ssize_t nread, nwritten;  // Keep track of bytes written and read
  char *out_ptr;

  // Open files
  src_fd = open(src, O_RDONLY);
  if (src_fd == -1){
    printf("ERROR: opening source file failed!\n");
    return 1;
  }

  dst_fd = open(dst, O_CREAT | O_WRONLY);
  if (dst_fd  == -1){
    printf("ERROR: opening destination file failed!\n");
    close(src_fd);
    return 1;
  }

  while (nread = read(src_fd, buf, BLOCKSIZE), nread > 0){
    out_ptr = buf;
    nwritten = 0;

    do {
      nwritten = write(dst_fd, out_ptr, nread);
      if (nwritten >= 0){
        nread -= nwritten;
        out_ptr += nwritten;
      }
      else if (errno != EINTR)
        return 1; // Ignore termination due to no data transfer
    } while (nread > 0);
  }

  close(dst_fd);
  close(src_fd);
  return 0;

}

// Removes all files from dst that do not exist in src.
// Returns 0 for success, 1 for failure.
int remove_files(char* src, char* dst){
  DIR *dirp_src, *dirp_dst;
  struct dirent *entry_src, *entry_dst;
  char src_file[BUF_LEN+1], dst_file[BUF_LEN+1];
  int found;
  
  if(!strncmp(src, dst, BUF_LEN))
    return 0; // Base case of recursive function
    
  dirp_dst = opendir(dst);
  if (dirp_dst == NULL){
    printf("ERROR: Could not open %s\n", dst);
    return 1;
  }

  for (;;){
    entry_dst = readdir(dirp_dst); // Read each dst entry
    found = 0;
    if(entry_dst == NULL)
      break;
    if (!strncmp(entry_dst->d_name, "..", 2) || !strncmp(entry_dst->d_name, ".", 1))
      continue; // Skip ".." and "."
    snprintf(src_file, BUF_LEN, "%s/%s", src, entry_dst->d_name);
    snprintf(dst_file, BUF_LEN, "%s/%s", dst, entry_dst->d_name);

    // Recurse up the directory before removing it
    if (entry_dst->d_type & DT_DIR)
      if(remove_files(src_file, dst_file))
        return 1;

    // Open source directory and check if file exists
    dirp_src = opendir(src);
    if(dirp_src == NULL){
      printf("Removing %s\n", dst_file);
      if (remove(dst_file)){
        printf("Error: Remove failed!\n");
        return 1;
      }
    }
    else{
      // Check each file in source dir for a match against dst dir
      for (;;){
        entry_src = readdir(dirp_src);
        if(entry_src == NULL)
          break;
        if (!strncmp(entry_src->d_name, "..", 2) || !strncmp(entry_src->d_name, ".", 1))
          continue; // Skip ".." and "."
        
        snprintf(src_file, BUF_LEN, "%s/%s", src, entry_src->d_name);

        //printf("Checking file %s against %s (Found=%d)\n", entry_dst->d_name, entry_src->d_name, found);
        if(!strncmp(entry_dst->d_name, entry_src->d_name, BUF_LEN)){
          found = 1;
          break;
        }
      }

      if (found == 0){
        printf("Removing %s\n", dst_file);
        if(remove(dst_file)){
          printf("Error: Remove failed!\n");
          return 1;
        }
      }
    }
  }

  return 0;
}
