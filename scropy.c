#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

void print_usage(char *program_name);

int main(int argc, char ** argv) {
  if(argc < 3) {
    // Insufficient arguments
    print_usage(argv[0]);
    return -1;
  }
  int fd_src = open(argv[1], O_RDONLY);
  if(fd_src == -1) {
    // Failed to open source file
    perror("open SOURCE");
    print_usage(argv[0]);
    return -1;
  }
  // Truncate or create destination with permissions 644
  int fd_dst = open(argv[2], O_CREAT|O_TRUNC|O_WRONLY, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
  if(fd_dst == -1) {
    // Failed to open/create destination file
    perror("open DEST");
    print_usage(argv[0]);
    return -1;
  }

  char buf[2];
  ssize_t nr;
  // Read SOURCE until EOF or error
  while((nr = read(fd_src, buf, 2)) != 0 && nr != -1) {
    if(nr == 2) {
      // Swap characters
      char tmp = *buf;
      *buf = *(buf+1);
      *(buf+1) = tmp;
    }
    ssize_t n = write(fd_dst, buf, nr);
    if(n == -1) {
      perror("write DEST");
    }
  }
  if(nr == -1) {
    perror("read SOURCE");
  }
  close(fd_src);
  close(fd_dst);
  return 0;
}

void print_usage(char *program_name) {
    fprintf(stderr, "usage: %s SOURCE DEST\n", program_name);
}
