#include <ncurses.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>

#define HEIGHT 40
#define WIDTH  40

#define c_ls_len 100

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

// test

struct test{
	int x,y;
};
struct test * t_ls[100];
pthread_mutex_t t_ls_mtx;
int t_sp = -1;
pthread_mutex_t t_sp_mtx;

void parse_test(const char * msg){
	if(msg[0] != 'T'){
		return;
	}
	int r = 0;
	int x = -1;
	int y = -1;
	for(int i = 1;msg[i] != 'X'; i++){
		if(msg[i] - 48 >= 0 && msg[i] - 48 <= 9){
			r *= 10;
			r += msg[i] - 48;
		}
		if(msg[i] == ' '){
			if(x < 0){
				x = r;
				r = 0;
			}
			else if(y < 0){
				y = r;
			}
		}
			


	}
		
	struct test * new = malloc(sizeof(struct test));
	new->x = x;
	new->y = y;

	pthread_mutex_lock(&t_ls_mtx);
	pthread_mutex_lock(&t_sp_mtx);
	t_sp++;
	t_ls[t_sp] = new;
	pthread_mutex_unlock(&t_ls_mtx);
	pthread_mutex_unlock(&t_ls_mtx);

	return;
}

//


char grid[HEIGHT][WIDTH];
pthread_mutex_t grid_mutex = PTHREAD_MUTEX_INITIALIZER;

struct city * c_ls[c_ls_len];
pthread_mutex_t c_ls_mtx;

int c_sp = -1;
pthread_mutex_t c_sp_mtx;

/* city */

void cr_city(int x, int y){
	if(x < 0 || x > WIDTH) return;
	if(y < 0 || y > HEIGHT) return;

	struct city * nc = malloc(sizeof(struct city));
	nc->x = x;
	nc->y = y;
	nc->rt = 0;
	nc->r1 = 0;
	nc->r2 = 0;
	nc->r3 = 0;
	nc->o = 0;
	nc->p = 100;

	pthread_mutex_lock(&c_ls_mtx);
	pthread_mutex_lock(&c_sp_mtx);
	c_sp++;
	c_ls[c_sp] = nc;
	pthread_mutex_unlock(&c_ls_mtx);
	pthread_mutex_unlock(&c_sp_mtx);
	
	return;
}

void draw_cities(){

	if(c_sp < 0) return;

	for(int i = 0; i <= c_sp; i++){
		pthread_mutex_lock(&grid_mutex);
		grid[c_ls[i]->y][c_ls[i]->x] = '@';
		pthread_mutex_unlock(&grid_mutex);
	}

	return;
}

/**/

void draw_testies(){

	if(t_sp < 0) return;

	for(int i = 0; i <= t_sp; i++){
		pthread_mutex_lock(&grid_mutex);
		grid[t_ls[i]->y][t_ls[i]->x] = '#';
		pthread_mutex_unlock(&grid_mutex);
	}

	return;
}

/* networking */


int sockfd;
char received_data[1024];

void send_to_server(const char *msg) {
	/**/ 
	 if (sockfd >= 0) {
        send(sockfd, msg, strlen(msg), 0);
    } else {
        fprintf(stderr, "Attempted to send on closed socket\n");
    }
    /**/
   
}



/* recv thread (unchanged) */
void* recv_thread_func(void *arg) {
    int sock = *(int*)arg; 
    free(arg);
    //send_to_server("hi");
    while (1) {
        int len = recv(sock, received_data, sizeof(received_data) - 1, 0);
        if (len > 0) {
            received_data[len] = '\0';
	    
            mvprintw(HEIGHT + 10, 0,"Received: %s", received_data);
	    parse_test(received_data);

        } else if (len == 0) {
            mvprintw(HEIGHT + 10, 0,"Server closed connection.");
            break;
        } else {

            perror("recv");
	   
            break;
        }
    }
    return NULL;
}

/**/

