void test_lseek(void);

void test_lseek(void) {
	printf("TESTING LSEEK...\n\n");

	printf("change seek position on a file opened for reading\n");
	int fd = open("seek.txt", O_RDONLY);
	if (fd >= 0) {
		char buf[101];
		int bytes = read(fd, buf, 4);
		buf[bytes] = '\0';
		assert(bytes == 4);
		assert(strcmp(buf, "a\nb\n") == 0);
		printf("bytes read - should be 4: %d\n", bytes);
		printf("check if buffer is correct - "
		       "a\\nb\\n': '%s'\n", buf);

		printf("start reading from end of file\n");
		int seek = lseek(fd, 20, SEEK_SET);
		assert(seek == 20);
		printf("new position should be 20: %d\n", seek);
		bytes = read(fd, buf, 4);
		assert(bytes == 0);
		printf("bytes read - should be 0: %d\n", bytes);

		printf("restart reading from start of file\n");
		seek = lseek(fd, 0, SEEK_SET);
		assert(seek == 0);
		printf("new position should be 0: %d\n", seek);
		char buf2[101];
		bytes = read(fd, buf2, 4);
		buf2[bytes] = '\0';
		assert(bytes == 4);
		assert(strcmp(buf2, "a\nb\n") == 0);
		printf("bytes read - should be 4: %d\n", bytes);
		printf("check if buffer is correct - "
		       "a\\nb\\n': '%s'\n", buf2);

		printf("start reading from middle of file\n");
		seek = lseek(fd, 8, SEEK_SET);
		assert(seek == 8);
		printf("new position should be 8: %d\n", seek);
		char buf3[101];
		bytes = read(fd, buf3, 4);
		buf3[bytes] = '\0';
		assert(bytes == 4);
		assert(strcmp(buf3, "e\nf\n") == 0);
		printf("bytes read - should be 4: %d\n", bytes);
		printf("check if buffer is correct - "
		       "e\\nf\\n': '%s'\n", buf3);

		printf("offset reading position using relative +ve offset\n");
		seek = lseek(fd, 2, SEEK_CUR);
		assert(seek == 14);
		printf("new position should be 14: %d\n", seek);
		char buf4[101];
		bytes = read(fd, buf4, 4);
		buf4[bytes] = '\0';
		assert(bytes == 4);
		assert(strcmp(buf4, "h\ni\n") == 0);
		printf("bytes read - should be 4: %d\n", bytes);
		printf("check if buffer is correct - "
		       "h\\ni\\n': '%s'\n", buf4);

		printf("offset reading position using relative -ve offset\n");
		seek = lseek(fd, -4, SEEK_CUR);
		assert(seek == 14);
		printf("new position should be 14: %d\n", seek);
		char buf5[101];
		bytes = read(fd, buf5, 4);
		buf5[bytes] = '\0';
		assert(bytes == 4);
		assert(strcmp(buf5, "h\ni\n") == 0);
		printf("bytes read - should be 4: %d\n", bytes);
		printf("check if buffer is correct - "
		       "h\\ni\\n': '%s'\n", buf5);

		printf("offset reading position by 0 using relative offset\n");
		seek = lseek(fd, 0, SEEK_CUR);
		assert(seek == 18);
		printf("new position should be 18: %d\n", seek);
		char buf6[101];
		bytes = read(fd, buf6, 4);
		buf6[bytes] = '\0';
		assert(bytes == 2);
		assert(strcmp(buf6, "j\n") == 0);
		printf("bytes read - should be 4: %d\n", bytes);
		printf("bytes read - should be 2: %d\n", bytes);
		printf("check if buffer is correct - "
		       "j\\n': '%s'\n", buf6);

		printf("offset reading position to 0 bytes past end of file\n");
		seek = lseek(fd, 0, SEEK_END);
		assert(seek == 20);
		printf("new position should be 20: %d\n", seek);
		bytes = read(fd, buf6, 11);
		assert(bytes == 0);
		printf("bytes read - should be 0: %d\n", bytes);

		printf("offset reading position to 7 bytes past end of file\n");
		seek = lseek(fd, 7, SEEK_END);
		assert(seek == 27);
		printf("new position should be 27: %d\n", seek);
		bytes = read(fd, buf6, 3);
		assert(bytes == 0);
		printf("bytes read - should be 0: %d\n", bytes);

		printf("offset reading position by -8 from the end\n");
		seek = lseek(fd, -8, SEEK_END);
		assert(seek == 12);
		printf("new position should be 12: %d\n", seek);
		char buf7[101];
		bytes = read(fd, buf7, 4);
		buf7[bytes] = '\0';
		assert(bytes == 4);
		assert(strcmp(buf7, "g\nh\n") == 0);
		printf("bytes read - should be 4: %d\n", bytes);
		printf("check if buffer is correct - "
		       "g\\nh\\n': '%s'\n", buf7);

		printf("offset reading position by -20 from the end\n");
		seek = lseek(fd, -20, SEEK_END);
		assert(seek == 0);
		printf("new position should be 0: %d\n", seek);
		char buf8[101];
		bytes = read(fd, buf8, 4);
		buf8[bytes] = '\0';
		assert(bytes == 4);
		assert(strcmp(buf8, "a\nb\n") == 0);
		printf("bytes read - should be 4: %d\n", bytes);
		printf("check if buffer is correct - "
		       "a\\nb\\n': '%s'\n", buf8);

		printf("offset reading position by -3 from the start\n");
		seek = lseek(fd, -3, SEEK_SET);
		printf("error: %s\n\n", strerror(errno));

		printf("offset reading position by -5 using relative offset\n");
		seek = lseek(fd, -5, SEEK_CUR);
		printf("error: %s\n\n", strerror(errno));

		printf("offset reading position by -21 from the end\n");
		seek = lseek(fd, -21, SEEK_END);
		printf("error: %s\n\n", strerror(errno));

		printf("try to lseek stdout\n");
		seek = lseek(STDOUT_FILENO, -12, SEEK_SET);
		printf("error: %s\n\n", strerror(errno));

		printf("try to lseek stderr\n");
		seek = lseek(STDERR_FILENO, 14, SEEK_CUR);
		printf("error: %s\n\n", strerror(errno));

		printf("try to lseek a directory\n");
		int dir = open("dir", O_RDONLY);
		seek = lseek(dir, 0, SEEK_END);
		printf("error: %s\n\n", strerror(errno));
		close(dir);

		printf("try to seek with an invalid whence\n");
		seek = lseek(fd, 38, 999);
		printf("error: %s\n\n", strerror(errno));

		printf("try to lseek an unopened fd\n");
		seek = lseek(10, 99, SEEK_SET);
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
		assert(bytes == 27);
		assert(strcmp(buf, "hello world\nthis is a test\n") == 0);
		printf("bytes written - should be 27: %d\n", bytes);
		printf("check buffer is still in tact - "
		       "should be 'hello world\\nthis is a test\\n': '%s'\n", buf);

		printf("move to start of file before second write\n");
		int seek = lseek(fd, 0, SEEK_SET);
		assert(seek == 0);
		printf("new position should be 0: %d\n", seek);
		char buf2[] = "blablabla\n";
		bytes = write(fd, buf2, 10);
		assert(bytes == 10);
		assert(strcmp(buf2, "blablabla\n") == 0);
		printf("bytes written - should be 10: %d\n", bytes);
		printf("check buffer is still in tact - "
		       "should be 'blablabla\\n': '%s'\n", buf2);

		int r = close(fd);
		if (r) printf("error (should not print!): %s\n\n", strerror(errno));

		fd = open("t2.txt", O_RDONLY);
		char buf3[101];
		bytes = read(fd, buf3, 99);
		buf3[bytes] = '\0';
		assert(bytes == 27);
		assert(strcmp(buf3, "blablabla\nd\nthis is a test\n") == 0);
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
		assert(bytes == 27);
		assert(strcmp(buf, "hello world\nthis is a test\n") == 0);
		printf("bytes written - should be 27: %d\n", bytes);
		printf("check buffer is still in tact - "
		       "should be 'hello world\\nthis is a test\\n': '%s'\n", buf);

		printf("check that which was just written by moving to start\n");
		int seek = lseek(fd, 0, SEEK_SET);
		assert(seek == 0);
		printf("new position should be 0: %d\n", seek);
		char buf2[101];
		bytes = read(fd, buf2, 99);
		buf2[bytes] = '\0';
		assert(bytes == 27);
		assert(strcmp(buf2, "hello world\nthis is a test\n") == 0);
		printf("bytes read - should be 27: %d\n", bytes);
		printf("check if buffer is correct - "
		       "should be 'hello world\\nthis is a test\\n': '%s'\n", buf2);

		printf("start writing at some offset past end of file\n");
		seek = lseek(fd, 50, SEEK_END);
		assert(seek == 77);
		printf("new position should be 77: %d\n", seek);
		char buf3[] = "blablabla\n";
		bytes = write(fd, buf3, 10);
		assert(bytes == 10);
		assert(strcmp(buf3, "blablabla\n") == 0);
		printf("bytes written - should be 10: %d\n", bytes);
		printf("check buffer is still in tact - "
		       "should be 'blablabla\\n': '%s'\n", buf3);

		printf("check new file contents by moving to start\n");
		seek = lseek(fd, -87, SEEK_END);
		assert(seek == 0);
		printf("new position should be 0: %d\n", seek);
		char buf4[101];
		bytes = read(fd, buf4, 99);
		buf4[bytes] = '\0';
		assert(bytes == 87);
		/* We need two assertions since there is a gap in the middle of the file which strcmp doesn't know about */
		assert(strcmp(buf4, "hello world\nthis is a test\n") == 0);
		assert(strcmp(&buf4[77], "blablabla\n") == 0);
		printf("bytes read - should be 87: %d\n", bytes);
		printf("check if buffer is correct - "
		       "should be 'hello world\\nthis is a test\\nblablabla\\n': '\n");
		for (int i = 0; i < bytes; ++i) printf("%c", buf4[i]);
		printf("'\n");

		printf("overwrite middle of file \n");
		seek = lseek(fd, 17, SEEK_SET);
		assert(seek == 17);
		printf("new position should be 17: %d\n", seek);
		char buf5[] = "not";
		bytes = write(fd, buf5, 3);
		assert(bytes == 3);
		assert(strcmp(buf5, "not") == 0);
		printf("bytes written - should be 3: %d\n", bytes);
		printf("check buffer is still in tact - "
		       "should be 'not': '%s'\n", buf5);

		printf("check new file contents by moving to start\n");
		seek = lseek(fd, -20, SEEK_CUR);
		assert(seek == 0);
		printf("new position should be 0: %d\n", seek);
		char buf6[101];
		bytes = read(fd, buf6, 99);
		buf6[bytes] = '\0';
		assert(bytes == 87);

		/* We need two assertions since there is a gap in the middle of the file which strcmp doesn't know about */
		assert(strcmp(buf6, "hello world\nthis nota test\n") == 0);
		assert(strcmp(&buf4[77], "blablabla\n") == 0);
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
