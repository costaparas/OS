void test_std_streams(void);

void test_std_streams(void) {
	printf("TESTING STDOUT AND STDERR...\n\n");

	char buf[] = "some text to be written to the std streams\n";
	printf("print to stdout\n");
	int bytes = write(STDOUT_FILENO, buf, strlen(buf));
	printf("wrote %d bytes to stdout\n", bytes);
	printf("print to stderr\n");
	write(STDERR_FILENO, buf, strlen(buf));
	printf("wrote %d bytes to stderr\n", bytes);

	/* NOTE: commented lines won't print anyway since stdout closed */

	printf("close std streams\n");
	int ret = close(STDOUT_FILENO);
	if (ret) {
		/* printf("error (should not print!): %s\n\n", strerror(errno)); */
	}
	ret = close(STDERR_FILENO);
	if (ret) {
		/* printf("error (should not print!): %s\n\n", strerror(errno)); */
	}

	/* printf("check that fds 1 and 2 can be used subsequently\n"); */
	int fd1 = open("file.txt", O_RDONLY);
	int fd2 = open("file.txt", O_RDONLY);
	int fd3 = open("file.txt", O_RDONLY);
	/* printf("fds: %d %d %d\n", fd1, fd2, fd3); */
	close(fd1);
	close(fd2);
	close(fd3);

	/* printf("reopen std streams\n"); */
	int fd = open("con:", O_WRONLY);
	printf("stdout open on fd %d\n", fd);
	fd = open("con:", O_WRONLY);
	printf("stderr open on fd %d\n", fd);
	printf("check that printing to std streams works again\n");
	bytes = write(STDOUT_FILENO, buf, strlen(buf));
	printf("wrote %d bytes to stdout\n", bytes);
	printf("print to stderr\n");
	write(STDERR_FILENO, buf, strlen(buf));
	printf("wrote %d bytes to stderr\n", bytes);
}
