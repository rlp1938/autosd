/*      autosd.c
 *
 *	Copyright 2016 Bob Parker rlp1938@gmail.com
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the Free Software
 *	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *	MA 02110-1301, USA.
*/


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <stdint.h>
#include "fileops.h"
#include "firstrun.h"
#include "getoptions.h"

typedef struct cfgdata {
	char cfgname[128];
	char cfgval[32];
} cfgdata;

typedef struct cfgprm {
	int batquit;	// battery % quit level
	int batmon;		// battery % to start monitoring
	int interval;	// when monitoring check interval minutes.
} cfgprm;

static cfgdata split_cfg_line(const char *cfgline);
static int inlist(const char *candidate, char **list);
static void freelist(char **list);
static void suicide(void);
static void is_this_first_run(char *progname);
static void check_prior_instance_running(char *progname);
static cfgprm get_config_parameters(const char *relpath);
static void sanity_check(int what, int lt, int gt, const char *thename);
static void check_power_status(int monitor, cfgprm prms);

int main(int argc, char **argv)
{
	options_t opts = process_options(argc, argv);
	is_this_first_run("autosd");
	check_prior_instance_running("autosd");
	cfgprm prms = get_config_parameters(".config/autosd/autosd.cfg");
	check_power_status(opts.monitor, prms);

	return 0;
}//main()

cfgdata split_cfg_line(const char *cfgline)
{
	cfgdata retval;
	memset (retval.cfgname, 0, 128);
	memset (retval.cfgval, 0, 32);
	char buf[NAME_MAX];
	strcpy(buf, cfgline);
	char *equals = strchr(buf, '=');
	if (!equals) {
		fprintf(stderr, "Corrupt config line: %s\n", cfgline);
		exit(EXIT_FAILURE);
	}
	*equals = '\0';
	strcpy(retval.cfgname, buf);
	equals++;
	while(isspace(*equals)) equals++;	// get past leading ' '
	strcpy(retval.cfgval, equals);
	// remove trailing ' '
	char *cp = &retval.cfgname[strlen(retval.cfgname)-1];
	while(isspace(*cp)) cp--;
	cp = &retval.cfgval[strlen(retval.cfgval)-1];
	while(isspace(*cp)) cp--;
	return retval;
} // split_cfg_line()

int inlist(const char *candidate, char **list)
{
	int idx = 0;
	while (list[idx]) {
		if (strcmp(candidate, list[idx]) == 0) return idx;
		idx++;
	}
	return -1;
} // inlist()

void freelist(char **list)
{	/* frees a NULL terminated list - the list items must have been
	malloc'd in some way.
	*/
	int lidx = 0;
	while (list[lidx]) {
		free(list[lidx]);
		lidx++;
	}
	free(list);
} // freelist()

void suicide(void)
{	/*
	* Shut myself down - and yes I need to be running as root to do it.
	*/
	char *command = "shutdown -h now";
	printf("fake shutdown.\n");
}

void is_this_first_run(char *progname)
{
	if (checkfirstrun(progname)) {
		// drop root priviledge first
		firstrun("autosd", "autosd.cfg", NULL);
		printf("A configuration file 'autosd.cfg' has been installed "
		"at $HOME/.config/autosd/. \nPlease edit this file to meet your"
		" requirements.\n");
		exit(EXIT_SUCCESS);
	}
} // is_this_first_run()

static void check_prior_instance_running(char *progname)
{
	char *prlist[2] = { progname, NULL };
	int res = isrunning(prlist);
	if (res > 1) {
		exit(EXIT_SUCCESS);
	}
} // check_prior_instance_running()

static cfgprm get_config_parameters(const char *relpath)
{
	char **cflines = readcfg(relpath);
	cfgprm prms;
	int cflidx = 0;
	while (cflidx < 3) {
		char *list[4] = {"check_interval", "monitor_level", "quit_level"
							,(char *)NULL };
		cfgdata cd = split_cfg_line(cflines[cflidx]);
		int res = inlist(cd.cfgname, list);
		switch (res)
		{
			case 0:
				prms.interval = strtol(cd.cfgval, NULL, 10);
				sanity_check(prms.interval, 1, 8*60, "check_interval");
				prms.interval *= 60; // given in min., but I want secs.
				break;
			case 1:
				prms.batmon = strtol(cd.cfgval, NULL, 10);
				sanity_check(prms.batmon, 10, 100, "monitor_level");
				break;
			case 2:
				prms.batquit = strtol(cd.cfgval, NULL, 10);
				sanity_check(prms.batquit, 1, 100, "quit_level");
				break;
			default:
				fprintf(stderr, "Unknown parameter name in config file:"
						" %s\n", cd.cfgname);
				exit(EXIT_FAILURE);
				break;
		} // switch(res)
		cflidx++;
	} // while()
	freelist(cflines);
	return prms;
} // get_config_parameters()

void sanity_check(int what, int lt, int gt, const char *thename)
{
	char *fmt = "Insane value for '%s' in config file.\n";
	if (what < lt || what > gt) {
		fprintf(stderr, fmt, thename);
		exit(EXIT_FAILURE);
	}
} // sanity_check()

static void check_power_status(int monitor, cfgprm prms)
{
	const char *ac0 = "/sys/class/power_supply/AC0/online";
	const char *bat0 = "/sys/class/power_supply/BAT0/capacity";
	char *sysbuf = readpseudofile(ac0, 'c');
	int poweroff = (sysbuf[0] == '0');
	sysbuf = readpseudofile(bat0, 's');
	int percent = strtol(sysbuf, NULL, 10);	// % battery charge
	while (poweroff) {
		if (percent < prms.batquit) suicide();
		if (percent > prms.batmon && !monitor) {
			return;	// back to cron
		}
		if (monitor) {
			fprintf(stdout, "Battery percentage: %d\n", percent);
		}
		sleep(prms.interval);
		sysbuf = readpseudofile(bat0, 's');
		percent = strtol(sysbuf, NULL, 10);
		sysbuf = readpseudofile(ac0, 'c');
		poweroff = (sysbuf[0] == '0');
	} // while(poweroff)
} // check_power_status()
