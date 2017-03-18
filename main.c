#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <getopt.h>
#include <unistd.h>
#include <git2.h>
#include <dmp.h>

#include "common.h"

#define exit_on_error(x) _exit_on_error(x, __FUNCTION__, __LINE__)
#define ONE_TIME fprintf(stdout, "%s-%d\n", __FUNCTION__, __LINE__); fflush(stdout);

static struct option _long_options[] = {
	{"help", no_argument, 0, 'h'},
	{"repository", required_argument, 0, 'r'},
	{"commits", required_argument, 0, 'c'},
	{"dry-run", no_argument, 0, 'd'},
	{"log", optional_argument, 0, 'l'},
	{0, 0, 0, 0}
};

static struct dm_oids _diff_data = {0};
//static struct dm_oid_data *_diff_data = NULL; 

static void _process_repo_path(struct dm_options *options, char *optarg)
{
	if(optarg[0] == '/') {
		options->repo_path = strdup(optarg);
	} else {
		char *cur_dir = get_current_dir_name();
		asprintf(&options->repo_path, "%s%s", cur_dir, optarg);
	}
	return;
}

static char **_explode(int *argc, char *str, char *exploder)
{
  int count = 0;
  char **argv;
  char *temp_args = strdup(str);
  char *new_temp_args = temp_args;
  char *temp;

  argv = calloc(1, sizeof(char *));
  while((temp = strtok(new_temp_args, exploder))) {
    new_temp_args = NULL;
    argv[count] = strdup(temp);
    count++;
    argv = realloc(argv, (count + 1) * sizeof(char *));
  }

  free(temp_args);
  *argc = count;

  return argv;
}

static void _free_explode(int argc, char **argv)
{
  int ix;
  for(ix = 0; ix < argc; ix++) {
    free(argv[ix]);
  }
  free(argv);
}

static void _print_help(int argc, char **argv)
{
	if(!argc) {
		abort();
	}
	printf("Usage: %s [-r <dir>] [-c <repo ids>] [-d ] [-l [file] ]\n\n", argv[0]);
	printf("Optional Arguments:\n");
	printf("-r, --repository <dir>: Directory of git repository\n");
	printf("-c, --commits <commit ids>: Commit id range to use for diff\n");
	printf("-d, --dry-run: don't write to patch files\n");
	printf("-l, --log[=file]: log everything to a file, optionally provide filename\n");
	return;
}

static void _exit_on_error(int error, const char *func, const int line)
{
	if(!error)
		return;
	const git_error *e = giterr_last();
	printf("%s-%d - Error %d/%d: %s\n", func, line, error, e->klass, e->message);
	exit(error);
}

int diff_output(const git_diff_delta *d, const git_diff_hunk *h,
    const git_diff_line *l, void *p)
{
	FILE *fp = (FILE*)p;

	(void)d; (void)h;

	if (!fp)
		fp = stdout;

	char buf[1024] = {0};
	char oid_str[1024] = {0};
	if(l->origin == GIT_DIFF_LINE_CONTEXT) {
		git_oid_tostr((char *)&oid_str, sizeof(oid_str), &d->old_file.id);
	} else {
		git_oid_tostr((char *)&oid_str, sizeof(oid_str), &d->new_file.id);
	}
	memcpy(&buf, l->content, l->content_len);
	char c = ' ';
	if (l->origin == GIT_DIFF_LINE_CONTEXT ||
	    l->origin == GIT_DIFF_LINE_ADDITION ||
	    l->origin == GIT_DIFF_LINE_DELETION)
		c = l->origin;

	fprintf(fp, "%s: %c%s", oid_str, c, buf);
	//fprintf(fp, "%c%s", c, buf);
	//fwrite(l->content, 1, l->content_len, fp);

	return 0;
}

int git_df_cb(const git_diff_delta *d, float progress, void *payload)
{
	if(_diff_data.length == 0) {
		_diff_data.data = calloc(1, sizeof(struct dm_oid_data));
		_diff_data.data->old_filename = strdup(d->old_file.path);
		_diff_data.data->new_filename = strdup(d->new_file.path);
		git_oid_cpy(&_diff_data.data->old_oid, &d->old_file.id);
		git_oid_cpy(&_diff_data.data->new_oid, &d->new_file.id);
		_diff_data.length++;
	} else {
		int ix = 0;
		bool new_filename = true;
		for(ix = 0; ix < _diff_data.length; ix++) {
			if(streq(_diff_data.data[ix].old_filename, d->old_file.path)) {
				new_filename = false;
				break;
			}
		}
		if(new_filename) {
			ix = _diff_data.length;
			_diff_data.length++;
			_diff_data.data = realloc(_diff_data.data,
			    (_diff_data.length) * sizeof(struct dm_oid_data));
			_diff_data.data[ix].old_filename = strdup(d->old_file.path);
			_diff_data.data[ix].new_filename = strdup(d->new_file.path);
			git_oid_cpy(&_diff_data.data[ix].old_oid, &d->old_file.id);
			git_oid_cpy(&_diff_data.data[ix].new_oid, &d->new_file.id);
		}
	}

	return 0;
}

