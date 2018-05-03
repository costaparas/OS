void test_close(void);

void test_close(void) {
	printf("TESTING CLOSE...\n\n");

	printf("close an open file\n");
	int fd = open("t1.txt", O_RDONLY);
	int ret = close(fd);
	if (ret) printf("error (should not print!): %s\n\n", strerror(errno));

	printf("close an unopened file\n");
	ret = close(10);
	printf("error: %s\n\n", strerror(errno));

	printf("close a file with fd >= OPEN_MAX\n");
	ret = close(1000);
	printf("error: %s\n\n", strerror(errno));
}
