# Text-Editor-In-C
A basic Text editor in C99 Language

## **Running the code**
***

* Install c99(or higher) compiler; on Linux, check by executing ``` cc --version``` command on terminal
* Install cmake to build files using MakeFile; on Linux, check by executing ``` make -v ``` command on terminal
* Then run the following code snippet in the same directory where you unzipped this repo:

Without Make:
```
cc stex.c -o stex
./stex
```

With Make:
```
make stex
./stex
```

## **Notes** :

### Step 1 - Conversion from Canonical Mode to Raw Mode
***

* Starting from the main function, we need to change the terminal mode to **'RAW'** mode. By default, the terminal is in **'CANONICAL'** mode i.e. it does not pass the input to the process unless you press the Enter key.

* We will need the *termios* library to change the terminal attributes so importing that firstly.

* Now we **disable** various attributes of the terminal using flags : 

   * *ECHO* [termios.h] - The ECHO attribute causes each key you type to be printed to the terminal, so you can see what you’re typing. We do not need such feature for the text-editor

   * *ICANON* [termios.h] - Turns off Canonical mode; enables us to read the input byte-by-byte rather than line by line

   * *ISIG* [termios.h] - Disable **Ctrl+Z** (26-byte SIGTSTP signal to the current process) and **Ctrl+C** (3-byte SIGINT signal to the current process) signals from user input

   * *IXON* [termios.h] - Turns off **Ctrl+S** and **Ctrl+Q** software control signals

   * *IEXTEN* [termios.h] - Turns off **Ctrl+V** and **Ctrl+O** software control signals

   * *ICRNL* [termios.h] - Turns off **Ctrl+M** and **ENTER** transformation to Carriage Returns by the terminal.

* Also disable many extra signals from wrong inputs. 

* Do not forget to disable the RAW mode at program exit; using **atexit()** from [stdlib.h] library.

* It’s time to clean up the code by adding some *error handling*:

    * perror() [stdio.h] - looks at the global errno variable and prints a descriptive error message

    * exit() [stdlib.h] - exit the program with an exit status of 1


### Step 2 - Enhancing the input and output
***

* Adding refresh screen everytime the input is received; do not forget to clear the screen and reposition the cursor on Ctrl+Q press
    * *editorRefreshScreen()* - 
    writes 4 bytes to the output screen;
        * **\x1b** - decimal 27 is the escape character,
        * [ - start of the escape sequence
        * 2J - clears the entire screen
        * H - repositions the cursor to the top of the screen
    
* Getting to know the size of the terminal - using the **ioctl** library

    * *getWindowSize()* - using the ioctl function, we check whether the winsize struct is present or not. If absent, we return -1. **TIOCGWINSZ** stands for Terminal IOCtl (which itself stands for Input/Output Control) Get WINdow SiZe.)

* ioctl() isn’t guaranteed to be able to request the window size on all systems, so providing a fallback method of getting the window size using the cursor positon - *getCursorPosition()*.

* Drawing '~' like VIM at the start of each row
    * *editorDrawRows()* - prints '~' at the start of each row

* Creating **Dynamic Strings** in C to write the buffer to the screen at once for performance enhancements.
Replacing all our **write()** calls with code that appends the string to a buffer, and then **write()** this buffer out at the end.
    * Creating a **buffer abuf{}** consisting of a pointer to our buffer in memory, and a length.
    * *abAppend()* - appends a string to a buffer
    * *abFree()* - frees the memory occupied by the buffer

* For performance improvements, we can just clear the right part of the cursor when drawing the row instead of clearing the screen everytime when we refresh the screen.
    * remove ~~abAppend(&ab, "\x1b[2J", 4)~~ from editor refresh screen for the above said reason.

### Step 3 - Focusing on input
***

* Get the cursor position : store the x and y co-ordinates of the cursor postion in the editorConfiguration struct.
    * E.cx is the horizontal coordinate of the cursor (the column) and E.cy is the vertical coordinate (the row). Initialize both of them to 0, as we want the cursor to start at the top-left of the screen

* Move the cursor to the x and y co-ordinates of the screen in the editor-refresh screen.
    * using **snprintf()** from [stdlib.h] library which formats and stores a series of characters and values in the array buffer


* Handle Arrow key escape sequences and change the position of the cursor on arrow key press

* Handle HOME_KEY, END_KEY, PAGE_UP, PAGE_DOWN AND DEL KEYS support along with their escape sequence handling. Following are escape sequences for some keys :
    * *'\x1b'* - Escape sequence starts  
        *   [A - ArrowUp
        *   [B - ArrowDown
        *   [C - ArrowRight
        *   [D - ArrowLeft
        *   [5~ - PageUp
        *   [6~ - PageDown
        *   [1~ , [7~ , [H , [OH - Home
        *   [4~ , [8~ , [F , [OF - End
        *   [3~ - Del
        * [You can find more escape sequences here :D](http://ascii-table.com/ansi-escape-sequences.php)