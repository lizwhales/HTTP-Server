#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <ctype.h>
#include<limits.h>
#include<errno.h>
#include <netdb.h>

#define IMPLEMENTS_IPV6
#define DEFAULT_MIME_TYPE "application/octet-stream"
#define MAX_NUM_CLIENTS 10
#define MAX_FILE_NAME_EXTENSION 128
#define MAX_FILE_NAME 256
#define MAX_GET_REQUEST_LENGTH (4 * 1024) // 4KB

char *web_root = NULL;
int server_socket_fd;

/* Verify if the file exist */
bool does_file_exist(char *file_name);

/* Create IPv4 server socket. */
int create_ipv4_socket(int port);

/* Create IPv6 server socket. */
int create_ipv6_socket(int port);

void *handle_incoming_client(void *arg);

void enable_socket(struct sockaddr* address, int socket_fd);

void signal_handler(int signal_number);

void install_signal_handlers(void );

/*
 Lowercase a string
 */
char *strlower(char *s)
{
  for (char *p = s; *p != '\0'; p++) {
    *p = tolower(*p);
  }
  return s;
}

/*
  Return a MIME type for a given filename
 */
char *mime_type_get(char *filename)
{
  char *ext = strrchr(filename, '.');

  if (ext == NULL) {
    return DEFAULT_MIME_TYPE;
  }

  ext++;
  strlower(ext);
  if (strcmp(ext, "html") == 0 || strcmp(ext, "htm") == 0) { return "text/html"; }
  if (strcmp(ext, "jpg") == 0) { return "image/jpeg"; }
  if (strcmp(ext, "css") == 0) { return "text/css"; }
  if (strcmp(ext, "js") == 0) { return "application/javascript"; }
  if (strcmp(ext, "json") == 0) { return "application/json"; }
  if (strcmp(ext, "txt") == 0) { return "text/plain"; }
  if (strcmp(ext, "gif") == 0) { return "image/gif"; }
  if (strcmp(ext, "png") == 0) { return "image/png"; }
  return DEFAULT_MIME_TYPE;
}

int get_interger_from_user_input(char* input)
{
  char *p;
  int num;
  errno = 0;
  long conv = strtol(input, &p, 10);
  if (errno != 0 || *p != '\0' || conv > INT_MAX || conv < INT_MIN) 
  {
    num = INT_MIN;
  } 
  else 
  {
      num = conv;
  }
  return num;

}

int main(int argc, char *argv[])
{
  int port, new_socket_fd, protocol_type;
  pthread_attr_t pthread_client_attr;
  pthread_t pthread;
  socklen_t client_address_len;
  struct sockaddr_in client_address;

// get parameters for the https address

  if  (argc != 4)
  {
    fprintf(stderr, "Usage: %s [protocol number] [port number] [path to web root]\n", argv[0]);
    exit(1);
  }

  /* Get protocol version from command line arguments or stdin */
  protocol_type = get_interger_from_user_input(argv[1]);
  if (protocol_type != 4 && protocol_type != 6)
  {
    fprintf(stderr, "Invalid protocol number.\n");
    exit(1);
  }

  /* Get port from command line arguments or stdin */
  port = get_interger_from_user_input(argv[2]);
  if (port < 0 || port > 65535)
  {
    fprintf(stderr, "Invalid port number.\n");
    exit(1);
  }

  /* Get web root from command line arguments or stdin */
  web_root = argv[3];
  if (web_root == NULL)
  {
    fprintf(stderr, "Invalid path to web root.\n");
    exit(1);
  }

  /* Check if the web root exist */
  if (!does_file_exist(web_root))
  {
    fprintf(stderr, "Invalid path to web root.\n");
    exit(1);
  }

// DONE INPUT VERFICATION AND NOW SOCKETING 

  /*Create the server socket */
  if (protocol_type == 4)
  {
    /* IPV4 type */
    server_socket_fd = create_ipv4_socket(port);
  }
  else if (protocol_type == 6)
  {
    /* IPV6 type */
    server_socket_fd = create_ipv6_socket(port);
  }
  else{
    exit(1);
  }
  
  /*Setup the signal handler*/
  install_signal_handlers();

  /* Initialise pthread attribute to create detached threads. DONT CARE ABOUT WAITING FOR IT TO EXIT  */
  if (pthread_attr_init(&pthread_client_attr) != 0) {
    perror("pthread_attr_init");
    exit(1);
  }
  /* Set the detach state of the thread attribute object to PTHREAD_CREATE_DETACHED */
  if (pthread_attr_setdetachstate(&pthread_client_attr, PTHREAD_CREATE_DETACHED) != 0) {
    perror("pthread_attr_setdetachstate");
    exit(1);
  }

  while (1) {

    /* Accept connection to client. */
    client_address_len = sizeof (client_address);

    /* Accept connection to client. */
    new_socket_fd = accept(server_socket_fd, (struct sockaddr *)&client_address, &client_address_len);
    if (new_socket_fd == -1) {
      perror("accept");
      continue;
    }

    printf("Client connected\n");
    unsigned int *thread_arg = (unsigned int *) malloc(sizeof(unsigned int));
    *thread_arg = new_socket_fd;

    /* Create NEW thread to serve connection to client.  USE HANDLE INCOMING CLIENT*/
    if (pthread_create(&pthread, &pthread_client_attr, handle_incoming_client, (void *) thread_arg) != 0) {
      perror("pthread_create");
      exit(1); 
    }
  }

  return 0;
}

