#include <stdio.h>
#include <git2.h>

#define exit_on_error(x) _exit_on_error(x, __FUNCTION__, __LINE__)

void _exit_on_error(int error, const char *func, const int line)
{
	if(!error)
		return;
	const git_error *e = giterr_last();
	printf("%s-%d - Error %d/%d: %s\n", func, line, error, e->klass, e->message);
	exit(error);
}

int main(int argc, char **argv)
{
	git_libgit2_init();
	git_oid oid;
	git_repository *repo = NULL;
	git_commit *commit = NULL;
	int error = git_repository_open(&repo, ".");
	exit_on_error(error);

	git_revwalk *walker;
	error = git_revwalk_new(&walker, repo);
	exit_on_error(error);
	error = git_revwalk_push_head(walker);
	while(!git_revwalk_next(&oid, walker)) {
		error = git_commit_lookup(&commit, repo, &oid);
		exit_on_error(error);
		const char *msg = git_commit_message(commit);
		printf("%s\n", msg);
	}

	return 0;
}
