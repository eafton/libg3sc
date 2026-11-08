#include <stdio.h>
#include <g3sc.h>

int main(int argc, char **argv) { 
	G3SCStyle *style;

	style = g3sc_style_new_from_file(argv[1]);
	g_object_unref(style);
	
	return 0;
}
