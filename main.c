#include <genesis.h>
#include <resources.h>
#include <string.h>
#include <tools.h>

//Edge of the playing field / screen
const int LEFT_EDGE = 0;
const int RIGHT_EDGE = 320;
const int TOP_EDGE = 0;
const int BOTTOM_EDGE = 224;

//Ball vars
Sprite* ball;
int ballPosX = 100;
int ballPosY = 100;
int ballVelX = 1;
int ballVelY = 1;
int ballWidth = 8;
int ballHeight = 8;
int ballReleased = FALSE;

//Brick vars
struct Brick
{
	Sprite *brickSprite;
	int brickPosX;
	int brickPosY;
	int destroyed;
};
struct Brick bricksArray[32];
int maxBricks = 32;
int currentBricksHit = 0;
const int BRICK_WIDTH = 32;
const int BRICK_HEIGHT = 16;

//Player vars
Sprite* player;
int playerPosX = 144;
const int PLAYER_POS_Y = 200;
int playerVelX = 0;
const int PLAYER_WIDTH = 32;
const int PLAYER_HEIGHT = 8;

//Score vars
int score = 0;
char labelScore[6] = "SCORE\0";
char strScore[3] = "0";

//UI vars
int gameOn = FALSE;
char msgStart[22] = "Press Start to Begin!\0";
char msgReset[37] = "Game Over! Press Start to play Again.";
char msgComplete[43] = "Congratulations! Start to play Again.";

//Juice
int flasing = FALSE;
int frames = 0;
int ball_color = 0;

//Invaders
int moves = 0;
int drops = 0;
int direction = 1;

//Forward
//Player
int playerVelDirection();

//Game Flow
void initGame();
void startGame();
void endGame();
void winGame();
void checkWin();
void reset();

//Game Logic
void positionPlayer();
int playerVelDirection();
void moveBall();
void checkPaddleBallCollision();
void checkBallBrickCollision();
void ballCollisionAction(int brickIndex);
void flashBall();
void makeBrickField();
void moveBrickField();

//UI
void showLogo();
void showText(char s[]);
void updateScoreDisplay();

//Helpers
int sign(int x);

//Controller Callback
void myJoyHandler(u16 joy, u16 changed, u16 state);

int main()
{
	//Initiate Sprite Engine
	SPR_init(0,0,0);
	
	//Fade in our sick logo
	showLogo();

	//Update + waitVsync = do stuff per frame
	int counter = 0;
	while (1)
	{
		if(gameOn)
		{
			counter++;
			moveBall();
			positionPlayer();
			flashBall();

			// Every 30 frames move the brick field Space Invaders Style
			if(counter % 30 == 0)
			{
				moveBrickField();
			}

			//Increase our frame counter
			if(counter % 60 == 0)
			{
				counter = 0;
			}
		}
		//Update the Sprite drawing
		SPR_update();
		VDP_waitVSync();
	}
	return(0);
}

//Game Flow
void initGame()
{
	//init game
	resetGame();

	//Play a sound effect and some chill beats
	XGM_setLoopNumber(10);
	XGM_setPCM(65, &start, 17664);
	XGM_startPlayPCM(65, 1, SOUND_PCM_CH2);
	XGM_startPlay(&music);

	//Init the Joypad
	JOY_init();
	//Defines a function for joystick callbacks
	JOY_setEventHandler(&myJoyHandler);

	//Load our tileset
	VDP_loadTileSet(bgtile.tileset, 1, DMA);
	//Set the current pallet
	VDP_setPalette(PAL1, bgtile.palette->data);
	//Fill the screen
	VDP_fillTileMapRect(PLAN_B, TILE_ATTR_FULL(PAL1, 0, FALSE, FALSE, 1), 0, 0, 40, 30);

	//Draw the UI on Plane A
	VDP_setTextPlan(PLAN_A);
	VDP_drawText(labelScore, 1, 1);
	//Draw the UI on Plane B
	VDP_setTextPlan(PLAN_B);
	VDP_drawText(labelScore, 1, 1);
	updateScoreDisplay();
	showText(msgStart);

	//Assign our Sprite
	ball = SPR_addSprite(&imgball, (playerPosX + 1) + (PLAYER_WIDTH / 2) - (ballWidth / 2), PLAYER_POS_Y - 10, TILE_ATTR(PAL1, 0, FALSE, FALSE));
	player = SPR_addSprite(&paddle, playerPosX, PLAYER_POS_Y, TILE_ATTR(PAL1, 0, FALSE, FALSE));
	ball_color = VDP_getPaletteColor(22);
}

