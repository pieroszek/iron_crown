// SERVER CODE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <pthread.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 13333
#define MAX_CLIENTS 10
#define BUF_SIZE 1024

#define army_len 1000 //a_ls len
#define fleet_len 1000 //f_ls len
#define city_len 1000 //c_ls len

struct city{
	int x, y;
	int rt; //resource type
	int r1, r2, r3; //r1 = food, r2 = wood, r3 = metal
	int o; //owner
	int p; //pop
};

struct army{
	int cx,cy; //curr x y
	int tx,ty; //target x y
	int m, s; // ismoving and size
	int o, f; // owner and isfighting
};

struct fleet{
	int cx, cy;
	int tx, ty;
	int o, m, rt, r, f;
};

// TEST 
struct test{
	int x, y;
};
struct test * t_ls[100];
pthread_mutex_t t_ls_mtx;
int t_sp = -1;
pthread_mutex_t t_sp_mtx;
/*
 * TEST to server 
 * TEST x y
 * TEST to client
 * T x y X
 * X is end of string for client parser
 */
//

/*
 *  NEW INSTANCE
 *  city (k is num of times)
 *  c k
 *  x y rt r1 r2 r3
 *  
 *  army (n is num of times)
 *  a n
 *  cx cy tx ty m s o f 
 *
 *  fleet (j is num of times)
 *  f j
 *  cx cy tx ty o m rt r f
 * 
 *  UPDATE INSTANCE
 *
 *  city 
 *  c u
 *  cx cy rt r1 r2 r3
 *
 *  army 
 *  a u
 *  cx cy tx ty 
 *
 *  fleet
 *  f u
 *  cx cy tx ty
 *
*/

struct army * a_ls[army_len];
pthread_mutex_t a_ls_mtx;

int a_sp = -1;
pthread_mutex_t a_sp_mtx;

//
typedef struct {
    int socket;
    int id;
    pthread_t thread;
    int active;
    // You can add more fields here (e.g. username, IP, etc.)
} client_t;

client_t clients[MAX_CLIENTS];
int client_id_counter = 1;
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
//

// Utility: send to one client
void send_to_client(int sock, const char *msg) {
    send(sock, msg, strlen(msg), 0);
}

// Utility: send to all clients
void broadcast(const char *msg) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active) {
            send_to_client(clients[i].socket, msg);
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

// helper func

int s_to_i(const char * msg, int start, int n){

	int r = 0;
	int cn = 0;
	
	for(int i = start; msg[i] != '\0'; i++){
		if(msg[i] - 48 >= 0 && msg[i] - 48 <= 9){
			
			r *= 10;
			r += msg[i] - 48;
		}
		if(msg[i] == ' '){
			if(cn == n){
				return r;
			}
			else{
				cn++;
				r = 0;
			}
		}
		
	}


	return r;
}

//

//
void parse_test(const char * msg){
	struct test * new = malloc(sizeof(struct test));
	int x = s_to_i(msg, 4, 1);
	int y = s_to_i(msg, 4, 2);
	printf("x: %d y: %d \n", x, y);
	new->x = x;
	new->y = y;

	pthread_mutex_lock(&t_ls_mtx);
	pthread_mutex_lock(&t_sp_mtx);
	
	t_sp++;
	t_ls[t_sp] = new;

	pthread_mutex_unlock(&t_ls_mtx);
	pthread_mutex_unlock(&t_sp_mtx);


}

void parse_input(const char * msg){
	if(msg[0] == 'T' && msg[3] == 'T'){
		printf("msg: %s\n", msg);
		parse_test(msg);
	}


	return;
}

//

// Handle a single client in a thread
void* handle_client(void *arg) {
    client_t *cli = (client_t*)arg;
    char buffer[BUF_SIZE];
    int len;

    printf("Client %d connected.\n", cli->id);
    send_to_client(cli->socket, "salutations client\n");
    printf("t_sp: %d\n", t_sp);
    if(t_sp >= 0){
        for(int i = 0; i <= t_sp; i++){
		char buffer[10];
		snprintf(buffer, sizeof(buffer),"T %d %d X",t_ls[i]->x,t_ls[i]->y);	
		printf("buffer: %s\n", buffer);
		send_to_client(cli->socket, buffer);
	}

			
    }
    while ((len = recv(cli->socket, buffer, BUF_SIZE - 1, 0)) > 0) {
        buffer[len] = '\0';
        printf("[Client %d] %s\n", cli->id, buffer);
	parse_input(buffer);

        // In the future, you can parse the buffer to update game state, etc.
    }

    // Client disconnected or error
    close(cli->socket);
    printf("Client %d disconnected.\n", cli->id);
    
    pthread_mutex_lock(&clients_mutex);
    cli->active = 0;
    pthread_mutex_unlock(&clients_mutex);

    return NULL;
}

//

int main() {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("Socket creation failed");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_sock);
        exit(1);
    }

    if (listen(server_sock, MAX_CLIENTS) < 0) {
        perror("Listen failed");
        close(server_sock);
        exit(1);
    }

    printf("Server listening on port %d...\n", PORT);

    while (1) {
        client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &addr_len);
        if (client_sock < 0) {
            perror("Accept failed");
            continue;
        }

        // Find a free slot
        pthread_mutex_lock(&clients_mutex);
        int found = 0;
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (!clients[i].active) {
                clients[i].socket = client_sock;
                clients[i].id = client_id_counter++;
                clients[i].active = 1;

                if (pthread_create(&clients[i].thread, NULL, handle_client, &clients[i]) != 0) {
                    perror("Thread creation failed");
                    clients[i].active = 0;
                    close(client_sock);
                } else {
                    pthread_detach(clients[i].thread); // Clean up automatically
                }

                found = 1;
                break;
            }
        }
        pthread_mutex_unlock(&clients_mutex);

        if (!found) {
            char *msg = "Server full. Try again later.\n";
            send(client_sock, msg, strlen(msg), 0);
            close(client_sock);
        }
    }

    close(server_sock);
    return 0;
}
