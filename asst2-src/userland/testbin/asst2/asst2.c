#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <err.h>
#include <errno.h>

#define MAX_BUF 500
char teststr[] = "The quick brown fox jumped over the lazy dog.";
char buf[MAX_BUF];

void test_open(void);
void test_close(void);
void test_read(void);
void test_write(void);
void test_std_streams(void);

int main(int argc, char *argv[]) {
	int fd, r, i, j, k;
	(void) argc;
	(void) argv;

	/* begin custom tests */
	test_open();
	test_close();
	test_read();
	test_write();
	test_std_streams();
	return 0;
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
			printf("ERROR  file contents mismatch\n");
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
			printf("ERROR  file contents mismatch\n");
			exit(1);
		}
		k++;
		j = (k + 5) % r;
	} while (k < 5);

	printf("* file lseek  okay\n");
	printf("* closing file\n");
	close(fd);

	return 0;
}

void test_open(void) {
	printf("TESTING OPEN...\n\n");

	printf("open a file for reading that doesn't exist\n");
	int fd = open("t1.txt", O_RDONLY);
	if (fd > 0) {
		int r = close(fd);
		if (r) printf("error (should not print!): %s\n\n", strerror(errno));
	} else {
		printf("error: %s\n\n", strerror(errno));
	}

	printf("open a file for writing that doesn't exist\n");
	fd = open("t1.txt", O_WRONLY);
	if (fd > 0) {
		int r = close(fd);
		if (r) printf("error (should not print!): %s\n\n", strerror(errno));
	} else {
		printf("error: %s\n\n", strerror(errno));
	}

	printf("open a file for reading and writing that doesn't exist\n");
	fd = open("t1.txt", O_RDWR);
	if (fd > 0) {
		int r = close(fd);
		if (r) printf("error (should not print!): %s\n\n", strerror(errno));
	} else {
		printf("error: %s\n\n", strerror(errno));
	}

	printf("open a file for writing with O_CREAT\n");
	fd = open("t1.txt", O_WRONLY | O_CREAT);
	if (fd > 0) {
		int r = close(fd);
		if (r) printf("error (should not print!): %s\n\n", strerror(errno));
	} else {
		printf("error (should not print!): %s\n\n", strerror(errno));
	}

	printf("open a file for writing with O_EXCL but not O_CREAT\n");
	fd = open("t1.txt", O_WRONLY | O_EXCL);
	if (fd > 0) {
		int r = close(fd);
		if (r) printf("error (should not print!): %s\n\n", strerror(errno));
	} else {
		printf("error (should not print!): %s\n\n", strerror(errno));
	}

	printf("open a file for writing with O_EXCL and O_CREAT\n");
	fd = open("t1.txt", O_WRONLY | O_CREAT | O_EXCL);
	if (fd > 0) {
		int r = close(fd);
		if (r) printf("error (should not print!): %s\n\n", strerror(errno));
	} else {
		printf("error: %s\n\n", strerror(errno));
	}

	printf("open an existing file for reading\n");
	fd = open("t1.txt", O_RDONLY);
	if (fd > 0) {
		int r = close(fd);
		if (r) printf("error (should not print!): %s\n\n", strerror(errno));
	} else {
		printf("error: %s\n\n", strerror(errno));
	}

	printf("open an existing file for writing\n");
	fd = open("t1.txt", O_WRONLY);
	if (fd > 0) {
		int r = close(fd);
		if (r) printf("error (should not print!): %s\n\n", strerror(errno));
	} else {
		printf("error: %s\n\n", strerror(errno));
	}

	printf("open an existing file for reading and writing\n");
	fd = open("t1.txt", O_RDWR);
	if (fd > 0) {
		int r = close(fd);
		if (r) printf("error (should not print!): %s\n\n", strerror(errno));
	} else {
		printf("error: %s\n\n", strerror(errno));
	}

	printf("open an existing file for writing with O_TRUNC\n");
	fd = open("t1.txt", O_WRONLY | O_TRUNC);
	if (fd > 0) {
		int r = close(fd);
		if (r) printf("error (should not print!): %s\n\n", strerror(errno));
	} else {
		printf("error: %s\n\n", strerror(errno));
	}

	printf("open an existing file for writing with O_APPEND\n");
	fd = open("t1.txt", O_WRONLY | O_APPEND);
	if (fd > 0) {
		int r = close(fd);
		if (r) printf("error (should not print!): %s\n\n", strerror(errno));
	} else {
		printf("error: %s\n\n", strerror(errno));
	}

	printf("open a file with invalid flags\n");
	fd = open("t1.txt", 999);
	if (fd > 0) {
		int r = close(fd);
		if (r) printf("error (should not print!): %s\n\n", strerror(errno));
	} else {
		printf("error: %s\n\n", strerror(errno));
	}

	printf("open a NULL file\n");
	fd = open(NULL, O_RDONLY);
	if (fd > 0) {
		int r = close(fd);
		if (r) printf("error (should not print!): %s\n\n", strerror(errno));
	} else {
		printf("error: %s\n\n", strerror(errno));
	}

	printf("open a directory for reading\n");
	fd = open("dir", O_RDONLY);
	if (fd > 0) {
		int r = close(fd);
		if (r) printf("error (should not print!): %s\n\n", strerror(errno));
	} else {
		printf("error (should not print!): %s\n\n", strerror(errno));
	}

	printf("open a directory for writing\n");
	fd = open("dir", O_WRONLY);
	if (fd > 0) {
		int r = close(fd);
		if (r) printf("error (should not print!): %s\n\n", strerror(errno));
	} else {
		printf("error: %s\n\n", strerror(errno));
	}

	printf("open a file in a non-existent directory\n");
	fd = open("fake-dir/file.txt", O_RDONLY);
	if (fd > 0) {
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
	if (ret) {
		printf("error (should not print!): %s\n\n", strerror(errno));
	}

	printf("close an unopened file\n");
	ret = close(10);
	if (ret) {
		printf("error: %s\n\n", strerror(errno));
	}

	printf("close a file with fd >= OPEN_MAX\n");
	ret = close(1000);
	if (ret) {
		printf("error: %s\n\n", strerror(errno));
	}
}

void test_read(void) {
	printf("TESTING READ...\n\n");

	printf("read from a file opened for reading only\n");
	int fd = open("file.txt", O_RDONLY);
	if (fd > 0) {
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
	if (fd > 0) {
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
	if (fd > 0) {
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
}

void test_write(void) {
	printf("TESTING WRITE...\n\n");

	printf("write to a file opened for writing only\n");
	int fd = open("t1.txt", O_WRONLY);
	if (fd > 0) {
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
	if (fd > 0) {
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
	if (fd > 0) {
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
	if (fd > 0) {
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
	if (fd > 0) {
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
		printf("bytes read - should be 29: %d\n", bytes);
		printf("check if buffer is correct - "
			"should be 'blablabla\\nshould be appended\\n': '%s'\n", buf2);
		r = close(fd);
		if (r) printf("error (should not print!): %s\n\n", strerror(errno));
	} else {
		printf("error (should not print!): %s\n\n", strerror(errno));
	}
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
		//printf("error (should not print!): %s\n\n", strerror(errno));
	}
	ret = close(2);
	if (ret) {
		//printf("error (should not print!): %s\n\n", strerror(errno));
	}

	//printf("check that fds 1 and 2 can be used subsequently\n");
	int fd1 = open("file.txt", O_RDONLY);
	int fd2 = open("file.txt", O_RDONLY);
	int fd3 = open("file.txt", O_RDONLY);
	//printf("fds: %d %d %d\n", fd1, fd2, fd3);
	close(fd1);
	close(fd2);
	close(fd3);

	//printf("reopen std streams\n");
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