void startGame()
{
	//Place our bricks
	makeBrickField();
	//Reset our score
	score = 0;
	updateScoreDisplay();

	//Place our ball center of our paddle and slightly above it
	ballPosX = playerPosX + 16;
	ballPosY = PLAYER_POS_Y - 10;

	//Randomly select which direction our ball will start off in when released
	ballVelX = playerVelDirection();
	ballVelY = -1;

	//Clear the text from the screen
	VDP_clearTextArea(0, 10, 40, 10);
	//Replace the tiles we've cleared
	VDP_fillTileMapRect(PLAN_B, TILE_ATTR_FULL(PAL1, 0, FALSE, FALSE, 1), 0, 10, 40, 10);
	gameOn = TRUE;
}

void endGame()
{
	//Play our gameOver Sound
	XGM_setPCM(65, &gameOver, 17408);
	XGM_startPlayPCM(65, 1, SOUND_PCM_CH2);
	//Show game over text
	showText(msgReset);
	//Set game state to off
	gameOn = FALSE;
}

void winGame()
{ 
	//Play our gameOver Sound
	XGM_setPCM(65, &win, 36352);
	XGM_startPlayPCM(65, 1, SOUND_PCM_CH2);
	//Show game over text
	showText(msgComplete);
	//Set game state to off
	gameOn = FALSE;
}

void resetGame()
{
	//Remove all existing sprites
	for (int i = 0; i < maxBricks; i++)
	{
		SPR_releaseSprite(bricksArray[i].brickSprite);
	}

	//Reset game state to off
	gameOn = FALSE;
	//Reset the ball and player and brick counter
	ballReleased = FALSE;
	playerPosX = 144;
	currentBricksHit = 0;
}

void checkWin()
{
	if (currentBricksHit >= maxBricks)
	{
		winGame();
	}
}

//Game Logic
void positionPlayer()
{
	//Add the player's velocity to its position
	playerPosX += playerVelX;

	//Keep the player within the bounds of the screen
	if (playerPosX < LEFT_EDGE)
	{
		playerPosX = LEFT_EDGE;
	}
	if (playerPosX + PLAYER_WIDTH > RIGHT_EDGE)
	{
		playerPosX = RIGHT_EDGE - PLAYER_WIDTH;
	}

	//Tell the sprite engine to position the sprite
	SPR_setPosition(player, playerPosX, PLAYER_POS_Y);
}

int playerVelDirection()
{
	//Randomly picks which direction the ball will go
	int x = random();
	if (x > 32767)
	{
		return -1;
	}
	else if (x < 32767)
	{
		return 1;
	}
}

void moveBall()
{
	if (ballReleased == TRUE)
	{
		//Check horizontal bounds
		if (ballPosX < LEFT_EDGE)
		{
			//Reverse the ball's velocity
			ballPosX = LEFT_EDGE;
			ballVelX = -ballVelX;
			//Play the wall hit sound
			XGM_setPCM(65, &wall, 6400);
			XGM_startPlayPCM(65, 1, SOUND_PCM_CH2);
		}
		else if (ballPosX + ballWidth > RIGHT_EDGE)
		{
			ballPosX = RIGHT_EDGE - ballWidth;
			ballVelX = -ballVelX;
			XGM_setPCM(65, &wall, 6400);
			XGM_startPlayPCM(65, 1, SOUND_PCM_CH2);
		}

		//Check vertical bounds
		if (ballPosY < TOP_EDGE)
		{
			ballPosY = TOP_EDGE;
			ballVelY = -ballVelY;
			XGM_setPCM(65, &wall, 6400);
			XGM_startPlayPCM(65, 1, SOUND_PCM_CH2);
		}
		else if (ballPosY + ballHeight > BOTTOM_EDGE)
		{
			endGame();
		}

		checkPaddleBallCollision();
		checkBallBrickCollision();
		//Ball Movement Logic here
		ballPosX += ballVelX;
		ballPosY += ballVelY;
	}
	else if (ballReleased == FALSE)
	{
		//Reset the ball position to over the paddle
		ballPosX = (playerPosX + 1) + (PLAYER_WIDTH / 2) - (ballWidth / 2);
		ballPosY = PLAYER_POS_Y - 10;
	}

	//Update the Sprites Pos
	SPR_setPosition(ball, ballPosX, ballPosY);
}

