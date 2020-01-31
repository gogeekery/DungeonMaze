#include <Windows.h>
#include <memory.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>


/*

	A simple procedurally generated 'dungeon maze'.
	Designed with speed and simplicity rather than superior maze quality.

	Notes (about the maps):

		The blue dot represents the starting point or 'stairs up'.
		The purple dot represents the end or 'stairs down'.

		The green dot(s) represent door(s).

*/





// ---------------------------------------------
// Type Defines --------------------------------

typedef unsigned char			tU8;
typedef unsigned short			tU16;
typedef unsigned long			tU32;
typedef unsigned long long		tU64;

typedef signed char				tS8;
typedef signed short			tS16;
typedef signed long				tS32;
typedef signed long long		tS64;


typedef char					bool;

enum {
	false, true
};



// ---------------------------------------------
// Function Defines ----------------------------

#define dMin(Var1, Var2) ((Var1 < Var2) ? Var1:Var2)
#define dMax(Var1, Var2) ((Var1 > Var2) ? Var1:Var2)

#define dClamp(Value, Min, Max) ((Value < Min) ? Min:(Value > Max) ? Max:Value)

#define dSign(Value)	(((Value)<0) ? -1:((Value)>0) ? 1:0)
#define dAbs(Value)		(((Value)<0) ? (-(Value)):((Value)))

#define dRand(Value)	(rand()%(Value))


// ---------------------------------------------
// Windows Defines and Data --------------------

#define WIN_NAME	"Fast dungeon/maze generator"

HDC		hWinDC;


// ---------------------------------------------
// Map Defines ---------------------------------

#define cTileSize		6									// Size of a tile (in pixels)

#define cMapWidth		90									// Size of map (in tiles)
#define cMapHeight		90

#define cRoomNum		(cMapWidth*cMapHeight)				// Attempted number of rooms
#define cRoomSize		(4)									// Size of a room range x to (x*2)
#define cHallChance		(4)									// 1/x chance that the room will be corridor in shape
#define cDoorChance		(2)									// 1/x chance the connection point will be a door

#define cMapSeed		9578768U							// Seed for generating the maps


// ---------------------------------------------
// Global vars ---------------------------------

tU8 gMapData[cMapWidth*cMapHeight];					// Holds the tiles on the map

tU32 gMapLevel = 0;									// More levels than you can count!



// Tiles ---------------------------------------

#define cFloor			0
#define cWall			1
#define cDoor			2
#define cStart			4
#define cEnd			5

#define cTileNum		6





// ====== Functions ==============================
// ==================================================

inline void genFillMap(const tU16 X, const tU16 Y, const tU8 W, const tU8 H, const tU8 Val) {
	/// Quickly fills a section of the map (larger rooms are faster)!

	static tU8 YP;
	for (YP = 0; YP < H; ++YP)
		memset(&gMapData[X+((Y+YP)*cMapWidth)], Val, W);

}


bool genConPoint(const tU16 X, const tU16 Y, tS8 *XDir, tS8 *YDir) {
	/// Returns true if a room/corridor can connect here.
	/// Sets XDir and YDir to the direction the corridor/room will expand.

	if (gMapData[X+(Y*cMapWidth)] != cWall	// Overlapping connection,
	|| (X >= cMapWidth || Y >= cMapHeight))	// Out of bounds
		return false;

	tU8 FreePaths = 0;

	if (gMapData[(X+1)+((Y)*cMapWidth)] == cFloor)		{*XDir = -1; *YDir = 0;  ++FreePaths;}
	if (gMapData[(X-1)+((Y)*cMapWidth)] == cFloor)		{*XDir = 1;  *YDir = 0;  ++FreePaths;}
	if (gMapData[(X)+((Y+1)*cMapWidth)] == cFloor)		{*XDir = 0;  *YDir = -1; ++FreePaths;}
	if (gMapData[(X)+((Y-1)*cMapWidth)] == cFloor)		{*XDir = 0;  *YDir = 1;  ++FreePaths;}

	if (FreePaths != 1) return false;		// Intersecting paths at connection point!
	return true;							// Otherwise, found a usable connection point!

}


