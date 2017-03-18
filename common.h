#ifndef COMMON_H
#define COMMON_H

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <git2.h>

#define streq !strcmp
#define ERROR -1
#define SUCCESS 0

struct dm_oid_data {
	char *old_filename;
	char *new_filename;
	char *old_data;
	git_oid old_oid;
	char *new_data;
	git_oid new_oid;
};

struct dm_oids {
	struct dm_oid_data *data;
	int32_t length;
};

struct dm_options {
	bool dry_run;
	bool log;
	char *log_filename;
	char *repo_path;
	char *commits;
};

#endif
