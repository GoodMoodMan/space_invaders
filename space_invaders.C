/* game1.c - main, prntr */

#include <dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ARROW_NUMBER 30
#define TARGET_NUMBER 2
#define ARRSIZE 1000



void interrupt (*int9Save)(void); /* Pointer to function */
void interrupt (*int8Save)(void); /* Pointer to function */

int current_fps = 1;
int current_freq = 100;
char display_draft[25][80];
char display_draft_color[25][80];

void PrintChar(int pos, char input_ch, char color);


char entered_ascii_codes[ARRSIZE];

char display[2001];
char display_color[2001];

char ch_arr[ARRSIZE];
int tail = -1;
int front = -1;
int rear = -1;


int initial_run = 1;
int gun_position;
int no_of_arrows;
int target_disp = 80 / TARGET_NUMBER;
char ch;

int no_of_targets;

int score = 0;
int stage = 1;

int int8_count = 0;


int frame_ready = 1;



void my_halt()
{
//	setvect(8, int8Save);
	setvect(9, int9Save);
	asm{
    PUSH  AX
    MOV   AX,2
    INT   10h
    POP   AX
    MOV   AX,4C00h
    INT   21h
  }
	

} // my_halt()

void interrupt new_int8(void)
{
  
  int8_count++;

  asm {
		MOV AL,20h
		OUT 20h,AL
  }
  
  if (int8_count >= (1193180 / (current_fps * 100)))
  {
    int8_count = 0;
    frame_ready = 1;
  }
}

//void interrupt new_int9(...)
void interrupt new_int9(void)
{
  int scan;
  char ascii;

  asm {
    PUSH    AX
    PUSH    BX
    IN      AL,60h  // Read keyboard scan code
    MOV     BL,AL   // Store the scan code in BL register
    IN      AL,61h  // Read port 61h
    MOV     AH,AL   // Store the value of port 61h in AH
    OR      AL,80h  // Set the keyboard acknowledge bit
    OUT     61h,AL  // Send the acknowledgment
    XCHG    AH,AL   // Restore the original value of port 61h
    OUT     61h,AL  // Send the original value back to port 61h
    MOV     AL,20h  // Send End-of-Interrupt (EOI) to PIC
    OUT     20h,AL
    MOV     AX,BX   // Move the scan code from BL to AX
    MOV     scan,AX // Store the scan code in 'scan' variable
    POP     BX
    POP     AX
  }

  ascii = 0;
  if ((scan & 0x80) == 0) {  // Check if the key is being pressed (bit 7 is clear)
    switch (scan & 0x7F) {   // Mask off the release bit (bit 7)
      case 1:  // Esc pressed
        my_halt();
        break;
      case 30:  // a pressed
        ascii = 'a';
        break;
      case 17:  // w pressed
        ascii = 'w';
        break;
      case 32:  // d pressed
        ascii = 'd';
        break;
      default:  // Any other key pressed
        break;
    }

    if ((ascii != 0) && (tail < ARRSIZE - 1)) {
      entered_ascii_codes[++tail] = ascii;
    }
  }
}

typedef struct position
{
  int x;
  int y;

} POSITION;


void set_control(int hertz)
{
  unsigned divisor = 1193180L / hertz;

  asm {
		CLI
		PUSH AX
		MOV AL,036h
		OUT 43h,AL
		MOV AX,divisor
		OUT 40h,AL
		MOV AL,AH
		OUT 40h,AL
		POP AX
  }

  int8Save = getvect(8);
  setvect(8, new_int8);
  asm{ STI };
}

void display_score_stage()
{
    int i, j;
    char score_str[10];
    char stage_str[10];
    char *stage_label = "Stage: ";

    // Convert score and stage to strings
    itoa(score, score_str, 10);
    itoa(stage, stage_str, 10);

    // Display "Score: "
    // char *score_label = "Score: ";
    // for (i = 0; i < strlen(score_label); i++)
    // {
    //     display_draft[1][i] = score_label[i];
    //     display_draft_color[1][i] = 0x0F; // White color
    // }

    // // Display the score value
    // for (j = 0; j < strlen(score_str); j++)
    // {
    //     display_draft[1][i + j] = score_str[j];
    //     display_draft_color[1][i + j] = 0x0F; // White color
    // }

    // Display "Stage: "
    for (i = 0; i < strlen(stage_label); i++)
    {
	        display_draft[1][70 + i] = stage_label[i];
        display_draft_color[1][70 + i] = 0x0F; // White color
    }

    // Display the stage value
    for (j = 0; j < strlen(stage_str); j++)
    {
        display_draft[1][70 + i + j] = stage_str[j];
        display_draft_color[1][70 + i + j] = 0x0F; // White color
    }
}

