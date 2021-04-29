#include <stdio.h>
#include <windows.h>
#include "SDL2/SDL.h"
#include "chip8.h"
#include <math.h>
#include <time.h>
#include <pthread.h> 

const char keyboard_map[CHIP8_TOTAL_KEYS] = {
    SDLK_0, SDLK_1, SDLK_2, SDLK_3, SDLK_4, SDLK_5,
    SDLK_6, SDLK_7, SDLK_8, SDLK_9, SDLK_a, SDLK_b,
    SDLK_c, SDLK_d, SDLK_e, SDLK_f};



void cls(HANDLE hConsole)
{
    COORD coordScreen = { 0, 0 };    // home for the cursor
    DWORD cCharsWritten;
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    DWORD dwConSize;

    // Get the number of character cells in the current buffer.
    if (!GetConsoleScreenBufferInfo(hConsole, &csbi))
    {
        return;
    }

    dwConSize = csbi.dwSize.X * csbi.dwSize.Y;

    // Fill the entire screen with blanks.
    if (!FillConsoleOutputCharacter(hConsole,        // Handle to console screen buffer
                                    (TCHAR)' ',      // Character to write to the buffer
                                    dwConSize,       // Number of cells to write
                                    coordScreen,     // Coordinates of first cell
                                    &cCharsWritten)) // Receive number of characters written
    {
        return;
    }

    // Get the current text attribute.
    if (!GetConsoleScreenBufferInfo(hConsole, &csbi))
    {
        return;
    }

    // Set the buffer's attributes accordingly.
    if (!FillConsoleOutputAttribute(hConsole,         // Handle to console screen buffer
                                    csbi.wAttributes, // Character attributes to use
                                    dwConSize,        // Number of cells to set attribute
                                    coordScreen,      // Coordinates of first cell
                                    &cCharsWritten))  // Receive number of characters written
    {
        return;
    }

    // Put the cursor at its home coordinates.
    SetConsoleCursorPosition(hConsole, coordScreen);
}

void *run_thread(void *vargp)
{

	struct chip8 *chip8 = (struct chip8*)vargp;
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	cls(hConsole);

	double deltaTime = 0;
	double frame = 0;
	double cpu_clk = 0;
	double cpu = 0;

	int iii = 0;
	double speed = 5e-4;

	struct timespec tstart = { 0,0 }, tend = { 0,0 };
	while (1) {

		clock_gettime(CLOCK_MONOTONIC, &tend);
		deltaTime = ((double)tend.tv_sec + 1.0e-9*tend.tv_nsec) - ((double)tstart.tv_sec + 1.0e-9*tstart.tv_nsec);
		clock_gettime(CLOCK_MONOTONIC, &tstart);

        frame += deltaTime;
		cpu += deltaTime;
		cpu_clk += deltaTime;

        if(cpu >= speed){

            if(chip8->registers.delay_timer > 0)
                chip8->registers.delay_timer--;
            else{
                unsigned short opcode = chip8_memory_get_short(&chip8->memory, chip8->registers.PC);
                chip8->registers.PC += 2;
                chip8_exec(chip8, opcode);
            }
            cpu = 0;
        }

        if (frame >= 0.1) {
            COORD pos = {0, 0};
            SetConsoleCursorPosition(hConsole, pos);


            int i = 0;
            for(i = 0; i < 12; i++)            
                printf(" V%02d|", i);
            printf("\n");

            for(i = 0; i < 12; i++)            
                printf(" %02x |", chip8->registers.V[i]);

            printf("\n\n");

            printf("  I   | dt | st |  PC  | SP |\n");
            printf(" %04x |", chip8->registers.I);
            printf(" %02x |", chip8->registers.delay_timer);
            printf(" %02x |", chip8->registers.sound_timer);
            printf(" %04x |", chip8->registers.PC);
            printf(" %02x |", chip8->registers.SP);
            frame = 0;
    
        }
    }
}


const double FREQ = 441.0f;
const int AMPLITUDE = 15000;
const int SAMPLE_RATE = 44100;

void audio_callback(void *user_data, Uint8 *raw_buffer, int bytes)
{
    Sint16 *buffer = (Sint16*)raw_buffer;
    int length = bytes / 2; // 2 bytes per sample for AUDIO_S16SYS
    int *sample_nr = (int*)user_data;
    int i;
    for(i = 0; i < length; i++, (*sample_nr)++)
    {
        double time = (double)(*sample_nr) / (double)SAMPLE_RATE;
        buffer[i] = (Sint16)(AMPLITUDE * sin(2.0f * M_PI * FREQ * time)); // render 441 HZ sine wave
    }
}


