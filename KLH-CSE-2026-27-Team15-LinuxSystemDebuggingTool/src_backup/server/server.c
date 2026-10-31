#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/wait.h>

#define PORT 8080
#define BUFFER_SIZE 262144

void send_response(int client, const char *body)
{
    char header[512];

    snprintf(header, sizeof(header),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain; charset=utf-8\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Methods: GET, OPTIONS\r\n"
        "Access-Control-Allow-Headers: Content-Type\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n"
        "\r\n",
        strlen(body));

    write(client, header, strlen(header));
    write(client, body, strlen(body));
}

void save_debug_log(const char *result, const char *program)
{
    time_t now = time(NULL);
    struct tm *time_info = localtime(&now);
    char filename[160];

    if (time_info == NULL) {
        fprintf(stderr, "[Log] Failed to get current time.\n");
        return;
    }

    if (strftime(filename, sizeof(filename),
                 "logs/debug_%Y%m%d_%H%M%S.log",
                 time_info) == 0) {
        fprintf(stderr, "[Log] Could not create filename.\n");
        return;
    }

    FILE *log_file = fopen(filename, "w");

    if (log_file == NULL) {
        perror("[Log] Could not create log file");
        return;
    }

    fprintf(log_file, "Selected program: %s\n\n", program);
    fprintf(log_file, "%s", result);
    fclose(log_file);

    printf("[Log] Debugging output saved to: %s\n", filename);
}

void run_debugger(int client, const char *program)
{
    FILE *fp;
    char output[4096];
    char result[BUFFER_SIZE] = "";

    const char *program_path;

    if (strcmp(program, "ls") == 0) {
        program_path = "/bin/ls";
    } else if (strcmp(program, "whoami") == 0) {
        program_path = "/usr/bin/whoami";
    } else {
        program = "target_program";
        program_path = "./target_program";
    }

    /*
     * The program name comes only from the allowlist above.
     * The debugger reads TARGET_PROGRAM when it starts.
     */
    if (setenv("TARGET_PROGRAM", program_path, 1) != 0) {
        send_response(client, "Failed to set target program.");
        return;
    }

    fp = popen("./debugger", "r");

    /*
     * popen has started its child process, which inherits
     * the environment. Remove the variable from the server.
     */
    unsetenv("TARGET_PROGRAM");

    if (fp == NULL) {
        send_response(client, "Failed to start debugger.");
        return;
    }

    while (fgets(output, sizeof(output), fp) != NULL) {
        size_t used = strlen(result);
        size_t incoming = strlen(output);

        if (used + incoming < sizeof(result) - 1) {
            memcpy(result + used, output, incoming + 1);
        } else {
            fprintf(stderr,
                    "[Warning] Debugger output reached buffer limit.\n");
            break;
        }
    }

    int status = pclose(fp);

    size_t len = strlen(result);

    while (len > 0 &&
           (result[len - 1] == '\n' || result[len - 1] == '\r')) {
        result[--len] = '\0';
    }

    if (len + 1 < sizeof(result)) {
        result[len++] = '\n';
        result[len] = '\0';
    }

    save_debug_log(result, program);

    if (status == -1) {
        fprintf(stderr,
                "[Server] Failed to collect debugger exit status.\n");
    } else if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
        fprintf(stderr,
                "[Server] Debugger exited with status %d.\n",
                WEXITSTATUS(status));
    }

    send_response(client, result);
}

int compare_log_files(const void *a, const void *b)
{
    const char *file_a = (const char *)a;
    const char *file_b = (const char *)b;

    /* Descending order: newest timestamp first */
    return strcmp(file_b, file_a);
}


void send_debug_history(int client)
{
    DIR *dir = opendir("logs");
    struct dirent *entry;

    char files[1000][256];
    int file_count = 0;

    char json[65536] = "[";
    int first = 1;

    if (dir == NULL) {
        send_response(client, "[]");
        return;
    }

    /* Collect valid log filenames */
    while ((entry = readdir(dir)) != NULL) {
        size_t len = strlen(entry->d_name);

        if (len != 25 ||
            strncmp(entry->d_name, "debug_", 6) != 0 ||
            entry->d_name[14] != '_' ||
            strcmp(entry->d_name + 21, ".log") != 0) {
            continue;
        }

        int valid = 1;

        for (int i = 6; i < 14; i++) {
            if (entry->d_name[i] < '0' ||
                entry->d_name[i] > '9') {
                valid = 0;
                break;
            }
        }

        for (int i = 15; i < 21; i++) {
            if (entry->d_name[i] < '0' ||
                entry->d_name[i] > '9') {
                valid = 0;
                break;
            }
        }

        if (!valid || file_count >= 1000) {
            continue;
        }

        snprintf(files[file_count], sizeof(files[file_count]),
                 "%s", entry->d_name);
        file_count++;
    }

    closedir(dir);

    /* Sort newest filename timestamp first */
    qsort(files, file_count, sizeof(files[0]), compare_log_files);

    /* Build the JSON response */
    for (int i = 0; i < file_count; i++) {
        char path[512];
        char line[256] = "Selected program: unknown";

        snprintf(path, sizeof(path), "logs/%s", files[i]);

        FILE *fp = fopen(path, "r");

        if (fp == NULL) {
            continue;
        }

        if (fgets(line, sizeof(line), fp) == NULL) {
            strcpy(line, "Selected program: unknown");
        }

        fclose(fp);

        const char *prefix = "Selected program: ";
        char *program = line;

        if (strncmp(line, prefix, strlen(prefix)) == 0) {
            program = line + strlen(prefix);
        }

        program[strcspn(program, "\r\n")] = '\0';

        if (strcmp(program, "ls") != 0 &&
            strcmp(program, "whoami") != 0 &&
            strcmp(program, "target_program") != 0) {
            strcpy(program, "unknown");
        }

        char item[1024];

snprintf(item, sizeof(item),
         "%s{\"file\":\"%s\",\"program\":\"%.255s\"}",
         first ? "" : ",", files[i], program);

        if (strlen(json) + strlen(item) + 2 >= sizeof(json)) {
            break;
        }

        strcat(json, item);
        first = 0;
    }

    strcat(json, "]");
    send_response(client, json);
}


