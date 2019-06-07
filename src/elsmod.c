#include <string.h>
#include <stdio.h>
#include <getopt.h>
#include <sysexits.h>
#include <stdint.h>
#include <stdlib.h>

#define MODS_FILE "/proc/modules"
#define MODS_FILE_READ_FMT "%ms %lu %lu %ms %ms 0x%lx"
#define SPACING "%-17"
#define MODS_FILE_PRINT_FMT SPACING"s" SPACING"lu" SPACING"lu" SPACING"s""\n"

struct mod_info{
	char *name;
	uint64_t size;
	uint64_t nr_users;
	char *users;
	char *status;
	uint64_t load_addr;
};


static inline int sort_by_name_asc (struct mod_info *lhs, struct mod_info *rhs)
{
	return strcmp(lhs->name, rhs->name);
}

static inline int sort_by_name_dsc (struct mod_info *lhs, struct mod_info *rhs)
{
	return strcmp(rhs->name, lhs->name);
}

static inline int sort_by_size_asc (struct mod_info *lhs, struct mod_info *rhs)
{
	return lhs->size > rhs->size;
}

static inline int sort_by_size_dsc (struct mod_info *lhs, struct mod_info *rhs)
{
	return rhs->size > lhs->size;
}


static int sort_by_nr_users_asc (struct mod_info *lhs, struct mod_info *rhs)
{
	return lhs->nr_users > rhs->nr_users;
}

static inline int sort_by_nr_users_dsc (struct mod_info *lhs, struct mod_info *rhs)
{
	return rhs->nr_users > lhs->nr_users;
}

int (*sort_mod_info)(struct mod_info *lhs, struct mod_info *rhs) = NULL;

int sort_func(const void *lhs, const void *rhs)
{

	return sort_mod_info((struct mod_info *)lhs,
		(struct mod_info *)rhs);
}

int main(int argc, char ** argv)
{
	int optc = -1;
	struct mod_info *mod_infos = NULL;
	uint64_t nr_mod_infos = 0;
	enum opts {
		SORT_METHOD_SIZE = 1,
		SORT_METHOD_NR_USERS = 2,
		SORT_METHOD_NAME = 4,
		SORT_ORDER_ASC = 8,
		SORT_ORDER_DESC = 16
	};
	int opts_mask = (SORT_METHOD_NAME | SORT_ORDER_ASC);
	uint64_t size_divisor = 1;

	char *usage = "elsmod [OPTIONS...]\n\
OPTIONS\n\
-f x,    sort by field name [n = name, s = size, u = num users] default n\n\
-o x,    order of sorting [a = ascending, d = descending] default a\n\
-u x,    display units of size [b = bytes, k = KB, m = MB] default b\n\
-h  ,    print help and exit\n";

	while((optc = getopt(argc,argv, "f::o::u::h")) != -1){
		switch(optc){
		case 'f':
			if (!optarg)
				break;

			if (!strcmp(optarg, "s"))
				opts_mask |= SORT_METHOD_SIZE;
			else if (!strcmp(optarg, "u"))
				opts_mask |= SORT_METHOD_NR_USERS;
			else if (strcmp(optarg, "n"))
				goto pr_usage_exit;

			break;
		case 'o':
			if (!optarg)
				break;

			if (!strcmp(optarg, "d"))
				opts_mask |= SORT_ORDER_DESC;
			else if (strcmp(optarg, "a"))
				goto pr_usage_exit;

			break;
		case 'u':
			if (!optarg)
				break;

			if (!strcmp(optarg, "k"))
				size_divisor = 1024;
			else if (!strcmp(optarg, "m"))
				size_divisor = 1048576;
			else if (strcmp(optarg, "b"))
				goto pr_usage_exit;
			break;
		case 'h':
			fprintf(stderr, "%s", usage);
			return 0;
		default:
			goto pr_usage_exit;
		}
	}

	if (opts_mask & SORT_METHOD_NAME) {
		sort_mod_info = sort_by_name_asc;
		if (opts_mask & SORT_ORDER_DESC)
			sort_mod_info = sort_by_name_dsc;
	}

	if (opts_mask & SORT_METHOD_SIZE) {
		sort_mod_info = sort_by_size_asc;
		if (opts_mask & SORT_ORDER_DESC)
			sort_mod_info = sort_by_size_dsc;
	}

	if (opts_mask & SORT_METHOD_NR_USERS) {
		sort_mod_info = sort_by_nr_users_asc;
		if (opts_mask & SORT_ORDER_DESC)
			sort_mod_info = sort_by_nr_users_dsc;
	}

	FILE *f = fopen(MODS_FILE, "r");
	if (!f) {
		perror(MODS_FILE);
		return EX_IOERR;
	}

	struct mod_info m = {0};
	while (fscanf(f, MODS_FILE_READ_FMT, &m.name, &m.size,
		&m.nr_users, &m.users, &m.status, &m.load_addr) != EOF) {
		/*TODO i have no clue WTH this entry shows up,
		INVESTIGATE*/
		if (!strcmp(m.name, "(OE)")) {
			free(m.name);
			if(m.status)
				free(m.status);
			if (m.users)
				free(m.users);
			continue;
		}
		mod_infos = realloc(mod_infos, (++nr_mod_infos *
			sizeof(struct mod_info)));
		memset(&mod_infos[nr_mod_infos - 1], 0,
			sizeof(struct mod_info));
		mod_infos [nr_mod_infos - 1] = m;
		memset(&m, 0, sizeof(struct mod_info));
	}
	fclose(f);

	qsort(mod_infos, nr_mod_infos, sizeof(struct mod_info), sort_func);

	fprintf(stdout, SPACING"s" SPACING"s" SPACING"s" SPACING"s""\n",
		"Module", "Size", "NumUsers", "Users");

	for (uint64_t i = 0; i < nr_mod_infos; i++) {
		struct mod_info m = mod_infos[i];
		fprintf(stdout, MODS_FILE_PRINT_FMT, m.name,
			(m.size / size_divisor),
			m.nr_users, m.users);
		free(m.name);
		if (m.users)
			free(m.users);
		if (m.status)
			free(m.status);
	}

	if (mod_infos)
		free(mod_infos);

	return 0;

pr_usage_exit:
	fprintf(stderr, "%s", usage);
	return EX_USAGE;
}