int main(int argc, char** argv)
{
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    cls(hConsole);

    if(argc < 2)
    {
        printf("You must provide a file to load\n");
        return -1;
    }

    const char* filename = argv[1];
    printf("The filename to load is: %s\n", filename);

    FILE* f = fopen(filename, "rb");
    if(!f)
    {
        printf("Failed to open the file");
        return -1;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char buf[size];
    int res = fread(buf, size, 1, f);
    if(res != 1)
    {
        printf("Failed to read from file");
        return -1;
    }

    struct chip8 chip8;
    chip8_init(&chip8);
    chip8_load(&chip8, buf, size);
    chip8_keyboard_set_map(&chip8.keyboard, keyboard_map);

    

    SDL_Init(SDL_INIT_EVERYTHING); 
    SDL_Window* window = SDL_CreateWindow(
        EMULATOR_WINDOW_TITLE, 
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        CHIP8_WIDTH * CHIP8_WINDOW_MULTIPLIER,
        CHIP8_HEIGHT * CHIP8_WINDOW_MULTIPLIER,
        SDL_WINDOW_SHOWN
    );

    if(SDL_Init(SDL_INIT_AUDIO) != 0) SDL_Log("Failed to initialize SDL: %s", SDL_GetError());

    int sample_nr = 0;

    SDL_AudioSpec want;
    want.freq = SAMPLE_RATE; // number of samples per second
    want.format = AUDIO_S16SYS; // sample type (here: signed short i.e. 16 bit)
    want.channels = 1; // only one channel
    want.samples = 2048; // buffer-size
    want.callback = audio_callback; // function SDL calls periodically to refill the buffer
    want.userdata = &sample_nr; // counter, keeping track of current sample number

    SDL_AudioSpec have;
    if(SDL_OpenAudio(&want, &have) != 0) SDL_LogError(SDL_LOG_CATEGORY_AUDIO, "Failed to open audio: %s", SDL_GetError());
    if(want.format != have.format) SDL_LogError(SDL_LOG_CATEGORY_AUDIO, "Failed to get the desired AudioSpec");
    
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_TEXTUREACCESS_TARGET);


	pthread_t tid;
	pthread_create(&tid, NULL, run_thread, (void *)&chip8);


    while(1){

  
        SDL_Event event;
        while(SDL_PollEvent(&event)){

            switch(event.type){

            
                case SDL_QUIT:
                    goto out;
                break;

                case SDL_KEYDOWN:{
                    char key = event.key.keysym.sym;
                    int vkey = chip8_keyboard_map(&chip8.keyboard, key);
                    if(vkey != -1)
                        chip8_keyboard_down(&chip8.keyboard, vkey);
                }
                break;

                case SDL_KEYUP:{
                    char key = event.key.keysym.sym;
                    int vkey = chip8_keyboard_map(&chip8.keyboard, key);
                    if(vkey != -1)
                        chip8_keyboard_up(&chip8.keyboard, vkey);
                }
                break;                
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
        SDL_RenderClear(renderer);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 0);

        int x, y;
        for(x = 0; x < CHIP8_WIDTH; x++)
        {
            for(y = 0; y < CHIP8_HEIGHT; y++)
            {

                if(chip8_screen_is_set(&chip8.screen, x, y))
                {
                    SDL_Rect r;
                    r.x = x * CHIP8_WINDOW_MULTIPLIER;
                    r.y = y * CHIP8_WINDOW_MULTIPLIER;
                    r.w = CHIP8_WINDOW_MULTIPLIER;
                    r.h = CHIP8_WINDOW_MULTIPLIER;
                    SDL_RenderFillRect(renderer, &r);
                }
            }
        }

        SDL_RenderPresent(renderer);

        /*
        if(chip8.registers.delay_timer > 0){
            Sleep(1);
            chip8.registers.delay_timer--;
        }
        */
        if(chip8.registers.sound_timer > 0){
            SDL_PauseAudio(0); // start playing sound
            SDL_Delay(chip8.registers.sound_timer*10); // wait while sound is playing
            SDL_PauseAudio(1); // stop playing sound
            chip8.registers.sound_timer = 0;
        }

  
    }

out:
    SDL_CloseAudio();
    SDL_DestroyWindow(window);
    return 0;
} 