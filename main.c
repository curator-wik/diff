#include <stdio.h>
#include <string.h>
#include <git2.h>
#include <dmp.h>

#define exit_on_error(x) _exit_on_error(x, __FUNCTION__, __LINE__)
#define ONE_TIME fprintf(stdout, "%s-%d\n", __FUNCTION__, __LINE__); fflush(stdout);

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

struct oid_data {
	char old[1024];
	git_oid old_oid;
	char new[1024];
	git_oid new_oid;
};

int git_df_cb(const git_diff_delta *d, float progress, void *payload)
{
	struct oid_data *od = (struct oid_data *)payload;
	char oid_str[1024] = {0};
	git_oid_tostr((char *)&oid_str, sizeof(oid_str), &d->old_file.id);
	sprintf(od->old, "%s", oid_str);
	git_oid_tostr((char *)&oid_str, sizeof(oid_str), &d->new_file.id);
	sprintf(od->new, "%s", oid_str);
	//printf("%s\n", oid_str);
	git_oid_cpy(&od->old_oid, &d->old_file.id);
	git_oid_cpy(&od->new_oid, &d->new_file.id);
	return 0;
}

int main(int argc, char **argv)
{
	git_libgit2_init();

	git_oid oid;
	git_repository *repo = NULL;
	git_commit *commit = NULL;
	git_revwalk *walker;
	git_diff *diff;
	char oid_str[1024] = {0};
	int error;

	error = git_repository_open(&repo, ".");
	exit_on_error(error);

	error = git_diff_index_to_workdir(&diff, repo, NULL, NULL);
	exit_on_error(error);

	error = git_diff_print(diff, GIT_DIFF_FORMAT_PATCH, diff_output, NULL);
	exit_on_error(error);


	struct oid_data od = {0};
	error = git_diff_foreach(diff, git_df_cb, NULL, NULL, NULL, &od);
	exit_on_error(error);

	printf("old: %s\n", od.old);
	printf("new: %s\n", od.new);

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
#if 0
	error = git_revwalk_new(&walker, repo);
	exit_on_error(error);

	error = git_revwalk_push_head(walker);
	exit_on_error(error);

	while(!git_revwalk_next(&oid, walker)) {
		error = git_commit_lookup(&commit, repo, &oid);
		exit_on_error(error);
		git_oid_tostr((char *)&oid_str, sizeof(oid_str), &oid);
		const char *msg = git_commit_message(commit);
		printf("oid: %s | message: %s\n", oid_str, msg);
	}
#endif

	dmp_diff *dmp_df = NULL;
	dmp_options opts;
	dmp_options_init(&opts);
	opts.match_threshold = 1;
	opts.patch_margin = 8;
	dmp_diff_from_strs(&dmp_df, &opts, old_file, new_file);
	dmp_diff_print_raw(stdout, dmp_df);
	dmp_diff_free(dmp_df);
	return 0;
}
