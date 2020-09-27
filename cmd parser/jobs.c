/* COMP 530: Tar Heel SHell
 *
 * This file implements functions related to launching
 * jobs and job control.
 */
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "thsh.h"
static char ** path_table;

/* Initialize the table of PATH prefixes.
 *
 * Split the result on the parenteses, and
 * remove any trailing '/' characters.
 * The last entry should be a NULL character.
 *
 * For instance, if one's PATH environment variable is:
 *  /bin:/sbin///
 *
 * Then path_table should be:
 *  path_table[0] = "/bin"
 *  path_table[1] = "/sbin"
 *  path_table[2] = '\0'
 *
 * Hint: take a look at getenv().  If you use getenv, do NOT
 *       modify the resulting string directly, but use
 *       malloc() or another function to allocate space and copy.
 *
 * Returns 0 on success, -errno on failure.
 */

//remove any trailing '/' characters.
void trimSlash(char * token) {
    for (int i = strlen(token) - 1; i >= 0; i--) {
        if (token[i] != '/') {
            token[i + 1] = '\0';
            break;
        }
    }
}

int init_path(void) {
    // get environment PATH variable
    char * s = getenv("PATH");
    if (s == NULL) {
        return errno;
    }
    // copy to another variable to avoid modifying the resulting string directly
    char * path = malloc(strlen(s) + 1);
    strcpy(path, s);

    // count the number of paths delimited by ':'
    int i, count;
    for (i = 0, count = 0; path[i]; i++)
        count += (path[i] == ':');

    // initialize path table
    path_table = malloc(sizeof( * path_table) * (count + 1));

    // use strtok to seperate string delimited by ':'
    char * token = strtok(path, ":");
    i = 0;
    while (token != NULL) {
        trimSlash(token);
        //put delimited entry in the path table
        path_table[i] = token;
        i++;
        token = strtok(NULL, ":");
    }
    // we could use this to assign the last entry of path table as "\0"
    // path_table[i]="/0"
    return 0;
}

/* Debug helper function that just prints
 * the path table out.
 */
void print_path_table() {
      if (path_table == NULL) {
        printf("XXXXXXX Path Table Not Initialized XXXXX\n");
        return;
      }

      printf("===== Begin Path Table =====\n");
      for (int i = 0; path_table[i]; i++) {
        printf("Prefix %2d: [%s]\n", i, path_table[i]);
      }
      printf("===== End Path Table =====\n");
}
