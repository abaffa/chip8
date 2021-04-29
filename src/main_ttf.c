#include <stdio.h>
#include <windows.h>
#include "SDL2/SDL.h"
#include "SDL2/SDL_ttf.h"

#include "chip8.h"


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

void print_font(SDL_Renderer* renderer, TTF_Font* sans, int x, int y, char *text){

    SDL_Color White = {255, 255, 255};  // this is the color in rgb format, maxing out all would give you the color white, and it will be your text's color

    SDL_Surface* surfaceMessage = TTF_RenderText_Solid(sans, text, White); // as TTF_RenderText_Solid could only be used on SDL_Surface then you have to create the surface first

    SDL_Texture* message = SDL_CreateTextureFromSurface(renderer, surfaceMessage); //now you can convert it into a texture

    SDL_Rect Message_rect; //create a rect
    Message_rect.x = x;  //controls the rect's x coordinate 
    Message_rect.y = y; // controls the rect's y coordinte
    Message_rect.w = surfaceMessage->w; // controls the width of the rect
    Message_rect.h = surfaceMessage->h; // controls the height of the rect

    //Mind you that (0,0) is on the top left of the window/screen, think a rect as the text's box, that way it would be very simple to understand

    //Now since it's a texture, you have to put RenderCopy in your game loop area, the area where the whole code executes

    SDL_RenderCopy(renderer, message, NULL, &Message_rect); //you put the renderer's name first, the Message, the crop size(you can ignore this if you don't want to dabble with cropping), and the rect which is the size and coordinate of your texture

    //SDL_FreeSurface(surfaceMessage);
    //SDL_DestroyTexture(message);
    
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

    //TTF_Init();
    //TTF_Font* sans = TTF_OpenFont("arial.ttf", 24); //this opens a font style and sets a size
    
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_TEXTUREACCESS_TARGET);
    Uint64 NOW = SDL_GetPerformanceCounter();
    Uint64 LAST = 0;
    double deltaTime = 0;
    double frame = 0;

    while(1){

        LAST = NOW;
        NOW = SDL_GetPerformanceCounter();

        deltaTime = (double)((NOW - LAST)*1000 / (double)SDL_GetPerformanceFrequency() );

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
            //Beep(15000, 100 * chip8.registers.sound_timer);
            chip8.registers.sound_timer = 0;
        }

        if(frame > 4){

            if(chip8.registers.delay_timer > 0)
                chip8.registers.delay_timer--;
            else{
                unsigned short opcode = chip8_memory_get_short(&chip8.memory, chip8.registers.PC);
                chip8.registers.PC += 2;
                chip8_exec(&chip8, opcode);
            }
            frame = 0;


            
            COORD pos = {0, 0};
            SetConsoleCursorPosition(hConsole, pos);


            int i = 0;
            for(i = 0; i < 12; i++)            
                printf(" V%02d|", i);
            printf("\n");

            for(i = 0; i < 12; i++)            
                printf(" %02x |", chip8.registers.V[i]);

            printf("\n\n");

            printf("  I   | dt | st |  PC  | SP |\n");
            printf(" %04x |", chip8.registers.I);
            printf(" %02x |", chip8.registers.delay_timer);
            printf(" %02x |", chip8.registers.sound_timer);
            printf(" %04x |", chip8.registers.PC);
            printf(" %02x |", chip8.registers.SP);
            
        }

        frame += deltaTime;

    }

out:
    SDL_DestroyWindow(window);
    return 0;
} 