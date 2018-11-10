#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <winsock2.h>



#include "pathfinder.h"



double
get_time_millis() {
	struct timeval tv;

#ifdef _WIN32

#define EPOCH_BIAS (116444736000000000)
#define UNITS_PER_SEC (10000000)
#define USEC_PER_SEC (1000000)
#define UNITS_PER_USEC (10)

	union {
		FILETIME ft_ft;
		uint64_t ft_64;
	} ft;

	GetSystemTimeAsFileTime(&ft.ft_ft);

	if ( ft.ft_64 < EPOCH_BIAS ) {
		return -1;
	}
	ft.ft_64 -= EPOCH_BIAS;
	tv.tv_sec = (long)( ft.ft_64 / UNITS_PER_SEC );
	tv.tv_usec = (long)( ( ft.ft_64 / UNITS_PER_USEC ) % USEC_PER_SEC );
#else
	gettimeofday(&tv, NULL);
#endif

	return (double)tv.tv_sec * 1000 + (double)tv.tv_usec / 1000;
}


void
find_result(void* ud, int x, int z) {
	//printf("%d,%d\n", x, z);
}

char*
load_tile(const char* file, int* width, int* heigh) {
	FILE* fp = fopen(file, "rb");
	int version;
	int gridSize;

	fread(&version, 1, sizeof( int ), fp);
	fread(width, 1, sizeof( int ), fp);
	fread(heigh, 1, sizeof( int ), fp);
	fread(&gridSize, 1, sizeof( int ), fp);

	int size = (*width) * (*heigh);
	char* data = (uint8_t*)malloc(size);
	memset(data, 0, size);
	fread(data, 1, size, fp);
	fclose(fp);

	return data;
}

int main(int argc,const char* argv[]) {
	FILE* fp = fopen(argv[1], "rb");

	int width;
	int heigh;
	char* data = load_tile(argv[1], &width, &heigh);

	struct pathfinder* finder = finder_create(width, heigh, (char*)data);

	free(data);

	double ti0 = get_time_millis();

	for ( int i = 1; i <= 1000;i++ )
	{
		finder_find(finder, 84, 183, 469, 183, 1, find_result, NULL, NULL, NULL, 50.0f);
	}
	double ti1 = get_time_millis();
	printf("find:%f\n", ti1 - ti0);

	int count = 1024 * 102;
	for ( int i = 1; i <= count; i++ )
	{
		int rx, rz;
		finder_raycast(finder, 90, 311, 333, 298, 1, &rx, &rz, NULL, NULL);
	}
	double ti2 = get_time_millis();
	printf("raycast:%f\n", ti2 - ti1);

	finder_release(finder);
	return 0;
}