#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>

#define W 80
#define H 24
#define D 75000
#define RW (W / 5)

#define ESC "\x1b"
#define C_RESET ESC "[0m"
#define C_GREEN ESC "[42m"
#define C_WHITE ESC "[47m"
#define C_BLACK ESC "[30m"
#define C_RED ESC "[41m"
#define C_YELLOW ESC "[43m"
#define C_MAGENTA ESC "[45m"
#define C_DARK ESC "[40m"
#define C_BOLD ESC "[1m"
#define C_HIDE_CURSOR ESC "[?25l"
#define C_SHOW_CURSOR ESC "[?25h"
#define CURSOR_POS(row, col) ESC "[" #row ";" #col "H"
#define CLEAR_SCREEN "clear"
#define ENTER_ALTERNATE_BUFFER "\033[?1049h"
#define EXIT_ALTERNATE_BUFFER "\033[?1049l"

int d = D, rw = RW, s = 0;

void set_mode(int e) {
    struct termios t;
    tcgetattr(STDIN_FILENO, &t);
    if (e) {
        t.c_lflag |= ICANON | ECHO;
    } else {
        t.c_lflag &= ~(ICANON | ECHO);
        t.c_cc[VMIN] = 1;
        t.c_cc[VTIME] = 0;
    }
    tcsetattr(STDIN_FILENO, TCSANOW, &t);
}

int kbhit(void) {
    struct termios t, o;
    int ch, f;
    tcgetattr(STDIN_FILENO, &o);
    t = o;
    t.c_lflag &= ~(ICANON | ECHO);
    t.c_cc[VMIN] = 1;
    t.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &t);
    f = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, f | O_NONBLOCK);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &o);
    fcntl(STDIN_FILENO, F_SETFL, f);
    if (ch != EOF) {
        ungetc(ch, stdin);
        return 1;
    } else {
        return 0;
    }
}

char get_input(void) {
    char c = getchar();
    tcflush(STDIN_FILENO, TCIFLUSH);
    return c;
}

void hide_cursor() {
    printf(C_HIDE_CURSOR);
    fflush(stdout);
}

void show_cursor() {
    printf(C_SHOW_CURSOR);
    fflush(stdout);
}

void enter_alternate_buffer(){
	printf(ENTER_ALTERNATE_BUFFER);
    fflush(stdout);
}

void exit_alternate_buffer(){
	printf(EXIT_ALTERNATE_BUFFER);
    fflush(stdout);
}

void generate_row(char row[], int prev_center, int *new_center) {
    static int curvature = 0;
    curvature += (rand() % 3) - 1;
    if (curvature < -1) {
        curvature = -1;
    } else if (curvature > 1) {
        curvature = 1;
    }
    *new_center = prev_center + curvature;
    if (*new_center - rw / 2 < 0) {
        *new_center = rw / 2;
    } else if (*new_center + rw / 2 >= W) {
        *new_center = W - rw / 2 - 1;
    }
    int start = *new_center - rw / 2;
    int end = *new_center + rw / 2;
    for (int i = 0; i < W; i++) {
        if (i < start || i > end) {
            row[i] = 'G';
        } else if (i == start || i == end) {
            row[i] = 'E';
        } else {
            row[i] = 'D';
        }
    }
}

void print_row(char row[], int row_num) {
    printf(CURSOR_POS(%d, 1), row_num + 1);
    for (int i = 0; i < W; i++) {
        if (row[i] == 'G') {
            printf(C_GREEN " " C_RESET);
        } else if (row[i] == 'E') {
            printf(C_WHITE " " C_RESET);
        } else if (row[i] == 'D') {
            printf(C_DARK " " C_RESET);
        } else {
            printf(" ");
        }
    }
}

int check_collision(int car_pos, char map[][W]) {
    if (map[19][car_pos - 1] == 'G' || map[19][car_pos] == 'G') {
        return 1;
    } else {
        return 0;
    }
}

void move_car(int *car_pos, char input) {
    if (input == 'a' && *car_pos > 1) {
        *car_pos -= 2;
    } else if (input == 'd' && *car_pos < W - 1) {
        *car_pos += 2;
    }
}

