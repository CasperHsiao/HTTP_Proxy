#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <syslog.h>

#include "proxy.hpp"

int main() {
  pid_t pid1, pid2, sid;

  // Fork off the parent process
  pid1 = fork();
  // Error check fork
  if (pid1 < 0) {
    exit(EXIT_FAILURE);
  }
  // Exit parent process
  if (pid1 > 0) {
    exit(EXIT_SUCCESS);
  }

  umask(0);

  pid2 = fork();
  if (pid2 < 0) {
    exit(EXIT_FAILURE);
  }
  if (pid2 > 0) {
    exit(EXIT_SUCCESS);
  }

  /* Open any logs here */

  // Create a new SID for the child process
  sid = setsid();
  if (sid < 0) {
    /* Log the failure */
    exit(EXIT_FAILURE);
  }

  // Change the current working directory to root
  if ((chdir("/")) < 0) {
    /* Log the failure */
    exit(EXIT_FAILURE);
  }

  /* Close out the standard file descriptors */
  int dev_null_fd;
  if ((dev_null_fd = open("/dev/null", O_RDWR)) == -1) {
    exit(EXIT_FAILURE);
  }
  dup2(dev_null_fd, STDIN_FILENO);
  dup2(dev_null_fd, STDOUT_FILENO);
  dup2(dev_null_fd, STDERR_FILENO);

  // Initialize the proxy daemon
  Proxy proxy;
  while (true) {
    proxy.run();
    sleep(30); /* wait 30 seconds */
  }
  exit(EXIT_SUCCESS);
}
