#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define EDITOR "/usr/bin/nvim"
#define NUM_OF_DAYS 3
#define PATH_SIZE 256

char **formatted_dates;

void fetch_dates();
void open_files(const char **);
void create_file(const char **);
void fetch_template_data(char **, const char const*);

/**
 * - Get env variable
 * - Fetch current timestamp with correct format
 * - Check if file exist
 *   - If it does, open it
 * - If not, format it
 */
int 
main(int argc, char *argv[]) {
    int i;
    time_t t;
    struct tm *tmp;
    char **note_paths;
    const int path_size = PATH_SIZE;


    // Get env variable
    const char* journal_path = getenv("JOURNAL_DIR");
    if (journal_path == NULL) {
        perror("getenv");
        exit(-1);
    }

    fetch_dates();

    note_paths = malloc(NUM_OF_DAYS * sizeof(char *));
    for (i = 0; i < NUM_OF_DAYS; i++) {
        note_paths[i] = malloc(path_size * sizeof(char));
        snprintf(note_paths[i], path_size, 
                "%s/%s.md", journal_path, formatted_dates[i]);
    }

    if (access(note_paths[0], F_OK) != -1) {
        open_files((const char **)note_paths);
    } else {
        create_file((const char **)note_paths);
    }


    for (i = 0; i < NUM_OF_DAYS; i++) {
        free(note_paths[i]);
        free(formatted_dates[i]);
    }
    free(note_paths);
    free(formatted_dates);
}

void 
open_files(const char **paths) {
    int total_len, buf_len, i = 0;
    pid_t pid;
    char *cmd;

    total_len = strlen((const char *)EDITOR);
    for (i = 0; i < NUM_OF_DAYS; i++) {
        total_len += strlen(paths[i]) + 5;
    }
    total_len += 5;

    buf_len = 0;
    cmd = (char *)alloca(total_len);
    buf_len = snprintf(cmd+buf_len, total_len-buf_len, "%s ", EDITOR);
    for (i = 0; i < NUM_OF_DAYS; i++) {
        buf_len += snprintf(cmd+buf_len, total_len-buf_len, "%s ", paths[i]);
    }

    switch ((pid = fork())) {
        case -1:
            perror("fork");
            exit(-1);
            break;
        case 0:
            /*if (-1 == execve(argv[0], (char **)argv, NULL)) {*/
            if (-1 == execl("/bin/sh", "/bin/sh", "-c", cmd, NULL)) {
                perror("execve");
                exit(-1);
            }
            break;
    }

    waitpid(pid, 0, 0);
}

void 
create_file(const char **paths) {
    FILE *file;
    int i;
    char *buffer;

    if ((file = fopen(paths[0], "w")) == NULL) {
        perror("fopen");
        exit(-1);
    }

    fetch_template_data(&buffer, paths[0]);

    fprintf(file, buffer);
    fclose(file);

    open_files(paths);
}

void
fetch_dates() {
    int i;
    struct tm *days[NUM_OF_DAYS];

    formatted_dates = malloc(NUM_OF_DAYS * sizeof(char *));

    for (i = 0; i < NUM_OF_DAYS; i++) {
        formatted_dates[i] = malloc(PATH_SIZE * sizeof(char));
        time_t now = time(NULL);
        now = now - i * (24 * 60 * 60);
        days[i] = localtime(&now);
        if (strftime(formatted_dates[i], PATH_SIZE, "%y-%m-%d", days[i]) == 0) {
            perror("strfntime");
            exit(-1);
        }
    }
}

void 
fetch_template_data(char **out, const char const *path) {
    int i, len = 0;
    const int template_size = 1000;
    char *buffer = alloca(template_size);

    len += snprintf(buffer+len, template_size-len, "%s\n", formatted_dates);
    len += snprintf(buffer+len, template_size-len, "\n");
    len += snprintf(buffer+len, template_size-len, "# Tasks\n");
    len += snprintf(buffer+len, template_size-len, "\n");

    *out = strndup(buffer, strlen(buffer));
}