void print_top_row(void) {
    printf(CURSOR_POS(1, 1) C_DARK);
    for (int i = 0; i < W; i++) {
        printf(" ");
    }
    printf(C_RESET);
}

void clear_map(char map[H][W], int prev_center) {
    for (int i = 0; i < H; i++) {
        for (int j = 0; j < W; j++) {
            if (j < prev_center - rw / 2 || j > prev_center + rw / 2) {
                map[i][j] = 'G';
            } else if (j == prev_center - rw / 2 || j == prev_center + rw / 2) {
                map[i][j] = 'E';
            } else {
                map[i][j] = 'D';
            }
        }
    }
}

void print_score(int s, char map[H][W]) {
    char score_text[20];
    snprintf(score_text, sizeof(score_text), "Score: %d", s);
    int row = 1;
    int col = 1;
    printf(ESC "[%d;%dH", row, col);
    for (int i = 0; score_text[i] != '\0'; i++) {
        if (map[row - 1][col - 1 + i] == 'G') {
            printf(C_GREEN C_BOLD C_BLACK "%c" C_RESET, score_text[i]);
        } else if (map[row - 1][col - 1 + i] == 'E') {
            printf(C_WHITE C_BOLD C_BLACK "%c" C_RESET, score_text[i]);
        } else if (map[row - 1][col - 1 + i] == 'D') {
            printf(C_RESET C_DARK C_BOLD "%c" C_RESET, score_text[i]);
        }
    }
    fflush(stdout);
}

void print_testing_message(int bot_active, char map[H][W]) {
    if (bot_active) {
        printf(CURSOR_POS(%d, %d) C_WHITE C_RED C_BOLD "TESTING" C_RESET, H, W - 7);
    } else {
        for (int i = 0; i < 7; i++) {
            int col = W - 7 + i;
            printf(CURSOR_POS(%d, %d), H, col);
            if (map[H-1][col] == 'G') {
                printf(C_GREEN " " C_RESET);
            } else if (map[H-1][col] == 'E') {
                printf(C_WHITE " " C_RESET);
            } else if (map[H-1][col] == 'D') {
                printf(C_DARK " " C_RESET);
            }
        }
    }
    fflush(stdout);
}

void print_start_menu(int selected_option, char map[H][W], int highest_score_single, int highest_score_red, int highest_score_yellow) {
    system(CLEAR_SCREEN);

    for (int i = 0; i < H; i++) {
        print_row(map[i], i);
    }

    int start_row = H / 2 - 2;
    int start_col = (W - 11) / 2;

    printf(CURSOR_POS(%d, %d) C_BOLD "CLI RACE" C_RESET, start_row, start_col + 3);

    printf(CURSOR_POS(%d, %d), start_row + 2, start_col);
    if (selected_option == 0) {
        printf(C_BOLD ">" C_RESET " Singleplayer");
    } else {
        printf("  Singleplayer");
    }

    printf(CURSOR_POS(%d, %d), start_row + 3, start_col);
    if (selected_option == 1) {
        printf(C_BOLD ">" C_RESET " Multiplayer");
    } else {
        printf("  Multiplayer");
    }

    printf(CURSOR_POS(%d, %d), start_row + 4, start_col);
    if (selected_option == 2) {
        printf(C_BOLD ">" C_RESET " Exit");
    } else {
        printf("  Exit");
    }

    int car_row = H / 2 + 8;
    int car_col = W / 2;

    if (selected_option == 0) {
        printf(CURSOR_POS(%d, %d) C_RED "**" C_RESET, car_row, car_col);

        if (highest_score_single > 0) {
            int highest_score_length = snprintf(NULL, 0, "%d", highest_score_single);
            int highest_score_start_col = car_col - 28 + (4 - highest_score_length);

            printf(CURSOR_POS(%d, %d) C_RED C_BOLD "Highest score: %d" C_RESET, car_row, highest_score_start_col, highest_score_single);
        }
    } else if (selected_option == 1) {
        printf(CURSOR_POS(%d, %d) C_RED "**" C_RESET, car_row, car_col - 2);
        printf(CURSOR_POS(%d, %d) C_YELLOW "**" C_RESET, car_row, car_col + 2);

        if (highest_score_red > 0) {
            int highest_score_length = snprintf(NULL, 0, "%d", highest_score_red);
            int highest_score_start_col = car_col - 28 + (4 - highest_score_length);

            printf(CURSOR_POS(%d, %d) C_RED C_BOLD "Highest score: %d" C_RESET, car_row, highest_score_start_col, highest_score_red);
        }

        if (highest_score_yellow > 0) {
            printf(CURSOR_POS(%d, %d) C_YELLOW C_BOLD "Highest score: %d" C_RESET, car_row, car_col + 12, highest_score_yellow);
        }
    }

    fflush(stdout);
}

