#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <err.h>
#include <errno.h>
#include <assert.h>

#define MAX_BUF 500
#define DUP2_BUF_SIZE 50
char teststr[] = "The quick brown fox jumped over the lazy dog.";
char buf[MAX_BUF];

void test_open(void);
void test_close(void);
void test_read(void);
void test_write(void);
void test_std_streams(void);
void test_lseek(void);
void test_dup2(void);

int main(int argc, char *argv[]) {
	int fd, r, i, j, k;
	(void) argc;
	(void) argv;

	/* begin custom tests */
	printf("BEGIN CUSTOM TESTS\n");
	test_open();
	test_close();
	test_read();
	test_write();
	test_std_streams();
	test_lseek();
	test_dup2();
	printf("END CUSTOM TESTS\n");
	/* end custom tests */

	printf("\n**********\n* File Tester\n");

	snprintf(buf, MAX_BUF, "**********\n* write() works for stdout\n");
	write(1, buf, strlen(buf));
	snprintf(buf, MAX_BUF, "**********\n* write() works for stderr\n");
	write(2, buf, strlen(buf));

	printf("**********\n* opening new file \"test.file\"\n");
	fd = open("test.file", O_RDWR | O_CREAT);
	printf("* open() got fd %d\n", fd);
	if (fd < 0) {
		printf("ERROR opening file: %s\n", strerror(errno));
		exit(1);
	}

	printf("* writing test string\n");
	r = write(fd, teststr, strlen(teststr));
	printf("* wrote %d bytes\n", r);
	if (r < 0) {
		printf("ERROR writing file: %s\n", strerror(errno));
		exit(1);
	}

	printf("* writing test string again\n");
	r = write(fd, teststr, strlen(teststr));
	printf("* wrote %d bytes\n", r);
	if (r < 0) {
		printf("ERROR writing file: %s\n", strerror(errno));
		exit(1);
	}
	printf("* closing file\n");
	close(fd);

	printf("**********\n* opening old file \"test.file\"\n");
	fd = open("test.file", O_RDONLY);
	printf("* open() got fd %d\n", fd);
	if (fd < 0) {
		printf("ERROR opening file: %s\n", strerror(errno));
		exit(1);
	}

	printf("* reading entire file into buffer \n");
	i = 0;
	do {
		printf("* attempting read of %d bytes\n", MAX_BUF - i);
		r = read(fd, &buf[i], MAX_BUF - i);
		printf("* read %d bytes\n", r);
		i += r;
	} while (i < MAX_BUF && r > 0);

	printf("* reading complete\n");
	if (r < 0) {
		printf("ERROR reading file: %s\n", strerror(errno));
		exit(1);
	}
	k = j = 0;
	r = strlen(teststr);
	do {
		if (buf[k] != teststr[j]) {
			printf("ERROR file contents mismatch\n");
			exit(1);
		}
		k++;
		j = k % r;
	} while (k < i);
	printf("* file content okay\n");

	printf("**********\n* testing lseek\n");
	r = lseek(fd, 5, SEEK_SET);
	if (r < 0) {
		printf("ERROR lseek: %s\n", strerror(errno));
		exit(1);
	}

	printf("* reading 10 bytes of file into buffer \n");
	i = 0;
	do {
		printf("* attempting read of %d bytes\n", 10 - i);
		r = read(fd, &buf[i], 10 - i);
		printf("* read %d bytes\n", r);
		i += r;
	} while (i < 10 && r > 0);
	printf("* reading complete\n");
	if (r < 0) {
		printf("ERROR reading file: %s\n", strerror(errno));
		exit(1);
	}

	k = 0;
	j = 5;
	r = strlen(teststr);
	do {
		if (buf[k] != teststr[j]) {
			printf("ERROR file contents mismatch\n");
			exit(1);
		}
		k++;
		j = (k + 5) % r;
	} while (k < 5);

	printf("* file lseek okay\n");
	printf("* closing file\n");
	close(fd);

	return 0;
}

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

