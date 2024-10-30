#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

char** split_command(const char* cmd) {
    char** parts = malloc(64 * sizeof(char*));
    char* cmd_copy = strdup(cmd);
    char* piece = strtok(cmd_copy, " ");
    int index = 0;

    while (piece != NULL) {
        parts[index++] = piece;
        piece = strtok(NULL, " ");
    }
    parts[index] = NULL;

    free(cmd_copy);
    return parts;
}

char* get_cmd_path(char* cmd) {
    if (cmd[0] == '/') return strdup(cmd);

    char* path_env = getenv("PATH");
    char* path_part = strtok(path_env, ":");
    char temp[256];

    while (path_part != NULL) {
        snprintf(temp, sizeof(temp), "%s/%s", path_part, cmd);
        if (access(temp, X_OK) == 0) {
            return strdup(temp);
        }
        path_part = strtok(NULL, ":");
    }
    return NULL;
}

int do_redir(char* in_file, char* cmd, char* out_file) {
    char** args = split_command(cmd);
    char* cmd_full_path = get_cmd_path(args[0]);
    if (!cmd_full_path) {
        perror("Couldn't find command");
        free(args);
        return 1;
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork failed");
        free(cmd_full_path);
        free(args);
        return 1;
    }

    if (pid == 0) {
        if (strcmp(in_file, "-") != 0) {
            int fd_in = open(in_file, O_RDONLY);
            if (fd_in == -1) {
                perror("Input file error");
                exit(1);
            }
            dup2(fd_in, STDIN_FILENO);
            close(fd_in);
        }

        if (strcmp(out_file, "-") != 0) {
            int fd_out = open(out_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd_out == -1) {
                perror("Output file error");
                exit(1);
            }
            dup2(fd_out, STDOUT_FILENO);
            close(fd_out);
        }

        execv(cmd_full_path, args);
        perror("execv failed");
        exit(1);
    } else {
        int status;
        waitpid(pid, &status, 0);
    }

    free(cmd_full_path);
    free(args);
    return 0;
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: redir <input> <command> <output>\n");
        return 1;
    }

    return do_redir(argv[1], argv[2], argv[3]);
}
