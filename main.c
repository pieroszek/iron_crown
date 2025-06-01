#include <ncurses.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>

#define HEIGHT 40
#define WIDTH 40
#define TICK_INTERVAL_MS 500

char grid[HEIGHT][WIDTH];
pthread_mutex_t grid_mutex;

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
	int o, m, rt, r;
};

struct city * c_ls[1024];
pthread_mutex_t c_ls_mutex;

int c_sp = -1;
pthread_mutex_t c_sp_mutex;

struct army * a_ls[1024];
pthread_mutex_t a_ls_mutex;

int a_sp = -1;
pthread_mutex_t a_sp_mutex;

struct fleet * f_ls[1024];
pthread_mutex_t f_ls_mutex;

int f_sp = -1;
pthread_mutex_t f_sp_mutex;

void create_fleet(int x, int y, int rt, int r){
	if(x < 0 || y < 0){
		return;
	}
	if(r < 0){
		return;
	}

	struct fleet * new = malloc(sizeof(struct fleet));
	new->cx = x;
	new->cy = y;
	new->tx = -1;
	new->ty = -1;
	new->o = 0;
	new->rt = rt;
	new->r = r;
	new->m = 0;

	pthread_mutex_lock(&f_ls_mutex);
	pthread_mutex_lock(&f_sp_mutex);
	f_sp++;
	f_ls[f_sp] = new;
	pthread_mutex_unlock(&f_ls_mutex);
	pthread_mutex_unlock(&f_sp_mutex);


}

void draw_fleet(){
	
	if(f_sp < 0){
		return;
	}
	pthread_mutex_lock(&grid_mutex);
	for(int i = 0; i <= f_sp; i++){
		grid[f_ls[i]->cy][f_ls[i]->cx] = 'C';
	}
	pthread_mutex_unlock(&grid_mutex);
		

}

void display_resource(int c){

	if(c < 0 || c > c_sp){
		return;
	}
	mvprintw(HEIGHT + 9,0, "city: %d type: %d", c, c_ls[c]->rt);
	mvprintw(HEIGHT + 10,0,"food: %d",c_ls[c]->r1);
	mvprintw(HEIGHT + 11,0,"wood: %d",c_ls[c]->r2);
	mvprintw(HEIGHT + 12,0,"metal: %d",c_ls[c]->r3);


}

int find_city(int x, int y){
	if(x < 0 || y < 0){
		return -1;
	}
	if(x > WIDTH || y > HEIGHT){
		return -1;
	}
	if(c_sp < 0){
		return -1;
	}

	for(int i = 0; i <= c_sp; i++){
		if(c_ls[i]->x == x){
			if(c_ls[i]->y == y){
				return i;
			}
		}
	}
	return -1;

}

void create_city(int x, int y, int rt){
	if(x >= WIDTH || x < 0){
		return;
	}
	if(y >= HEIGHT || y < 0){
		return;
	}
	
	struct city * new = malloc(sizeof(struct city));
	new->x = x;
	new->y = y;
	new->rt = rt;
	new->r1 = 0;
	new->r2 = 0;
	new->r3 = 0;
	new->o = 0;
	new->p = 100;

	pthread_mutex_lock(&c_ls_mutex);
	c_sp++;
	c_ls[c_sp] = new;
	pthread_mutex_unlock(&c_ls_mutex);



	

}

void prod_cities(){
	if(c_sp < 0){
		return;
	}

	for(int i = 0; i <= c_sp; i++){
		if(c_ls[i]->rt == 0){
			c_ls[i]->r1 += 1;
		}
		else if(c_ls[i]->rt == 1){
			c_ls[i]->r2 += 1;
		}
		else if(c_ls[i]->rt == 2){
			c_ls[i]->r3 += 1;
		}
	}

}

void cra_fleet_city(int city, int rt, int r){
	if(city < 0 || rt < 0 || r <= 0){
		return;
	}
	if(city > c_sp){
		return;
	}

	pthread_mutex_lock(&c_ls_mutex);

	if(rt == 0){
		c_ls[city]->r1 -= r;
	}
	else if(rt == 1){
		c_ls[city]->r2 -= r;
	}
	else if(rt == 2){
		c_ls[city]->r3 -= r;
	}


	pthread_mutex_unlock(&c_ls_mutex);


}

