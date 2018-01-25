#include "EllersMazer.h"

boolean EllersMazer::isThereWall(const byte col, const byte row, byte wallMask) const
{
	cell data = getCell(col, row);
	return (data & wallMask) == wallMask;
}

cell EllersMazer::getCell(byte col, byte row) const
{
	cell data = mazeData[row*this->rowSize + col / 2];
	if (col % 2 == 0) {
		data = data >> 4;
	}
	return data & RIGHT_4BITS;
}

void EllersMazer::setCell(byte col, byte row, cell newData) {
	cell packedData = mazeData[row*this->rowSize + col / 2];
	if (col % 2 == 0) {
		packedData = ((newData << 4) & LEFT_4BITS) | (packedData & RIGHT_4BITS);
	}
	else {
		packedData = (packedData & LEFT_4BITS) | (newData & RIGHT_4BITS);
	}
	mazeData[row*this->rowSize + col / 2] = packedData;
}

void EllersMazer::setCellGroup(byte col, byte row, byte group)
{
	cell data = getCell(col, row);
	data = (data & (~CELL_GROUP_MASK)) + group & CELL_GROUP_MASK;
	setCell(col, row, data);
}

byte EllersMazer::getCellGroup(const byte col, const byte row) const
{
	cell data = getCell(col, row);
	data &= CELL_GROUP_MASK;
	return data;
}

void EllersMazer::setWall(const byte col, const byte row, const byte wallMask)
{
	cell data = getCell(col, row);
	data |= wallMask;
	setCell(col, row, data);
}

void EllersMazer::clearWall(const byte col, const byte row, const byte wallMask)
{
	cell data = getCell(col, row);
	data &= ~wallMask;
	setCell(col, row, data);
}

void EllersMazer::pushAvailableGroup(const byte group)
{
	// WARNING: Range check disabled explicitly
	availableGroupsStack[availableGroupsStackIndex++] = group;
}

byte EllersMazer::popAvailableGroup()
{
	// WARNING: Range check disabled explicitly
	return availableGroupsStack[availableGroupsStackIndex--];
}

EllersMazer::EllersMazer(byte width, byte height)
{
	if (width > MAX_ELLERS_MAZE_SIZE)
		this->width = MAX_ELLERS_MAZE_SIZE;
	else
		this->width = width;

	if (height > MAX_ELLERS_MAZE_SIZE)
		this->height = MAX_ELLERS_MAZE_SIZE;
	else
		this->height = height;
	// we need only 4 bits to store the cell data. So we pack 2 cells in 1 byte
	this->rowSize = this->width / 2 + this->width % 2;
	mazeData = new cell[this->height * this->rowSize];
	memset(mazeData, 0, this->height * this->rowSize);
}


EllersMazer::~EllersMazer()
{
	delete[] mazeData;
}

#ifdef _DEBUG
void EllersMazer::debugPrintMaze() {
	Serial.println();
	Serial.print(" ");
	for (byte i = 0; i < width; i++) {
		Serial.print("__ ");
	}
	Serial.println();
	for (byte row = 0; row < height; row++) {
		for (byte col = 0; col < width; col++) {
			if (col == 0) {
				Serial.print("|");
			}
			if (isThereWall(col, row, BOTTOM_WALL_MASK)
				|| row == height - 1) {
				//				Serial.print(getCellGroup(col, row));
				Serial.print("__");
			}
			else {
				//				Serial.print(getCellGroup(col, row));
				Serial.print("  ");
			};
			if (isThereWall(col, row, RIGHT_WALL_MASK)
				|| col == width - 1) {
				Serial.print("|");
			}
			else {
				Serial.print(" ");
			};
		}
		Serial.println();
	}
}
#endif

void EllersMazer::build()
{
	randomSeed(millis() + analogRead(0));
	for (byte row = 0; row < height; row++) {
		processRow(row);
		if (row != height - 1) {
			memcpy(&mazeData[(row + 1)*rowSize], &mazeData[row*rowSize], rowSize);
		}
	}
#ifdef _DEBUG
	debugPrintMaze();
#endif
}

