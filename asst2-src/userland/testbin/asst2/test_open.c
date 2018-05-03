void test_open(void);

void test_open(void) {
	printf("TESTING OPEN...\n\n");

	printf("open a file for reading that doesn't exist\n");
	int fd = open("t1.txt", O_RDONLY);
	if (fd >= 0) {
		int r = close(fd);
		if (r) printf("error (should not print!): %s\n\n", strerror(errno));
	} else {
		printf("error: %s\n\n", strerror(errno));
	}

	printf("open a file for writing that doesn't exist\n");
	fd = open("t1.txt", O_WRONLY);
	if (fd >= 0) {
		int r = close(fd);
		if (r) printf("error (should not print!): %s\n\n", strerror(errno));
	} else {
		printf("error: %s\n\n", strerror(errno));
	}

	printf("open a file for reading and writing that doesn't exist\n");
	fd = open("t1.txt", O_RDWR);
	if (fd >= 0) {
		int r = close(fd);
		if (r) printf("error (should not print!): %s\n\n", strerror(errno));
	} else {
		printf("error: %s\n\n", strerror(errno));
	}

	printf("open a file for writing with O_CREAT\n");
	fd = open("t1.txt", O_WRONLY | O_CREAT);
	if (fd >= 0) {
		int r = close(fd);
		if (r) printf("error (should not print!): %s\n\n", strerror(errno));
	} else {
		printf("error (should not print!): %s\n\n", strerror(errno));
	}

	printf("open a file for writing with O_EXCL but not O_CREAT\n");
	fd = open("t1.txt", O_WRONLY | O_EXCL);
	if (fd >= 0) {
		int r = close(fd);
		if (r) printf("error (should not print!): %s\n\n", strerror(errno));
	} else {
		printf("error (should not print!): %s\n\n", strerror(errno));
	}

	printf("open a file for writing with O_EXCL and O_CREAT\n");
	fd = open("t1.txt", O_WRONLY | O_CREAT | O_EXCL);
	if (fd >= 0) {
		int r = close(fd);
		if (r) printf("error (should not print!): %s\n\n", strerror(errno));
	} else {
		printf("error: %s\n\n", strerror(errno));
	}

	printf("open an existing file for reading\n");
	fd = open("t1.txt", O_RDONLY);
	if (fd >= 0) {
		int r = close(fd);
		if (r) printf("error (should not print!): %s\n\n", strerror(errno));
	} else {
		printf("error: %s\n\n", strerror(errno));
	}

	printf("open an existing file for writing\n");
	fd = open("t1.txt", O_WRONLY);
	if (fd >= 0) {
		int r = close(fd);
		if (r) printf("error (should not print!): %s\n\n", strerror(errno));
	} else {
		printf("error: %s\n\n", strerror(errno));
	}

	printf("open an existing file for reading and writing\n");
	fd = open("t1.txt", O_RDWR);
	if (fd >= 0) {
		int r = close(fd);
		if (r) printf("error (should not print!): %s\n\n", strerror(errno));
	} else {
		printf("error: %s\n\n", strerror(errno));
	}

	printf("open an existing file for writing with O_TRUNC\n");
	fd = open("t1.txt", O_WRONLY | O_TRUNC);
	if (fd >= 0) {
		int r = close(fd);
		if (r) printf("error (should not print!): %s\n\n", strerror(errno));
	} else {
		printf("error: %s\n\n", strerror(errno));
	}

	printf("open an existing file for writing with O_APPEND\n");
	fd = open("t1.txt", O_WRONLY | O_APPEND);
	if (fd >= 0) {
		int r = close(fd);
		if (r) printf("error (should not print!): %s\n\n", strerror(errno));
	} else {
		printf("error: %s\n\n", strerror(errno));
	}

	printf("open a file with invalid flags\n");
	fd = open("t1.txt", 999);
	if (fd >= 0) {
		int r = close(fd);
		if (r) printf("error (should not print!): %s\n\n", strerror(errno));
	} else {
		printf("error: %s\n\n", strerror(errno));
	}

	printf("open a NULL file\n");
	fd = open(NULL, O_RDONLY);
	if (fd >= 0) {
		int r = close(fd);
		if (r) printf("error (should not print!): %s\n\n", strerror(errno));
	} else {
		printf("error: %s\n\n", strerror(errno));
	}

	printf("open a directory for reading\n");
	fd = open("dir", O_RDONLY);
	if (fd >= 0) {
		int r = close(fd);
		if (r) printf("error (should not print!): %s\n\n", strerror(errno));
	} else {
		printf("error (should not print!): %s\n\n", strerror(errno));
	}

	printf("open a directory for writing\n");
	fd = open("dir", O_WRONLY);
	if (fd >= 0) {
		int r = close(fd);
		if (r) printf("error (should not print!): %s\n\n", strerror(errno));
	} else {
		printf("error: %s\n\n", strerror(errno));
	}

	printf("open a file in a non-existent directory\n");
	fd = open("fake-dir/file.txt", O_RDONLY);
	if (fd >= 0) {
		int r = close(fd);
		if (r) printf("error (should not print!): %s\n\n", strerror(errno));
	} else {
		printf("error: %s\n\n", strerror(errno));
	}
}
