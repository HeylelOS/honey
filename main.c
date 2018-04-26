#include <hny.h>

int
main(int argc,
	char **argv) {
	const char *name = "honey";
	struct hny_geist *geist = hny_alloc_geister(&name, 1);

	hny_t hny = hny_create("prefix/");

	hny_execute(hny, HnyActionSetup, geist);

	hny_destroy(hny);

	hny_free_geister(geist, 1);

	return 0;
}