void EllersMazer::processRow(byte row)
{
	if (row != height - 1) {
		//remove the right walls
		for (byte col = 0; col < width - 1; col++) {
			cell data = getCell(col, row);
			data &= ~RIGHT_WALL_MASK;
			//remove bottom walls and clear set of cells having them
			if ((data & BOTTOM_WALL_MASK) == BOTTOM_WALL_MASK) {
				data = 0;
			}
			setCell(col, row, data);
		}
	}


	byte lastGroup = 1;
#define GROUP_INVERSE_MASK CELL_GROUP_MASK
	// Join any cells not members of any set to their own unique set
	boolean lastCellWasEmpty = false;
	boolean inverseGroup = false;
	for (byte col = 0; col < width; col++)
	{
		byte currentGroup = getCellGroup(col, row);
		if (currentGroup == 0) {
			lastGroup ^= GROUP_INVERSE_MASK; // 1 <-> 2
			setCellGroup(col, row, lastGroup);
			lastCellWasEmpty = true;
			inverseGroup = false;
		}
		else {
			if (lastCellWasEmpty && lastGroup == currentGroup) {
				inverseGroup = true;
			}
			if (inverseGroup) {
				currentGroup ^= GROUP_INVERSE_MASK; // 1 <-> 2
				setCellGroup(col, row, currentGroup);
			}
			lastCellWasEmpty = false;
			lastGroup = currentGroup;
		}
	}
	if (row != height - 1) {
		placeRightWalls(row);
	}
	else {
		placeRightWallsLast(row);
	}
	// Create bottom - walls, moving from left to right
	byte prevGroup = 0;
	byte currentGroup = 0;
	byte cellsWOBottom = 0;
	for (byte col = 0; col < width; col++)
	{
		currentGroup = getCellGroup(col, row);
		if (currentGroup != prevGroup) {
			// new cells set, reset counter of cells wihtout bottom wall
			cellsWOBottom = 1;
		}
		else {
			cellsWOBottom++;
		}
		//Randomly decide to add a wall or not.Make sure that each set has at least one cell without a bottom - wall(This prevents isolations)
		// always add bottom walls for the last row
		boolean placeBottomWall = false;
		if (row == height - 1) {
			placeBottomWall = true;
		}
		else {
			byte rnd = random(0, 100);
			if (rnd <= widthToHeightFactor
				|| row == height - 1)
			{
				placeBottomWall = true;

				// check if it is the only cell in it's group
				byte nextGroup = 0;
				if (col + 1 < width) {
					nextGroup = getCellGroup(col + 1, row);
				}
				if (nextGroup != currentGroup &&
					cellsWOBottom <= 1)
				{
					placeBottomWall = false;
				}
			}
			prevGroup = currentGroup;
		}

		if (placeBottomWall) {
			setWall(col, row, BOTTOM_WALL_MASK);
			cellsWOBottom--;
		}
	}

}

void EllersMazer::placeRightWalls(const byte row)
{
	// Create right-walls, moving from left to right:
	for (byte col = 0; col < width - 1; col++)
	{
		byte rnd = random(0, 100);
		// Randomly decide to add a wall or not
		// If the current cell and the cell to the right are members of the same set, always create a wall between them. (This prevents loops)
		byte currentGroup = getCellGroup(col, row);
		byte nextGroup = getCellGroup(col + 1, row);
		if (rnd > widthToHeightFactor ||
			currentGroup == nextGroup)
		{
			setWall(col, row, RIGHT_WALL_MASK);
		}
		else {
			// If you decide not to add a wall, union the sets to which the current cell and the cell to the right are members.
			for (byte i = col + 1; i < width; i++) {
				byte group = getCellGroup(i, row);
				if (currentGroup == group) {
					col += 1;
				}
				else {
					currentGroup = 0;
				}
				group = group ^ GROUP_INVERSE_MASK;
				setCellGroup(i, row, group);
			}
		}
	}
}

void EllersMazer::placeRightWallsLast(const byte row)
{
	for (byte col = 0; col < width - 1; col++)
	{
		byte currentGroup = getCellGroup(col, row);
		byte nextGroup = getCellGroup(col + 1, row);
		if (currentGroup != nextGroup) {
			clearWall(col, row, RIGHT_WALL_MASK);
			while (col < width - 1 && nextGroup != currentGroup)
			{
				col++;
				setCellGroup(col, row, currentGroup);
				if (col < width - 1) {
					nextGroup = getCellGroup(col, row);
				}
			}
		}
	}
}
