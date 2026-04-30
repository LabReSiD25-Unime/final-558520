#ifndef CLIENT_H
#define CLIENT_H

void cmd_log(FILE *log_file, const char *cmd);
void cmd_list(int clientfd, int data_fd, const char *cwd, FILE *log_file);
void cmd_retr(int client_fd, int data_fd, const char *cwd, const char *filename, FILE *log_file);
void cmd_stor(int client_fd, int data_fd, const char *cwd, const char *filename, FILE *log_file);
void ftp_reply(int fd, FILE *log_file, int code, const char *msg);
void cmd_cwd(int client_fd, char *cwd, const char *arg, FILE *log_file);
void handle_session(int client_fd);

#endif