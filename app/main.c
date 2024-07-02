#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>

#include "../driver/src/mydev.h"

int main(int argc, char **argv)
{
	int fd  = open("/dev/" CLASS_NAME, O_RDWR);
	if (fd == -1) {
		printf("error opening device /dev/" CLASS_NAME "\n");
		return -1;
	}

	printf("device opened success.\n");
	if (argc < 2) {
		return 1;
	}
	uint32_t val = strtol(argv[1], NULL, 16);
	write(fd, &val, sizeof(val));
	close(fd);
	return 0;
}
