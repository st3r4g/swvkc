	const int size = lseek(fd, 0, SEEK_END);
	lseek(fd, 0, SEEK_SET);
	unsigned char *buffer = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (buffer == MAP_FAILED) {
		perror("ERROR");
		return;
	}
	for (int i=5504*768-60; i<5504*768-30; i++)
		errlog("[%d] byte %i: %x", size, i, buffer[i]);
//	errlog("size %d", );
//	FILE* file = fopen("image.data", "w");
//	fwrite(buffer, size, 1, file);
//	fclose(file);
	munmap(buffer, size);
//	free(buffer);*/
	/*const int size = 1366*768*4;
	char *buffer = malloc(size);
	read(fd, buffer, 256);
	for (int i=0; i<256; i++)
		errlog("%d", buffer[i]);
	free(buffer);*/

