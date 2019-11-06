#pragma once

#include "Sail/entities/systems/BaseComponentSystem.h"

class Scene;
class Model;

struct Rect {
	int posx;
	int posy;
	int sizex;
	int sizey;
	int doors = 0;
};
struct Clutter {
	float posx;
	float posy;
	float height;
	float rot;
	int size;
};


enum TileModel {
	ROOM_FLOOR,
	ROOM_WALL,
	ROOM_DOOR,
	ROOM_CEILING,
	ROOM_CORNER,
	CORRIDOR_FLOOR,
	CORRIDOR_WALL,
	CORRIDOR_DOOR,
	CORRIDOR_CEILING,
	CORRIDOR_CORNER,
	NUMBOFMODELS
};

enum ClutterModel {
	CLUTTER_LO,
	CLUTTER_MO,
	CLUTTER_SO,
	SAFTBLANDARE,
	NUMBOFCLUTTER
};

enum Direction {
	NONE = 0,
	UP = 1,
	RIGHT = 2,
	DOWN = 4,
	LEFT = 8
};

class LevelSystem final: public BaseComponentSystem {
public:
	LevelSystem();
	~LevelSystem();

	void generateMap();
	void createWorld(const std::vector<Model*>& tileModels, Model* bb);
	void destroyWorld();
	void addClutterModel(const std::vector<Model*>& clutterModels, Model* bb);

	glm::vec3 getSpawnPoint();

	void stop();
	const int getAreaType(float posX, float posY);

	int xsize;
	int ysize;
	int*** tileArr; //0 is tileID, 1 is typeID, 2 is door
	float hallwayThreshold = 0.3f; // percentage of level that can be corridors
	int minSplitSize = 5; //minimum size for splitting chunks
	int minRoomSize = 1; //minimum side of a room
	int roomMaxSize = 36;//maximum area of a room
	int roomSplitStop = 25;//percentage to stop a room from being split into smaller ones
	int doorModifier = 15;//percentage to spawn a door
	int clutterModifier = 85;//percentage to add clutter
	int seed;//seed for generation

	int totalArea;
	int numberOfRooms = 1;
	int tileSize;
	float tileHeight = 0.8f;
	int tileOffset = 0;









	std::vector<glm::vec3> spawnPoints;
private:
	std::queue<Rect> chunks;
	std::queue<Rect> blocks;
	std::queue<Rect> hallways;
	std::queue<Rect> rooms;
	std::queue<Rect> matched;
	std::queue<Clutter>largeClutter;
	std::queue<Clutter>mediumClutter;
	std::queue<Clutter>smallClutter;
	int randomizeTileId(std::vector<int>* tiles);
	void findPossibleTiles(std::vector<int>* mapPointer,int posx, int posy);
	void splitChunk();
	void splitBlock();
	void matchRoom();
	int checkBorder(Rect rekt);
	bool splitDirection(bool ns);
	void addSpawnPoints();
	void addDoors();
	void addMapModel(Direction dir, int typeID, int doors, const std::vector<Model*>& tileModels, float tileSize,float tileHeight, int tileOffset, int i, int j, Model* bb);
	void addTile(int tileId, int typeId, int doors,const std::vector<Model*>& tileModels, float tileSize,float tileHeight, float tileOffset, int i, int j, Model* bb);
	bool hasDoor(Direction dir, int doors);
	void generateClutter();
};