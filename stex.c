#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

/*** includes ***/
#include<ctype.h>
#include<stdio.h>
#include<errno.h>
#include<termios.h>
#include<sys/types.h>
#include<sys/ioctl.h>
#include<unistd.h>
#include<stdlib.h>
#include<string.h>

#define STEX_VERSION "0.0.1"
// 0x1F - decimal 27 escape sequence
#define CTRL_KEY(k) ((k) & 0x1F)

/*** DATA STRUCTURES ***/

// datatype for storing a row of a text-editor
typedef struct erow {
	int size;
	char* chars;
} erow;

struct editorConfiguration {
	int cx, cy;
	/* row of the file the user is currently scrolled */
	int rowOffset;
	
	int screencols;
	int screenrows;
	
	int numRows;
	erow *row; // array for each row 
	
	/* stores original terminal attributes	*/
	struct termios original_termios;
};

struct editorConfiguration E;

enum editorKeys{
	ARROW_LEFT = 1000,
	ARROW_RIGHT,
	ARROW_UP,
	ARROW_DOWN,
	DEL_KEY,
	HOME_KEY,
	END_KEY,
	PAGE_UP,
	PAGE_DOWN
} ;

/*** TERMINAL ***/
struct abuf {
  char *b;
  int len;
};

#define ABUF_INIT {NULL, 0}

/* error logging function */
void die(const char *s) {
  write(STDOUT_FILENO, "\x1b[2J", 4);
  write(STDOUT_FILENO, "\x1b[H", 3);
  perror(s);
  exit(1);
}

void abAppend(struct abuf *ab,const char *s, int len){
	char *new = realloc(ab->b, ab->len + len);
	if(new == NULL) return;
	memcpy(&new[ab->len], s, len);
	ab->b = new;
	ab->len += len;
}

void abFree(struct abuf *ab){
	free(ab->b);
}

int keyRead(){
  int nread;
  char c;
  while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
    if (nread == -1 && errno != EAGAIN) die("read");
  }

  // handle escape sequences
   if (c == '\x1b') {
		char seq[3];
		if(read(STDIN_FILENO, &seq[0], 1) != 1) 
	   					return '\x1b';
		if(read(STDIN_FILENO, &seq[1], 1) != 1) 
	   					return '\x1b';

		if(seq[0] == '['){
			if(seq[1] >= '0' && seq[1] <= '9'){
				if(read(STDIN_FILENO, &seq[2], 1) != 1) 
	   					return '\x1b';
				if(seq[2] == '~'){
					switch(seq[1]){
						case '1': return HOME_KEY;
						case '3': return DEL_KEY;
						case '4': return END_KEY;
						case '5': return PAGE_UP;
						case '6': return PAGE_DOWN;
						case '7': return HOME_KEY;
						case '8': return END_KEY;
					}
				}
			}else{
				// escape sequences for arrow keys
				switch(seq[1]){
					case 'A' : return ARROW_UP;
					case 'B' : return ARROW_DOWN;
					case 'C' : return ARROW_RIGHT;
					case 'D' : return ARROW_LEFT;
					case 'H' : return HOME_KEY;
          			case 'F' : return END_KEY;
				}
			}
		}else if(seq[0] == 'O'){
			switch (seq[1]) {
				case 'H': return HOME_KEY;
				case 'F': return END_KEY;
			}
		}

		return '\x1b';
	} else {
    	return c;
  }
}

void exitRawMode(){
	/* leave the terminal attributes as they were when exiting */
	if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.original_termios) == -1) die("tcsetattr");
}

void enterRawMode(){
	if (tcgetattr(STDIN_FILENO, &E.original_termios) == -1) die("tcgetattr");
	/* whenever exiting the program, restore terminal attribute states */
	atexit(exitRawMode);

	struct termios raw = E.original_termios;
	// flag - IXON turns ctrl+s && ctrl+q software signals off
	// flag - ICRNL turns ctrl+m carriage return off
	raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
	// flag - OPOST turns post-processing of output off
	raw.c_oflag &= ~(OPOST);
	raw.c_cflag |= (CS8);
	// flag - ICANON turns canonical mode off
	// flag - ISIG turns ctrl+c && ctrl+z signals off
	raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);

	/* adding timeouts for read */
	raw.c_cc[VMIN] = 0;
  	raw.c_cc[VTIME] = 1;

	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) die("tcsetattr");
}

int getCursorPosition(int *rows, int *cols) {
  char buf[32];
  unsigned int i = 0;
  if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) return -1;
  while (i < sizeof(buf) - 1) {
    if (read(STDIN_FILENO, &buf[i], 1) != 1) break;
    if (buf[i] == 'R') break;
    i++;
  }
  buf[i] = '\0';
  if (buf[0] != '\x1b' || buf[1] != '[') return -1;
  if (sscanf(&buf[2], "%d;%d", rows, cols) != 2) return -1;
  return 0;
}

int getWindowSize(int *rows, int *cols){
	struct winsize ws;
	if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0){
		// moving to the bottom-rightmost pixel to get rows and columns 
		if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) return -1;
		return getCursorPosition(rows, cols);
	}else{
		*cols = ws.ws_col;
		*rows = ws.ws_row;
		return 0;
	}
}

/*** ROW OPERATIONS ***/
void editorAppendRow(char *s, size_t linelen){
	E.row = realloc(E.row, sizeof(erow) * (E.numRows + 1));
	int at = E.numRows;
	E.row[at].size = linelen;
	E.row[at].chars = malloc(linelen + 1);
	memcpy(E.row[at].chars, s, linelen);
	E.row[at].chars[linelen] = '\0';
	E.numRows++;
}

