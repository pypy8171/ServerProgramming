#pragma once

#define SERVER_PORT		9000

#define MAX_USER	1
#define MAX_BUFFER  512

#define START_X 300
#define START_Y 50

#define BOARD_SIZE 600
#define BOARD_GAP 5
#define BOARD_GAPNUM 8

#define WORLD_HEIGHT BOARD_SIZE
#define WORLD_WIDTH BOARD_SIZE

#define CS_NONE		0
#define CS_UP		1
#define CS_DOWN		2
#define CS_LEFT		3
#define CS_RIGHT	4

#define SC_POS		2

struct sc_packet_pos
{
	char size;
	char type;
	//char id;
	char x;
	char y;
};

#pragma pack(push ,1)
struct cs_packet_keyinput
{
	char size;
	char type;
};

struct cs_packet_up
{
	char	size;
	char	type;
};

struct cs_packet_down
{
	char	size;
	char	type;
};

struct cs_packet_left
{
	char	size;
	char	type;
};

struct cs_packet_right
{
	char	size;
	char	type;
};

#pragma pack (pop)