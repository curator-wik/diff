#include <stdio.h>
#include <string.h>
#include <git2.h>

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

static int _diff_stuff(const git_diff_delta *delta, const git_diff_hunk *hunk,
    const git_diff_line *line, void *payload)
{
	char buf[line->content_len + 1];
	memset(&buf, 0, line->content_len + 1);
	//printf("%d\n", line->content_len);
	//int offset = line->content_offset > 0 ? line->content_offset : 0;
	strncpy(buf, line->content, line->content_len);
	if(line->origin == 'H') {
		printf("%s\n", buf);
	} else {
		printf("%c %s", line->origin, buf);
	}
#if 0
	if(hunk && hunk->header) {
		printf("%s", hunk->header);
	}
#endif
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

	error = git_diff_print(diff, GIT_DIFF_FORMAT_PATCH, _diff_stuff, NULL);
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
