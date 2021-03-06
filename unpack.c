/*
 * unpack.c
 *
 * Copyright 2012 Emilio López <turl@tuxfamily.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 *
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <endian.h>

#include "bootheader.h"

#define ERROR(...) do { fprintf(stderr, __VA_ARGS__); return 1; } while(0)

int main(int argc, char *argv[])
{
	char *origin;
	char *bzImage;
	char *ramdisk;
	char *second;
	FILE *forigin;
	FILE *fbzImage;
	FILE *framdisk;
	FILE *fsecond;
	uint32_t bzImageLen;
	uint32_t ramdiskLen;
	uint32_t secondLen;
	uint32_t missing;
	char buf[BUFSIZ];
	size_t size;

	if (argc != 5)
		ERROR("Usage: %s <image to unpack> <bzImage out> <ramdisk out> <second out>\n", argv[0]);

	origin = argv[1];
	bzImage = argv[2];
	ramdisk = argv[3];
	second = argv[4];
	forigin = fopen(origin, "r");
	fbzImage = fopen(bzImage, "w");
	framdisk = fopen(ramdisk, "w");
	fsecond = fopen(second, "w");
	if (!forigin || !bzImage || !framdisk || !fsecond)
		ERROR("ERROR: failed to open origin or output images\n");

	/* Read bzImage length from the image to unpack */
	if (fseek(forigin, 8, SEEK_SET) == -1) //address can be found after ANDROID! magic aka 8 Bytes
		ERROR("ERROR: failed to seek on image\n");

	if (fread(&bzImageLen, sizeof(uint32_t), 1, forigin) != 1)
		ERROR("ERROR: failed to read bzImage length\n");
	else
		bzImageLen = le32toh(bzImageLen);
	
	fprintf(stderr, "bzImagelen: %d\n", bzImageLen);

	/* Read ramdisk length from the image to unpack */
	if (fseek(forigin, 2*8, SEEK_SET) == -1)
		ERROR("ERROR: failed to seek on image\n");

	if (fread(&ramdiskLen, sizeof(uint32_t), 1, forigin) != 1)
		ERROR("ERROR: failed to read ramdisk length\n");
	else
		ramdiskLen = le32toh(ramdiskLen);

	fprintf(stderr, "ramdisklen: %d\n", ramdiskLen);

	//patch for 2nd stage
	if (fseek(forigin, 3*8, SEEK_SET) == -1)
		ERROR("ERROR: failed to seek on image\n");
	
	if(fread(&secondLen, sizeof(uint32_t), 1, forigin) != 1)
		ERROR("ERROR: failed to read second stage length\n");
	else
		secondLen = le32toh(secondLen);
		
	fprintf(stderr, "secondLen: %d\n", secondLen);

	/* Copy bzImage */
	if (fseek(forigin, sizeof(struct bootheader), SEEK_SET) == -1)
		ERROR("ERROR: failed to seek when copying bzImage\n");

	missing = bzImageLen;
	while (missing > 0) {
		size = fread(buf, 1, ((missing > BUFSIZ) ? BUFSIZ : missing), forigin);
		if (size != 0) {
			fwrite(buf, 1, size, fbzImage);
			missing -= size;
		}
	}

	/* Copy ramdisk */
	if (fseek(forigin, (sizeof(struct bootheader)+bzImageLen), SEEK_SET) == -1)
		ERROR("ERROR: failed to seek when copying ramdisk\n");

	missing = ramdiskLen;
	while (missing > 0) {
		size = fread(buf, 1, ((missing > BUFSIZ) ? BUFSIZ : missing), forigin);
		if (size != 0) {
			fwrite(buf, 1, size, framdisk);
			missing -= size;
		}
	}

	//secondstage patch
	if (fseek(forigin, (sizeof(struct bootheader)+bzImageLen+ramdiskLen), SEEK_SET) == -1)
		ERROR("ERROR: failed to seek when copying second stage\n");
	
	missing = secondLen;
	while (missing > 0) {
		size = fread(buf, 1, ((missing > BUFSIZ) ? BUFSIZ : missing), forigin);
		if (size != 0) {
			fwrite(buf,1,size, fsecond);
			missing -= size;
		}
	}

	return 0;
}