//AABB Collision
void checkPaddleBallCollision()
{
	//Check if the left edge of the ball is to the left of the right edfe of the paddle
	//Check if the right edge of the ball is to the right of the left edge of the paddle
	if (ballPosX < playerPosX + PLAYER_WIDTH && ballPosX + ballWidth > playerPosX)
	{
		//Check if we are inside the vertical bounds of the player paddle
		if (ballPosY < PLAYER_POS_Y + PLAYER_HEIGHT && ballPosY + ballHeight >= PLAYER_POS_Y)
		{
			XGM_setPCM(65, &hit, 4352);
			XGM_startPlayPCM(65, 1, SOUND_PCM_CH2);
			//SND_startPlay_4PCM(&hit, 9728, SOUND_PCM_CH1, SOUND_PCM_CH3);
			//A collision is happening!
			ballPosY = PLAYER_POS_Y - ballHeight - 1;
			ballVelY = -ballVelY;

			flasing = TRUE;
		}
	}
}

//TODO: Refactor
void checkBallBrickCollision()
{
	for (int i = 0; i < maxBricks; i++)
	{
		if (bricksArray[i].destroyed == FALSE)
		{
			//Check Bottom brick collision
			if (ballPosX < bricksArray[i].brickPosX + BRICK_WIDTH - 2 && ballPosX + ballWidth > bricksArray[i].brickPosX + 2)
			{
				if (ballPosY < bricksArray[i].brickPosY + BRICK_HEIGHT && ballPosY + ballHeight >= bricksArray[i].brickPosY + 20)
				{
					ballCollisionAction(i);
					ballVelY = -ballVelY;
				}
			}
			//Check Top BrickCollision
			if (ballPosX < bricksArray[i].brickPosX + BRICK_WIDTH - 2 && ballPosX + ballWidth > bricksArray[i].brickPosX + 2)
			{
				if (ballPosY < bricksArray[i].brickPosY + BRICK_HEIGHT - 20 && ballPosY + ballHeight >= bricksArray[i].brickPosY)
				{
					ballCollisionAction(i);
					ballVelY = -ballVelY;
				}
			}
			//Check Left Side
			if (ballPosX < bricksArray[i].brickPosX + BRICK_WIDTH - 28 && ballPosX + ballWidth > bricksArray[i].brickPosX)
			{
				if (ballPosY < bricksArray[i].brickPosY + BRICK_HEIGHT - 4 && ballPosY + ballHeight >= bricksArray[i].brickPosY + 4)
				{
					ballCollisionAction(i);
					ballVelX = -ballVelX;
				}
			}
			//Check Right Side
			if (ballPosX < bricksArray[i].brickPosX + BRICK_WIDTH && ballPosX + ballWidth > bricksArray[i].brickPosX + 28)
			{
				if (ballPosY < bricksArray[i].brickPosY + BRICK_HEIGHT - 4 && ballPosY + ballHeight >= bricksArray[i].brickPosY + 4)
				{
					ballCollisionAction(i);
					ballVelX = -ballVelX;
				}
			}
		}
	}
}

void ballCollisionAction(int brickIndex)
{
	XGM_setPCM(66, &hitBrick, 4608);
	XGM_startPlayPCM(66, 1, SOUND_PCM_CH2);
	
	//Destroy Brick
	SPR_releaseSprite(bricksArray[brickIndex].brickSprite);
	bricksArray[brickIndex].destroyed = TRUE;

	flasing = TRUE;
	score++;
	updateScoreDisplay();
	if (score % 10 == 0)
	{
		ballVelX += sign(ballVelX);
		ballVelY += sign(ballVelY);
	}
	currentBricksHit++;
	checkWin();
}

