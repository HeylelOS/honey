/* SPDX-License-Identifier: BSD-3-Clause */
#include "hny_prefix.h"

#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <errno.h>

#include "config.h"

struct hny_dir {
	DIR *dirp;              /**< Currently iterated instance */
	struct dirent *current; /**< Latest iteration in the directory */
};

struct hny_dirstack {
	struct hny_dir *dirs; /**< Stack of currently iterated directories */
	int capacity;         /**< Capacity of dirs */
	int top;              /**< Current stack top */
};

static int
hny_dirstack_push(struct hny_dirstack *stack, int dirfd, const char *path) {
	const int newtop = stack->top + 1, fd = openat(dirfd, path, O_RDONLY);
	DIR *dirp;

	if (fd < 0) {
		return errno;
	}

	dirp = fdopendir(fd);
	if (dirp == NULL) {
		const int errcode = errno;
		close(fd);
		return errcode;
	}

	if (newtop == stack->capacity) {
		const size_t newcapacity = stack->capacity * 2;
		struct hny_dir * const newdirs = realloc(stack->dirs, newcapacity * sizeof (*newdirs));

		if (newdirs == NULL) {
			const int errcode = errno;
			closedir(dirp);
			return errcode;
		}

		stack->dirs = newdirs;
		stack->capacity = newcapacity;
	}

	stack->dirs[newtop].dirp = dirp;
	/* stack->dirs[newtop].current undefined */
	stack->top = newtop;

	return 0;
}

static inline void
hny_dirstack_pop(struct hny_dirstack *stack) {
	closedir(stack->dirs[stack->top].dirp);
	stack->top--;
}

static inline bool
hny_is_dot_or_dot_dot(const char *name) {
	return name[0] == '.' && (name[1] == '\0' || (name[1] == '.' && name[2] == '\0'));
}

static int
hny_remove_package(struct hny *hny, const char *package) {
	struct hny_dirstack stack;
	int errcode;

	/* Stack initialization */
	stack.top = -1;
	stack.capacity = 1;
	stack.dirs = malloc(stack.capacity * sizeof (*stack.dirs));
	if (stack.dirs == NULL) {
		return errno;
	}
	errcode = hny_dirstack_push(&stack, dirfd(hny->dirp), package);

	if (errcode == 0) {
		/* Iteration */
		while (errno = 0, stack.dirs[stack.top].current = readdir(stack.dirs[stack.top].dirp), stack.dirs[stack.top].current != NULL || stack.top != 0) {

			if (stack.dirs[stack.top].current != NULL) {
				/* If we have an entry */
				const struct dirent * const entry = stack.dirs[stack.top].current;
				DIR * const dirp = stack.dirs[stack.top].dirp;

				if (entry->d_type != DT_DIR) {
					/* If the entry is not a directory, unlink it */
					if (unlinkat(dirfd(dirp), entry->d_name, 0) != 0) {
						errcode = errno;
						break;
					}
				} else if (!hny_is_dot_or_dot_dot(entry->d_name)) {
					/* If the entry is an effective directory, push it */
					errcode = hny_dirstack_push(&stack, dirfd(dirp), entry->d_name);
					if (errcode != 0) {
						break;
					}
				}
			} else if (errno == 0) {
				/* Reached directory end, pop it and rmdir it from its parent */
				hny_dirstack_pop(&stack);

				if (unlinkat(dirfd(stack.dirs[stack.top].dirp), stack.dirs[stack.top].current->d_name, AT_REMOVEDIR) != 0) {
					errcode = errno;
					break;
				}
			} else {
				/* Directory read error */
				errcode = errno;
				break;
			}
		}
	}

	/* Stack cleanup */
	while (stack.top > 0) {
		hny_dirstack_pop(&stack);
	}
	free(stack.dirs);

	/* Removing the package directory if everything was removed */
	if (errcode == 0) {
		if (unlinkat(dirfd(hny->dirp), package, AT_REMOVEDIR) != 0) {
			errcode = errno;
		}
	}

	return errcode;
}

int
hny_remove(struct hny *hny, const char *entry) {
	int errcode = 0;

	switch (hny_type_of(entry)) {
	case HNY_TYPE_PACKAGE:
		errcode = hny_remove_package(hny, entry);
		break;
	case HNY_TYPE_GEIST:
		if (unlinkat(dirfd(hny->dirp), entry, 0) != 0) {
			errcode = errno;
		}
		break;
	case HNY_TYPE_NONE:
		errcode = EINVAL;
		break;
	}

	return errcode;
}