void test_write(void) {
	printf("TESTING WRITE...\n\n");

	printf("write to a file opened for writing only\n");
	int fd = open("t1.txt", O_WRONLY);
	if (fd >= 0) {
		char buf[] = "hello world\n";
		int bytes = write(fd, buf, 12);
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
		printf("bytes written - should be 0: %d\n", bytes);
		int r = close(fd);
		if (r) printf("error (should not print!): %s\n\n", strerror(errno));

		printf("check that the file did not change\n");
		fd = open("t1.txt", O_RDONLY);
		char buf2[51];
		bytes = read(fd, buf2, 49);
		buf2[bytes] = '\0';
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
		printf("bytes written - should be 47: %d\n", bytes);
		char buf2[101];
		bytes = read(fd, buf2, 99);
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

void test_std_streams(void) {
	printf("TESTING STDOUT AND STDERR...\n\n");

	char buf[] = "some text to be written to the std streams\n";
	printf("print to stdout\n");
	int bytes = write(1, buf, strlen(buf));
	printf("wrote %d bytes to stdout\n", bytes);
	printf("print to stderr\n");
	write(2, buf, strlen(buf));
	printf("wrote %d bytes to stderr\n", bytes);

	/* NOTE: commented lines won't print anyway since stdout closed */

	printf("close std streams\n");
	int ret = close(1);
	if (ret) {
		/* printf("error (should not print!): %s\n\n", strerror(errno)); */
	}
	ret = close(2);
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
	bytes = write(1, buf, strlen(buf));
	printf("wrote %d bytes to stdout\n", bytes);
	printf("print to stderr\n");
	write(2, buf, strlen(buf));
	printf("wrote %d bytes to stderr\n", bytes);
}

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
		lseek(1, -12, SEEK_SET);
		printf("error: %s\n\n", strerror(errno));

		printf("try to lseek stderr\n");
		lseek(2, 14, SEEK_CUR);
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
		if (r) printf("errintor (should not print!): %s\n\n", strerror(errno));
	} else {
		printf("error (should not print!): %s\n\n", strerror(errno));
	}
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

void test_dup2(void) {
	printf("TESTING DUP2...\n\n");

	// Magic constants
	const int TEMP_FD = 24;
	const int TEMP_FD2 = 30;

	// Some lines to print out
	const char *DUP_LINE1 = "hello there mr duplicated fd\n";
	const char *DUP_LINE2 = "stdout or stdsnout that is the question\n";
	const char *DUP_LINE3 = "s u p e r  s t r e t c h y  l i n e\n";

	char buf1[DUP2_BUF_SIZE] = {0};
	char buf2[DUP2_BUF_SIZE] = {0};
	char buf3[DUP2_BUF_SIZE] = {0};

	int fd, fd2, fd3;

	// Test 1 - connect a file to stdout, temporarily close original stdout, then print some lines to the file
	printf("############################################\n");
	printf("dup2 test 1\n");
	fd = open_fd_helper("dup2.txt", O_WRONLY || O_CREAT);
	// Keep a copy of STDOUT
	int stdout_copy = dup2_helper(STDOUT_FILENO, TEMP_FD);

	printf("##############################################\n");
	printf("All stdout after this block should be in dup2.txt...\n");
	printf("dup2.txt should contain:\n");
	printf(DUP_LINE1); printf(DUP_LINE2); printf(DUP_LINE3);
	printf("##############################################\n");

	dup2_helper(fd, STDOUT_FILENO); // Duplicate fd to STDOUT

	// Write some lines to dup2.txt
	printf(DUP_LINE1); printf(DUP_LINE2); printf(DUP_LINE3);

	// Cleanup
	dup2_helper(stdout_copy, STDOUT_FILENO); // Restore STDOUT to STDOUT_FILENO
	close_fd_helper(stdout_copy);
	close_fd_helper(fd);

	printf("##############################################\n");
	printf("All stdout before here should be in dup2.txt...\n");
	printf("##############################################\n");
	printf("dup2 test 1 passed\n");
	printf("############################################\n");

	// Test 2 - duplicate a FD for reading, and try read from both FDs
	memset(buf1, 0, DUP2_BUF_SIZE); memset(buf2, 0, DUP2_BUF_SIZE); memset(buf3, 0, DUP2_BUF_SIZE);
	printf("############################################\n");
	printf("dup2 test 2\n");
	fd = open_fd_helper("dup2_alphabet.txt", O_RDONLY);
	fd2 = dup2_helper(fd, TEMP_FD);

	assert(read(fd, buf1, 13) == 13);
	assert(read(fd2, buf2, 13) == 13);
	assert(strcmp(buf1, "ABCDEFGHIJKLM") == 0);
	assert(strcmp(buf2, "NOPQRSTUVWXYZ") == 0);

	close_fd_helper(fd2);
	close_fd_helper(fd);
	printf("dup2 test 2 passed\n");
	printf("############################################\n");

	// Test 3 - duplicate a FD for reading, close the original FD and try to read from it (should fail),
	// then read from duplicated fd
	memset(buf1, 0, DUP2_BUF_SIZE); memset(buf2, 0, DUP2_BUF_SIZE); memset(buf3, 0, DUP2_BUF_SIZE);
	printf("############################################\n");
	printf("dup2 test 3\n");
	printf("BUF1: |%s|\n", buf1);
	fd = open_fd_helper("dup2_alphabet.txt", O_RDONLY);
	fd2 = dup2_helper(fd, TEMP_FD);

	close_fd_helper(fd); // Close original FD

	assert(read(fd, buf1, 13) == -1);  // Should fail since closed FD
	assert(read(fd2, buf2, 13) == 13); // Read first half of alphabet to buf2
	assert(strcmp(buf1, "") == 0);
	assert(strcmp(buf2, "ABCDEFGHIJKLM") == 0);

	printf("dup2 test 3 passed\n");
	printf("############################################\n");

	// Test 4 - duplicate a FD for reading, and close both the duplicated and original FDs.
	// Try to read from them (both should fail)
	printf("############################################\n");
	printf("dup2 test 4\n");
	memset(buf1, 0, DUP2_BUF_SIZE); memset(buf2, 0, DUP2_BUF_SIZE); memset(buf3, 0, DUP2_BUF_SIZE);
	fd = open_fd_helper("dup2_alphabet.txt", O_RDONLY);
	fd2 = dup2_helper(fd, TEMP_FD);

	// Close both FDs
	close_fd_helper(fd);
	close_fd_helper(fd2);

	// Both should be -1 since FDs are closed
	assert(read(fd, buf1, 13) == -1);
	assert(read(fd2, buf2, 13) == -1);
	assert(strcmp(buf1, "") == 0);
	assert(strcmp(buf2, "") == 0);

	printf("dup2 test 4 passed\n");
	printf("############################################\n");

	// Test 5 - duplicate a FD and make a duplicate of the duplicated FD, then close the original 2 FDs.
	// Try read from all three (first 2 should fail, last duplicate should succeed)
	printf("############################################\n");
	printf("dup2 test 5\n");
	memset(buf1, 0, DUP2_BUF_SIZE); memset(buf2, 0, DUP2_BUF_SIZE); memset(buf3, 0, DUP2_BUF_SIZE);

	fd = open_fd_helper("dup2_alphabet.txt", O_RDONLY);
	fd2 = dup2_helper(fd, TEMP_FD);
	fd3 = dup2_helper(fd2, TEMP_FD2);

	// Close the first 2 FDs
	close_fd_helper(fd);
	close_fd_helper(fd2);

	assert(read(fd, buf1, 13) == -1); // Should be -1 since fd is closed
	assert(read(fd2, buf2, 13) == -1); // Should be -1 since fd is closed
	assert(read(fd3, buf3, 13) == 13); // Read first half of alphabet to buf3
	assert(strcmp(buf1, "") == 0);
	assert(strcmp(buf2, "") == 0);
	assert(strcmp(buf3, "ABCDEFGHIJKLM") == 0);

	close_fd_helper(fd3);
	printf("dup2 test 5 passed\n");
	printf("############################################\n");

	// TODO: test dup2 with lseek (different offsets etc.), invalid file names, invalid file descriptors, writing
}
