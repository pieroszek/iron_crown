/* CLIENT SIDE */

/* TO DO */
/*       */
/*       */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ncurses.h>

// ==================== CONSTANTS ====================
#define PORT 8080
#define GRID_SIZE 40
#define CELL_SPACING 2
#define MAX_CITIES 100
#define BUFFER_SIZE 256
#define MESSAGE_SIZE 32
#define REFRESH_RATE 50000 // 50ms refresh rate
#define MAX_ARMIES 100

// ==================== DATA STRUCTURES ====================
typedef struct {
        int x;
        int y;
        int p;
        int o;
        int rt;
        int r;
} city_t;

typedef struct {
	int x;
	int y;
	int s;
	int o;
} army_t;

typedef struct {
        int sockfd;
        pthread_t network_thread;
        volatile int running;
        char grid[GRID_SIZE][GRID_SIZE];
        city_t cities[MAX_CITIES];
        int city_count;
	army_t armies[MAX_ARMIES];
	int army_count;

        pthread_mutex_t cities_mutex;
        pthread_mutex_t grid_mutex;
	pthread_mutex_t armies_mutex;

        int needs_redraw; // Flag to indicate UI needs refresh
} client_state_t;

// ==================== NETWORK FUNCTIONS ====================
void* network_handler(void* arg) {
        client_state_t* state = (client_state_t*)arg;
        char buffer[BUFFER_SIZE];
        
        while (state->running) {
                int bytes = recv(state->sockfd, buffer, sizeof(buffer) - 1, 0);
                if (bytes <= 0) {
                        state->running = 0;
                        break;
                }
                
                buffer[bytes] = '\0';
                
                // Parse server updates
                int x, y, p, o, rt, r, s;
                
                if (sscanf(buffer, "SET %d %d", &x, &y) == 2) {
                        if (x >= 0 && x < GRID_SIZE && y >= 0 && y < GRID_SIZE) {
                                pthread_mutex_lock(&state->grid_mutex);
                                state->grid[y][x] = 'X';
                                pthread_mutex_unlock(&state->grid_mutex);
                                state->needs_redraw = 1; // Trigger redraw
                        }
                } else if (sscanf(buffer, "CITY %d %d %d %d %d %d", &x, &y, &p, &o, &rt, &r) == 6) {
                        pthread_mutex_lock(&state->cities_mutex);
                        if (state->city_count < MAX_CITIES) {
                                state->cities[state->city_count].x = x;
                                state->cities[state->city_count].y = y;
                                state->cities[state->city_count].p = p;
                                state->cities[state->city_count].o = o;
                                state->cities[state->city_count].rt = rt;
                                state->cities[state->city_count].r = r;
                                state->city_count++;
                                state->needs_redraw = 1; // Trigger redraw
                        }
                        pthread_mutex_unlock(&state->cities_mutex);
				
		  
                } else if (sscanf(buffer, "ARMY %d %d %d %d", &x, &y, &s, &o) == 4){
			pthread_mutex_lock(&state->armies_mutex);
			if (state->army_count < MAX_ARMIES) {
				state->armies[state->army_count].x = x;
				state->armies[state->army_count].y = y;
				state->armies[state->army_count].s = s;
				state->armies[state->army_count].o = o;
				state->army_count++;
				state->needs_redraw = 1;
			}
			pthread_mutex_unlock(&state->armies_mutex);

		} else if (strncmp(buffer, "END_INIT", 8) == 0) {
                        state->needs_redraw = 1; // Trigger redraw after initial state
                }
        }
        
        return NULL;
}

void send_click(client_state_t* state, int x, int y) {
        char msg[MESSAGE_SIZE];
        snprintf(msg, sizeof(msg), "CLICK %d %d\n", x, y);
        send(state->sockfd, msg, strlen(msg), 0);
}

int connect_to_server(client_state_t* state, const char* server_ip) {
        struct sockaddr_in serv_addr;
        
        state->sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (state->sockfd < 0) {
                return -1;
        }
        
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(PORT);
        
        if (inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0) {
                return -1;
        }
        
        if (connect(state->sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
                return -1;
        }
        
        return 0;
}

// ==================== UI FUNCTIONS ====================
void init_grid(client_state_t* state) {
        pthread_mutex_lock(&state->grid_mutex);
        for (int y = 0; y < GRID_SIZE; y++) {
                for (int x = 0; x < GRID_SIZE; x++) {
                        state->grid[y][x] = '.';
                }
        }
        pthread_mutex_unlock(&state->grid_mutex);
}

void draw_cities(client_state_t* state) {
        pthread_mutex_lock(&state->cities_mutex);
        for (int i = 0; i < state->city_count; i++) {
                mvprintw(state->cities[i].y, state->cities[i].x * CELL_SPACING, "%c", '@');
        }
        pthread_mutex_unlock(&state->cities_mutex);
}

void draw_grid(client_state_t* state) {
        clear();
        
        // Draw grid cells
        pthread_mutex_lock(&state->grid_mutex);
        for (int y = 0; y < GRID_SIZE; y++) {
                for (int x = 0; x < GRID_SIZE; x++) {
                        mvprintw(y, x * CELL_SPACING, "%c", state->grid[y][x]);
                }
        }
        pthread_mutex_unlock(&state->grid_mutex);
        
        // Draw cities
        draw_cities(state);
        
        // Add some UI information
        mvprintw(GRID_SIZE + 1, 0, "Cities: %d | Click to place X | F1 to quit", state->city_count);
        refresh();
        
        state->needs_redraw = 0; // Reset redraw flag
}

void handle_click(client_state_t* state, MEVENT* event) {
        int grid_x = event->x / CELL_SPACING;
        int grid_y = event->y;
        
        if (grid_x >= 0 && grid_x < GRID_SIZE && grid_y >= 0 && grid_y < GRID_SIZE) {
                send_click(state, grid_x, grid_y);
        }
}

void init_ncurses() {
        initscr();
        cbreak();
        noecho();
        curs_set(0);
        mousemask(BUTTON1_CLICKED, NULL);
        keypad(stdscr, TRUE);
        timeout(100); // Non-blocking input with 100ms timeout
}

// ==================== MAIN APPLICATION ====================
int main() {
        client_state_t state = {
                .sockfd = 0,
                .running = 1,
                .city_count = 0,
                .cities_mutex = PTHREAD_MUTEX_INITIALIZER,
                .grid_mutex = PTHREAD_MUTEX_INITIALIZER,
                .needs_redraw = 1 // Start with needing a redraw
        };
        
        // Connect to server
        if (connect_to_server(&state, "127.0.0.1") < 0) {
                perror("Connection failed");
                return 1;
        }
        
        // Initialize UI
        init_ncurses();
        init_grid(&state);
        
        // Start network thread
        pthread_create(&state.network_thread, NULL, network_handler, &state);
        
        // Main input loop
        MEVENT event;
        int ch;
        
        while (state.running) {
                // Check for input with timeout
                ch = getch();
                
                if (ch == KEY_MOUSE && getmouse(&event) == OK) {
                        handle_click(&state, &event);
                } else if (ch == KEY_F(1)) {
                        state.running = 0;
                        break;
                }
                
                // Redraw if needed or periodically
                if (state.needs_redraw) {
                        draw_grid(&state);
                }
                
                // Small delay to prevent high CPU usage
                usleep(REFRESH_RATE);
        }
        
        // Cleanup
        state.running = 0;
        pthread_join(state.network_thread, NULL);
        endwin();
        close(state.sockfd);
        
        return 0;
}
