#define _GNU_SOURCE
#include <cover/suite.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <err.h>

#define HNY_TEST_ARCHIVE "test/archive.hny"
#define HNY_TEST_PREFIX "test/prefix"

#define hny(args) hny_at(args, __FILE__, __LINE__)

static void
removeall_at(int atfd, const char *path) {
	DIR *dirp;

	{ /* Relative opendir */
		const int fd = openat(atfd, path, O_RDONLY);

		if (fd < 0) {
			err(EXIT_FAILURE, "open %s", path);
		}

		dirp = fdopendir(fd);
		if (dirp == NULL) {
			err(EXIT_FAILURE, "opendir %s", path);
		}
	}

	struct dirent *entry;
	while (errno = 0, entry = readdir(dirp)) {
		if (*entry->d_name != '.') {
			int flags = 0;
			switch (entry->d_type) {
			case DT_DIR:
				removeall_at(dirfd(dirp), entry->d_name);
				flags = AT_REMOVEDIR;
				/* fallthrough */
			case DT_REG:
			case DT_LNK:
				if (unlinkat(dirfd(dirp), entry->d_name, flags) != 0) {
					err(EXIT_FAILURE, "unlink %s", path);
				}
				break;
			}
		}
	}

	if (errno != 0) {
		err(EXIT_FAILURE, "readdir");
	}

	closedir(dirp);
}

void
cover_suite_init(int argc, char **argv) {

	{ /* Create the test archive */
		const char * const xzexe = getenv("XZ_EXE");
		char *xzcommand;
		FILE *output;

		if (asprintf(&xzcommand, "%s -C crc32 --lzma2 > "HNY_TEST_ARCHIVE, xzexe) < 0) {
			err(EXIT_FAILURE, "asprintf");
		}

		output = popen(xzcommand, "w");
		if (output == NULL) {
			err(EXIT_FAILURE, "popen");
		}

		{ /* Print archive entries in CPIO ODC format */
			const uid_t uid = geteuid();
			const gid_t gid = getegid();

			fprintf(output, "070707004021002167040755%.6o%.6o0000020000000000000000000000500000000000pkg/", uid, gid);
			fputc('\0', output);

			fprintf(output, "070707004021002170100755%.6o%.6o0000020000000000000000000001200000000046pkg/clean", uid, gid);
			fputc('\0', output);
			fprintf(output, "#!/bin/sh\necho \"Test Archive - Clean\"\n");

			fprintf(output, "070707004021002170100755%.6o%.6o0000020000000000000000000001200000000046pkg/setup", uid, gid);
			fputc('\0', output);
			fprintf(output, "#!/bin/sh\necho \"Test Archive - Setup\"\n");

			fprintf(output, "0707070000000000000000000000000000000000010000000000000000000001300000000000TRAILER!!!");
			fputc('\0', output);
		}

		if (pclose(output) < 0) {
			err(EXIT_FAILURE, "pclose %s", xzcommand);
		}

		free(xzcommand);
	}

	{ /* Setup prefix directory */
		char *path;

		if (mkdir(HNY_TEST_PREFIX, 0777) != 0) {
			if (errno == EEXIST) {
				/* Better obliterate content when beginning tests, so if it fails we still have progression artifacts */
				removeall_at(AT_FDCWD, HNY_TEST_PREFIX);
			} else {
				err(EXIT_FAILURE, "mkdir %s", HNY_TEST_PREFIX);
			}
		}

		path = realpath(HNY_TEST_PREFIX, NULL);
		setenv("HNY_PREFIX", path, 1); /* Expects absolute path */
		free(path);
	}

	/* Empty umask to assert file modes */
	umask(0);
}

