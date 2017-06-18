#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "filesystem.h"

void usage(char *exec){
	printf("%s -format\n", exec);
	printf("%s -create <disk file> <simulated file>\n", exec);
	printf("%s -read <disk file> <simulated file>\n", exec);
    printf("%s -ls <absolute directory path>\n", exec);
	printf("%s -del <simulated file>\n", exec);
	printf("%s -mkdir <absolute directory path>\n", exec);
	printf("%s -rmdir <absolute directory path>\n", exec);
}


int main(int argc, char **argv){
	
	if(argc<2){
		usage(argv[0]);
	}else{
		/* Disk formating. */
		if( !strcmp(argv[1], "-format")){
			fs_format();
		}
		
        if( !strcmp(argv[1], "-create")){
			fs_create(argv[2], argv[3]);
		}

        if( !strcmp(argv[1], "-read")){
			fs_read(argv[2], argv[3]);
		}

        if( !strcmp(argv[1], "-del")){
			fs_del(argv[2]);
		}

        if( !strcmp(argv[1], "-ls")){
			fs_ls(argv[2]);
		}

        if( !strcmp(argv[1], "-mkdir")){
			fs_mkdir(argv[2]);
		}

        if( !strcmp(argv[1], "-rmdir")){
			fs_rmdir(argv[2]);
		}

	}	
	
	
	/* Create a map of used/free disk sectors. */
	fs_free_map("log.dat");
	
	return 	0; 
}
	