bool genCanAdd(const tU16 X, const tU16 Y, const tU8 W, const tU8 H) {
	/// Returns true if a room/corridor can be added here.

	if (X+W >= cMapWidth || Y+H >= cMapHeight)	// No out of bounds
		return false;

	static tU8 XP, YP;

	for (XP = 0; XP < W; ++XP) for (YP = 0; YP < H; ++YP)
		if (gMapData[X+XP+((Y+YP)*cMapWidth)] != cWall)
			return false;					// Intersects another room

	return true;

}



void genDungeonMaze() {

	tU16
		ConX, ConY,											// Connection point (where it will connect to another room)
		StartX, StartY,										// The starting position (where the player starts)
		EndX, EndY,											// The ending position (the maze exit)
		XPos, YPos;											// Position	of the room/corridor

	tU8	XSize, YSize;										// Size of the room/corridor (width/height)
	tS8	XDir, YDir;											// Room extending direction (from connection point) +/-


	// The Seed is Level based (so the levels are the same every time visited)
	// and because there is nearly infinite Levels, the game will never end!

	srand((tU32)(gMapLevel+cMapSeed));						// More levels than seconds in 100 years (unsigned for safe overflows).


	memset(gMapData, cWall, cMapWidth*cMapHeight);			// Fill (clear) the Map with walls...

	XSize = dRand(cRoomSize)+cRoomSize;						// Create the starting room...
	YSize = dRand(cRoomSize)+cRoomSize;
	EndX = StartX = dRand(cMapWidth-XSize-XSize)+XSize;
	EndY = StartY = dRand(cMapHeight-YSize-YSize)+YSize;

	genFillMap(StartX, StartY, XSize, YSize, cFloor);
	gMapData[StartX+dRand(XSize)+((StartY+dRand(YSize))*cMapWidth)] = cStart;


	tU32 Room = 0;
	bool EndCreated = false;								// Has the ending point been made yet?

	while (Room++ < cRoomNum) {

		XSize = dRand(cRoomSize)+cRoomSize;					// The width/height of this attempted room
		YSize = dRand(cRoomSize)+cRoomSize;

		do {

			ConX = dRand(cMapWidth-XSize)+XSize-1;			// Choose a random position (within the map)
			ConY = dRand(cMapHeight-YSize)+YSize-1;

		} while (!genConPoint(ConX, ConY, &XDir, &YDir));	// Sets the XDir/YDir



		if (!dRand(cHallChance)) {							// Make the room corridor shaped

			if (!dRand(2))	XSize = 1, YSize *= 2;			// Choose direction: Vert or Horz
			else			YSize = 1, XSize *= 2;

		}

		if (XDir == 0) {									// Get the room/corridor starting position

			XPos = ConX-(XSize/2);
			YPos = ((YDir > 0)?(ConY+YDir):(ConY-YSize));

		} else {

			YPos = ConY-(YSize/2);
			XPos = ((XDir > 0)?(ConX+XDir):(ConX-XSize));

		}


		// Add the room or corridor to the connection point (with/without a door)!
		if (genCanAdd(XPos-1, YPos-1, XSize+2, YSize+2)) {

			bool Door = (dRand(cDoorChance) == 0);

			gMapData[ConX+(ConY*cMapWidth)] = (Door)?(cDoor):(cFloor);	// Place the door
			genFillMap(XPos, YPos, XSize, YSize, cFloor);				// Make the room/corridor


			// In a room far away, without a door add an 'end' point (for demonstration)
			if (!Door && XSize > 1 && YSize > 1
			&& (dAbs(XPos-StartX) >= dAbs(XPos-EndX)
			|| dAbs(YPos-StartY) >= dAbs(YPos-EndY))) {

				EndX = XPos+dRand(XSize);					// Create the new best end point...
				EndY = YPos+dRand(YSize);

			}


		}

	}

	gMapData[EndX+(EndY*cMapWidth)] = cEnd;

}