void print_pause_menu(int selected_option, int car_pos1, int car_pos2, char map[H][W]) {
    int text_length = 4;
    int start_row = H / 2;
    int start_col = (W - text_length) / 2;

    int road_start_row = start_row - 1;
    int road_start_col = (W - 10) / 2;

    for (int i = 0; i < 6; i++) {
        printf(CURSOR_POS(%d, %d) C_WHITE " " C_RESET, road_start_row + i, road_start_col);
        printf(CURSOR_POS(%d, %d) C_DARK "          " C_RESET, road_start_row + i, road_start_col + 1);
        printf(CURSOR_POS(%d, %d) C_WHITE " " C_RESET, road_start_row + i, road_start_col + 11);
    }

    printf(CURSOR_POS(%d, %d) C_DARK C_BOLD "PAUSE" C_RESET, start_row, start_col + 1);

    int option1_length = 7;
    int option1_start_col = (W - option1_length) / 2;
    printf(CURSOR_POS(%d, %d), start_row + 2, option1_start_col + 1);
    if (selected_option == 0) {
        printf(C_BOLD ">" C_RESET " Resume");
    } else {
        printf("  Resume");
    }

    int option2_length = 7;
    int option2_start_col = (W - option2_length) / 2;
    printf(CURSOR_POS(%d, %d), start_row + 3, option2_start_col + 1);
    if (selected_option == 1) {
        printf(C_BOLD ">" C_RESET " Quit");
    } else {
        printf("  Quit");
    }

    if (selected_option == 1) {
    	int positions[] = {car_pos1 - 1, car_pos1, car_pos2 - 1, car_pos2};
    	for (int i = 0; i < 4; i++) {
    	    printf(CURSOR_POS(20, %d), positions[i] + 1);
    	    if (map[19][positions[i]] == 'E') {
    	        printf(C_WHITE " " C_RESET);
    	    } else if (map[19][positions[i]] == 'D') {
    	        printf(C_DARK " " C_RESET);
    	    }
    	}
    } else {
    	if (car_pos1 == car_pos2)
     	{
      		printf(CURSOR_POS(20, %d) C_MAGENTA "**" C_RESET, car_pos1);
      	} else {
       		if (car_pos1 != 0){
        		printf(CURSOR_POS(20, %d) C_RED "**" C_RESET, car_pos1);
         	}
         	if (car_pos2 != 0){
         		printf(CURSOR_POS(20, %d) C_YELLOW "**" C_RESET, car_pos2);
          	}
       	}
    }

    fflush(stdout);
}

void print_crash_menu(int selected_option) {
    int text_length = 10;
    int start_row = H / 2;
    int start_col = (W - text_length) / 2;
    printf(CURSOR_POS(%d, %d) C_WHITE C_RED C_BOLD "GAME OVER!" C_RESET, start_row, start_col);

    int prompt_length = 17;
    int prompt_start_col = (W - prompt_length) / 2;
    printf(CURSOR_POS(%d, %d) C_WHITE C_RED C_BOLD "Play again? [" C_RESET, start_row + 1, prompt_start_col);

    if (selected_option == 0) {
        printf(C_RED C_WHITE C_BOLD ESC "[31mY" C_RESET C_WHITE C_RED "/n]" C_RESET);
    } else {
        printf(C_WHITE C_RED "Y/" C_WHITE C_BOLD ESC "[31mn" C_RESET C_WHITE C_RED "]" C_RESET);
    }

    fflush(stdout);
}

