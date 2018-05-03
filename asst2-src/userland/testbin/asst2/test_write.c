void test_write(void);

void test_write(void) {
	printf("TESTING WRITE...\n\n");

	printf("write to a file opened for writing only\n");
	int fd = open("t1.txt", O_WRONLY);
	if (fd >= 0) {
		char buf[] = "hello world\n";
		int bytes = write(fd, buf, 12);
		assert(bytes == 12);
		printf("bytes written - should be 12: %d\n", bytes);
		printf("check buffer is still in tact - "
		       "should be 'hello world\\n': '%s'\n", buf);

		int r = close(fd);
		if (r) printf("error (should not print!): %s\n\n", strerror(errno));

		printf("check that the file was actually written to\n");
		fd = open("t1.txt", O_RDONLY);
		char buf2[51];
		bytes = read(fd, buf2, 49);
		buf2[bytes] = '\0';
		assert(bytes == 12);
		printf("bytes read - should be 12: %d\n", bytes);
		printf("check if buffer is correct - "
		       "should be 'hello world\\n': '%s'\n", buf2);

		r = close(fd);
		if (r) printf("error (should not print!): %s\n\n", strerror(errno));
	} else {
		printf("error (should not print!): %s\n\n", strerror(errno));
	}

	printf("write to a file opened for reading only\n");
	fd = open("t1.txt", O_RDONLY);
	if (fd >= 0) {
		char buf[11];
		write(fd, buf, 9);
		printf("error: %s\n\n", strerror(errno));
		int r = close(fd);
		if (r) printf("error (should not print!): %s\n\n", strerror(errno));
	} else {
		printf("error (should not print!): %s\n\n", strerror(errno));
	}

	printf("write to a file opened for reading and writing\n");
	fd = open("t1.txt", O_RDWR);
	if (fd >= 0) {
		char buf[] = "this is a test\n";
		int bytes = write(fd, buf, 15);
		assert(bytes == 15);
		printf("bytes written - should be 15: %d\n", bytes);
		printf("check buffer is still in tact - "
		       "should be 'this is a test': '%s'\n", buf);

		int r = close(fd);
		if (r) printf("error (should not print!): %s\n\n", strerror(errno));

		printf("check that the file was overwritten from earlier write\n");
		fd = open("t1.txt", O_RDONLY);
		char buf2[51];
		bytes = read(fd, buf2, 49);
		buf2[bytes] = '\0';
		assert(bytes == 15);
		printf("bytes read - should be 15: %d\n", bytes);
		printf("check if buffer is correct - "
		       "should be 'this is a test\\n': '%s'\n", buf2);

		r = close(fd);
		if (r) printf("error (should not print!): %s\n\n", strerror(errno));
	} else {
		printf("error (should not print!): %s\n\n", strerror(errno));
	}

	printf("write to a file opened for writing with O_TRUNC\n");
	fd = open("t1.txt", O_WRONLY | O_TRUNC);
	if (fd >= 0) {
		char buf[] = "blablabla\n";
		int bytes = write(fd, buf, 10);
		assert(bytes == 10);
		printf("bytes written - should be 10: %d\n", bytes);
		printf("check buffer is still in tact - "
		       "should be 'blablabla\\n': '%s'\n", buf);

		int r = close(fd);
		if (r) printf("error (should not print!): %s\n\n", strerror(errno));

		printf("check that the file was overwritten from earlier write\n");
		fd = open("t1.txt", O_RDONLY);
		char buf2[51];
		bytes = read(fd, buf2, 49);
		buf2[bytes] = '\0';
		assert(bytes == 10);
		printf("bytes read - should be 10: %d\n", bytes);
		printf("check if buffer is correct - "
		       "should be 'blablabla\\n': '%s'\n", buf2);

		r = close(fd);
		if (r) printf("error (should not print!): %s\n\n", strerror(errno));
	} else {
		printf("error (should not print!): %s\n\n", strerror(errno));
	}

	printf("write to a file opened for writing with O_APPEND\n");
	fd = open("t1.txt", O_WRONLY | O_APPEND);
	if (fd >= 0) {
		char buf[] = "should be appended\n";
		int bytes = write(fd, buf, 19);
		assert(bytes == 19);
		printf("bytes written - should be 19: %d\n", bytes);
		printf("check buffer is still in tact - "
		       "should be 'should be appended\\n': '%s'\n", buf);

		int r = close(fd);
		if (r) printf("error (should not print!): %s\n\n", strerror(errno));

		printf("check that the file was appended to\n");
		fd = open("t1.txt", O_RDONLY);
		char buf2[51];
		bytes = read(fd, buf2, 49);
		buf2[bytes] = '\0';
		assert(bytes == 29);
		printf("bytes read - should be 29: %d\n", bytes);
		printf("check if buffer is correct - "
		       "should be 'blablabla\\nshould be appended\\n': '%s'\n", buf2);

		r = close(fd);
		if (r) printf("error (should not print!): %s\n\n", strerror(errno));
	} else {
		printf("error (should not print!): %s\n\n", strerror(errno));
	}

	printf("write a null buffer to a file opened for writing\n");
	fd = open("t1.txt", O_WRONLY);
	if (fd >= 0) {
		write(fd, NULL, 10);
		printf("error: %s\n\n", strerror(errno));
		int r = close(fd);
		if (r) printf("error (should not print!): %s\n\n", strerror(errno));

		printf("check that the file did not change\n");
		fd = open("t1.txt", O_RDONLY);
		char buf[51];
		int bytes = read(fd, buf, 49);
		buf[bytes] = '\0';
		assert(bytes == 29);
		printf("bytes read - should be 29: %d\n", bytes);
		printf("check if buffer is correct - "
		       "should be 'blablabla\\nshould be appended\\n': '%s'\n", buf);

		r = close(fd);
		if (r) printf("error (should not print!): %s\n\n", strerror(errno));
	} else {
		printf("error (should not print!): %s\n\n", strerror(errno));
	}

	printf("write 0 bytes from a buffer to a file opened for writing\n");
	fd = open("t1.txt", O_WRONLY);
	if (fd >= 0) {
		char buf[101];
		int bytes = write(fd, buf, 0);
		assert(bytes == 0);
		printf("bytes written - should be 0: %d\n", bytes);
		int r = close(fd);
		if (r) printf("error (should not print!): %s\n\n", strerror(errno));

		printf("check that the file did not change\n");
		fd = open("t1.txt", O_RDONLY);
		char buf2[51];
		bytes = read(fd, buf2, 49);
		buf2[bytes] = '\0';
		assert(bytes == 29);
		printf("bytes read - should be 29: %d\n", bytes);
		printf("check if buffer is correct - "
		       "should be 'blablabla\\nshould be appended\\n': '%s'\n", buf2);

		r = close(fd);
		if (r) printf("error (should not print!): %s\n\n", strerror(errno));
	} else {
		printf("error (should not print!): %s\n\n", strerror(errno));
	}

	printf("write to a file and then read from it while in RDWR mode\n");
	fd = open("t1.txt", O_RDWR);
	if (fd >= 0) {
		char buf[] = "this new text will overwrite\nthe previous file\n";
		int bytes = write(fd, buf, 47);
		assert(bytes == 47);
		printf("bytes written - should be 47: %d\n", bytes);
		char buf2[101];
		bytes = read(fd, buf2, 99);
		assert(bytes == 0);
		printf("bytes read - should be 0: %d\n", bytes);
		int r = close(fd);
		if (r) printf("error (should not print!): %s\n\n", strerror(errno));
	} else {
		printf("error (should not print!): %s\n\n", strerror(errno));
	}

	printf("try to write to an unopened fd\n");
	char buf[11];
	write(10, buf, 9);
	printf("error: %s\n\n", strerror(errno));
}
