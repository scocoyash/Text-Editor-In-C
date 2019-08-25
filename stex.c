/*** includes ***/
#include<ctype.h>
#include<stdio.h>
#include<errno.h>
#include<termios.h>
#include<unistd.h>
#include<stdlib.h>

#define CTRL_KEY(k) ((k) & 0x1F)

/*** data structures ***/

/** @param 
	original_terminos
	stores original terminal attributes
*/
struct termios original_termios;

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
  perror(s);
  exit(1);
}

void exitRawMode(){
	/* leave the terminal attributes as they were when exiting */
	if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_termios) == -1) die("tcsetattr");
}

void enterRawMode(){
	if (tcgetattr(STDIN_FILENO, &original_termios) == -1) die("tcgetattr");
	/* whenever exiting the program, restore terminal attribute states */
	atexit(exitRawMode);

	struct termios raw = original_termios;
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

/*** input ***/

void editorKeyPress(){
	char c = keyRead();
	switch(c){
		case CTRL_KEY('q'):
      	exit(0);
      	break;
	}

	//TODO : handle Ctrl key sequences
	return;
}

/*** output ***/

void editorRefreshScreen() {
  write(STDOUT_FILENO, "\x1b[2J", 4);
}

/*** init ***/

int main(){
	enterRawMode();
	
	while (1) {
		editorRefreshScreen();
    	editorKeyPress();
	}
	
	return 0;
}
