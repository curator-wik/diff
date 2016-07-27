#include <stdio.h>
#include <git2.h>

int main(int argc, char **argv)
{
	git_libgit2_init();
	
	git_repository *repo = NULL;
	int error = git_repository_open(&repo, ".");
	if (error < 0) {
		const git_error *e = giterr_last();
		printf("Error %d/%d: %s\n", error, e->klass, e->message);
		exit(error);
	}

	printf("Hello World\n");
}
