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
#include <sys/sysinfo.h>

#include <X11/Xlib.h>

char *timezon = "Asia/Kolkata";
char *tzutc = "UTC";
char *tzberlin = "Europe/Berlin";

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
loadavg(void)
{
	double avgs[3];

	if (getloadavg(avgs, 3) < 0)
		return smprintf("");

	return smprintf("%.2f %.2f %.2f", avgs[0], avgs[1], avgs[2]);
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

	if (fgets(line, sizeof(line)-1, fd) == NULL) {
		fclose(fd);
		return NULL;
	}
	fclose(fd);

	return smprintf("%s", line);
}

char *
getbattery(char *base)
{
	char *co, status;
	int percent;
        
	co = readfile(base, "capacity");
	sscanf(co, "%d", &percent);
	free(co);
	
	co = readfile(base, "status");
	if (!strncmp(co, "Discharging", 11)) {
		free(co);
		return smprintf("󰂍 %d", percent);
	} else if(!strncmp(co, "Charging", 8)) {
		free(co);
		return smprintf("󰂐 %d", percent);
	} else if(!strncmp(co, "Full", 4)) {
	       	free(co);
		return smprintf("󰁹 %d", percent);
	} else {
		free(co);
		return smprintf("󰁹 %d", percent);
	}
}

char *
gettemperature(char *base, char *sensor)
{
	char *co;

	co = readfile(base, sensor);
	if (co == NULL)
		return smprintf("");
	return smprintf("%02.0f°C", atof(co) / 1000);
}

char *
execscript(char *cmd)
{
	FILE *fp;
	char retval[1025], *rv;

	memset(retval, 0, sizeof(retval));

	fp = popen(cmd, "r");
	if (fp == NULL)
		return smprintf("");

	rv = fgets(retval, sizeof(retval), fp);
	pclose(fp);
	if (rv == NULL)
		return smprintf("");
	retval[strlen(retval)-1] = '\0';

	return smprintf("%s", retval);
}

char *
getmemory(void)
{
    FILE *fp;
    char line[256];
    double total_mem = 0, avail_mem = 0;
    
    fp = fopen("/proc/meminfo", "r");
    if (fp == NULL)
        return smprintf("");
    
    while (fgets(line, sizeof(line), fp)) {
        if (sscanf(line, "MemTotal: %lf kB", &total_mem) == 1) {
            continue;
        } else if (sscanf(line, "MemAvailable: %lf kB", &avail_mem) == 1) {
            break;
        }
    }
    
    fclose(fp);
    
    // Calculate memory usage percentage
    double totalPhysMem = total_mem / (1024 * 1024);
    double availPhysMem = avail_mem / (1024 * 1024);
    int memoryUsage = (int)((totalPhysMem - availPhysMem) * 100 / totalPhysMem);

    return smprintf("%.2lf GB (%d%%)", totalPhysMem - availPhysMem, memoryUsage);
}
/*
char *
getmemory(void)
{
    struct sysinfo memInfo;
    sysinfo(&memInfo);
    long long totalPhysMem = memInfo.totalram * memInfo.mem_unit / (1024 * 1024 * 1024);
    long long freePhysMem = memInfo.freeram * memInfo.mem_unit / (1024 * 1024 * 1024);
    int memoryUsage = (int)((totalPhysMem - freePhysMem) * 100 / totalPhysMem);

    return smprintf("%lld GB / %lld GB (%d%%)", totalPhysMem - freePhysMem, totalPhysMem, memoryUsage);
}
*/

char *
getvolume() {
	FILE *fp;
	int volume;
	fp = popen("pamixer --get-volume", "r");
	fscanf(fp, "%d", &volume);
	pclose(fp);
	return smprintf("%d%%", volume);
}

int
main(void)
{
	char *status;
	// char *avgs;
	char *bat;
	char *time;
	char *date;
	// char *tmutc;
	// char *tmbln;
	char *t0;
	char *memory;
	char *volume;
	// char *t1;
	// char *kbmap;
	// char *surfs;

	if (!(dpy = XOpenDisplay(NULL))) {
		fprintf(stderr, "dwmstatus: cannot open display.\n");
		return 1;
	}

	for(;;sleep(3)) {
		// avgs = loadavg();
		bat = getbattery("/sys/class/power_supply/BAT1");
		time = mktimes("%I:%M %p", timezon);
		date = mktimes("%a %d-%m-%Y", timezon);
		memory = getmemory();
		volume = getvolume();
		// tmutc = mktimes("%H:%M", tzutc);
		// tmbln = mktimes("KW %W %a %d %b %H:%M %Z %Y", tzberlin);
		// kbmap = execscript("setxkbmap -query | grep layout | cut -d':' -f 2- | tr -d ' '");
		// surfs = execscript("surf-status");
		t0 = gettemperature("/sys/devices/virtual/thermal/thermal_zone0", "temp");
		// t1 = gettemperature("/sys/devices/virtual/thermal/thermal_zone1", "temp");

		status = smprintf("  %s |  %s |   %s | %s |   %s |   %s |", t0, volume, memory, bat, date, time);
		setstatus(status);

		// free(surfs);
		// free(kbmap);
		free(t0);
		free(memory);
		free(volume);
		// free(t1);
		// free(avgs);
		free(bat);
		free(time);
		free(date);
		// free(tmutc);
		// free(tmbln);
		free(status);
	}

	XCloseDisplay(dpy);

	return 0;
}