void restart_game()
{
//    int i;
// CHANGE
    int i,j;


    // Reset game variables
    initial_run = 1;
    no_of_arrows = 0;
    gun_position = 39;

    // Clear the display draft
    for (i = 0; i < 25; i++)
//        for (int j = 0; j < 80; j++) {
        for (j = 0; j < 80; j++) {
            display_draft[i][j] = ' ';
            display_draft_color[i][j] = 0x07;
        }

    // Increase frequency and FPS
    current_freq += 10;
    current_fps++;

    // Increase stage
    stage++;

    // Update the PIT frequency and frame rate
//    set_control(1193180 / current_freq);
}

void PrintChar(int pos, char input_ch, char color){ // Prints a char to "pos" at the screen
	int print_pos = pos*2;
		asm{ 
			PUSH 	AX 
			PUSH 	ES
			PUSH 	SI
			PUSH 	BX
			MOV     AX,0B800h // Segment address of memory on color adapter
			MOV     ES,AX 	// Set up extra segment register
			MOV     SI,0 	// Initial offset address into segment
			MOV 	SI,print_pos // Moving to SI the position on the screen we want to print to
			MOV 	BL,input_ch	// Moving to BL the char we want to print
			MOV		BH,color		// Attributes
			MOV 	WORD PTR ES:[SI], BX	// Printing
			POP 	BX
			POP 	SI
			POP 	ES
			POP 	AX
		}
}


void displayer(void)
{
    int i;
    for (i = 0; i < 2000; i++) {
        PrintChar(i, display[i], display_color[i]);
    }
}  // displayer


void receiver()
{
  char temp;
  while (tail > -1)
  {
    temp = entered_ascii_codes[tail];
    rear++;
    tail--;
    if (rear < ARRSIZE)
      ch_arr[rear] = temp;
    if (front == -1)
      front = 0;
  } // while

} //  receiver


POSITION target_pos[TARGET_NUMBER];
POSITION arrow_pos[ARROW_NUMBER];

void game_over()
{
  int i, j;
  char *message = "GAME OVER PRESS ESC TO EXIT";
  int message_len = strlen(message);
  int start_col = (80 - message_len) / 2;

  for (i = 0; i < 25; i++)
    for (j = 0; j < 80; j++) {
      display_draft[i][j] = ' '; // Clear the display draft
      display_draft_color[i][j] = 0x07; // Set the default color (white on black)
    }

  // Display the game over message

  for (i = 0; i < message_len; i++)
  {
    display_draft[12][start_col + i] = message[i];
    display_draft_color[12][start_col + i] = 0x0C; // Set the color attribute (red on black)
  }

  // Update the display arrays
  for (i = 0; i < 25; i++)
    for (j = 0; j < 80; j++) {
      display[i * 80 + j] = display_draft[i][j];
      display_color[i * 80 + j] = display_draft_color[i][j];
    }

  displayer(); // Display the game over screen
  

  
}

