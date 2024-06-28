#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/fcntl.h>
#include <arpa/inet.h>
#include <pthread.h>

#define BUFF_SIZE 1024
#define BACKLOG 10

char recv_data[BUFF_SIZE];
char *method;
char *req_file;
char *http_vs;
int port;
struct sockaddr_in server_addr, client_addr;
socklen_t cliaddr_size;
int server_sock, client_sock;

char * cont_type() {
	if (strstr(req_file, ".html") != NULL) return "text/html";
	else if (strstr(req_file, ".gif") != NULL) return "image/gif";
	else if (strstr(req_file, ".jpeg") != NULL) return "image/jpeg";
	else if (strstr(req_file, ".mp3") != NULL) return "audio/mpeg";
	else if (strstr(req_file, ".pdf") != NULL) return "application/pdf";
	else return "text/plain";
}

int main(int argc, char* argv[]) {
	if (argc != 2) {
		fprintf(stderr, "Usage: %s <port number>\n", argv[0]);
		exit(1);
	}
	// Get my server's port number
	port = atoi(argv[1]);
	if (port < 1025 || port > 65535) {
		fprintf(stderr, "Invalid port number. Choose a port between 1025 and 65535.\n");
		exit(1);
	}
	
	// Create a server socket
	if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket() failed");
		exit(1);
	}
	
	// Add information(the internet-specific) to created server socket
	bzero(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	
	// Bind a socket to a local IP address and port number
	if (bind(server_sock, (struct sockaddr *) &server_addr, sizeof(server_addr)) == -1) {
		perror("bind() failed");
		close(server_sock);
		exit(1);
	}
	
	/* Put socket into passive state
	  (wait for connections rather than initiate a connection)*/
	if (listen(server_sock, BACKLOG)==-1) {
		perror("listen() failed");
		close(server_sock);
		exit(1);
	}
	
	while (1) {
		// Accept a new connection
		cliaddr_size = sizeof(client_addr);
		if ((client_sock = accept(server_sock, (struct sockaddr *) &client_addr, &cliaddr_size)) == -1) {
			perror("accept() failed");
			close(server_sock);
			continue;
		}
		
		// Message requested
		printf("server: got connection from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
		if (recv(client_sock, recv_data, sizeof(recv_data)-1, 0) == -1) {
			perror("recv() failed");
			exit(1);
		}
		
		// Part A: Dump request messages to the console
		printf("%s\n", recv_data);
		
		// Part B: Parse request line to method, request_file, http_version
		char *request_line[3];
		method = (request_line[0] = strtok(recv_data, " "));
		req_file = (request_line[1] = strtok(NULL, " "));
		http_vs = (request_line[2] = strtok(NULL, " "));
		
		// GET handling
		if (req_file != NULL) {
			if (strncmp(method, "GET", 3) == 0) {
				char path[BUFF_SIZE];
				memset(path,0,sizeof(path));
				unsigned long file_len = 0;
				strcat(path, getenv("PWD"));
				
				// Request default file
				if (strcmp(req_file, "/") == 0) {
					// Send status line (protocol status code status phrase)
					strcat(path, "/default.html");

					// Get the requested file's length
					int file = open(path, O_RDONLY);
					int len = 0;
					int tmp[BUFF_SIZE];
					memset(tmp, 0, BUFF_SIZE * sizeof(*tmp));
					while (1) {
						len = read(file, tmp, BUFF_SIZE);
						if (len <= 0)
							break;
						file_len += len;
						memset(tmp, 0, BUFF_SIZE * sizeof(*tmp));
					}
					close(file);
						
					// Generate header
					struct stat st1, st2; // file status information
					struct tm *tm1, *tm2; // time information
					char header[BUFF_SIZE];
					memset(header,0,sizeof(header));
					char lasttime[50];
					memset(lasttime, 0, sizeof(lasttime));
					char ntime[50];
					memset(ntime, 0, sizeof(ntime));
					time_t t;
					char etag[50];
					memset(etag, 0, sizeof(etag));
					if (lstat(path, &st1) == -1) {
						perror("lstat() failed");
						exit(1);
					}
					if (stat(path, &st2) == -1) {
						perror("stat() failed");
						exit(1);
					}
					// Create etag based on last modified time of the file
					sprintf(etag, "\"%lx-%lx\"", (unsigned long)st2.st_mtime, (unsigned long)st2.st_size);
					// Convert the last-modified time to GMT string format
					tm1 = gmtime((const time_t *) &st1.st_mtime);
					strftime(lasttime, sizeof(lasttime), "%a, %d %b %Y %H:%M:%S GMT\r\n", tm1);
					// Convert current time to GMT string format
					t = time(NULL);
					tm2 = gmtime(&t);
					strftime(ntime, sizeof(ntime), "%a, %d %b %Y %H:%M:%S GMT\r\n", tm2);
					// Write the header including the HTTP status line
					sprintf(header, "HTTP/1.1 200 OK\r\nDate: %sServer: localhost:%d\r\nLast-Modified: %sETag: %s\r\nAccept-Ranges: bytes\r\nContent-Length: %lu\r\nContent-Type: text/html\r\n\r\n", ntime, port, lasttime, etag, file_len);
					send(client_sock, header, strlen(header), 0);
					
					// Send the requested file
					// Open the requested file for reading
					FILE *file_p = NULL;
					// If the file cannot be opened, send a 404 Not Found response
					if ((file_p = fopen(path, "rb")) == NULL) {
						char *err = "HTTP/1.1 404 Not Found\r\n";
						send(client_sock, err, strlen(err), 0);
						perror("open() failed");
						exit(1);
					}
					else {
						// If the file is successfully opened,
						// read and send its contents in chunks
						char buffer[BUFF_SIZE];
						memset(buffer,0,sizeof(buffer));
						size_t bytes_read;
						while ((bytes_read = fread(buffer, 1, sizeof(buffer), file_p)) > 0) {
							send(client_sock, buffer, bytes_read, 0);
						}
					}
					fclose(file_p);
				}
				// Request other files (not default)
				else {
					strcat(path, req_file);
					
					// Check if the requested file exists with read and write permissions
					if (access(path, R_OK | W_OK) == 0) {
						// Get the requested file's length
						int file = open(path, O_RDONLY);
						int len = 0;
						int tmp[BUFF_SIZE];
						memset(tmp, 0, BUFF_SIZE * sizeof(*tmp));
						while (1) {
							len = read(file, tmp, BUFF_SIZE);
							if (len <= 0)
								break;
							file_len += len;
							memset(tmp, 0, BUFF_SIZE * sizeof(*tmp));
						}
						close(file);
						
						// Generate header
						struct stat st1, st2; // file status information
						struct tm *tm1, *tm2; // time information
						char header[BUFF_SIZE];
						memset(header,0,sizeof(header));
						char lasttime[50];
						memset(lasttime, 0, sizeof(lasttime));
						char ntime[50];
						memset(ntime, 0, sizeof(ntime));
						time_t t;
						char etag[50];
						memset(etag, 0, sizeof(etag));
						if (lstat(path, &st1) == -1) {
							perror("lstat() failed");
							exit(1);
						}
						if (stat(path, &st2) == -1) {
							perror("stat() failed");
							exit(1);
						}
						// Create etag based on last modified time of the file
						sprintf(etag, "\"%lx-%lx\"", (unsigned long)st2.st_mtime, (unsigned long)st2.st_size);
						// Convert the last-modified time to GMT string format
						tm1 = gmtime((const time_t *) &st1.st_mtime);
						strftime(lasttime, sizeof(lasttime), "%a, %d %b %Y %H:%M:%S GMT\r\n", tm1);
						// Convert current time to GMT string format
						t = time(NULL);
						tm2 = gmtime(&t);
						strftime(ntime, sizeof(ntime), "%a, %d %b %Y %H:%M:%S GMT\r\n", tm2);
						// Write the header including the HTTP status line
						sprintf(header, "HTTP/1.1 200 OK\r\nDate: %sServer: localhost:%d\r\nLast-Modified: %sETag: %s\r\nAccept-Ranges: bytes\r\nContent-Length: %lu\r\nContent-Type: %s\r\n\r\n", ntime, port, lasttime, etag, file_len, cont_type());
						send(client_sock, header, strlen(header), 0);

						// Send the requested file
						// Open the requested file for reading
						FILE *file_p = NULL;
						// If the file cannot be opened, send a 404 Not Found response
						if ((file_p = fopen(path, "rb")) == NULL) {
							char *err = "HTTP/1.1 404 Not Found\r\n";
							send(client_sock, err, strlen(err), 0);
							perror("open() failed");
							exit(1);
						}
						else {
							// If the file is successfully opened, read and send its contents in chunks
							char buffer[BUFF_SIZE];
							memset(buffer,0,sizeof(buffer));
							size_t bytes_read;
							while ((bytes_read = fread(buffer, 1, sizeof(buffer), file_p)) > 0) {
								send(client_sock, buffer, bytes_read, 0);
								signal(SIGPIPE, SIG_IGN); // for .mp3 error
							}
						}
						fclose(file_p);
					}
					else {
						// If the file does not exist, send a 404 Not Found response
						send(client_sock, "HTTP/1.1 404 Not Found\r\n", 24, 0);
					}
				}
			}
		}
		// Close client socket after handling
		close(client_sock);
	}
	// Close server socket before exiting
	close(server_sock);
	
	return 0;
}