void cra_army_city(int city){
	if(city < 0 || city > c_sp){
		return;
	}

	pthread_mutex_lock(&c_ls_mutex);

	c_ls[city]->r1 -= 1000;
	c_ls[city]->r3 -= 100;


	pthread_mutex_unlock(&c_ls_mutex);



}


void draw_cities(){
	
	for(int i = 0; i <= c_sp; i++){
		int x = c_ls[i]->x;
		int y = c_ls[i]->y;
		pthread_mutex_lock(&grid_mutex);
		grid[y][x] = '@';
		grid[y+1][x] = 'X';
		grid[y-1][x] = 'X';
		grid[y][x+1] = 'X';
		grid[y][x-1] = 'X';
		grid[y-1][x-1] = 'X';
		grid[y+1][x+1] = 'X';
		grid[y-1][x+1] = 'X';
		grid[y+1][x-1] = 'X';
		pthread_mutex_unlock(&grid_mutex);
	


	}


}


void create_army(int x, int y){
	struct army * new = malloc(sizeof(struct army));
	new->cx = x;
	new->cy = y;
	new->m = 0;
	new->s = 1;
	new->tx = -1;
	new->ty = -1;
	new->o = 0;
	new->f = 0;
	pthread_mutex_lock(&a_sp_mutex);
	a_sp++;
	pthread_mutex_unlock(&a_sp_mutex);
	pthread_mutex_lock(&a_ls_mutex);
	a_ls[a_sp] = new;
	pthread_mutex_unlock(&a_ls_mutex);
}

int is_position_occupied(int x, int y, int self_index, int type) {

    // type 1 is army type 2 is fleet
    for (int i = 0; i <= a_sp; i++) {
        if (i == self_index) continue; // skip self
        if (a_ls[i]->cx == x && a_ls[i]->cy == y) {
            return 1; // occupied
        }

    }
    for(int i = 0; i <= f_sp; i++){

	if(i == self_index) continue;
	if(f_ls[i]->cx == x && f_ls[i]->cy == y) {
		return 1;
	}
    }

        return 0;
}

void fight_army(int army1, int army2){
	if(army1 < 0 || army2 < 0){
		mvprintw(HEIGHT + 6, 0,"err fa()");
		return;
	}
	if(a_ls[army1]->o == a_ls[army2]->o){
		mvprintw(HEIGHT + 6, 0,"same owner");
		return;
	}
	pthread_mutex_lock(&a_ls_mutex);
	a_ls[army1]->s = 0;
	a_ls[army2]->s = 0;

	pthread_mutex_unlock(&a_ls_mutex);

}

void draw_armies(){
	pthread_mutex_lock(&grid_mutex);
	for(int i = 0; i <=a_sp;i++){
		if(a_ls[i]->s > 0){
			grid[a_ls[i]->cy][a_ls[i]->cx] = 'A';
		}
	}
	pthread_mutex_unlock(&grid_mutex);

}

