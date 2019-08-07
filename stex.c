#include<ctype.h>
#include<stdio.h>
#include<termios.h>
#include<unistd.h>
#include<stdlib.h>

/** @param 
	original_terminos
	stores original terminal attributes
*/
struct termios original_termios;

void exitRawMode(){
	// leave the terminal attributes as they were when exiting
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_termios);
}

void enterRawMode(){
	tcgetattr(STDIN_FILENO, &original_termios);
	// whenever exiting the program, restore terminal attribute states
	atexit(exitRawMode);

	struct termios raw = original_termios;
	// flag - IXON turns ctrl+s && ctrl+q software signals off
	// flag - ICRNL turns ctrl+m carriage return off
	raw.c_iflag &= ~(ICRNL | IXON);
	// flag - OPOST turns post-processing of output off
	raw.c_oflag &= ~(OPOST);
	// flag - ICANON turns canonical mode off
	// flag - ISIG turns ctrl+c && ctrl+z signals off
	raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

int main(){
	enterRawMode();
	char c;
	while(read(STDIN_FILENO, &c, 1) == 1 && c!='q'){
	 if (iscntrl(c)) {
     	  printf("%d\n", c);
    	 } else {
      	  printf("%d ('%c')\r\n", c, c);
    	 }
	}
	return 0;
}