static void
hny_at(char * const arguments[], const char *filename, int lineno) {
	const char * const hnyexe = getenv("HNY_EXE");

	/* Write command on stderr */
	fputs(hnyexe, stderr);
	for (char * const *current = arguments + 1; *current != NULL; current++) {
		fprintf(stderr, " %s", *current);
	}
	fputc('\n', stderr);

	const pid_t pid = fork();
	switch (pid) {
	case -1:
		perror("fork");
		cover_fail_at("Unable to create child process", filename, lineno);
		break;
	case 0:
		/* Redirect stdout to stderr to avoid colisions with TAP output */
		if (dup2(STDERR_FILENO, STDOUT_FILENO) != STDOUT_FILENO) {
			err(EXIT_FAILURE, "dup2");
		}
		execv(hnyexe, arguments);
		err(EXIT_FAILURE, "execv %s %s", hnyexe, *arguments);
		break;
	default: {
		int wstatus;

		if (waitpid(pid, &wstatus, 0) != pid) {
			perror("waitpid");
		}

		/* Simple status check */
		if (WIFEXITED(wstatus)) {
			const int status = WEXITSTATUS(wstatus);
			if (status != 0) {
				fprintf(stderr, "exit status: %d\n", status);
				cover_fail_at("Invalid return status", filename, lineno);
			}
		} else if(WIFSIGNALED(wstatus)) {
			const int signo = WTERMSIG(wstatus);
			fprintf(stderr, "terminated with signal: %s %d\n", strsignal(signo), signo);
			cover_fail_at("Process terminated with signal", filename, lineno);
		}
	}	break;
	}
}

static void
test_hny(void) {
	struct stat st;

	{ /* honey extract */
		char * const cmd0[] = { "hny", "extract", "archive-1.0.0", HNY_TEST_ARCHIVE, NULL };

		hny(cmd0);

		cover_assert(lstat(HNY_TEST_PREFIX"/archive-1.0.0", &st) == 0, "stat archive-1.0.0");
		cover_assert(st.st_mode == (S_IFDIR | 0755), "archive-1.0.0 is not a directory");

		cover_assert(lstat(HNY_TEST_PREFIX"/archive-1.0.0/pkg", &st) == 0, "stat archive-1.0.0/pkg");
		cover_assert(st.st_mode == (S_IFDIR | 0755), "archive-1.0.0/pkg is not a directory");

		cover_assert(lstat(HNY_TEST_PREFIX"/archive-1.0.0/pkg/clean", &st) == 0, "stat archive-1.0.0/pkg/clean");
		cover_assert(st.st_mode == (S_IFREG | 0755), "archive-1.0.0/pkg/clean is not an executable regular file");

		cover_assert(lstat(HNY_TEST_PREFIX"/archive-1.0.0/pkg/setup", &st) == 0, "stat archive-1.0.0/pkg/setup");
		cover_assert(st.st_mode == (S_IFREG | 0755), "archive-1.0.0/pkg/setup is not an executable regular file");
	}

	{/* honey shift */
		char * const cmd0[] = { "hny", "shift", "archive", "archive-1.0.0", NULL };
		char * const cmd1[] = { "hny", "shift", "arxiv", "archive", NULL };

		hny(cmd0);

		cover_assert(lstat(HNY_TEST_PREFIX"/archive", &st) == 0, "stat archive");
		cover_assert(st.st_mode == (S_IFLNK | 0777), "archive is not a symbolic link");

		hny(cmd1);

		cover_assert(lstat(HNY_TEST_PREFIX"/arxiv", &st) == 0, "stat arxiv");
		cover_assert(st.st_mode == (S_IFLNK | 0777), "arxiv is not a symbolic link");
	}

	{/* honey status */
		char * const cmd0[] = { "hny", "status", "archive", NULL };
		char * const cmd1[] = { "hny", "status", "arxiv", NULL };

		hny(cmd0);
		hny(cmd1);
	}

	{/* honey setup */
		char * const cmd0[] = { "hny", "setup", "arxiv", NULL };

		hny(cmd0);
	}

	{/* honey list */
		char * const cmd0[] = { "hny", "list", NULL };

		hny(cmd0);
	}

	{/* honey clean */
		char * const cmd0[] = { "hny", "clean", "arxiv", NULL };

		hny(cmd0);
	}

	{/* honey remove */
		char * const cmd0[] = { "hny", "remove", "arxiv", NULL };
		char * const cmd1[] = { "hny", "remove", "archive", "archive-1.0.0", NULL };

		hny(cmd0);

		/* Check whether removal removed referenced files instead of symbolic link */
		cover_assert(lstat(HNY_TEST_PREFIX"/archive", &st) == 0, "stat archive");
		cover_assert(lstat(HNY_TEST_PREFIX"/archive-1.0.0", &st) == 0, "stat archive-1.0.0");

		hny(cmd1);
	}
}

const struct cover_case cover_suite[] = {
	COVER_SUITE_TEST(test_hny),
	COVER_SUITE_END,
};
