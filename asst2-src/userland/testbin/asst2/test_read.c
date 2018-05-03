void test_read(void);

void test_read(void) {
	printf("TESTING READ...\n\n");

	printf("read from a file opened for reading only\n");
	int fd = open("file.txt", O_RDONLY);
	if (fd >= 0) {
		char buf[101];
		int bytes = read(fd, buf, 99);
		buf[bytes] = '\0';
		printf("bytes read - should be 27: %d\n", bytes);
		printf("check if buffer is correct - "
		       "should be 'hello world\\nthis is a test\\n': '%s'\n", buf);

		printf("check that subsequent read returns nothing\n");
		bytes = read(fd, buf, 99);
		printf("bytes read - should be 0: %d\n", bytes);
		printf("check buffer is still in tact - "
		       "should be 'hello world\\nthis is a test\\n': '%s'\n", buf);

		int r = close(fd);
		if (r) printf("error (should not print!): %s\n\n", strerror(errno));
	} else {
		printf("error (should not print!): %s\n\n", strerror(errno));
	}

	printf("read from a file opened for writing only\n");
	fd = open("t1.txt", O_WRONLY);
	if (fd >= 0) {
		char buf[11];
		read(fd, buf, 9);
		printf("error: %s\n\n", strerror(errno));
		int r = close(fd);
		if (r) printf("error (should not print!): %s\n\n", strerror(errno));
	} else {
		printf("error (should not print!): %s\n\n", strerror(errno));
	}

	printf("read from a file opened for reading and writing\n");
	fd = open("file.txt", O_RDWR);
	if (fd >= 0) {
		char buf[101];
		int bytes = read(fd, buf, 99);
		buf[bytes] = '\0';
		printf("bytes read - should be 27: %d\n", bytes);
		printf("check if buffer is correct - "
		       "should be 'hello world\\nthis is a test\\n': '%s'\n", buf);

		printf("check that subsequent read returns nothing\n");
		bytes = read(fd, buf, 99);
		printf("bytes read - should be 0: %d\n", bytes);
		printf("check buffer is still in tact - "
		       "should be 'hello world\\nthis is a test\\n': '%s'\n", buf);

		int r = close(fd);
		if (r) printf("error (should not print!): %s\n\n", strerror(errno));
	} else {
		printf("error (should not print!): %s\n\n", strerror(errno));
	}

	printf("opening two instances of the same file for reading\n");
	int fd1 = open("file.txt", O_RDONLY);
	int fd2 = open("file.txt", O_RDONLY);
	if (fd1 >= 0 && fd2 >= 0) {
		char buf1[101];
		int bytes = read(fd1, buf1, 99);
		buf1[bytes] = '\0';
		printf("bytes read - should be 27: %d\n", bytes);
		printf("check if buffer is correct - "
		       "should be 'hello world\\nthis is a test\\n': '%s'\n", buf1);

		char buf2[101];
		bytes = read(fd2, buf2, 99);
		buf2[bytes] = '\0';
		printf("bytes read - should be 27: %d\n", bytes);
		printf("check if buffer is correct - "
		       "should be 'hello world\\nthis is a test\\n': '%s'\n", buf2);

		int r = close(fd1);
		if (r) printf("error (should not print!): %s\n\n", strerror(errno));
		r = close(fd2);
		if (r) printf("error (should not print!): %s\n\n", strerror(errno));
	} else {
		printf("error (should not print!): %s\n\n", strerror(errno));
	}

	printf("read into a null buffer from a file opened for reading\n");
	fd = open("file.txt", O_RDONLY);
	if (fd >= 0) {
		read(fd, NULL, 10);
		printf("error: %s\n\n", strerror(errno));
		int r = close(fd);
		if (r) printf("error (should not print!): %s\n\n", strerror(errno));

		printf("check that the file did not change\n");
		fd = open("file.txt", O_RDONLY);
		char buf[101];
		int bytes = read(fd, buf, 99);
		buf[bytes] = '\0';
		printf("bytes read - should be 27: %d\n", bytes);
		printf("check if buffer is correct - "
		       "should be 'hello world\\nthis is a test\\n': '%s'\n", buf);

		r = close(fd);
		if (r) printf("error (should not print!): %s\n\n", strerror(errno));
	} else {
		printf("error (should not print!): %s\n\n", strerror(errno));
	}

	printf("read 0 bytes into a buffer from a file opened for reading\n");
	fd = open("file.txt", O_RDONLY);
	if (fd >= 0) {
		char buf[101];
		int bytes = read(fd, buf, 0);
		printf("bytes read - should be 0: %d\n", bytes);
		int r = close(fd);
		if (r) printf("error (should not print!): %s\n\n", strerror(errno));

		printf("check that the file did not change\n");
		fd = open("file.txt", O_RDONLY);
		char buf2[101];
		bytes = read(fd, buf2, 99);
		buf2[bytes] = '\0';
		printf("bytes read - should be 27: %d\n", bytes);
		printf("check if buffer is correct - "
		       "should be 'hello world\\nthis is a test\\n': '%s'\n", buf2);

		r = close(fd);
		if (r) printf("error (should not print!): %s\n\n", strerror(errno));
	} else {
		printf("error (should not print!): %s\n\n", strerror(errno));
	}

	printf("try to read from an unopened fd\n");
	char buf[11];
	read(10, buf, 9);
	printf("error: %s\n\n", strerror(errno));
}