void send_debug_log(int client, const char *filename)
{
    size_t len = strlen(filename);

    /* Expected format: debug_YYYYMMDD_HHMMSS.log */
    if (len != 25 ||
        strncmp(filename, "debug_", 6) != 0 ||
        filename[14] != '_' ||
        strcmp(filename + 21, ".log") != 0) {
        send_response(client, "Invalid log filename.");
        return;
    }

    /* Validate date and time characters */
    for (size_t i = 6; i < 14; i++) {
        if (filename[i] < '0' || filename[i] > '9') {
            send_response(client, "Invalid log filename.");
            return;
        }
    }

    for (size_t i = 15; i < 21; i++) {
        if (filename[i] < '0' || filename[i] > '9') {
            send_response(client, "Invalid log filename.");
            return;
        }
    }

    char path[128];
    snprintf(path, sizeof(path), "logs/%s", filename);

    FILE *fp = fopen(path, "r");

    if (fp == NULL) {
        send_response(client, "Log file not found.");
        return;
    }

    char *content = malloc(262144);

    if (content == NULL) {
        fclose(fp);
        send_response(client, "Not enough memory to read log.");
        return;
    }

    size_t count = fread(content, 1, 262143, fp);
    content[count] = '\0';
    fclose(fp);

    send_response(client, content);
    free(content);
}


int main(void)
{
    int server_fd;
    int client;

    struct sockaddr_in address;
    char buffer[4096];

    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd < 0) {
        perror("Socket failed");
        return 1;
    }

    int option = 1;

    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR,
               &option, sizeof(option));

    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address,
             sizeof(address)) < 0) {
        perror("Bind failed");
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, 5) < 0) {
        perror("Listen failed");
        close(server_fd);
        return 1;
    }

    printf("=====================================\n");
    printf("     LINUX DEBUGGER WEB SERVER\n");
    printf("=====================================\n");
    printf("Server running at http://localhost:%d\n", PORT);
    printf("Supported targets: target_program, ls, whoami\n");

    while (1) {
        socklen_t address_length = sizeof(address);

        client = accept(server_fd,
                        (struct sockaddr *)&address,
                        &address_length);

        if (client < 0) {
            perror("Accept failed");
            continue;
        }

        memset(buffer, 0, sizeof(buffer));

        ssize_t bytes_read = read(client, buffer, sizeof(buffer) - 1);

        if (bytes_read <= 0) {
            close(client);
            continue;
        }

        buffer[bytes_read] = '\0';

        printf("[Server] Request received\n");

        
if (strncmp(buffer, "OPTIONS ", 8) == 0) {
    send_response(client, "");
}
else if (strncmp(buffer, "GET /history ", 13) == 0) {
    send_debug_history(client);
}
else if (strncmp(buffer, "GET /log?file=", 14) == 0) {
    char filename[128] = "";
    char *start = buffer + 14;
    size_t i = 0;

    while (start[i] != '\0' &&
           start[i] != ' ' &&
           start[i] != '&' &&
           start[i] != '\r' &&
           start[i] != '\n' &&
           i < sizeof(filename) - 1) {
        filename[i] = start[i];
        i++;
    }

    filename[i] = '\0';
    send_debug_log(client, filename);
}
else if (strncmp(buffer, "GET /run", 8) == 0 &&
         (buffer[8] == ' ' || buffer[8] == '?')) {

    char program[64] = "target_program";
    char *query = strchr(buffer, '?');

    if (query != NULL) {
        char *parameter = strstr(query, "program=");

        if (parameter != NULL) {
            parameter += strlen("program=");
            size_t i = 0;

            while (parameter[i] != '\0' &&
                   parameter[i] != '&' &&
                   parameter[i] != ' ' &&
                   parameter[i] != '\r' &&
                   parameter[i] != '\n' &&
                   i < sizeof(program) - 1) {
                program[i] = parameter[i];
                i++;
            }

            program[i] = '\0';
        }
    }

    if (strcmp(program, "target_program") != 0 &&
        strcmp(program, "ls") != 0 &&
        strcmp(program, "whoami") != 0) {
        send_response(client, "Invalid target program.");
    } else {
        printf("[Server] Selected target: %s\n", program);
        run_debugger(client, program);
    }
}
else {
    send_response(
        client,
        "Linux System Debugging Tool Server is running."
    );
}


        close(client);
    }

    close(server_fd);
    return 0;
}
