/* $Id$ */
/* Copyright (c) 2015-2022 Pierre Pronchery <khorben@defora.org> */
/* This file is part of DeforaOS Desktop Locker */
/* Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY ITS AUTHORS AND CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. */



#include <dirent.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dlfcn.h>
#include "Locker.h"

/* constants */
#define PROGNAME_AUTH	"auth"


/* private */
/* prototypes */
static int _auth(void);

static int _dlerror(char const * message, int ret);
static int _error(char const * message, char const * error, int ret);
static int _perror(char const * message, int ret);


/* functions */
/* applets */
static int _auth(void)
{
	int ret = 0;
	const char path[] = "../src/auth";
#ifdef __APPLE__
	const char ext[] = ".dylib";
#else
	const char ext[] = ".so";
#endif
	DIR * dir;
	struct dirent * de;
	size_t len;
	char * s;
	void * p;
	LockerAuthDefinition * lad;

	if((dir = opendir(path)) == NULL)
		return -_perror(path, 1);
	while((de = readdir(dir)) != NULL)
	{
		if((len = strlen(de->d_name)) < sizeof(ext))
			continue;
		if(strcmp(&de->d_name[len - sizeof(ext) + 1], ext) != 0)
			continue;
		if((s = malloc(sizeof(path) + len + 1)) == NULL)
		{
			ret += _perror(de->d_name, 1);
			continue;
		}
		snprintf(s, sizeof(path) + len + 1, "%s/%s", path, de->d_name);
		if((p = dlopen(s, RTLD_LAZY)) == NULL)
		{
			ret += _dlerror(s, 1);
			free(s);
			continue;
		}
		if((lad = dlsym(p, "plugin")) == NULL)
			ret += _dlerror(s, 1);
		else
		{
			if(lad->icon == NULL)
				ret += _error(s, "No icon defined", 1);
			else
				printf("\n%s: %s (%s)\n%s\n", de->d_name,
						lad->name, lad->icon,
						(lad->description != NULL)
						? lad->description
						: "(no description)");
			if(lad->get_widget == NULL)
				ret += _error(s, "No widget available", 1);
			if(lad->action == NULL)
				ret += _error(s, "No action handler", 1);
		}
		free(s);
		dlclose(p);
	}
	closedir(dir);
	return ret;
}


/* dlerror */
static int _dlerror(char const * message, int ret)
{
	fputs(PROGNAME_AUTH ": ", stderr);
	fprintf(stderr, "%s: %s\n", message, dlerror());
	return ret;
}


/* error */
static int _error(char const * message, char const * error, int ret)
{
	fputs(PROGNAME_AUTH ": ", stderr);
	fprintf(stderr, "%s: %s\n", message, error);
	return ret;
}


/* perror */
static int _perror(char const * message, int ret)
{
	fputs(PROGNAME_AUTH ": ", stderr);
	perror(message);
	return ret;
}


/* public */
/* functions */
/* main */
int main(void)
{
	return (_auth() == 0) ? 0 : 2;
}