void handle_start_menu(int *selected_option, char map[H][W], int highest_score_single, int highest_score_red, int highest_score_yellow) {
    print_start_menu(*selected_option, map, highest_score_single, highest_score_red, highest_score_yellow);
    while (1) {
        if (kbhit()) {
            char input = get_input();
            if (input == '\x1b') {
                char seq[3];
                seq[0] = get_input();
                if (seq[0] == '[') {
                    seq[1] = get_input();
                    if (seq[1] == 'A') {
                        *selected_option = (*selected_option + 2) % 3;
                        print_start_menu(*selected_option, map, highest_score_single, highest_score_red, highest_score_yellow);
                    } else if (seq[1] == 'B') {
                        *selected_option = (*selected_option + 1) % 3;
                        print_start_menu(*selected_option, map, highest_score_single, highest_score_red, highest_score_yellow);
                    }
                }
            } else if (input == 'w' || input == 'W') {
                *selected_option = (*selected_option + 2) % 3;
                print_start_menu(*selected_option, map, highest_score_single, highest_score_red, highest_score_yellow);
            } else if (input == 's' || input == 'S') {
                *selected_option = (*selected_option + 1) % 3;
                print_start_menu(*selected_option, map, highest_score_single, highest_score_red, highest_score_yellow);
            } else if (input == '\n' || input == '\r') {
                if (*selected_option == 0) {
                    break;
                } else if (*selected_option == 1) {
                    break;
                } else if (*selected_option == 2) {
                    system(CLEAR_SCREEN);
                    exit_alternate_buffer();
                    set_mode(1);
                    show_cursor();
                    exit(0);
                }
            }
        }
    }
}

void handle_pause_menu(int car_pos1, int car_pos2, char map[H][W], int *exit_to_start_menu) {
    int selected_option = 0;
    print_pause_menu(selected_option, car_pos1, car_pos2, map);
    while (1) {
        if (kbhit()) {
            char pause_input = get_input();
            if (pause_input == '\x1b') {
                if (kbhit()) {
                    char seq[3];
                    seq[0] = get_input();
                    if (seq[0] == '[') {
                        seq[1] = get_input();
                        if (seq[1] == 'A') {
                            selected_option = (selected_option + 1) % 2;
                            print_pause_menu(selected_option, car_pos1, car_pos2, map);
                        } else if (seq[1] == 'B') {
                            selected_option = (selected_option + 1) % 2;
                            print_pause_menu(selected_option, car_pos1, car_pos2, map);
                        }
                    }
                } else {
                    system(CLEAR_SCREEN);
                    break;
                }
            } else if (pause_input == 'w' || pause_input == 'W') {
                selected_option = (selected_option + 1) % 2;
                print_pause_menu(selected_option, car_pos1, car_pos2, map);
            } else if (pause_input == 's' || pause_input == 'S') {
                selected_option = (selected_option + 1) % 2;
                print_pause_menu(selected_option, car_pos1, car_pos2, map);
            } else if (pause_input == 'r' || pause_input == 'R') {
                system(CLEAR_SCREEN);
                break;
            } else if (pause_input == 'q' || pause_input == 'Q') {
                *exit_to_start_menu = 1;
                return;
            } else if (pause_input == '\n' || pause_input == '\r') {
                if (selected_option == 0) {
                    system(CLEAR_SCREEN);
                    break;
                } else if (selected_option == 1) {
                    *exit_to_start_menu = 1;
                    return;
                }
            }
        }
    }
}

int handle_crash_menu() {
    int selected_option = 0;
    print_crash_menu(selected_option);
    while (1) {
        if (kbhit()) {
            char input = get_input();
            if (input == '\x1b') {
                if (kbhit()) {
                    char seq[3];
                    seq[0] = get_input();
                    if (seq[0] == '[') {
                        seq[1] = get_input();
                        if (seq[1] == 'D') {
                            selected_option = 0;
                            print_crash_menu(selected_option);
                        } else if (seq[1] == 'C') {
                            selected_option = 1;
                            print_crash_menu(selected_option);
                        }
                    }
                }
            } else if (input == 'a' || input == 'A') {
                selected_option = 0;
                print_crash_menu(selected_option);
            } else if (input == 'd' || input == 'D') {
                selected_option = 1;
                print_crash_menu(selected_option);
            } else if (input == 'y' || input == 'Y') {
                return 1;
            } else if (input == 'n' || input == 'N') {
                return 0;
            } else if (input == '\n' || input == '\r') {
                if (selected_option == 0) {
                    return 1;
                } else if (selected_option == 1) {
                    return 0;
                }
            }
        }
    }
}

