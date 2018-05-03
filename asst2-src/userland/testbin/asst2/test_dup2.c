#define NUM_DUP2_TESTCASES 5

#define DUP2_BUF_SIZE 50
#define TEMP_FD 24
#define TEMP_FD2 30

/* Some test strings */
const char *DUP_LINE1 = "hello there mr duplicated fd\n";
const char *DUP_LINE2 = "stdout or stdsnout that is the question\n";
const char *DUP_LINE3 = "s u p e r  s t r e t c h y  l i n e\n";

/* Some buffers for dup2 testing */
char buf1[DUP2_BUF_SIZE] = {0};
char buf2[DUP2_BUF_SIZE] = {0};
char buf3[DUP2_BUF_SIZE] = {0};

/* Function prototypes */
void test_dup2(void);
static void close_fd_helper (int fd);
static int dup2_helper (int oldfd, int newfd);
static int open_fd_helper (const char *path, int flags);

/* Test case prototypes */
static void test1 (void);
static void test2 (void);
static void test3 (void);
static void test4 (void);
static void test5 (void);

void test_dup2() {
	printf("TESTING DUP2...\n\n");

	/* Array of pointers to each test case */
	void (*test_cases[NUM_DUP2_TESTCASES])() = {&test1, &test2, &test3, &test4, &test5};

	/* Run each of the tests in test_cases */
	for (int i = 0; i < NUM_DUP2_TESTCASES; i++) {
		printf("############################################\n");
		printf("dup2 test %d\n", i + 1);

		/* Clear out our buffers and run the test case */
		memset(buf1, 0, DUP2_BUF_SIZE); memset(buf2, 0, DUP2_BUF_SIZE); memset(buf3, 0, DUP2_BUF_SIZE);
		(test_cases[i])();

		printf("dup2 test %d passed\n", i + 1);
	}

	printf("############################################\n");

	/* TODO: test dup2 with lseek (different offsets etc.), invalid file names, invalid file descriptors, writing */
}

/* Test 1 - connect a file to stdout, temporarily close original stdout, then print some lines to the file */
static void test1 () {
	int fd = open_fd_helper("dup2.txt", O_WRONLY || O_CREAT);

	/* Keep a copy of STDOUT */
	int stdout_copy = dup2_helper(STDOUT_FILENO, TEMP_FD);

	printf("##############################################\n");
	printf("All stdout after this block should be in dup2.txt...\n");
	printf("dup2.txt should contain:\n");
	printf(DUP_LINE1); printf(DUP_LINE2); printf(DUP_LINE3);
	printf("##############################################\n");

	dup2_helper(fd, STDOUT_FILENO); /* Duplicate fd to STDOUT */

	/* Write some lines to dup2.txt */
	printf(DUP_LINE1); printf(DUP_LINE2); printf(DUP_LINE3);

	/* Cleanup */
	dup2_helper(stdout_copy, STDOUT_FILENO); /* Restore STDOUT to STDOUT_FILENO */
	close_fd_helper(stdout_copy);
	close_fd_helper(fd);

	printf("##############################################\n");
	printf("All stdout before here should be in dup2.txt...\n");
}

/* Test 2 - duplicate a FD for reading, and try read from both FDs */
static void test2 () {
	int fd = open_fd_helper("dup2_alphabet.txt", O_RDONLY);
	int fd2 = dup2_helper(fd, TEMP_FD);

	assert(read(fd, buf1, 13) == 13);
	assert(read(fd2, buf2, 13) == 13);
	assert(strcmp(buf1, "ABCDEFGHIJKLM") == 0);
	assert(strcmp(buf2, "NOPQRSTUVWXYZ") == 0);

	close_fd_helper(fd2);
	close_fd_helper(fd);
}

/* Test 3 - duplicate a FD for reading, close the original FD and try to read from it (should fail),
 * then read from duplicated fd */
static void test3 () {
	int fd = open_fd_helper("dup2_alphabet.txt", O_RDONLY);
	int fd2 = dup2_helper(fd, TEMP_FD);

	close_fd_helper(fd); /* Close original FD */

	assert(read(fd, buf1, 13) == -1);  /* Should fail since closed FD */
	assert(read(fd2, buf2, 13) == 13); /* Read first half of alphabet to buf2 */
	assert(strcmp(buf1, "") == 0);
	assert(strcmp(buf2, "ABCDEFGHIJKLM") == 0);
}

/* Test 4 - duplicate a FD for reading, and close both the duplicated and original FDs.
 * Try to read from them (both should fail)
 */
static void test4 () {
	int fd = open_fd_helper("dup2_alphabet.txt", O_RDONLY);
	int fd2 = dup2_helper(fd, TEMP_FD);

	/* Close both FDs */
	close_fd_helper(fd);
	close_fd_helper(fd2);

	/* Both should be -1 since FDs are closed */
	assert(read(fd, buf1, 13) == -1);
	assert(read(fd2, buf2, 13) == -1);
	assert(strcmp(buf1, "") == 0);
	assert(strcmp(buf2, "") == 0);
}

/* Test 5 - duplicate a FD and make a duplicate of the duplicated FD, then close the original 2 FDs.
 * Try read from all three (first 2 should fail, last duplicate should succeed)
 */
static void test5 () {
	int fd = open_fd_helper("dup2_alphabet.txt", O_RDONLY);
	int fd2 = dup2_helper(fd, TEMP_FD);
	int fd3 = dup2_helper(fd2, TEMP_FD2);

	/* Close the first 2 FDs */
	close_fd_helper(fd);
	close_fd_helper(fd2);

	assert(read(fd, buf1, 13) == -1); /* Should be -1 since fd is closed */
	assert(read(fd2, buf2, 13) == -1); /* Should be -1 since fd is closed */
	assert(read(fd3, buf3, 13) == 13); /* Read first half of alphabet to buf3 */
	assert(strcmp(buf1, "") == 0);
	assert(strcmp(buf2, "") == 0);
	assert(strcmp(buf3, "ABCDEFGHIJKLM") == 0);

	close_fd_helper(fd3);
}

/* Calls close on a FD and checks for errors, terminating (via failed assert) if unsuccessful */
static void close_fd_helper(int fd) {
	int r = close(fd);
	if (r) printf("error (should not print!): %s\n\n", strerror(errno));
	assert(r == 0);
}

/* Calls dup2 on a pair of FDs, displaying the error code and terminating (via failed assert) if unsuccessful */
static int dup2_helper(int oldfd, int newfd) {
	int duplicated = dup2(oldfd, newfd);
	if (duplicated == -1) printf("error (should not print!): %s\n\n", strerror(errno));
	assert(duplicated == newfd);
	return newfd;
}

/* Calls open and checks whether we get a valid FD, terminating (via failed assert) if unsuccesful */
static int open_fd_helper(const char *path, int flags) {
	int fd = open(path, flags);
	if (fd < 0) {
		printf("error (should not print!): %s\n\n", strerror(errno));
	}
	assert(fd >= 0);
	return fd;
}

