# cs 3350 tankbattle
# to compile your project, type make and press enter
CFLAGS = -I ./include
LFLAGS = -lrt -lX11 -lGLU -lGL -lm

all: tankbattle

tankbattle: tankbattle.cpp
	g++ $(CFLAGS) tankbattle.cpp libggfonts.a -Wall -Wextra $(LFLAGS) -o tankbattle 

clean:
	rm -f tankbattle
	rm -f *.o