int find_army(int x, int y){
	if(x < 0 || y < 0){
		return -1;
	}
	if(x > WIDTH || y > HEIGHT){
		return -1;
	}

	for(int i = 0; i <= a_sp; i++){
		if(a_ls[i]->cx == x){
			if(a_ls[i]->cy == y){
				return i;
			}
		}
	}
	return -1;
}
int find_fleet(int x, int y){
	if(x < 0 || x > WIDTH){
		return -1;
	}
	if(y < 0 || y > HEIGHT){
		return -1;
	}
	

	for(int i = 0; i <= f_sp; i++){
		if(f_ls[i]->cx == x){
			if(f_ls[i]->cy == y){
				return i;
			}
		}
	}
	return -1;

}
void move_army(int ca) {
    if (ca < 0 || ca > a_sp) return;

    pthread_mutex_lock(&a_ls_mutex);
    struct army *a = a_ls[ca];

    if (a->cx == a->tx && a->cy == a->ty) {
        a->m = 0;
        a->tx = -1;
        a->ty = -1;
        pthread_mutex_unlock(&a_ls_mutex);
        return;
    }

    int next_x = a->cx;
    int next_y = a->cy;

    // Determine next intended position
    if (a->cx < a->tx) next_x++;
    else if (a->cx > a->tx) next_x--;

    if (a->cy < a->ty) next_y++;
    else if (a->cy > a->ty) next_y--;

    pthread_mutex_lock(&grid_mutex);

    // Only move if the next cell is '.' or 'A'
    // Check if another army is already there
    /* // old check
    if (!is_position_occupied(next_x, next_y, ca, 1)) {
        a->cx = next_x;
        a->cy = next_y;
    } 
    else if(is_position_occupied(next_x,next_y, ca, 1) && a_ls[find_army(next_x,next_y)]->o > 0){
	
	fight_army(ca, find_army(next_x, next_y));
	a->m = 0;
	a->tx = -1;
	a->ty = -1;
	
    }
    else {
        // Optional: stop if blocked
        a->m = 0;
        a->tx = -1;
        a->ty = -1;
	
    }   
    */

    if(find_army(next_x, next_y) >= 0){
	fight_army(ca, find_army(next_x, next_y));
	a->m = 0;
	a->tx = -1;
	a->ty = -1;
    }
    else if(find_fleet(next_x, next_y) >= 0){
	    // fight_fleet(find_fleet....
	a->m = 0;
	a->tx = -1;
	a->ty = -1;
    }
    else{
	a->cx = next_x;
        a->cy = next_y;
    }

	




    pthread_mutex_unlock(&grid_mutex);
    pthread_mutex_unlock(&a_ls_mutex);
}



void move_fleet(int ca) {
    if (ca < 0 || ca > f_sp){
	    return;
    }

    pthread_mutex_lock(&f_ls_mutex);
    struct fleet * a = f_ls[ca];

    if (a->cx == a->tx && a->cy == a->ty) {
        a->m = 0;
        a->tx = -1;
        a->ty = -1;
        pthread_mutex_unlock(&f_ls_mutex);
        return;
    }

    int next_x = a->cx;
    int next_y = a->cy;

    // Determine next intended position
    if (a->cx < a->tx) next_x++;
    else if (a->cx > a->tx) next_x--;

    if (a->cy < a->ty) next_y++;
    else if (a->cy > a->ty) next_y--;

    pthread_mutex_lock(&grid_mutex);

    // Only move if the next cell is '.' or 'A'
    // Check if another army is already there
    /* old check code
    if (!is_position_occupied(next_x, next_y, ca, 2)) {
        a->cx = next_x;
        a->cy = next_y;
    } 
    else if(is_position_occupied(next_x,next_y, ca, 2) && f_ls[find_fleet(next_x,next_y)]->o > 0){
	
	//fight_army(ca, find_army(next_x, next_y));
	a->m = 0;
	a->tx = -1;
	a->ty = -1;
	
    }
    else {
        // Optional: stop if blocked
        a->m = 0;
        a->tx = -1;
        a->ty = -1;
	
    }   
    */
    if(find_army(next_x, next_y) >= 0){
	//fleet resupply army()
	a->m = 0;
	a->tx = -1;
	a->ty = -1;
    }
    else if(find_fleet(next_x, next_y) >= 0){
	    //
	a->m = 0;
	a->tx = -1;
	a->ty = -1;
    }
    else{
	a->cx = next_x;
        a->cy = next_y;
    }



    pthread_mutex_unlock(&grid_mutex);
    pthread_mutex_unlock(&f_ls_mutex);
}



void move_fleeties(){

	for(int i = 0; i <= f_sp; i++){
		if(f_ls[i]->m != 0){
			move_fleet(i);
		}
	}
}


void move_armies(){

	for(int i = 0; i <= a_sp; i++){
		if(a_ls[i]->m != 0){
			move_army(i);
		}
	}
}

void newt_fleet(int x, int y, int ca){
	if(ca < 0 || ca > f_sp){
		return;
	}
	if(x < 0 || y < 0){
		return;
	}
	if(x >= WIDTH || y >= HEIGHT){
		return;
	}
	pthread_mutex_lock(&f_ls_mutex);
	f_ls[ca]->tx = x;
	f_ls[ca]->ty = y;
	f_ls[ca]->m = 1;

	pthread_mutex_unlock(&f_ls_mutex);


}