/* tick */
int cl_st = 0;
void * game_tick_thread_func(void * arg){
	free(arg);
	usleep(1000);
	if(cl_st < 10) cl_st++;
	else cl_st = 0;

	return NULL;
}
/**/

/* Draw a simple 2D grid: each cell is 1 row × 3 columns */
void draw_grid() {
    pthread_mutex_lock(&grid_mutex);
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            int scr_y = y;       // 1 row per cell
            int scr_x = x * 2; 
            mvprintw(scr_y, scr_x, "%c", grid[y][x]);
        }
    }
    pthread_mutex_unlock(&grid_mutex); 
   
    refresh();
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <IP> <PORT>\n", argv[0]);
        return 1;
    }

    const char *server_ip = argv[1];
    int         port      = atoi(argv[2]);

    struct sockaddr_in server_addr;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return 1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port   = htons(port);
    inet_pton(AF_INET, server_ip, &server_addr.sin_addr);

    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        close(sockfd);
        return 1;
    }

    /* spawn tick thread */
    pthread_t game_tick_thread;
    if (pthread_create(&game_tick_thread, NULL, game_tick_thread_func, NULL) != 0) {
        perror("pthread_create (game_tick_thread)");
        return 1;
    }
    pthread_detach(game_tick_thread);  

    /**/

    /* Spawn recv thread */
    int *sock_ptr = malloc(sizeof(int));
    if (!sock_ptr) { perror("malloc"); exit(1); }
    *sock_ptr = sockfd;

    pthread_t recv_thread;
    if (pthread_create(&recv_thread, NULL, recv_thread_func, sock_ptr) != 0) {
        perror("pthread_create");
        free(sock_ptr);
        close(sockfd);
        exit(1);
    }
    pthread_detach(recv_thread);

    /* ─── ncurses initialization ─────────────────────────────────────────── */
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);             // enable arrow keys / function keys
    mousemask(ALL_MOUSE_EVENTS, NULL); // enable basic mouse events
    nodelay(stdscr, FALSE);           // getch() will block
    curs_set(0);                      // hide the cursor

    /* Initialize the grid contents to '.' before drawing */
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            grid[y][x] = '.';
        }
    }

    /* Initial draw */
    draw_grid();
    mvprintw(HEIGHT + 1, 0, "Connected to server. (Press 'q' to quit)");

    /* ─── Main input loop ────────────────────────────────────────────────── */
    MEVENT event;
    while (1) {
        int ch = getch();
        if (ch == KEY_MOUSE) {
            if (getmouse(&event) == OK) {
		
                int mx = event.x;
                int my = event.y;
                /* Translate to grid coords: each cell is 3 columns wide, 1 row tall */
                int gx = mx / 2;
                int gy = my / 1;

                if (gx >= 0 && gx < WIDTH && gy >= 0 && gy < HEIGHT) {
                    pthread_mutex_lock(&grid_mutex);
                    /* toggle between '.' and 'X' on click */
                    if (grid[gy][gx] == '.') grid[gy][gx] = '#';
                    else                    grid[gy][gx] = '.';
			
		    char str_to_serv[100];
		    snprintf(str_to_serv,sizeof(str_to_serv), "TEST %d %d", gx, gy);
		    send_to_server(str_to_serv);
		    

                    /* Redraw only that cell (same calculation as draw_grid) */
                    int scr_y = gy;       
                    int scr_x = gx * 2;   
                    mvprintw(scr_y, scr_x, "%c", grid[gy][gx]);
                    pthread_mutex_unlock(&grid_mutex);
		    mvprintw(HEIGHT, 0, "	                       ");
		    mvprintw(HEIGHT + 10, 0, "	                       ");
		    mvprintw(HEIGHT, 0, "x: %d, y: %d", scr_x / 2, scr_y);
                    refresh();
                }
            }
        }
 
        else if (ch == 'q' || ch == 'Q') {
            break;
        }
        /* else: ignore other keys */
    }

    endwin();    /* restore terminal */
    close(sockfd);
    return 0;
}
   
 