/* Create IPv6 server socket. */
int create_ipv6_socket(int port)
{
  struct addrinfo address;
  int socket_fd;
  /* trying new method */
  memset(&address, 0, sizeof(address));
  address.ai_family = AF_INET6;
  address.ai_socktype = SOCK_STREAM;
  address.ai_flags = AI_PASSIVE;
 

  /* Initialise IPv6 address 
  memset(&address, 0, sizeof (address));

  address.sin6_family = AF_INET6;
  address.sin6_addr = in6addr_any;
  address.sin6_port  = htons(port);
*/
  /* Create TCP socket. */
  if ((socket_fd = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP)) == -1)
  {
    perror("socket");
    exit(1);
  }

  enable_socket((struct sockaddr*)&address, socket_fd);
  return socket_fd;

}

void enable_socket(struct sockaddr* address, int socket_fd)
{
  
  /* Bind address to socket. */
  if (bind(socket_fd, (struct sockaddr *) address, sizeof ((*address))) < 0) {
    perror("bind");
    exit(1);
  }

  /* Listen on socket. */
  if (listen(socket_fd, MAX_NUM_CLIENTS) == -1) {
    perror("listen");
    exit(1);
  }

  // Configure server socket
  int enable = 1;
  setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));
}

/* Create IPv4 server socket. */
int create_ipv4_socket(int port)
{
  struct sockaddr_in address;
  int socket_fd;

  /* Initialise IPv4 address. */
  memset(&address, 0, sizeof (address));
  address.sin_family = AF_INET;
  address.sin_port = htons(port);
  address.sin_addr.s_addr = INADDR_ANY;

  /* Create TCP socket. */
  if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
  {
    perror("socket");
    exit(1);
  }

  enable_socket((struct sockaddr*)&address, socket_fd);
  return socket_fd;
}

/* Setup signal handler. */

void install_signal_handlers() {

  /* Assign signal handlers to signals. */
  if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
    perror("signal");
    exit(1);
  }

  if (signal(SIGTERM, signal_handler) == SIG_ERR) {
    perror("signal");
    exit(1);
  }
  if (signal(SIGINT, signal_handler) == SIG_ERR) {
    perror("signal");
    exit(1);
  }
}

/* Check if it's a valid GET Req*/
bool is_valid_http_request(char *request) {
  if (strncmp(request, "GET", 3) == 0) {
    return true;
  }
  return false;
}

/*Parse HTTP Request:
Request of the following type are supported GET /index.llll HTTP/1.0 */
bool parse_http_request(char *request, char *file_name, char *file_extension) {
  char *pointer_start_filename = strchr(request, '/');
  if (pointer_start_filename == NULL) {
    return false;
  }
  char *pointer_start_HTTP = strchr(pointer_start_filename, 'H');
  if (pointer_start_HTTP == NULL) {
    return false;
  }
  pointer_start_HTTP--;

  strncpy(file_name, pointer_start_filename, pointer_start_HTTP - pointer_start_filename );
  file_name[pointer_start_HTTP - pointer_start_filename ] = '\0';

// IF THE FILENAME DOESN;T HAVE AN EXTENSION
  char *pointer_start_extension = strchr(file_name, '.');
  if (pointer_start_extension == NULL)
  {
    // case : No filename extension
    memset(file_extension, 0x00, MAX_FILE_NAME_EXTENSION);
  }
  else
  {
    strncpy(file_extension, pointer_start_extension + 1, strlen(file_name) - (pointer_start_extension - file_name));
    file_extension[strlen(file_name) - (pointer_start_extension - file_name)] = '\0';
  }

  printf("File name = %s\n", file_name);
  return true;

}