void flashBall()
{
	if (flasing == TRUE)
	{
		//Flashing code goes here
		frames++;

		if (frames % 4 == 0)
		{
			VDP_setPaletteColor(22, ball_color);
		}
		else if (frames % 2 == 0)
		{
			VDP_setPaletteColor(22, RGB24_TO_VDPCOLOR(0xffffff));
		}

		if (frames > 12)
		{
			flasing = FALSE;
			frames = 0;
			VDP_setPaletteColor(22, ball_color);
		}
	}
}

void makeBrickField()
{
	int x = 1;
	int y = 1;
	for (int i = 0; i < maxBricks; i++)
	{
		bricksArray[i].brickSprite = SPR_addSprite(&bricks, x * 32, y * 16, TILE_ATTR(PAL1, 0, FALSE, FALSE));
		bricksArray[i].brickPosX = x * 32;
		bricksArray[i].brickPosY = y * 16;

		x++;

		if (x > 8)
		{
			y++;
			x = 1;
		}
	}
}

//Space Invaders style movement
void moveBrickField()
{
	moves++;
	if (moves <= 6)
	{
		for (int i = 0; i < maxBricks; i++)
		{
			bricksArray[i].brickPosX += 3 * direction;
			SPR_setPosition(bricksArray[i].brickSprite, bricksArray[i].brickPosX, bricksArray[i].brickPosY);
		}
	}
	if (moves > 6)
	{
		moves = 0;
		if (direction > 0)
		{
			direction = -1;
		}
		else
		{
			direction = 1;
		}
		for (int i = 0; i < maxBricks; i++)
		{
			bricksArray[i].brickPosY += 3;
			SPR_setPosition(bricksArray[i].brickSprite, bricksArray[i].brickPosX, bricksArray[i].brickPosY);
		}
	}
}

//UI
void showLogo()
{
	u16 logoTileW = smbLogo.w / 8;
	u16 logoTileH = smbLogo.h / 8;
	VDP_loadBMPTileData(smbLogo.image, 3, logoTileW, logoTileH, logoTileW);
	VDP_setPalette(PAL3, smbLogo.palette->data);
	VDP_fillTileMapRectInc(PLAN_A, TILE_ATTR_FULL(PAL3, 0, FALSE, FALSE, 3), 4, 9, logoTileW, logoTileH);

	VDP_fadePal(PAL3, palette_black, smbLogo.palette->data, 60, 1);

	VDP_waitFadeCompletion();
	waitTick(TICKPERSECOND * 4);
	VDP_fadePal(PAL3, smbLogo.palette->data, palette_black, 60, 1);
	VDP_waitFadeCompletion();
	initGame();
}

void showText(char s[])
{
	VDP_drawText(s, 20 - strlen(s) / 2, 15);
}

void updateScoreDisplay()
{
	sprintf(strScore, "%d", score);
	//This clears the tiles where the text was too
	VDP_clearText(1, 2, 3);
	//Add the tiles behind the text again
	VDP_fillTileMapRect(PLAN_B, TILE_ATTR_FULL(PAL1, 0, FALSE, FALSE, 1), 0, 0, 40, 30);
	VDP_drawText(strScore, 1, 2);
}

//Helpers
int sign(int x)
{
	return (x > 0) - (x < 0);
}

//Controller Callbacks
void myJoyHandler(u16 joy, u16 changed, u16 state)
{
	//If the player 1 port Joypad was firing an event
	if (joy == JOY_1)
	{
		if (state & BUTTON_START)
		{
			if (!gameOn)
			{
				resetGame();
				startGame();
			}
		}

		if (state & BUTTON_A || state & BUTTON_B || state & BUTTON_C)
		{
			if (ballReleased == FALSE)
			{
				ballReleased = TRUE;
			}
		}

		//Set the player velocity if the left or right dpad are pressed;
		//Set velocity to 0 if no direction is pressed
		//State = This will be 1 if the button is currently pressed and 0 if it isn’t.
		if (state & BUTTON_RIGHT)
		{
			playerVelX = 3;
		}
		else if (state & BUTTON_LEFT)
		{
			playerVelX = -3;
		}
		else
		{
			//changed = This tells us whether the state of a button has changed over the last frame.
			//If the current state is different from the state in the previous frame, this will be 1 (otherwise 0).
			if ((changed & BUTTON_RIGHT) | (changed & BUTTON_LEFT))
			{
				playerVelX = 0;
			}
		}
	}
}