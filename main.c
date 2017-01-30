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

	//fprintf(fp, "%s: %c%s", oid_str, c, buf);
	fprintf(fp, "%c%s", c, buf);
	//fwrite(l->content, 1, l->content_len, fp);

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


	return 0;
}
