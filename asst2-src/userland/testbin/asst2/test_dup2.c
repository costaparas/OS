#define NUM_DUP2_TESTCASES 11

#define DUP2_BUF_SIZE 50
#define TEMP_FD 24
#define TEMP_FD2 30

/* Some test strings */
const char *DUP_LINE1 = "hello there mr duplicated fd\n";
const char *DUP_LINE2 = "stdout or stdsnout that is the question\n";
const char *DUP_LINE3 = "s u p e r  s t r e t c h y  l i n e\n";

const char *ALPHABET_LEFT = "ABCDEFGHIJKLM";
const char *ALPHABET_RIGHT = "NOPQRSTUVWXYZ";
const char *ALPHABET_FULL = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

/* Some buffers for dup2 testing */
char dup2_buf1[DUP2_BUF_SIZE] = {0};
char dup2_buf2[DUP2_BUF_SIZE] = {0};
char dup2_buf3[DUP2_BUF_SIZE] = {0};

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
static void test6 (void);
static void test7 (void);
static void test8 (void);
static void test9 (void);
static void test10 (void);
static void test11 (void);

/*
 * Driver function to run all test cases
 */
void test_dup2() {
	printf("TESTING DUP2...\n\n");

	/* Array of pointers to each test case */
	void (*test_cases[NUM_DUP2_TESTCASES])() = {
		test1, test2, test3, test4, test5, test6, test7, test8, test9, test10, test11
	};

	/* Run each of the tests in test_cases */
	for (int i = 0; i < NUM_DUP2_TESTCASES; i++) {
		printf("############################################\n");
		printf("dup2 test %d\n", i + 1);

		/* Clear out our buffers and run the test case */
		memset(dup2_buf1, 0, DUP2_BUF_SIZE); memset(dup2_buf2, 0, DUP2_BUF_SIZE); memset(dup2_buf3, 0, DUP2_BUF_SIZE);
		(test_cases[i])();

		printf("dup2 test %d passed\n", i + 1);
	}

	printf("############################################\n");
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

	assert(read(fd, dup2_buf1, 13) == 13);
	assert(read(fd2, dup2_buf2, 13) == 13);
	assert(strcmp(dup2_buf1, ALPHABET_LEFT) == 0);
	assert(strcmp(dup2_buf2, ALPHABET_RIGHT) == 0);

	close_fd_helper(fd2);
	close_fd_helper(fd);
}

/*
 * Test 3 - duplicate a FD for reading, close the original FD and try to read from it (should fail),
 * then read from duplicated fd
 */
static void test3 () {
	int fd = open_fd_helper("dup2_alphabet.txt", O_RDONLY);
	int fd2 = dup2_helper(fd, TEMP_FD);

	close_fd_helper(fd); /* Close original FD */

	assert(read(fd, dup2_buf1, 13) == -1);  /* Should fail since closed FD */
	assert(read(fd2, dup2_buf2, 13) == 13); /* Read first half of alphabet to dup2_buf2 */
	assert(strcmp(dup2_buf1, "") == 0);
	assert(strcmp(dup2_buf2, ALPHABET_LEFT) == 0);
}

/*
 * Test 4 - duplicate a FD for reading, and close both the duplicated and original FDs.
 * Try to read from them (both should fail)
 */
static void test4 () {
	int fd = open_fd_helper("dup2_alphabet.txt", O_RDONLY);
	int fd2 = dup2_helper(fd, TEMP_FD);

	/* Close both FDs */
	close_fd_helper(fd);
	close_fd_helper(fd2);

	/* Both should be -1 since FDs are closed */
	assert(read(fd, dup2_buf1, 13) == -1);
	assert(read(fd2, dup2_buf2, 13) == -1);
	assert(strcmp(dup2_buf1, "") == 0);
	assert(strcmp(dup2_buf2, "") == 0);
}

/*
 * Test 5 - duplicate a FD and make a duplicate of the duplicated FD, then close the original 2 FDs.
 * Try read from all three (first 2 should fail, last duplicate should succeed)
 */
static void test5 () {
	int fd = open_fd_helper("dup2_alphabet.txt", O_RDONLY);
	int fd2 = dup2_helper(fd, TEMP_FD);
	int fd3 = dup2_helper(fd2, TEMP_FD2);

	/* Close the first 2 FDs */
	close_fd_helper(fd);
	close_fd_helper(fd2);

	assert(read(fd, dup2_buf1, 13) == -1); /* Should be -1 since fd is closed */
	assert(read(fd2, dup2_buf2, 13) == -1); /* Should be -1 since fd is closed */
	assert(read(fd3, dup2_buf3, 13) == 13); /* Read first half of alphabet to dup2_buf3 */
	assert(strcmp(dup2_buf1, "") == 0);
	assert(strcmp(dup2_buf2, "") == 0);
	assert(strcmp(dup2_buf3, ALPHABET_LEFT) == 0);

	close_fd_helper(fd3);
}

/*
 * Test 6 - duplicate a FD and write from both FDs
 */
