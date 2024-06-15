#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_mixer.h>

#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480

#define BOARD_WIDTH 40
#define BOARD_HEIGHT 28

#define SNAKE_SIZE 16
#define FOOD_SIZE 16

#define UP 0
#define RIGHT 1
#define DOWN 2
#define LEFT 3

typedef struct
{
    int x[BOARD_WIDTH * BOARD_HEIGHT];
    int y[BOARD_WIDTH * BOARD_HEIGHT];
    int length;
    int direction;
} Snake;

TTF_Font *font = NULL;
SDL_Color white = {255, 255, 255, 255}; // RGBA
SDL_Color green = {50, 255, 255, 50}; // RGBA
SDL_Color red = {255, 50, 50, 255}; // RGBA
SDL_Color blue = {50, 50, 255, 255}; // RGBA
Mix_Chunk *eat_sound = NULL;            // Variable globale pour le son

int food_x, food_y;
int game_over = 0;
int high_score = 0;
SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;

// Ajoutez ces déclarations de variables globales
int border_x = 1;
int border_y = 1;
int border_width = BOARD_WIDTH - 2;
int border_height = BOARD_HEIGHT - 2;

void init_game(Snake *snake);
void draw_game(Snake *snake);
void move_snake(Snake *snake);
void generate_food();
int check_collision(Snake *snake);
void load_high_score();
void save_high_score(int score);
int ask_replay();
void wait_for_p_key();

int main(int argc, char *argv[])
{
    SDL_SetMainReady();
    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();
    font = TTF_OpenFont("c:\\Windows\\Fonts\\Arial.ttf", 24);
    if (!font)
    {
        printf("TTF_OpenFont: %s\n", TTF_GetError());
    }
    Mix_OpenAudio(22050, MIX_DEFAULT_FORMAT, 2, 4096); // Initialisez SDL_mixer
    eat_sound = Mix_LoadWAV("pacman_eatfruit.wav");    // Chargez le son

    window = SDL_CreateWindow("Snake SDL Game", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    Snake snake;
    init_game(&snake);

    SDL_Event event;
    int quit = 0;
    load_high_score();
    while (!quit)
    {
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_QUIT:
                quit = 1;
                break;
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym)
                {
                case SDLK_UP:
                    if (snake.direction != DOWN)
                        snake.direction = UP;
                    break;
                case SDLK_RIGHT:
                    if (snake.direction != LEFT)
                        snake.direction = RIGHT;
                    break;
                case SDLK_DOWN:
                    if (snake.direction != UP)
                        snake.direction = DOWN;
                    break;
                case SDLK_LEFT:
                    if (snake.direction != RIGHT)
                        snake.direction = LEFT;
                    break;
                case SDLK_p:
                    wait_for_p_key();
                    break;
                }
                break;
            }
        }

        move_snake(&snake);

        if (check_collision(&snake))
        {
            game_over = 1;
            int final_score = snake.length - 1;
            //printf("Le serpent a avalé %d jetons rouges.\n", final_score);
            save_high_score(final_score);
            int replay = ask_replay();
            if (replay)
            {
                init_game(&snake);
                game_over = 0;
            }
            else
            {
                quit = 1;
            }
        }

        draw_game(&snake);

        SDL_Delay(150-(snake.length-1)*2);
    }
    Mix_FreeChunk(eat_sound); // Libérez la mémoire
    Mix_CloseAudio();         // Fermez SDL_mixer
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

void init_game(Snake *snake)
{
    snake->x[0] = BOARD_WIDTH / 2;
    snake->y[0] = BOARD_HEIGHT / 2;
    snake->length = 1;
    snake->direction = RIGHT;

    generate_food();
}
void load_high_score()
{
    FILE *file = fopen("high_score.txt", "r");
    if (file != NULL)
    {
        fscanf(file, "%d", &high_score);
        fclose(file);
    }
}
void save_high_score(int score)
{
    if (score > high_score)
    {
        high_score = score;
        FILE *file = fopen("high_score.txt", "w");
        if (file != NULL)
        {
            fprintf(file, "%d", high_score);
            fclose(file);
        }
    }
}
// Ajoutez une fonction pour dessiner le score
void draw_score(int score)
{
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
    char score_text[50];
    sprintf(score_text, "Score: %d", score);
    SDL_Surface *surface = TTF_RenderText_Solid(font, score_text, blue);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_Rect text_rect = {WINDOW_WIDTH - 130, WINDOW_HEIGHT - 30, 100, 30};
    SDL_RenderCopy(renderer, texture, NULL, &text_rect);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}