void newt_army(int x, int y, int ca){
	if(ca < 0 || ca > a_sp){
		return;
	}
	if(x < 0 || y < 0){
		return;
	}
	if(x >= WIDTH || y >= HEIGHT){
		return;
	}
	pthread_mutex_lock(&a_ls_mutex);
	a_ls[ca]->tx = x;
	a_ls[ca]->ty = y;
	a_ls[ca]->m = 1;
	pthread_mutex_unlock(&a_ls_mutex);
}


void draw_grid() {
    pthread_mutex_lock(&grid_mutex);
    for (int y = 0; y < HEIGHT; y++)
        for (int x = 0; x < WIDTH; x++)
            mvaddch(y, x * 2, grid[y][x]);
    pthread_mutex_unlock(&grid_mutex);
    pthread_mutex_lock(&grid_mutex);
    for (int y = 0; y < HEIGHT; y++)
        for (int x = 0; x < WIDTH; x++)
            grid[y][x] = '.';
    pthread_mutex_unlock(&grid_mutex);

    draw_cities();
    //move_armies(); 
    //move_fleeties();
    draw_armies();
    draw_fleet();
    refresh();
}
void man_ref(int min, int max){
	if(min < 0 || max < 0) return;
	if(min > max) return;
	

	for(int i = min; i < max; i++){
		mvprintw(i, 0, "                     ");
	}



}

int tn = 0;
int cc = -1;
void* tick_thread_func(void* arg) {
    while (1) {
        usleep(TICK_INTERVAL_MS * 1000);
	prod_cities();
	
	tn++;
	move_armies();
	move_fleeties();
	

        //create_army(10,10);
    }
    return NULL;
}
int state = 0;
int ca = -1;
int t = 0;
int sel_fleet = -1;
int sel_army = -1;
int cs = 0;

int ra1 = -1;
int ra2 = -1;
int ra3 = -1;

