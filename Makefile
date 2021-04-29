INCLUDES= -I ./include
FLAGS = -g

OBJECTS=./build/chip8_memory.o ./build/chip8_stack.o ./build/chip8_keyboard.o ./build/chip8_screen.o  ./build/chip8.o

all: ${OBJECTS}
	gcc ${FLAGS} ${INCLUDES} ./src/main.c ${OBJECTS} -L ./lib -lmingw32 -lSDL2main -lSDL2 -lSDL2_ttf -o ./bin/main.exe

./build/chip8_memory.o:src/chip8_memory.c
	gcc ${FLAGS} ${INCLUDES} ./src/chip8_memory.c -c -o ./build/chip8_memory.o

./build/chip8_stack.o:src/chip8_stack.c
	gcc ${FLAGS} ${INCLUDES} ./src/chip8_stack.c -c -o ./build/chip8_stack.o
	
./build/chip8_keyboard.o:src/chip8_keyboard.c
	gcc ${FLAGS} ${INCLUDES} ./src/chip8_keyboard.c -c -o ./build/chip8_keyboard.o

./build/chip8_screen.o:src/chip8_screen.c
	gcc ${FLAGS} ${INCLUDES} ./src/chip8_screen.c -c -o ./build/chip8_screen.o	
	
./build/chip8.o:src/chip8.c
	gcc ${FLAGS} ${INCLUDES} ./src/chip8.c -c -o ./build/chip8.o

clean:
	del build\* /q