/*
 * Copy me if you can.
 * by 20h
 */
#define _BSD_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <strings.h>
#include <sys/time.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <X11/Xlib.h>

char *time_zone = "Canada/Pacific";

static Display *dpy;

char *
smprintf(char *fmt, ...)
{
	va_list fmtargs;
	char *ret;
	int len;

	va_start(fmtargs, fmt);
	len = vsnprintf(NULL, 0, fmt, fmtargs);
	va_end(fmtargs);

	ret = malloc(++len);
	if (ret == NULL) {
		perror("malloc");
		exit(1);
	}

	va_start(fmtargs, fmt);
	vsnprintf(ret, len, fmt, fmtargs);
	va_end(fmtargs);

	return ret;
}

void
settz(char *tzname)
{
	setenv("TZ", tzname, 1);
}

char *
mktimes(char *fmt, char *tzname)
{
	char buf[129];
	time_t tim;
	struct tm *timtm;

	settz(tzname);
	tim = time(NULL);
	timtm = localtime(&tim);
	if (timtm == NULL)
		return smprintf("");

	if (!strftime(buf, sizeof(buf)-1, fmt, timtm)) {
		fprintf(stderr, "strftime == 0\n");
		return smprintf("");
	}

	return smprintf("%s", buf);
}

void
setstatus(char *str)
{
	XStoreName(dpy, DefaultRootWindow(dpy), str);
	XSync(dpy, False);
}

char *
readfile(char *base, char *file)
{
	char *path, line[513];
	FILE *fd;

	memset(line, 0, sizeof(line));

	path = smprintf("%s/%s", base, file);
	fd = fopen(path, "r");
	free(path);
	if (fd == NULL)
		return NULL;

	if (fgets(line, sizeof(line)-1, fd) == NULL)
		return NULL;
	fclose(fd);

	return smprintf("%s", line);
}

char *
getbattery(char *base)
{
	char *co, status1, status2;
	char status_discharging[] = "⟿ ";
	char status_charging[] = "⇜";
	int descap, remcap;
	int status_indicator = 0;

	descap = -1;
	remcap = -1;

	co = readfile(base, "present");
	if (co == NULL)
		return smprintf("");
	if (co[0] != '1') {
		free(co);
		return smprintf("not present");
	}
	free(co);

	co = readfile(base, "charge_full_design");
	if (co == NULL) {
		co = readfile(base, "energy_full_design");
		if (co == NULL)
			return smprintf("");
	}
	sscanf(co, "%d", &descap);
	free(co);

	co = readfile(base, "charge_now");
	if (co == NULL) {
		co = readfile(base, "energy_now");
		if (co == NULL)
			return smprintf("");
	}
	sscanf(co, "%d", &remcap);
	free(co);

	co = readfile(base, "status");
	if (!strncmp(co, "Discharging", 11)) {
		status_indicator = -1;
	} else if(!strncmp(co, "Charging", 8)) {
		status_indicator = 1;
	} else {
		status1 = '[';
		status2 = ']';
		status_indicator = 0;
	}

	if (remcap < 0 || descap < 0)
		return smprintf("invalid");

	if (status_indicator == -1) {
		return smprintf("%.0f%%%s", ((float)remcap / (float)descap) * 100, status_discharging);
	} else if (status_indicator == 1) {
		return smprintf("%.0f%%%s", ((float)remcap / (float)descap) * 100, status_charging);
	} else {
		return smprintf("%c%.0f%%%c", status1, ((float)remcap / (float)descap) * 100, status2);
	}
}

int
main(void)
{
	char *status;
	char *bat1;
	char *bat2;
	char *current_time;

	if (!(dpy = XOpenDisplay(NULL))) {
		fprintf(stderr, "dwmstatus: cannot open display.\n");
		return 1;
	}

	for (;;sleep(60)) {
		bat1 = getbattery("/sys/class/power_supply/BAT0");
		bat2 = getbattery("/sys/class/power_supply/BAT1");
		current_time = mktimes("%a %d %b %H:%M", time_zone);

		status = smprintf(" %s|%s %s ",
				  bat1, bat2, current_time);
		setstatus(status);

		free(bat1);
		free(bat2);
		free(current_time);
		free(status);
	}

	XCloseDisplay(dpy);

	return 0;
}
