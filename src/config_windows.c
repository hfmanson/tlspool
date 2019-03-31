#include <stdio.h>
#include <windows.h>

char szPipename[1024];

void cfg_socketname (char *item, int itemno, char *value) {
	if (strlen (value) + 1 > sizeof (szPipename)) {
		fprintf (stderr, "Socket path too long: %s\n", value);
		exit (1);
	}
	strcpy (szPipename, value);
}

