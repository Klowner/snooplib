// easily snoop all file open() calls
// Mark Riedesel <mark@klowner.com>

#define _GNU_SOURCE
#include <string.h>
#include <dlfcn.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define NUM_NODESET_NODES 128

static void destructor() __attribute__((destructor));

typedef int (*orig_open_fn)(const char *pathname, int flags);

struct tree_node_t {
	const char *name;
	unsigned long hash;
	struct tree_node_t *next;
	struct tree_nodeset_t *children;
};

struct tree_nodeset_t {
	struct tree_node_t *node[NUM_NODESET_NODES];
};


static struct tree_node_t *s_root_node = NULL;
static pthread_mutex_t s_mutex = PTHREAD_MUTEX_INITIALIZER;

static void tree_nodeset_free(struct tree_nodeset_t *nodeset);
static void tree_node_free(struct tree_node_t *node);


static unsigned long sdbm(const char *str, unsigned int len) {
	unsigned long hash = 0;
	int c;

	while ((c = *str++) && len--) {
		hash = c + (hash << 6) + (hash << 16) - hash;
	}

	return hash;
}


static unsigned long path_head_hash(const char *path, unsigned int *length) {
	const char *next_slash = strpbrk(path + 1, "\\/\0");
	int head_length = 0;

	if (next_slash != NULL) {
		head_length = next_slash - path - 1;
	} else {
		head_length = strlen(path) - 1;
	}

	if (length) {
		*length = head_length > 0 ? head_length : 0;
	}
	return sdbm(path + 1, head_length);
}


static struct tree_node_t* tree_node_create() {
	return (struct tree_node_t*)calloc(1, sizeof(struct tree_node_t));
}


static struct tree_nodeset_t* tree_nodeset_create() {
	return (struct tree_nodeset_t*)calloc(1, sizeof(struct tree_nodeset_t));
}


static void tree_nodeset_free(struct tree_nodeset_t *nodeset) {
	for (unsigned int i = 0; i < NUM_NODESET_NODES; i++) {
		if (nodeset->node[i]) {
			tree_node_free(nodeset->node[i]);
		}
	}

	free(nodeset);
}


static void tree_node_free(struct tree_node_t *node) {
	if (node->children) {
		tree_nodeset_free(node->children);
		node->children = NULL;
	}

	if (node->next) {
		tree_node_free(node->next);
		node->next = NULL;
	}

	free((void*)node->name);
	free(node);
}


static struct tree_node_t* tree_nodeset_add(struct tree_node_t *node, const unsigned long hash, const char *path, const unsigned int len) {
	struct tree_nodeset_t *nodeset;
	struct tree_node_t **insert = NULL;
	char *node_name = strndup(path + 1, len);

	// Ensure there's a set of nodes attached to the current node
	if (node->children == NULL) {
		node->children = tree_nodeset_create();
	}

	nodeset = node->children;

	// Determine node insertion point.
	insert = &nodeset->node[hash % NUM_NODESET_NODES];

	// If a node exists at this position...
	while (*insert != NULL) {
		if ((*insert)->hash == hash && strcmp((*insert)->name, node_name) == 0) {
			// If the current insert node exists and has an identical
			// hash AND string value then return this node.
			free(node_name);
			return *insert;
		} else {
			// Otherwise there was a hash collision, so check
			// the next node for a match if it's available.
			insert = &(*insert)->next;
		}
	}

	// No matches were found, insert the new node.
	*insert = tree_node_create();
	(*insert)->name = node_name;
	(*insert)->hash = hash;
	return *insert;
}


static void tree_node_addpath(struct tree_node_t *node, const char *path) {
	const char *p = path;
	struct tree_node_t *n = node;

	while (*p) {
		unsigned int len;
		unsigned long hash = path_head_hash(p, &len);
		n = tree_nodeset_add(n, hash, p, len);
		p = p + len + 1;
	}
}


static void tree_node_dump(FILE *fd, struct tree_node_t *node, const char *path) {
	char *abspath = NULL;

	if (path) {
		abspath = calloc(1, strlen(path) + strlen(node->name) + 2);
		strcpy(abspath, path);
		strcat(abspath, "/");
		strcat(abspath, node->name);
	} else {
		abspath = strdup("");
	}


	if (node->children) {
		for (int i = 0; i < NUM_NODESET_NODES; i++) {
			struct tree_node_t *child = node->children->node[i];
			while (child) {
				tree_node_dump(fd, child, abspath);
				child = child->next;
			}
		}
	} else {
		fprintf(fd, "%s\n", abspath);
	}

	free(abspath);
}


static void record(const char *pathname) {
	pthread_mutex_lock(&s_mutex);
	if (!s_root_node) {
		s_root_node = tree_node_create();
		s_root_node->name = strdup("");
	}
	tree_node_addpath(s_root_node, pathname);
	pthread_mutex_unlock(&s_mutex);
}


static orig_open_fn get_orig_open() {
	static orig_open_fn fn = NULL;
	if (!fn) {
		fn = (orig_open_fn)dlsym(RTLD_NEXT, "open");
	}
	return fn;
}

static void destructor() {
	if (s_root_node == NULL) {
		return;
	}

	const char *file_path = getenv("SNOOPLIB_OUTPUT_PATH");
	FILE *fd = NULL;

	if (file_path) {
		fd = fopen(file_path, "w");
		pthread_mutex_lock(&s_mutex);
		tree_node_dump(fd, s_root_node, NULL);
		pthread_mutex_unlock(&s_mutex);
		fclose(fd);
	} else {
		pthread_mutex_lock(&s_mutex);
		fprintf(stdout, "snooplib hint: set SNOOPLIB_OUTPUT_PATH\n");
		fprintf(stdout, "---SNOOPLIB BEGIN---\n");
		tree_node_dump(stdout, s_root_node, NULL);
		fprintf(stdout, "---SNOOPLIB END---\n");
		pthread_mutex_unlock(&s_mutex);
	}

	tree_node_free(s_root_node);
	s_root_node = NULL;
}


int open(const char *pathname, int flags, ...) {
	record(pathname);
	orig_open_fn orig_open = get_orig_open();
	return orig_open(pathname, flags);
}