void bot(int *car_pos, char map[H][W]) {
    if (map[18][*car_pos - 1] == 'G' || map[18][*car_pos] == 'G') {
        if (map[18][*car_pos - 2] != 'G') {
        	move_car(car_pos, 'a');
        } else if (map[18][*car_pos + 1] != 'G') {
       		move_car(car_pos, 'd');
        }
    }
}

void singleplayer_game_loop(int *highest_score_single, char map[H][W]) {
    int prev_center = W / 2;
    rw = RW;
    clear_map(map, prev_center);

    system(CLEAR_SCREEN);
    int car_pos = W / 2;
    s = 0;
    d = D;
    int bot_active = 0;
    int exit_to_start_menu = 0;

    while (1) {
        if (kbhit()) {
            char input = get_input();
            if (input == '\x1b') {
                if (kbhit()) {
                    char seq[3];
                    seq[0] = get_input();
                    if (seq[0] == '[') {
                        seq[1] = get_input();
                        if (seq[1] == 'D') {
                            move_car(&car_pos, 'a');
                        } else if (seq[1] == 'C') {
                            move_car(&car_pos, 'd');
                        }
                    }
                } else {
                    handle_pause_menu(car_pos, 0, map, &exit_to_start_menu);
                    if (exit_to_start_menu) {
                   		if (s > *highest_score_single) {
                           *highest_score_single = s;
                        }
                        break;
                    }
                }
            } else if (input == 't' || input == 'T') {
                bot_active = !bot_active;
            } else {
                move_car(&car_pos, input);
            }
        }

        if (bot_active) {
            bot(&car_pos, map);
        }

        char new_row[W];
        int new_center;
        generate_row(new_row, prev_center, &new_center);
        prev_center = new_center;

        for (int i = H - 1; i > 0; i--) {
            for (int j = 0; j < W; j++) {
                map[i][j] = map[i - 1][j];
            }
        }

        for (int j = 0; j < W; j++) {
            map[0][j] = new_row[j];
        }

        for (int i = 0; i < H; i++) {
            print_row(map[i], i);
        }

        s++;

        print_score(s, map);
        printf(CURSOR_POS(20, %d) C_RED "**" C_RESET, car_pos);

        print_testing_message(bot_active, map);

        if (check_collision(car_pos, map)) {
        	if (s > *highest_score_single) {
         		*highest_score_single = s;
         	}
            if (handle_crash_menu() == 1) {
                singleplayer_game_loop(highest_score_single, map);
                return;
            } else {
                exit_to_start_menu = 1;
                break;
            }
        }

        fflush(stdout);
        usleep(d);

        if (s % 100 == 0 && rw > 8) {
            rw--;
        }
        d -= 10;
    }

    if (exit_to_start_menu) {
        rw = RW;
        clear_map(map, W / 2);
        system(CLEAR_SCREEN);
    }
}