static void test6 () {
	int fd = open_fd_helper("dup2_write.txt", O_WRONLY | O_CREAT);
	int fd2 = dup2_helper(fd, TEMP_FD);

	assert(write(fd, DUP_LINE1, 29) == 29);
	assert(write(fd2, DUP_LINE2, 40) == 40);
	assert(write(fd2, DUP_LINE3, 36) == 36);

	/* Cleanup */
	close_fd_helper(fd);
	close_fd_helper(fd2);

	/* Reopen the file, read from it into dup2_buf1/2/3 and check that our write was successful.
	   Uses the other fd from writing to check reading (e.g. line 1: write fd, read fd2) */
	fd = open_fd_helper("dup2_write.txt", O_RDONLY);
	fd2 = dup2_helper(fd, TEMP_FD);

	assert(read(fd2, dup2_buf1, 29) == 29); /* Read first line to dup2_buf1 */
	assert(read(fd, dup2_buf2, 40) == 40); /* Read second line to dup2_buf2 */
	assert(read(fd, dup2_buf3, 36) == 36); /* Read third line to dup2_buf3 */

	assert(strcmp(dup2_buf1, DUP_LINE1) == 0);
	assert(strcmp(dup2_buf2, DUP_LINE2) == 0);
	assert(strcmp(dup2_buf3, DUP_LINE3) == 0);

	close_fd_helper(fd);
	close_fd_helper(fd2);

	printf("##############################################\n");
	printf("dup2_write.txt should contain (identical to dup2.txt - run `diff dup2_write.txt dup2.txt`):\n");
	printf(DUP_LINE1); printf(DUP_LINE2); printf(DUP_LINE3);
	printf("##############################################\n");
}

/*
 * Test 7 - duplicate a FD, write from both FDs then lseek and do more writes
 */
static void test7 () {
	int fd = open_fd_helper("dup2_lseek.txt", O_WRONLY | O_CREAT | O_TRUNC);
	int fd2 = dup2_helper(fd, TEMP_FD);

	assert(write(fd, ALPHABET_LEFT, 13) == 13);
	for (int i = 1; i < 14; i++) {
		assert(lseek(fd, i, SEEK_SET) == i);
		assert(write(fd, ALPHABET_LEFT, 13) == 13);
	}

	assert(lseek(fd, 13, SEEK_SET) == 13);
	assert(write(fd, ALPHABET_RIGHT, 13) == 13);

	/* Cleanup */
	close_fd_helper(fd);
	close_fd_helper(fd2);

	/* Reopen the file, read from it into dup2_buf1 and check that our write was successful. */
	fd = open_fd_helper("dup2_lseek.txt", O_RDONLY);

	assert(read(fd, dup2_buf1, 500) == 26); /* Read line to dup2_buf1 */
	assert(strcmp(dup2_buf1, "AAAAAAAAAAAAANOPQRSTUVWXYZ") == 0);

	/* lseek to position 13 and read remaining data into buf2 */
	assert(lseek(fd, 13, SEEK_SET) == 13);
	assert(read(fd, dup2_buf2, 500) == 13); /* Read remaining file to dup2_buf2 */
	assert(strcmp(dup2_buf2, "NOPQRSTUVWXYZ") == 0);

	close_fd_helper(fd);

	printf("##############################################\n");
	printf("dup2_lseek.txt should contain:\n");
	printf("AAAAAAAAAAAAANOPQRSTUVWXYZ\n");
	printf("##############################################\n");
}

/*
 * Test 8 - duplicate on top of an already opened fd and check that they point to the same file
 */

static void test8 () {
	int fd = open("dup2_alphabet.txt", O_RDONLY);
	int fd2 = open("dup2.txt", O_RDONLY); /* File choice doesn't matter - will be closed */
	assert(fd != fd2);

	int fd3 = dup2(fd, fd2);
	assert(fd3 == fd2);

	/* Check that fd and fd2 point to the same file by reading their contents into buffers and comparing */
	assert(read(fd, dup2_buf1, 500) == 26); /* Read file to dup2_buf1 */
	assert(lseek(fd2, 0, SEEK_SET) == 0);
	assert(read(fd2, dup2_buf2, 500) == 26); /* Read file to dup2_buf2 */
	assert(strcmp(dup2_buf1, dup2_buf2) == 0);
	assert(strcmp(dup2_buf1, ALPHABET_FULL) == 0);

	close_fd_helper(fd);
	close_fd_helper(fd2); /* fd2 == fd3 so don't need to close fd3 */
}

/*
 * Test 9 - test duplicating invalid FDs (free (i.e. unused), negative and FDs outside of valid range)
 * We can't use dup2_helper since it will fail the assertion
 */
static void test9 () {
	int unopenedFd = TEMP_FD;
	int fd2 = dup2(unopenedFd, TEMP_FD2);

	assert(fd2 == -1);
	assert(read(fd2, dup2_buf1, 500) == -1);

	int invalidFd1 = 99999;
	fd2 = dup2(invalidFd1, TEMP_FD2);

	assert(fd2 == -1);
	assert(read(fd2, dup2_buf1, 500) == -1);

	int invalidFd2 = -10;
	fd2 = dup2(invalidFd2, TEMP_FD2);

	assert(fd2 == -1);
	assert(read(fd2, dup2_buf1, 500) == -1);
}

/*
 * Test 10 - try use the same fd as old and new
 */
static void test10 () {
	int fd = open("dup2.txt", O_RDONLY);
	int fd2 = dup2(fd, fd);
	assert(fd != -1);
	assert(fd == fd2);
}

/*
 * Test 11 - open a file, dupe its file descriptor, do a read, close it and do another read from the duplicated FD.
 * Check that the offset hasn't been reset!
 */
static void test11 () {
	int fd = open("dup2_alphabet.txt", O_RDONLY);
	int fd2 = dup2_helper(fd, TEMP_FD);

	assert(read(fd, dup2_buf1, 13) == 13);
	assert(strcmp(dup2_buf1, ALPHABET_LEFT) == 0);

	close_fd_helper(fd);

	assert(read(fd2, dup2_buf2, 13) == 13);
	assert(strcmp(dup2_buf2, ALPHABET_RIGHT) == 0);

	assert(read(fd2, dup2_buf3, 13) == 0);

	close_fd_helper(fd2);
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
