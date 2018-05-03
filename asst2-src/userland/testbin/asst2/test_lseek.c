void test_lseek(void);

void test_lseek(void) {
	printf("TESTING LSEEK...\n\n");

	printf("change seek position on a file opened for reading\n");
	int fd = open("seek.txt", O_RDONLY);
	if (fd >= 0) {
		char buf[101];
		int bytes = read(fd, buf, 4);
		buf[bytes] = '\0';
		printf("bytes read - should be 4: %d\n", bytes);
		printf("check if buffer is correct - "
		       "a\\nb\\n': '%s'\n", buf);

		printf("start reading from end of file\n");
		lseek(fd, 20, SEEK_SET);
		bytes = read(fd, buf, 4);
		printf("bytes read - should be 0: %d\n", bytes);

		printf("restart reading from start of file\n");
		lseek(fd, 0, SEEK_SET);
		char buf2[101];
		bytes = read(fd, buf2, 4);
		buf2[bytes] = '\0';
		printf("bytes read - should be 4: %d\n", bytes);
		printf("check if buffer is correct - "
		       "a\\nb\\n': '%s'\n", buf2);

		printf("start reading from middle of file\n");
		lseek(fd, 8, SEEK_SET);
		char buf3[101];
		bytes = read(fd, buf3, 4);
		buf3[bytes] = '\0';
		printf("bytes read - should be 4: %d\n", bytes);
		printf("check if buffer is correct - "
		       "e\\nf\\n': '%s'\n", buf3);

		printf("offset reading position using relative +ve offset\n");
		lseek(fd, 2, SEEK_CUR);
		char buf4[101];
		bytes = read(fd, buf4, 4);
		buf4[bytes] = '\0';
		printf("bytes read - should be 4: %d\n", bytes);
		printf("check if buffer is correct - "
		       "h\\ni\\n': '%s'\n", buf4);

		printf("offset reading position using relative -ve offset\n");
		lseek(fd, -4, SEEK_CUR);
		char buf5[101];
		bytes = read(fd, buf5, 4);
		buf5[bytes] = '\0';
		printf("bytes read - should be 4: %d\n", bytes);
		printf("check if buffer is correct - "
		       "h\\ni\\n': '%s'\n", buf5);

		printf("offset reading position by 0 using relative offset\n");
		lseek(fd, 0, SEEK_CUR);
		char buf6[101];
		bytes = read(fd, buf6, 4);
		buf6[bytes] = '\0';
		printf("bytes read - should be 2: %d\n", bytes);
		printf("check if buffer is correct - "
		       "j\\n': '%s'\n", buf6);

		printf("offset reading position to 0 bytes past end of file\n");
		lseek(fd, 0, SEEK_END);
		bytes = read(fd, buf6, 11);
		printf("bytes read - should be 0: %d\n", bytes);

		printf("offset reading position to 7 bytes past end of file\n");
		lseek(fd, 7, SEEK_END);
		bytes = read(fd, buf6, 3);
		printf("bytes read - should be 0: %d\n", bytes);

		printf("offset reading position by -8 from the end\n");
		lseek(fd, -8, SEEK_END);
		char buf7[101];
		bytes = read(fd, buf7, 4);
		buf7[bytes] = '\0';
		printf("bytes read - should be 4: %d\n", bytes);
		printf("check if buffer is correct - "
		       "g\\nh\\n': '%s'\n", buf7);

		printf("offset reading position by -20 from the end\n");
		lseek(fd, -20, SEEK_END);
		char buf8[101];
		bytes = read(fd, buf8, 4);
		buf8[bytes] = '\0';
		printf("bytes read - should be 4: %d\n", bytes);
		printf("check if buffer is correct - "
		       "a\\nb\\n': '%s'\n", buf8);

		printf("offset reading position by -3 from the start\n");
		lseek(fd, -3, SEEK_SET);
		printf("error: %s\n\n", strerror(errno));

		printf("offset reading position by -5 using relative offset\n");
		lseek(fd, -5, SEEK_CUR);
		printf("error: %s\n\n", strerror(errno));

		printf("offset reading position by -21 from the end\n");
		lseek(fd, -21, SEEK_END);
		printf("error: %s\n\n", strerror(errno));

		printf("try to lseek stdout\n");
		lseek(STDOUT_FILENO, -12, SEEK_SET);
		printf("error: %s\n\n", strerror(errno));

		printf("try to lseek stderr\n");
		lseek(STDERR_FILENO, 14, SEEK_CUR);
		printf("error: %s\n\n", strerror(errno));

		printf("try to lseek a directory\n");
		int dir = open("dir", O_RDONLY);
		lseek(dir, 0, SEEK_END);
		printf("error: %s\n\n", strerror(errno));
		close(dir);

		printf("try to seek with an invalid whence\n");
		lseek(fd, 38, 999);
		printf("error: %s\n\n", strerror(errno));

		printf("try to lseek an unopened fd\n");
		lseek(10, 99, SEEK_SET);
		printf("error: %s\n\n", strerror(errno));

		int r = close(fd);
		if (r) printf("error (should not print!): %s\n\n", strerror(errno));
	} else {
		printf("error (should not print!): %s\n\n", strerror(errno));
	}

	printf("change seek position on a file opened for writing\n");
	fd = open("t2.txt", O_WRONLY | O_CREAT);
	if (fd >= 0) {
		char buf[] = "hello world\nthis is a test\n";
		int bytes = write(fd, buf, 27);
		printf("bytes written - should be 27: %d\n", bytes);
		printf("check buffer is still in tact - "
		       "should be 'hello world\\nthis is a test\\n': '%s'\n", buf);

		printf("move to start of file before second write\n");
		lseek(fd, 0, SEEK_SET);
		char buf2[] = "blablabla\n";
		bytes = write(fd, buf2, 10);
		printf("bytes written - should be 10: %d\n", bytes);
		printf("check buffer is still in tact - "
		       "should be 'blablabla\\n': '%s'\n", buf2);

		int r = close(fd);
		if (r) printf("error (should not print!): %s\n\n", strerror(errno));

		fd = open("t2.txt", O_RDONLY);
		char buf3[101];
		bytes = read(fd, buf3, 99);
		buf3[bytes] = '\0';
		printf("bytes read - should be 27: %d\n", bytes);
		printf("check if buffer is correct - "
		       "should be 'blablabla\\nd\\nthis is a test\\n': '%s'\n", buf3);

		r = close(fd);
		if (r) printf("error (should not print!): %s\n\n", strerror(errno));
	} else {
		printf("error (should not print!): %s\n\n", strerror(errno));
	}

	printf("change seek position on a file opened for reading and writing\n");
	fd = open("t3.txt", O_RDWR | O_CREAT);
	if (fd >= 0) {
		char buf[] = "hello world\nthis is a test\n";
		int bytes = write(fd, buf, 27);
		printf("bytes written - should be 27: %d\n", bytes);
		printf("check buffer is still in tact - "
		       "should be 'hello world\\nthis is a test\\n': '%s'\n", buf);

		printf("check that which was just written by moving to start\n");
		lseek(fd, 0, SEEK_SET);
		char buf2[101];
		bytes = read(fd, buf2, 99);
		buf2[bytes] = '\0';
		printf("bytes read - should be 27: %d\n", bytes);
		printf("check if buffer is correct - "
		       "should be 'hello world\\nthis is a test\\n': '%s'\n", buf2);

		printf("start writing at some offset past end of file\n");
		lseek(fd, 50, SEEK_END);
		char buf3[] = "blablabla\n";
		bytes = write(fd, buf3, 10);
		printf("bytes written - should be 10: %d\n", bytes);
		printf("check buffer is still in tact - "
		       "should be 'blablabla\\n': '%s'\n", buf3);

		printf("check new file contents by moving to start\n");
		lseek(fd, -87, SEEK_END);
		char buf4[101];
		bytes = read(fd, buf4, 99);
		buf4[bytes] = '\0';
		printf("bytes read - should be 87: %d\n", bytes);
		printf("check if buffer is correct - "
		       "should be 'hello world\\nthis is a test\\nblablabla\\n': '\n");
		for (int i = 0; i < bytes; ++i) printf("%c", buf4[i]);
		printf("'\n");

		printf("overwrite middle of file \n");
		lseek(fd, 17, SEEK_SET);
		char buf5[] = "not";
		bytes = write(fd, buf5, 3);
		printf("bytes written - should be 3: %d\n", bytes);
		printf("check buffer is still in tact - "
		       "should be 'not': '%s'\n", buf5);

		printf("check new file contents by moving to start\n");
		lseek(fd, -20, SEEK_CUR);
		char buf6[101];
		bytes = read(fd, buf6, 99);
		buf6[bytes] = '\0';
		printf("bytes read - should be 87: %d\n", bytes);
		printf("check if buffer is correct - "
		       "should be 'hello world\\nthis nota test\\nblablabla\\n': '\n");
		for (int i = 0; i < bytes; ++i) printf("%c", buf6[i]);
		printf("'\n");

		int r = close(fd);
		if (r) printf("error (should not print!): %s\n\n", strerror(errno));
	} else {
		printf("error (should not print!): %s\n\n", strerror(errno));
	}
}