/* Verify if the file exist */
bool does_file_exist(char *file_name) {
  bool does_exist = false;

  if( access( file_name, F_OK ) == 0 ) {
    does_exist = true;
  } else {
    does_exist = false;
  }

  if (strstr(file_name, "..")) {
    does_exist = false;
  }
  return does_exist;
}

/* Determine the size of the file */
unsigned int get_file_size(char *file_name) {
  struct stat st;
  stat(file_name, &st);
  return st.st_size;
}

/* Create and send the HTTP Success Response */
void create_n_send_http_response_success (int client_socket, char *file_name, char *response)
{
  unsigned int file_size = get_file_size(file_name);

  char *content_type = mime_type_get(file_name);
  sprintf(response, "HTTP/1.0 200 OK \r\n" 
  "Server: UBUNTU \r\n"  
  "MIME-version: 1.0\r\n"
  "Content-Type: %s\r\n"
  "Content-Length: %d\r\n\r\n", content_type, file_size);

  char *buffer = (char *) malloc(file_size + strlen(response));
  if (buffer == NULL) {

    perror("malloc");
    exit(1);
  }

  memcpy(buffer, response, strlen(response));

  // map file to memory
  int fd = open(file_name, O_RDONLY);
  char *file_content = mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
  close(fd);

  memcpy(buffer + strlen(response), file_content, file_size);

  // send file content
  size_t sent_bytes = send(client_socket, buffer, file_size + strlen(response), 0);
  if (sent_bytes < (file_size + strlen(response) -1)) {
    perror("send");
    exit(1);
  }
  // UNMAP THE FILE
  munmap(file_content, file_size);
}

void create_http_failure_response (char *response)
{
  sprintf(response, "HTTP/1.1 404 Not Found \r\n"
                    "Server: UBUNTU \r\n"
                    "Content-Type: text/html \r\n"
                    "Content-Length: 168 \r\n"
                    "Connection: close \r\n\r\n"
                    "<html> \r\n"
                    "<head><title>404 Not Found</title></head> \r\n"
                    "<body bgcolor=\"white\"> \r\n"
                    "<center><h1>404 Not Found</h1></center> \r\n"
                    "<hr><center>UBUNTU</center> \r\n"
                    "</body>\r\n"
                    "</html>\r\n");
}


void *handle_incoming_client(void *arg)
{
  // FREE MEM IF NEEDED
  int client_socket = *(int*) arg;
  free(arg);

  char request[MAX_GET_REQUEST_LENGTH] = {0};
  char response[MAX_GET_REQUEST_LENGTH] = {0};

  /* Receive request from client. */ // GET STATIC GET REQUEST 
  if (recv(client_socket, request, MAX_GET_REQUEST_LENGTH, 0) == -1) {
    perror("recv");
  }

  /* Parse request. */
  char file_name[MAX_FILE_NAME] = {0};
  char file_extension[MAX_FILE_NAME_EXTENSION] = {0};
  if (!is_valid_http_request(request)) {
    memset(response, 0, sizeof(response));
    create_http_failure_response(response);
    send(client_socket, response, strlen(response), 0);
    // PTHREAD_EXIT(NULL)
  }
  
  if (parse_http_request(request, file_name, file_extension))
  printf("filename is %s\n", file_name);
  {
    /* Check if file exists. */
    char fully_qualified_name[FILENAME_MAX] = {0};
    sprintf(fully_qualified_name, "%s%s",web_root,file_name);
    printf("full filename is %s\n", fully_qualified_name);
    bool does_exist = does_file_exist(fully_qualified_name);
    if (does_exist) {
      memset(response, 0, sizeof(response));
      create_n_send_http_response_success(client_socket, fully_qualified_name, response);
    }
    else
    {
      memset(response, 0, sizeof(response));
      create_http_failure_response(response);
      send(client_socket, response, strlen(response), 0);
    }
  }
  // PTHREAD_EXIT(NULL)
  return NULL;
}

void signal_handler(int signal_number)
{
  close(server_socket_fd);
  exit(0);
}