void draw_game(Snake *snake)
{
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(renderer);

    // Draw border
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
    SDL_Rect border_rect = {border_x * SNAKE_SIZE, border_y * SNAKE_SIZE, border_width * SNAKE_SIZE, border_height * SNAKE_SIZE};
    SDL_RenderDrawRect(renderer, &border_rect);

// Dessiner la tête du serpent
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
    SDL_Rect head_rect = {snake->x[0] * SNAKE_SIZE, snake->y[0] * SNAKE_SIZE, SNAKE_SIZE, SNAKE_SIZE};
    SDL_RenderFillRect(renderer, &head_rect);

    // Dessiner les yeux
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    int eye_size = SNAKE_SIZE / 4; // Taille des yeux (un quart de la taille du serpent)
    int eye_x = snake->x[0] * SNAKE_SIZE + SNAKE_SIZE / 4; // Position x du premier œil
    int eye_y = snake->y[0] * SNAKE_SIZE + SNAKE_SIZE / 4; // Position y du premier œil
    SDL_Rect eye1_rect = {eye_x, eye_y, eye_size, eye_size};
    SDL_RenderFillRect(renderer, &eye1_rect);

    eye_x = snake->x[0] * SNAKE_SIZE + SNAKE_SIZE / 2; // Position x du deuxième œil
    SDL_Rect eye2_rect = {eye_x, eye_y, eye_size, eye_size};
    SDL_RenderFillRect(renderer, &eye2_rect);

    // Dessiner le reste du corps
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
    for (int i = 1; i < snake->length; i++) {
        SDL_Rect rect = {snake->x[i] * SNAKE_SIZE, snake->y[i] * SNAKE_SIZE, SNAKE_SIZE, SNAKE_SIZE};
        SDL_RenderFillRect(renderer, &rect);
    }
    //Dessiner la nourriture
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_Rect food_rect = {food_x * FOOD_SIZE, food_y * FOOD_SIZE, FOOD_SIZE, FOOD_SIZE};
    SDL_RenderFillRect(renderer, &food_rect);

    // Draw score
    draw_score(snake->length - 1);

    // Dessiner le high score
    SDL_SetRenderDrawColor(renderer, 0, 255, 128, SDL_ALPHA_OPAQUE);
    char high_score_text[50];
    sprintf(high_score_text, "High Score: %d", high_score);
    SDL_Surface *surface = TTF_RenderText_Solid(font, high_score_text, green);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_Rect text_rect = {30, WINDOW_HEIGHT - 30, 200, 30};
    SDL_RenderCopy(renderer, texture, NULL, &text_rect);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);

    SDL_RenderPresent(renderer);
}

void move_snake(Snake *snake)
{
    int prev_x = snake->x[0];
    int prev_y = snake->y[0];

    switch (snake->direction)
    {
    case UP:
        snake->y[0]--;
        break;
    case RIGHT:
        snake->x[0]++;
        break;
    case DOWN:
        snake->y[0]++;
        break;
    case LEFT:
        snake->x[0]--;
        break;
    }

    for (int i = 1; i < snake->length; i++)
    {
        int temp_x = snake->x[i];
        int temp_y = snake->y[i];
        snake->x[i] = prev_x;
        snake->y[i] = prev_y;
        prev_x = temp_x;
        prev_y = temp_y;
    }

    if (snake->x[0] == food_x && snake->y[0] == food_y)
    {
        snake->length++;
        Mix_PlayChannel(-1, eat_sound, 0); // Jouez le son
        generate_food();
    }
}

void generate_food()
{
    srand(time(NULL));
    food_x = border_x + rand() % border_width;
    food_y = border_y + rand() % border_height;
}

int check_collision(Snake *snake)
{
    if (snake->x[0] < border_x || snake->x[0] >= border_x + border_width || snake->y[0] < border_y || snake->y[0] >= border_y + border_height)
    {
        return 1;
    }

    for (int i = 1; i < snake->length; i++)
    {
        if (snake->x[0] == snake->x[i] && snake->y[0] == snake->y[i])
        {
            return 1;
        }
    }
    return 0;
}

int ask_replay()
{
    SDL_Event event;
    int replay = -1; // Initialiser replay à -1 pour indiquer qu'aucun choix n'a été fait

    while (replay == -1)
    {
        SDL_PollEvent(&event);
        if (event.type == SDL_KEYDOWN)
        {
            switch (event.key.keysym.sym)
            {
            case SDLK_y:
                replay = 1;
                break;
            case SDLK_n:
                replay = 0;
                break;
            default:
                break;
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
        SDL_RenderClear(renderer);

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
        SDL_Surface *surface = TTF_RenderText_Solid(font, "Play Again ? (y/n)", red);
        SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_Rect text_rect = {WINDOW_WIDTH / 2 - 100, WINDOW_HEIGHT / 2 - 25, 200, 50};
        SDL_RenderCopy(renderer, texture, NULL, &text_rect);
        SDL_FreeSurface(surface);
        SDL_DestroyTexture(texture);

        SDL_RenderPresent(renderer);
    }

    return replay;
}
void wait_for_p_key()
{
    SDL_Event event;
    int waiting = 1;

    while (waiting)
    {
        SDL_WaitEvent(&event);
        if (event.type == SDL_KEYDOWN)
        {
            if (event.key.keysym.sym == SDLK_p)
            {
                waiting = 0;
            }
        }
    }
}