/*** FILE I/O ***/
void editorFileOpen(char *filename){
	FILE *f = fopen(filename, "r");
	if(!f) die("fopen");

	char *line = NULL;
	size_t linecap = 0;
	ssize_t linelen ;
	while ((linelen = getline(&line, &linecap, f)) != -1) {
		while(linelen > 0 && (line[linelen - 1] == '\n' || 
							  line[linelen - 1] == '\r'))
		linelen--;
		editorAppendRow(line, linelen);
	}
	// release memory
	free(line);
	fclose(f);
}

/*** INPUT ***/
void editorMoveCursor(int key){
	switch(key){
		case ARROW_UP 	:
			if(E.cy != 0) 
				E.cy--; 
			break;
		case ARROW_DOWN : 
			if(E.cy != E.numRows) 
				E.cy++; 
			break;
		case ARROW_RIGHT : 
			if (E.cx != E.screencols - 1)
				E.cx++; 
			break;
		case ARROW_LEFT : 
			if(E.cx != 0)
				E.cx--; 
			break;
		default : break;
	}
}

void editorKeyPress(){
	int c = keyRead();
	switch(c){
		case CTRL_KEY('q'):
			write(STDOUT_FILENO, "\x1b[2J", 4);
      		write(STDOUT_FILENO, "\x1b[H", 3);
      		exit(0);
      		break;
		
		case HOME_KEY : 
			E.cx = 0;
			break;
		case END_KEY : 
			E.cx = E.screencols - 1; 
			break;
		case DEL_KEY : 
			break; // TODO : add logic here
		
		case PAGE_UP:
		case PAGE_DOWN : 
			{
			int times = E.screenrows;
			while (times--)
			editorMoveCursor(c == PAGE_UP ? ARROW_UP : ARROW_DOWN);
			}
      		break;
		
		case ARROW_UP	:
		case ARROW_DOWN	:
		case ARROW_RIGHT:
		case ARROW_LEFT	:
			editorMoveCursor(c);
			break;
	}
}


/*** OUTPUT ***/

void verticalScroll() {
  if (E.cy < E.rowOffset) {
	  // if the cursor is above the visible window
	  E.rowOffset = E.cy;
  }
  if (E.cy >= E.rowOffset + E.screenrows) {
	  // if cursor is below the bottom of visible window
	  E.rowOffset = E.cy - E.screenrows + 1;
  }
}

/**
 * @brief adds '~' character at the start of each row
*/
void editorDrawRows(struct abuf *ab) {
  for (int y = 0; y < E.screenrows; y++) {
	int filerow = y + E.rowOffset;
	if(filerow >= E.numRows){
		if (E.numRows == 0 && y == E.screenrows / 3) {
			// display the text message only if no text file to read
			char welcome[80];
			int welcomelen = snprintf(welcome, sizeof(welcome),
				"Stex editor -- version %s", STEX_VERSION);
			if (welcomelen > E.screencols) welcomelen = E.screencols;
			int padding = (E.screencols - welcomelen) / 2;
			if (padding) {
				abAppend(ab, "~", 1);
				padding--;
			}
			while (padding--) abAppend(ab, " ", 1);

			abAppend(ab, welcome, welcomelen);
		} else {
			abAppend(ab, "~", 1);
		}
	} else{
		// display file 
		int len = E.row[filerow].size;
		if(len > E.screencols) len = E.screencols;
		abAppend(ab, E.row[filerow].chars, len);
	}
	
    // write(STDOUT_FILENO, "~", 1);
	
	// erasing the right part of each line before drawing
	abAppend(ab, "\x1b[K", 3);
    if (y < E.screenrows - 1) {
		abAppend(ab, "\r\n", 2);
		 // TODO : terminal status bar display
      // write(STDOUT_FILENO, "\r\n", 2);
    }
  }
}

void editorRefreshScreen() {
  // write(STDOUT_FILENO, "\x1b[2J", 4);
  // write(STDOUT_FILENO, "\x1b[H", 3);
  verticalScroll();
  struct abuf ab = ABUF_INIT;
  
  // hide the cursor before drawing screen 
  abAppend(&ab, "\x1b[?25l", 6);

  // the below commented line clears the whole screen
  // abAppend(&ab, "\x1b[2J", 4);
  
  abAppend(&ab, "\x1b[H", 3);
  // drawing screen
  editorDrawRows(&ab);
  
  // postions the cursor at current cx, cy
  char buf[32];
  snprintf(buf, sizeof(buf), "\x1b[%d;%dH", E.cy+1, E.cx+1);
  abAppend(&ab, buf, strlen(buf));
  
  // show the cursor after drawing screen
  abAppend(&ab, "\x1b[?25h", 6);

  write(STDOUT_FILENO, ab.b, ab.len);
  abFree(&ab);
}

/*** INIT ***/
void initEditor() {
  E.cx = E.cy = 0;
  E.row = NULL;
  E.rowOffset = 0;
  E.numRows = 0; // TODO : make this dynamic; increment as per lines
  if (getWindowSize(&E.screenrows, &E.screencols) == -1) 
  		die("getWindowSize");
}

int main(int argc, char *argv[]){
	enterRawMode();
	initEditor();
	if(argc >= 2){
		editorFileOpen(argv[1]);
	}
	while (1) {
		editorRefreshScreen();
    	editorKeyPress();
	}
	
	return 0;
}
