/*
 * getoptions.c
 * Copyright 2016 Bob Parker <rlp1938@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
*/

#include "getoptions.h"

static const char helpmsg[] =
  "\tUsage: autosd [option]\n"
  "\tNB on first run a file, 'autosd.cfg' is installed at "
  "$HOME/.config/autosd/.\n"

  "\n\tOptions:\n"
  "\t-h, --help\n\tDisplays this help message, then quits.\n"
  "\t-m, --monitor\n"
  "\t prints stats when running off battery so as to help choose the"
  " best\n\tparameters in the config file. \n"
  ;

options_t
process_options(int argc, char **argv)
{

	static const char optstr[] = ":hm";

	options_t opts = { 0 };

	int opt;

	while (1) {
		int this_option_optind = optind ? optind : 1;
		int option_index = 0;
		static struct option long_options[] = {
			{"help", 0,	0,	'h' },
			{"monitor",	0,	0,	'm'},
			{0,	0,	0,	0 }
		};

		opt = getopt_long(argc, argv, optstr, long_options,
							&option_index);
		if (opt == -1)
			break;
		switch (opt) {
			case 0:
				switch (option_index) {
				} // switch(option_index)
				break;
			case 'h':
				dohelp(0);
				break;
			case 'm':
				opts.monitor = 1;
				break;
			case ':':
				fprintf(stderr, "Option %s requires an argument\n",
							argv[this_option_optind]);
				dohelp(1);
				break;
			case '?':
				fprintf(stderr, "Unknown option: %s\n",
								argv[this_option_optind]);
				dohelp(1);
				break;
		}

	} // while(1)
	return opts;
} // process_options()

void dohelp(int forced)
{
  fputs(helpmsg, stderr);
  exit(forced);
}