void multiplayer_game_loop(int *highest_score_red, int *highest_score_yellow, char map[H][W]) {
    int prev_center = W / 2;
    rw = RW;
    clear_map(map, prev_center);

    system(CLEAR_SCREEN);
    int car_pos1 = W / 2 - 2;
    int car_pos2 = W / 2 + 2;
    int car1_alive = 1;
    int car2_alive = 1;
    int car1_crash_row = 20;
    int car2_crash_row = 20;
    int score_red = 0;
    int score_yellow = 0;
    s = 0;
    d = D;
    int exit_to_start_menu = 0;

    while (1) {
        if (kbhit()) {
            char input = get_input();
            if (input == '\x1b') {
                if (kbhit()) {
                    char seq[3];
                    seq[0] = get_input();
                    if (seq[0] == '[') {
                        seq[1] = get_input();
                        if (seq[1] == 'D') {
                            if (car2_alive) move_car(&car_pos2, 'a');
                        } else if (seq[1] == 'C') {
                            if (car2_alive) move_car(&car_pos2, 'd');
                        }
                    }
                } else {
                    handle_pause_menu(car_pos1, car_pos2, map, &exit_to_start_menu);
                    if (exit_to_start_menu) {
                   		if (car1_alive && s > *highest_score_red) {
                            *highest_score_red = s;
                        }
                        if (car2_alive && s > *highest_score_yellow) {
                            *highest_score_yellow = s;
                        }
                        break;
                    }
                }
            } else if (input == 'a' || input == 'A') {
                if (car1_alive) move_car(&car_pos1, 'a');
            } else if (input == 'd' || input == 'D') {
                if (car1_alive) move_car(&car_pos1, 'd');
            }
        }

        char new_row[W];
        int new_center;
        generate_row(new_row, prev_center, &new_center);
        prev_center = new_center;

        for (int i = H - 1; i > 0; i--) {
            for (int j = 0; j < W; j++) {
                map[i][j] = map[i - 1][j];
            }
        }

        for (int j = 0; j < W; j++) {
            map[0][j] = new_row[j];
        }

        for (int i = 0; i < H; i++) {
            print_row(map[i], i);
        }

        s++;

        print_score(s, map);
        if (car1_alive && car2_alive && car_pos1 == car_pos2) {
            printf(CURSOR_POS(20, %d) C_MAGENTA "**" C_RESET, car_pos1);
        } else {
            if (car1_alive) {
                printf(CURSOR_POS(20, %d) C_RED "**" C_RESET, car_pos1);
            } else if (car1_crash_row < H) {
                printf(CURSOR_POS(%d, %d) C_RED "**" C_RESET, car1_crash_row, car_pos1);
                car1_crash_row++;
            } else {
                car_pos1 = 0;
            }
            if (car2_alive) {
                printf(CURSOR_POS(20, %d) C_YELLOW "**" C_RESET, car_pos2);
            } else if (car2_crash_row < H) {
                printf(CURSOR_POS(%d, %d) C_YELLOW "**" C_RESET, car2_crash_row, car_pos2);
                car2_crash_row++;
            } else {
                car_pos2 = 0;
            }
        }

        if (car1_alive && check_collision(car_pos1, map)) {
            car1_alive = 0;
            score_red = s;
            if (score_red > *highest_score_red) {
                *highest_score_red = score_red;
            }
        }
        if (car2_alive && check_collision(car_pos2, map)) {
            car2_alive = 0;
            score_yellow = s;
            if (score_yellow > *highest_score_yellow) {
                *highest_score_yellow = score_yellow;
            }
        }

        if (!car1_alive && !car2_alive) {
            if (handle_crash_menu() == 1) {
                multiplayer_game_loop(highest_score_red, highest_score_yellow, map);
                return;
            } else {
                exit_to_start_menu = 1;
                break;
            }
        }

        fflush(stdout);
        usleep(d);

        if (s % 100 == 0 && rw > 8) {
            rw--;
        }
        d -= 10;
    }

    if (exit_to_start_menu) {
        rw = RW;
        clear_map(map, W / 2);
        system(CLEAR_SCREEN);
    }
}

int main() {
    char map[H][W];
    srand(time(NULL));

    int highest_score_single = 0;
    int highest_score_red = 0;
    int highest_score_yellow = 0;

    enter_alternate_buffer();
    set_mode(0);
    hide_cursor();

    int prev_center = W / 2;
    clear_map(map, prev_center);

    int selected_option = 0;
    handle_start_menu(&selected_option, map, highest_score_single, highest_score_red, highest_score_yellow);

    while (1) {
        if (selected_option == 0) {
            singleplayer_game_loop(&highest_score_single, map);
        } else if (selected_option == 1) {
            multiplayer_game_loop(&highest_score_red, &highest_score_yellow, map);
        }
        handle_start_menu(&selected_option, map, highest_score_single, highest_score_red, highest_score_yellow);
    }

    return 0;
}
