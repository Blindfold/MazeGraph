#ifndef __ELLERSMAZER_H
#define __ELLERSMAZER_H
#include <Arduino.h>

#define MAX_ELLERS_MAZE_SIZE 30

typedef byte cell;
#define RIGHT_WALL_MASK  B1000
#define BOTTOM_WALL_MASK B0100
#define CELL_GROUP_MASK  B0011
#define RIGHT_4BITS B00001111
#define LEFT_4BITS B11110000

class EllersMazer
{
private:
	byte * mazeData;
	byte rowSize;
	byte width, height;
	byte availableGroupsStack[MAX_ELLERS_MAZE_SIZE];
	byte availableGroupsStackIndex = 0;

	boolean isThereWall(const byte col, const byte row, byte wallMask) const;
	cell getCell(const byte col, const byte row) const;
	void setCell(const byte col, const byte row, const cell data);
	void setCellGroup(const byte col, const byte row, const byte group);
	byte getCellGroup(const byte col, const byte row) const;
	void setWall(const byte col, const byte row, const byte wallMask);
	void clearWall(const byte col, const byte row, const byte wallMask);

	void pushAvailableGroup(const byte group);
	byte popAvailableGroup();
	void processRow(const byte row);
	void placeRightWalls(const byte row);
	void placeRightWallsLast(const byte row);
public:
	byte widthToHeightFactor = 50; // between 1 and 99. Setting this >50 will make coridors spread more horizontally
	EllersMazer(byte width, byte height);
	~EllersMazer();
#ifdef _DEBUG
	void debugPrintMaze();
#endif
	void build();
};

#endif // __ELLERSMAZER_H