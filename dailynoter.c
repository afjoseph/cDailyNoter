#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define EDITOR "/usr/bin/nvim"
#define JOURNAL_DIR "journal"
#define NOTES_DIR "/home/cheese/drive/notes/"
#define NUM_OF_DAYS 3
#define PATH_SIZE 512

char **formatted_dates;

void fetch_dates();
void open_files(const char **);
void create_file(const char **);
void fetch_template_data(char **, const char *);

/**
 * - Get env variable
 * - Fetch current timestamp with correct format
 * - Check if file exist
 *   - If it does, open it
 * - If not, format it
 */
int 
main() {
    int i;
    char **file_paths;
    const int path_size = PATH_SIZE;

    // Get env variable
    /*const char* notes_dir_path = getenv("NOTES_DIR");*/
    /*if (notes_dir_path == NULL) {*/
        /*perror("getenv");*/
        /*exit(-1);*/
    /*}*/
    const char *notes_dir_path = NOTES_DIR;

    fetch_dates();

    // cd into dir to allow vim to fuzzy search notes directory
    chdir(notes_dir_path);

    // Get file paths
    file_paths = (char **) malloc(NUM_OF_DAYS * sizeof(char *));
    for (i = 0; i < NUM_OF_DAYS; i++) {
        file_paths[i] = (char *)malloc(path_size * sizeof(char));
        snprintf(file_paths[i], path_size, 
                "%s/%s/%s.rst", notes_dir_path, JOURNAL_DIR, formatted_dates[i]);
    }

    if (access(file_paths[0], F_OK) != -1) {
        open_files((const char **)file_paths);
    } else {
        create_file((const char **)file_paths);
    }


    for (i = 0; i < NUM_OF_DAYS; i++) {
        free(file_paths[i]);
        free(formatted_dates[i]);
    }
    free(file_paths);
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

    formatted_dates = (char **)malloc(NUM_OF_DAYS * sizeof(char *));

    for (i = 0; i < NUM_OF_DAYS; i++) {
        formatted_dates[i] = (char *)malloc(PATH_SIZE * sizeof(char));
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
fetch_template_data(char **out, const char *path) {
    int len = 0;
    const int template_size = 1000;
    char *buffer = (char *)alloca(template_size);

    len += snprintf(buffer+len, template_size-len, "%s\n", formatted_dates[0]);
    len += snprintf(buffer+len, template_size-len, "\n");
    len += snprintf(buffer+len, template_size-len, "# Tasks\n");

    *out = strndup(buffer, strlen(buffer));
}
