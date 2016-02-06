/*
 * fileops.h
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

#ifndef _FILEOPS_H
#define _FILEOPS_H
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdarg.h>
#include <getopt.h>
#include <ctype.h>
#include <limits.h>
#include <linux/limits.h>
#include <libgen.h>

#define _GNU_SOURCE 1

typedef struct fdata {
	char *from;
	char *to;
}fdata;

fdata readtextfile(const char *filename, off_t extra, int fatal);
fdata readfile(const char *filename, off_t extra, int fatal);
void writefile(const char *to_write, const char *from, const char *to,
				const char *mode);
FILE *dofopen(const char *fn, const char *fmode);
int direxists(const char *path);
fdata mem2str(char *pfrom, char *pto);
int fileexists(const char *path);
void doread(int fd, size_t bcount, char *result);
void dowrite(int fd, char *writebuf);
int getans(const char *prompt, const char *choices);
int isrunning(char **proglist);
char *gettmpfn(void);
char **readcfg(const char *relpath);
char *readpseudofile(const char *path, const char datatype);
int dostat(const char *fn, struct stat *sb, int fatal);
void *docalloc(size_t nmemb, size_t size, const char *func);
size_t dofread(const char *fn, void *fro, size_t nbytes, FILE *fpi);
char *get_realpath_home(const char *relpath);
void comment_text_to_space(char *from, const char *to);
int count_cfg_data_lines(char *from, char *to);
void set_cfg_lines(char **lines, int numlines, char *from, char *to);
int doopen(const char *fn, const char *mode);

#endif
