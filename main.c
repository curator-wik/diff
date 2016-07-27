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
	git_revwalk *walker;
	char oid_str[1024] = {0};
	int error;

	error = git_repository_open(&repo, ".");
	exit_on_error(error);

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

	return 0;
}
