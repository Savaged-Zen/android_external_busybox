/* vi: set sw=4 ts=4: */
/*
 * uniq implementation for busybox
 *
 * Copyright (C) 2005  Manuel Novoa III  <mjn3@codepoet.org>
 *
 * Licensed under the GPL v2 or later, see the file LICENSE in this tarball.
 */

/* BB_AUDIT SUSv3 compliant */
/* http://www.opengroup.org/onlinepubs/007904975/utilities/uniq.html */

#include "libbb.h"

int uniq_main(int argc, char **argv) MAIN_EXTERNALLY_VISIBLE;
int uniq_main(int argc UNUSED_PARAM, char **argv)
{
	const char *input_filename;
	unsigned skip_fields, skip_chars, max_chars;
	unsigned opt;
	char *cur_line;
	const char *cur_compare;

	enum {
		OPT_c = 0x1,
		OPT_d = 0x2, /* print only dups */
		OPT_u = 0x4, /* print only uniq */
		OPT_f = 0x8,
		OPT_s = 0x10,
		OPT_w = 0x20,
	};

	skip_fields = skip_chars = 0;
	max_chars = INT_MAX;

	opt_complementary = "f+:s+:w+";
	opt = getopt32(argv, "cduf:s:w:", &skip_fields, &skip_chars, &max_chars);
	argv += optind;

	input_filename = argv[0];
	if (input_filename) {
		const char *output;

		if (input_filename[0] != '-' || input_filename[1]) {
			close(STDIN_FILENO); /* == 0 */
			xopen(input_filename, O_RDONLY); /* fd will be 0 */
		}
		output = argv[1];
		if (output) {
			if (argv[2])
				bb_show_usage();
			if (output[0] != '-' || output[1]) {
				// Won't work with "uniq - FILE" and closed stdin:
				//close(STDOUT_FILENO);
				//xopen3(output, O_WRONLY | O_CREAT | O_TRUNC, 0666);
				xmove_fd(xopen3(output, O_WRONLY | O_CREAT | O_TRUNC, 0666), STDOUT_FILENO);
			}
		}
	}

	cur_compare = cur_line = NULL; /* prime the pump */

	do {
		unsigned i;
		unsigned long dups;
		char *old_line;
		const char *old_compare;

		old_line = cur_line;
		old_compare = cur_compare;
		dups = 0;

		/* gnu uniq ignores newlines */
		while ((cur_line = xmalloc_fgetline(stdin)) != NULL) {
			cur_compare = cur_line;
			for (i = skip_fields; i; i--) {
				cur_compare = skip_whitespace(cur_compare);
				cur_compare = skip_non_whitespace(cur_compare);
			}
			for (i = skip_chars; *cur_compare && i; i--) {
				++cur_compare;
			}

			if (!old_line || strncmp(old_compare, cur_compare, max_chars)) {
				break;
			}

			free(cur_line);
			++dups;	 /* testing for overflow seems excessive */
		}

		if (old_line) {
			if (!(opt & (OPT_d << !!dups))) { /* (if dups, opt & OPT_u) */
				if (opt & OPT_c) {
					/* %7lu matches GNU coreutils 6.9 */
					printf("%7lu ", dups + 1);
				}
				printf("%s\n", old_line);
			}
			free(old_line);
		}
	} while (cur_line);

	die_if_ferror(stdin, input_filename);

	fflush_stdout_and_exit(EXIT_SUCCESS);
}
