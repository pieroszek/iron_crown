# iron_crown
A 2d statetgy ascii game in the terminal


## install 
I used ncurses, i use:
``brew install ncurses``

Then to compile, i use:
``gcc -std=c99 -Wall -Werror --save-temps -fsanitize=address main.c -o game -I/opt/homebrew/opt/ncurses/include -L/opt/homebrew/opt/ncurses/lib -lncurses``

Then to run, use: 
``./game``
