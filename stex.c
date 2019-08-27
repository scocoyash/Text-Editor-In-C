/*** includes ***/
#include<ctype.h>
#include<stdio.h>
#include<errno.h>
#include<termios.h>
#include<sys/ioctl.h>
#include<unistd.h>
#include<stdlib.h>

#define CTRL_KEY(k) ((k) & 0x1F)

/*** data structures ***/

struct editorConfiguration {
	int screencols;
	int screenrows;
	/* stores original terminal attributes	*/
	struct termios original_termios;
};

struct editorConfiguration E;

/*** terminal ***/

int keyRead(){
  int nread;
  char c;
  while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
    if (nread == -1 && errno != EAGAIN) die("read");
  }

  //TODO : handle escape sequences
  return c;
}

/* error logging function */
void die(const char *s) {
  write(STDOUT_FILENO, "\x1b[2J", 4);
  write(STDOUT_FILENO, "\x1b[H", 3);
  perror(s);
  exit(1);
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

int getWindowSize(int *rows, int *cols){
	struct winsize ws;
	if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0){
		return -1;
	}else{
		*cols = ws.ws_col;
		*rows = ws.ws_row;
		return 0;
	}
}

/*** input ***/

void editorKeyPress(){
	char c = keyRead();
	switch(c){
		case CTRL_KEY('q'):
		write(STDOUT_FILENO, "\x1b[2J", 4);
      	write(STDOUT_FILENO, "\x1b[H", 3);
      	exit(0);
      	break;
	}

	//TODO : handle Ctrl key sequences
	return;
}

/*** output ***/

void editorRefreshScreen() {
  write(STDOUT_FILENO, "\x1b[2J", 4);
  write(STDOUT_FILENO, "\x1b[H", 3);
}

/*** init ***/

void initEditor() {
  if (getWindowSize(&E.screenrows, &E.screencols) == -1) die("getWindowSize");
}

int main(){
	enterRawMode();
	initEditor();
	
	while (1) {
		editorRefreshScreen();
    	editorKeyPress();
	}
	
	return 0;
}
