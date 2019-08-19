# Text-Editor-In-C
A basic Text editor in C99 Language

## Notes :

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