void updater()
{
  int i, j;
  // targets flag, 0 if all targets down, otherwise 1
  int all_targets_hit = 1;

  if (initial_run == 1)
  {
    initial_run = 0;
    no_of_arrows = 0;

    no_of_targets = TARGET_NUMBER;

    gun_position = 39;

    target_pos[0].x = 3;
    target_pos[0].y = 0;

    for (i = 1; i < TARGET_NUMBER; i++)
    {
      target_pos[i].x = i * target_disp;
      target_pos[i].y = 0;
    } // for
    for (i = 0; i < ARROW_NUMBER; i++)
      arrow_pos[i].x = arrow_pos[i].y = -1;

  } // if (initial_run ==1)

  while (front != -1)
  {
    ch = ch_arr[front];
    if (front != rear)
      front++;
    else
      front = rear = -1;

    if ((ch == 'a') || (ch == 'A'))
      if (gun_position >= 2)
        gun_position--;
      else
        ;
    else if ((ch == 'd') || (ch == 'D'))
      if (gun_position <= 78)
        gun_position++;
      else
        ;
    else if ((ch == 'w') || (ch == 'W'))
      if (no_of_arrows < ARROW_NUMBER)
      {
        arrow_pos[no_of_arrows].x = gun_position;
        arrow_pos[no_of_arrows].y = 23;
        no_of_arrows++;
      } // if
  }     // while(front != -1)

  ch = 0;
  for (i = 0; i < 25; i++)
    for (j = 0; j < 80; j++)
      display_draft[i][j] = ' '; // blank





  display_draft[22][gun_position] = '^';
  display_draft[23][gun_position - 1] = '/';
  display_draft[23][gun_position] = '|';
  display_draft[23][gun_position + 1] = '\\';
  display_draft[24][gun_position] = '|';

  display_draft_color[22][gun_position] = 0x0A;
  display_draft_color[23][gun_position - 1] = 0x0A;
  display_draft_color[23][gun_position] = 0x0A;
  display_draft_color[23][gun_position + 1] = 0x0A;
  display_draft_color[24][gun_position] = 0x0A;

  for (i = 0; i < ARROW_NUMBER; i++)
    if (arrow_pos[i].x != -1)
    {
      if (arrow_pos[i].y > 0)
        arrow_pos[i].y--;
      display_draft[arrow_pos[i].y][arrow_pos[i].x] = '^';
      display_draft[arrow_pos[i].y + 1][arrow_pos[i].x] = '|';
      display_draft_color[arrow_pos[i].y][arrow_pos[i].x] = 0x0E;
      display_draft_color[arrow_pos[i].y + 1][arrow_pos[i].x] = 0x0E;
      // CHECK IF TARGET IS HIT
      for (j = 0; j < TARGET_NUMBER; j++)
      {

        if (arrow_pos[i].x == target_pos[j].x) {
          if (arrow_pos[i].y == target_pos[j].y || arrow_pos[i].y == target_pos[j].y-1 || arrow_pos[i].y == target_pos[j].y+1) {
            target_pos[j].x = target_pos[j].y = -1;
          }
        }
      }
    } // if

  for (i = 0; i < TARGET_NUMBER; i++)
  {
    if (target_pos[i].x != -1)
    {
      all_targets_hit = 0;
      if (target_pos[i].y < 22)
        target_pos[i].y++;
      else 
        game_over(); // Call the game_over function if any target reaches the bottom

      
      display_draft[target_pos[i].y][target_pos[i].x] = '*';
      display_draft_color[target_pos[i].y][target_pos[i].x] = 0x0C;
    } // if
  }

  

  // Check if all targets are hit
  if (all_targets_hit)
      restart_game();

  display_score_stage();


  // Update the display arrays
  for (i = 0; i < 25; i++)
    for (j = 0; j < 80; j++) {
      display[i * 80 + j] = display_draft[i][j];
      display_color[i * 80 + j] = display_draft_color[i][j];
    }
  display[2000] = '\0';

} // updater

main()
{
  


  asm{
		PUSH 	AX
		PUSH 	DI
		PUSH	ES
		PUSH	CX
		MOV		AH,0 // Select function = 'Set mode'
		MOV		AL,3 // 40 by 25 color image
		INT		10h // Adapter initialized.Page 0 displayed
		MOV		AX,0B800h // Segment address of memory on color adapter
		MOV		ES, AX // Set up extra segment register
		MOV		DI, 0  // Initial offset address into segment
		MOV		AL, ' ' // Character space to fill adapter memory
		MOV		AH, 03h // Attribute byte 
		MOV		CX, 1000 // Initialize count, 1 Screen
		CLD				// Write forward
		REP		STOSW // Write
		POP		CX
		POP		ES
		POP		DI
		POP		AX
	}
// CHANGE
  // set_control(1193180 / 100); // Set PIT frequency to 100 Hz
// END OF CHANGE
  int9Save = getvect(9);
  setvect(9, new_int9);


  while (1)
  {
//    if (frame_ready)
 //   {
//      frame_ready = 1;

// CHANGE
     sleep(2);      
// END OF CHANGE
  //   frame_ready = 1;

      receiver();
      updater();
      displayer();
//    }
  } // while

  

  return 0;

} // main