int main() {
    pthread_mutex_init(&grid_mutex, NULL);
	pthread_mutex_init(&f_ls_mutex, NULL);
	pthread_mutex_init(&a_ls_mutex, NULL);
	pthread_mutex_init(&f_sp_mutex, NULL);
	pthread_mutex_init(&a_sp_mutex, NULL);
    initscr();
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);
    mousemask(ALL_MOUSE_EVENTS, NULL);
    timeout(0);
    start_color();
    use_default_colors();
	
    

    // Init grid
    pthread_mutex_lock(&grid_mutex);
    for (int y = 0; y < HEIGHT; y++)
        for (int x = 0; x < WIDTH; x++)
            grid[y][x] = '.';
    pthread_mutex_unlock(&grid_mutex);

    pthread_t tick_thread;
    pthread_create(&tick_thread, NULL, tick_thread_func, NULL);

    MEVENT event;
    int ch;
    int running = 1;

    while (running) {
        ch = getch();
	mvprintw(HEIGHT + 4, 0,"%d", tn);
	man_ref(HEIGHT + 9, HEIGHT + 13);
	display_resource(cc);
	mvprintw(HEIGHT + 14, 0, "[ Create Fleet ]");
	mvprintw(HEIGHT + 13, 0, "[ Create Army ]");
        if (ch == 'q') {
            running = 0;
        } else if (ch == KEY_MOUSE) {
            if (getmouse(&event) == OK) {
                int gx = event.x / 2;
                int gy = event.y;
                if (gx >= 0 && gx < WIDTH && gy >= 0 && gy < HEIGHT) {
                    //pthread_mutex_lock(&grid_mutex);
                    //grid[gy][gx] = '#';
                    //pthread_mutex_unlock(&grid_mutex);
		    mvprintw(HEIGHT + 5,0,"state: %d", state);
		    if(state == 0 || state == 1 || state == 2){
			create_city(gx,gy, t);
			t++;
			state++;
			    
		    }
		    else if(state == 3){
			    //display_resource(find_city(gx,gy));
			    if(find_city(gx,gy) >= 0){
			    	cc = find_city(gx, gy);
			    }
			    else if(find_army(gx,gy) >= 0){
				    //code for move army
				    sel_army = find_army(gx,gy);
			    }
			    else if(find_fleet(gx,gy) >= 0){
				    //code for fleet
				    sel_fleet = find_fleet(gx,gy);
				    mvprintw(HEIGHT + 8, 0, "                             ");
				    mvprintw(HEIGHT + 8, 0, "fleet of type %d, amount: %d ", f_ls[sel_fleet]->rt, f_ls[sel_fleet]->r);
			    }
			    else{
				if(sel_fleet != -1){
 					newt_fleet(gx,gy, sel_fleet);
					sel_fleet = -1;
				}
				else if(sel_army != -1){
					newt_army(gx,gy, sel_army);
					sel_army = -1;
				}
			    }

		    }
		    man_ref(HEIGHT, HEIGHT + 3);		
		    mvprintw(HEIGHT, 0, "gy: %d, gx: %d", gy, gx);
		    mvprintw(HEIGHT + 1, 0, "fa: %d, ff: %d, fc: %d", find_army(gx,gy), find_fleet(gx,gy), find_city(gx,gy));
		    mvprintw(HEIGHT + 2, 0, "sel_army: %d, sel_fleet: %d", sel_army, sel_fleet);

                }
		// Handle clicking "Create Fleet"
		if (gy == HEIGHT + 14 && event.x >= 0 && event.x <= 15) {
		    if (cc >= 0) {
			// Example: Create fleet from city at cc
			//create_fleet(c_ls[cc]->x+2, c_ls[cc]->y, 1, 10);
			//cra_fleet_city(cc, 1, 10);
			
			//mvprintw(HEIGHT + 15, 0, "Fleet created!");
			cs = 1;
			ra1 = c_ls[cc]->r1 / 4;
			ra2 = c_ls[cc]->r2 / 4;
			ra3 = c_ls[cc]->r3 / 4;
			man_ref(HEIGHT + 15, HEIGHT + 18);
			mvprintw(HEIGHT + 15, 0, "- [ FOOD ] %d", c_ls[cc]->r1 / 4);
			mvprintw(HEIGHT + 16, 0, "- [ WOOD ] %d" ,c_ls[cc]->r2 / 4);
			mvprintw(HEIGHT + 17, 0, "- [ METAL ] %d", c_ls[cc]->r3 / 4);

					
		    } else {
			mvprintw(HEIGHT + 18, 0, "No city selected");
		    }

		}
		if (gy == HEIGHT + 13 && event.x >= 0 && event.x <= 15) {
		    if (cc >= 0) {
			// Example: Create fleet from city at cc
			create_army(c_ls[cc]->x+2, c_ls[cc]->y);
			cra_army_city(cc);
			mvprintw(HEIGHT + 18, 0, "Army created!");
		    } else {
			mvprintw(HEIGHT + 18, 0, "No city selected");
		    }
		}

		if(cs == 1 && gy == HEIGHT + 15 && event.x >= 0 && event.x <= 15){
			cs = 0;
			create_fleet(c_ls[cc]->x+2, c_ls[cc]->y, 0, ra1);
			cra_fleet_city(cc, 0, ra1);
			ra1 = -1;

		}
		else if(cs == 1 && gy == HEIGHT + 16 && event.x >= 0 && event.x <= 15){
			cs = 0;
			create_fleet(c_ls[cc]->x+2, c_ls[cc]->y, 1, ra2);
			cra_fleet_city(cc, 1, ra2);
			ra2 = -1;
		}
		else if(cs == 1 && gy == HEIGHT + 17 && event.x >= 0 && event.x <= 15){
			cs = 0;
			create_fleet(c_ls[cc]->x+2, c_ls[cc]->y, 2, ra3);
			cra_fleet_city(cc, 2, ra3);
			ra3 = -1;
		}




	

            }
        }

        draw_grid();
	usleep(10 * 1000);  // small delay
    }

    endwin();
    pthread_cancel(tick_thread);
    pthread_join(tick_thread, NULL);
    pthread_mutex_destroy(&grid_mutex);
 pthread_mutex_destroy(&a_ls_mutex);
 pthread_mutex_destroy(&a_sp_mutex);
 pthread_mutex_destroy(&f_ls_mutex);
 pthread_mutex_destroy(&f_sp_mutex);
 pthread_mutex_destroy(&c_ls_mutex);
 pthread_mutex_destroy(&c_sp_mutex);

    return 0;
}