void genMazeDraw() {			// System specific drawing...

	HBRUSH Colors[cTileNum];

	Colors[cFloor]	= CreateSolidBrush(RGB(255,255,255));	// White
	Colors[cWall]	= CreateSolidBrush(RGB(64,64,64));		// DkGray
	Colors[cDoor]	= CreateSolidBrush(RGB(0,255,0));		// Green

	Colors[cStart]	= CreateSolidBrush(RGB(0,0,255));		// Blue
	Colors[cEnd]	= CreateSolidBrush(RGB(255,0,255));		// Purple


	tU16 XP, YP, Obj;
	RECT dstRect;

	for (XP = 0; XP < cMapWidth; ++XP)
	for (YP = 0; YP < cMapHeight; ++YP) {

		dstRect.top		= (YP*cTileSize);
		dstRect.left	= (XP*cTileSize);
		dstRect.bottom	= (YP*cTileSize)+cTileSize;
		dstRect.right	= (XP*cTileSize)+cTileSize;

		FillRect(hWinDC, &dstRect, Colors[gMapData[XP+(YP*cMapWidth)]]);

	}

	for (Obj = 0; Obj < cTileNum; ++Obj)
		DeleteObject(Colors[Obj]);

}




// =======================================================================
// ====================== Win Main -------------------------------------------

// Windows Event Procedure Handler
LRESULT CALLBACK WndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) {

	static PAINTSTRUCT lPS;

	switch (Msg) {

		case WM_DESTROY:
			PostQuitMessage(0);
		break;

		case WM_KEYDOWN:

			// ESC to quit
			if (wParam == VK_ESCAPE)
				DestroyWindow(hWnd);

			// Press left/right to change level (unsigned integer overflow is defined behavior)
			gMapLevel += (wParam == VK_RIGHT)-(wParam == VK_LEFT);

			genDungeonMaze();
			genMazeDraw();

		break;

		case WM_PAINT:

			BeginPaint(hWnd, &lPS);
			genMazeDraw();
			EndPaint(hWnd, &lPS);

		break;

		default:
			return DefWindowProc(hWnd, Msg, wParam, lParam);

	}

	return 0;

}

int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR CmdLineV, int CmdLineC) {

	WNDCLASSEX	wClass;
	HWND		hWnd;
	MSG			Msg;

	// The Window class structure
	wClass.cbSize			= sizeof(WNDCLASSEX);
	wClass.style			= CS_HREDRAW | CS_VREDRAW;
	wClass.lpfnWndProc		= WndProc;
	wClass.cbClsExtra		= 0;
	wClass.cbWndExtra		= 0;
	wClass.hInstance		= hInst;
	wClass.hIcon			= NULL;
	wClass.hCursor			= NULL;
	wClass.hbrBackground	= NULL;
	wClass.lpszMenuName		= NULL;
	wClass.lpszClassName	= WIN_NAME;
	wClass.hIconSm			= NULL;

	// Register
	RegisterClassEx(&wClass);


	// Create the main Window:
	hWnd = CreateWindowEx(
			0, WIN_NAME, WIN_NAME,
			WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
			0, 0, (cMapWidth)*cTileSize, (cMapHeight)*cTileSize,
			NULL, NULL, hInst, NULL
		);

	hWinDC = GetDC(hWnd);
	ShowWindow(hWnd, SW_SHOW);      // Display window
	UpdateWindow(hWnd);             // Update window


	genDungeonMaze();
	genMazeDraw();

	while (GetMessage(&Msg, NULL, 0, 0)) {
		TranslateMessage(&Msg);
		DispatchMessage(&Msg);
	}

	return Msg.wParam;

}
