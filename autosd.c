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
	char cfgname[NAME_MAX];
	char cfgval[NAME_MAX];
} cfgdata;

typedef struct cfgprm {
	int batquit;	// battery % quit level
	int batmon;		// battery % to start monitoring
	int interval;	// when monitoring check interval minutes.
} cfgprm;

static cfgdata split_cfg_line(char *cfgline);
static int inlist(const char *candidate, char **list);
static void freelist(char **list);
static void suicide(void);
static void is_this_first_run(char *progname);
static void check_prior_instance_running(char *progname);
static cfgprm get_config_parameters(const char *relpath);
static void sanity_check(int what, int lt, int gt, const char *thename);
static void check_power_status(int monitor, cfgprm prms);
static void check_set_config_values(int res, cfgprm *prms, cfgdata cd);
static void get_cfg_name(char *src, char *name, const char sep);
static void get_cfg_value(char *src, char *val, const char sep);
static void strip_space(char *buf);

int main(int argc, char **argv)
{
	options_t opts = process_options(argc, argv);
	is_this_first_run("autosd");
	check_prior_instance_running("autosd");
	cfgprm prms = get_config_parameters(".config/autosd/autosd.cfg");
	check_power_status(opts.monitor, prms);

	return 0;
}//main()

cfgdata split_cfg_line(char *cfgline)
{
	cfgdata retval;
	get_cfg_name(cfgline, retval.cfgname, '=');
	get_cfg_value(cfgline, retval.cfgval, '=');
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
{	/* Shut myself down - must be root. */
	char *command = "dbus-send --system "
	"--dest=org.freedesktop.ConsoleKit --type=method_call --print-reply"
	" --reply-timeout=2000 /org/freedesktop/ConsoleKit/Manager "
	"org.freedesktop.ConsoleKit.Manager.Stop";
	int res = system(command);
	if (res == -1) {
		fprintf(stderr, "Command failed: %s\n", command);
		exit(EXIT_FAILURE);
	}

} // suicide()

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
		check_set_config_values(res, &prms, cd);
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

void check_set_config_values(int res, cfgprm *prms, cfgdata cd)
{
	switch (res)
	{
		case 0:
			prms->interval = strtol(cd.cfgval, NULL, 10);
			sanity_check(prms->interval, 1, 8*60, "check_interval");
			prms->interval *= 60; // given in min., but I want secs.
			break;
		case 1:
			prms->batmon = strtol(cd.cfgval, NULL, 10);
			sanity_check(prms->batmon, 10, 100, "monitor_level");
			break;
		case 2:
			prms->batquit = strtol(cd.cfgval, NULL, 10);
			sanity_check(prms->batquit, 1, 100, "quit_level");
			break;
		default:
			fprintf(stderr, "Unknown parameter name in config file:"
					" %s\n", cd.cfgname);
			exit(EXIT_FAILURE);
			break;
	} // switch(res)
} // check_set_config_values()

void get_cfg_name(char *src, char *name, const char sep)
{
	char buf[NAME_MAX];
	char *begin = src;
	char *end = strchr(src, sep);
	size_t nlen;
	if (!end) {
		fprintf(stderr, "No valid separator in 'get_cfg_name()': %c\n",
				sep);
		exit(EXIT_FAILURE);
	}
	nlen = end - begin;
	if (nlen > NAME_MAX - 1) {
		fprintf(stderr, "String too large in 'get_cfg_name()': %lu\n",
				nlen);
		exit(EXIT_FAILURE);
	}
	strncpy(buf, begin, nlen);
	buf[nlen] = '\0';
	strip_space(buf);
	strcpy(name, buf);
} // get_cfg_name()

void get_cfg_value(char *src, char *val, const char sep)
{
	char buf[NAME_MAX];
	char *begin = strchr(src, sep);
	if (!begin) {
		fprintf(stderr, "No valid separator in 'get_cfg_value()': %c\n",
				sep);
		exit(EXIT_FAILURE);
	}
	begin++;	// get past sep
	char *end = begin + strlen(src) - 1;
	size_t nlen;
	nlen = end - begin;
	if (nlen > NAME_MAX - 1) {
		fprintf(stderr, "String too large in 'get_cfg_value()': %lu\n",
				nlen);
		exit(EXIT_FAILURE);
	}
	strncpy(buf, begin, nlen);
	buf[nlen] = '\0';
	strip_space(buf);
	strcpy(val, buf);
} // get_cfg_value()

static void strip_space(char *buf)
{
	char *target = strdup(buf);
	char *begin = target;
	while (isspace(*begin)) begin++;
	char *end = target + strlen(target) - 1;
	while (isspace(*end)) end--;
	size_t len = end - begin + 1;
	strncpy(buf, begin, len);
	buf[len] = '\0';
	free(target);
} // strip_space()