int main(int argc, char **argv)
{
	int c;
	struct dm_options options = {0};
	while(true) {
		int option_index = 0;
		c = getopt_long(argc, argv, "hd:l::r:c:", _long_options, &option_index);
		if(c == -1) {
			break;
		}
		switch(c) {
			case 'h':
				_print_help(argc, argv);
				return 0;
			case 'd':
				options.dry_run = true;
				break;
			case 'l':
				options.log = true;
				if(optarg) {
					options.log_filename = strdup(optarg);
				}
				break;
			case 'r':
				_process_repo_path(&options, optarg);
				break;
			case 'c':
				options.commits = strdup(optarg);
				break;
			case '?':
				_print_help(argc, argv);
				return 1;
			default:
				break;
		}
	}
	if(!options.repo_path) {
		options.repo_path = strdup(".");
	}
	if(!options.commits) {
		printf("Error: commit range is required\n");
		return ERROR;
	}
	git_libgit2_init();

	git_oid oid;
	git_repository *repo = NULL;
	git_commit *commit = NULL;
	git_revwalk *walker;
	git_diff *diff;
	git_tree *from, *to;

	int error;
	int idx;

	error = git_repository_open(&repo, options.repo_path);
	exit_on_error(error);

	//error = git_diff_index_to_workdir(&diff, repo, NULL, NULL);
	int commit_ids_num = 0;
	char **commit_ids = _explode(&commit_ids_num, options.commits, "..");

	git_oid cmt1, cmt2;
	git_commit *gcmt1, *gcmt2;
	error = git_oid_fromstr(&cmt1, commit_ids[0]);
	exit_on_error(error);
	error = git_oid_fromstr(&cmt2, commit_ids[1]);
	exit_on_error(error);

	error = git_commit_lookup(&gcmt1, repo, &cmt1);
	exit_on_error(error);
	error = git_commit_lookup(&gcmt2, repo, &cmt2);
	exit_on_error(error);

	error = git_commit_tree(&from, gcmt1);
	exit_on_error(error);
	error = git_commit_tree(&to, gcmt2);
	exit_on_error(error);
#if 0
	error = git_tree_lookup(&from, repo, &cmt1);
	exit_on_error(error);
	error = git_tree_lookup(&to, repo, &cmt2);
	exit_on_error(error);
#endif

	error = git_diff_tree_to_tree(&diff, repo, from, to, NULL);
	exit_on_error(error);

#if 0
	error = git_diff_print(diff, GIT_DIFF_FORMAT_PATCH, diff_output, NULL);
	exit_on_error(error);
#endif

	error = git_diff_foreach(diff, git_df_cb, NULL, NULL, NULL, NULL);
	exit_on_error(error);

	for(idx = 0; idx < _diff_data.length; idx++) {
		printf("old filename: %s\n", _diff_data.data[idx].old_filename);
		printf("new filename: %s\n", _diff_data.data[idx].new_filename);
		git_blob *old_blob, *new_blob;
		bool no_old = false, no_new = false;
		error = git_blob_lookup(&old_blob, repo, &_diff_data.data[idx].old_oid);
		if(error) {
			no_old = true;
		}
		//exit_on_error(error);
		error = git_blob_lookup(&new_blob, repo, &_diff_data.data[idx].new_oid);
		if(error) {
			no_new = true;
		}
#if 0		
		if(error) {
			git_oid work_oid;
			//error = git_blob_create_fromworkdir(&work_oid, repo, "main.c");
			error = git_blob_create_fromworkdir(&work_oid, repo, _diff_data.data[idx].new_filename);
			error = git_blob_lookup(&new_blob, repo, &work_oid);
			exit_on_error(error);
		}
#endif
		char *old_file, *new_file;
		if(no_old) {
			old_file = strdup("");
		} else {
			old_file = strdup(git_blob_rawcontent(old_blob));
		}

		if(no_new) {
			new_file = strdup("");
		} else {
			new_file = strdup(git_blob_rawcontent(new_blob));
		}

		char *write_file;
		asprintf(&write_file, "%s/%s.patch", options.repo_path,
		    _diff_data.data[idx].new_filename);
		FILE *out = fopen(write_file, "w");

		dmp_diff *dmp_df = NULL;
		dmp_diff_from_strs(&dmp_df, NULL, old_file, new_file);
		dmp_diff_print_patch(out, dmp_df);
		dmp_diff_free(dmp_df);
		fclose(out);
		free(write_file);

	}

#if 0
	//printf("old: %s\n", od.old);
	//printf("new: %s\n", od.new);
	
	for(idx = 0; idx < _diff_data.length; idx++) {
		git_blob *old_blob, *new_blob;
		error = git_blob_lookup(&old_blob, repo, &od.old_oid);
		exit_on_error(error);
	//printf("%s\n", git_blob_rawcontent(old_blob));
		error = git_blob_lookup(&new_blob, repo, &od.new_oid);
		if(error) {
			git_oid work_oid;
			error = git_blob_create_fromworkdir(&work_oid, repo, "main.c");
		}
	error = git_blob_lookup(&new_blob, repo, &od.new_oid);
	exit_on_error(error);

	char *old_file = strdup(git_blob_rawcontent(old_blob));
	char *new_file = strdup(git_blob_rawcontent(new_blob));

	dmp_diff *dmp_df = NULL;
	dmp_diff_from_strs(&dmp_df, NULL, old_file, new_file);
	dmp_diff_print_patch(stdout, dmp_df);
	dmp_diff_free(dmp_df);
#endif
	return 0;
}
