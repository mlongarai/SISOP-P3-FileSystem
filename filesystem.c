#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include "libdisksimul.h"
#include "filesystem.h"

/**
 * @brief Format disk.
 * 
 */
int fs_format(){
	int ret, i;
	struct root_table_directory root_dir;
	struct sector_data sector;
	
	if ( (ret = ds_init(FILENAME, SECTOR_SIZE, NUMBER_OF_SECTORS, 1)) != 0 ){
		return ret;
	}
	
	memset(&root_dir, 0, sizeof(root_dir));
	
	root_dir.free_sectors_list = 1; /* first free sector. */
	
	ds_write_sector(0, (void*)&root_dir, SECTOR_SIZE);
	
	/* Create a list of free sectors. */
	memset(&sector, 0, sizeof(sector));
	
	for(i=1;i<NUMBER_OF_SECTORS;i++){
		if(i<NUMBER_OF_SECTORS-1){
			sector.next_sector = i+1;
		}else{
			sector.next_sector = 0;
		}
		ds_write_sector(i, (void*)&sector, SECTOR_SIZE);
	}
	
	ds_stop();
	
	printf("Disk size %d kbytes, %d sectors.\n", (SECTOR_SIZE*NUMBER_OF_SECTORS)/1024, NUMBER_OF_SECTORS);
	
	return 0;
}

/**
 * @brief Create a new file on the simulated filesystem.
 * @param input_file Source file path.
 * @param simul_file Destination file path on the simulated file system.
 * @return 0 on success.
 */
int fs_create(char* input_file, char* simul_file){
	int ret;
	
	if ( (ret = ds_init(FILENAME, SECTOR_SIZE, NUMBER_OF_SECTORS, 0)) != 0 ){
		return ret;
	}

	/* Write the code to load a new file to the simulated filesystem. */
	
	ds_stop();
	
	return 0;
}

/**
 * @brief Read file from the simulated filesystem.
 * @param output_file Output file path.
 * @param simul_file Source file path from the simulated file system.
 * @return 0 on success.
 */
int fs_read(char* output_file, char* simul_file){
	int ret;
	if ( (ret = ds_init(FILENAME, SECTOR_SIZE, NUMBER_OF_SECTORS, 0)) != 0 ){
		return ret;
	}
	
	/* Write the code to read a file from the simulated filesystem. */
	
	ds_stop();
	
	return 0;
}

/**
 * @brief Delete file from file system.
 * @param simul_file Source file path.
 * @return 0 on success.
 */
int fs_del(char* simul_file){
	int ret;
	if ( (ret = ds_init(FILENAME, SECTOR_SIZE, NUMBER_OF_SECTORS, 0)) != 0 ){
		return ret;
	}
	
	/* Write the code delete a file from the simulated filesystem. */
	
	ds_stop();
	
	return 0;
}

/**
 * @brief List files from a directory.
 * @param simul_file Source file path.
 * @return 0 on success.
 */
int fs_ls(char *dir_path){
	
	int ret,i;
	int dirAddr;
	struct root_table_directory root_dir;
	struct table_directory directory;

	if ( (ret = ds_init(FILENAME, SECTOR_SIZE, NUMBER_OF_SECTORS, 0)) != 0 ){
		return ret;
	}

	/* move to the next free sector. */
	ds_read_sector(0,(void*)&root_dir, SECTOR_SIZE);

	if(!strcmp(dir_path,"/")) {
		printf("/\n");
		for(i=0;i<14;i++) {
			if(root_dir.entries[i].sector_start != 0) {
				if(root_dir.entries[i].dir) {
					printf("d %s\n",root_dir.entries[i].name);
					} else {
						printf("f %s            %u bits\n",root_dir.entries[i].name,root_dir.entries[i].size_bytes);
				}
			}
		}
	} else { 
		printf("%s\n", dir_path);
		dirAddr = navigate(dir_path);
		
		/* move to the next free sector. */
		ds_read_sector(dirAddr,(void*)&directory, SECTOR_SIZE);
		for(i=0;i<15;i++) {
			if(directory.entries[i].sector_start != 0) {
				if(directory.entries[i].dir) {
					printf("d %s\n",directory.entries[i].name); 
					} else {
						printf("f %s           %u bits\n",directory.entries[i].name, directory.entries[i].size_bytes);
				}
			}
		}
	}
	
	ds_stop();

	return 0;
}

/**
 * @brief Create a new directory on the simulated filesystem.
 * @param directory_path directory path.
 * @return 0 on success.
 */
int fs_mkdir(char* directory_path){
	int ret, i = 0, success = 0, dirAddr;
	char* nameDir; 
	char* pathAux;
	char * str = (char *) malloc(100);
	struct root_table_directory root_dir;
	struct table_directory directory = {0}, dirEntry  = {0};
	struct sector_data sector;
	
	/* Initialize Virtual Memory */
	if ( (ret = ds_init(FILENAME, SECTOR_SIZE, NUMBER_OF_SECTORS, 0)) != 0 ){
		return ret;
	}
	
	/* Code to create a new directory. */
	ds_read_sector(0,(void*)&root_dir, SECTOR_SIZE); //Read root dir
	
	strcpy(str,directory_path);
	pathAux = strtok(directory_path, "/");

	while(pathAux != NULL) { 
		nameDir = pathAux;
		pathAux =  strtok(NULL, "/");
		i++;
	}

	if(i > 1) {
		dirAddr = navigate(str);
		ds_read_sector(dirAddr,(void*)&directory, SECTOR_SIZE);
		for(i= 0;i<16;i++) {
			if(directory.entries[i].sector_start == 0) {
				directory.entries[i].dir = 1;
				strcpy(directory.entries[i].name, nameDir);
				directory.entries[i].size_bytes = 0;
				directory.entries[i].sector_start = root_dir.free_sectors_list;
				success = 1;
				ds_read_sector(directory.entries[i].sector_start,(void*)&sector, SECTOR_SIZE);
				root_dir.free_sectors_list = sector.next_sector;
				ds_write_sector(dirAddr, (void*)&directory, SECTOR_SIZE); 
				ds_write_sector(directory.entries[i].sector_start, (void*)&dirEntry, SECTOR_SIZE);
				break;
			}
		}
	} else {
		for(i= 0;i<15;i++) {
			if(root_dir.entries[i].sector_start == 0) {
				root_dir.entries[i].dir = 1;
				strcpy(root_dir.entries[i].name, nameDir);
				root_dir.entries[i].size_bytes = 0;
				root_dir.entries[i].sector_start = root_dir.free_sectors_list;
				success = 1;
				ds_read_sector(root_dir.entries[i].sector_start,(void*)&sector, SECTOR_SIZE);
				root_dir.free_sectors_list = sector.next_sector;
				ds_write_sector(root_dir.entries[i].sector_start, (void*)&dirEntry, SECTOR_SIZE);
				break;
			}
		}
	}

	if(success){
		printf("Directory created!\n");
	} else {
		printf("Definition Error");
	}

	ds_write_sector(0, (void*)&root_dir, SECTOR_SIZE);
	ds_stop();
	
	return 0;
}

/**
 * @brief Remove directory from the simulated filesystem.
 * @param directory_path directory path.
 * @return 0 on success.
 */

int fs_rmdir(char *directory_path){
	int ret, i=0, j=0,dirAddress,success;
	char* nameDir = (char *) malloc(100);
	char* pathAux;
	char * str = (char *) malloc(100);
	char* PathAbove;
	struct root_table_directory root_dir;
	struct table_directory directory, rmDir;
	struct sector_data sector;
	
	if ( (ret = ds_init(FILENAME, SECTOR_SIZE, NUMBER_OF_SECTORS, 0)) != 0 ){
		return ret;
	}
	strcpy(str,directory_path);
	pathAux = strtok(directory_path, "/");
	ds_read_sector(0,(void*)&root_dir, SECTOR_SIZE);

	while(pathAux != NULL) { 
		nameDir = pathAux;
		pathAux =  strtok(NULL, "/");
		i++;
	}

	if(i > 1) {
		PathAbove = (char*)malloc(100);
		strncpy( PathAbove, str, (strlen(str) - strlen(nameDir)) -1 );
		dirAddress = navigate(PathAbove);
		ds_read_sector(dirAddress,(void*)&directory, SECTOR_SIZE);
		for(i= 0;i<16;i++) {
			if(!strcmp(directory.entries[i].name,nameDir)) {
				ds_read_sector(directory.entries[i].sector_start,(void*)&rmDir, SECTOR_SIZE);
				for(j= 0;j<16;j++) {
					if(rmDir.entries[i].sector_start != 0) {
						printf("Is not empty!\n");
						ds_stop();
						return -1;
					}
				}
				memset(&sector, 0, SECTOR_SIZE);
				sector.next_sector = root_dir.free_sectors_list;
				root_dir.free_sectors_list = directory.entries[i].sector_start;
				ds_write_sector(directory.entries[i].sector_start,(void*)&sector, SECTOR_SIZE);
				directory.entries[i].sector_start = 0;
				ds_write_sector(dirAddress, (void*)&directory, SECTOR_SIZE);
				success = 1;
				break;
			}
		}

	} else { 
		for(i= 0;i<15;i++) {
			if(!strcmp(root_dir.entries[i].name,nameDir)) {
				ds_read_sector(root_dir.entries[i].sector_start,(void*)&rmDir, SECTOR_SIZE);
				for(j= 0;j<16;j++) {
					if(rmDir.entries[i].sector_start != 0) {
						printf("Is not empty!\n");
						ds_stop();
						return -1;
					}
				}
				memset(&sector, 0, SECTOR_SIZE);
				sector.next_sector = root_dir.free_sectors_list;
				ds_write_sector(root_dir.entries[i].sector_start,(void*)&sector, SECTOR_SIZE);
				root_dir.free_sectors_list = root_dir.entries[i].sector_start;
				root_dir.entries[i].sector_start = 0;
				success = 1;
				break;
			}
		}
	}

	if(success) { 
		printf("Directory removed!\n");

	} else {
		printf("Definition Error");
	}

	ds_write_sector(0, (void*)&root_dir, SECTOR_SIZE);
	ds_stop();
	
	return 0;

}


int navigate(char* directory_path) { 
	char* pathAux = strtok(directory_path, "/");
	struct root_table_directory root_dir;
	struct table_directory directory;
	int i, sectorAddress;

	ds_read_sector(0,(void*)&root_dir, SECTOR_SIZE);

	for(i=0; i<15; i++) {
		if (!strcmp(root_dir.entries[i].name, pathAux) && root_dir.entries[i].sector_start != 0 && root_dir.entries[i].dir) { 
			ds_read_sector(root_dir.entries[i].sector_start,(void*)&directory,SECTOR_SIZE);
			sectorAddress = root_dir.entries[i].sector_start;
			while( (pathAux = strtok(NULL, "/")) != NULL) {
				for(i=0; i<16; i++) {
					if((!strcmp(directory.entries[i].name,pathAux) == 0) && directory.entries[i].sector_start != 0 && directory.entries[i].dir) {
						sectorAddress = directory.entries[i].sector_start;
					}
				}
			}
			return sectorAddress;
		}
	}
	printf("Directory not found!\n");
	return 0; 
}

/**
 * @brief Generate a map of used/available sectors. 
 * @param log_f Log file with the sector map.
 * @return 0 on success.
 */
int fs_free_map(char *log_f){
	int ret, i, next;
	struct root_table_directory root_dir;
	struct sector_data sector;
	char *sector_array;
	FILE* log;
	int pid, status;
	int free_space = 0;
	char* exec_params[] = {"gnuplot", "sector_map.gnuplot" , NULL};

	if ( (ret = ds_init(FILENAME, SECTOR_SIZE, NUMBER_OF_SECTORS, 0)) != 0 ){
		return ret;
	}
	
	/* each byte represents a sector. */
	sector_array = (char*)malloc(NUMBER_OF_SECTORS);

	/* set 0 to all sectors. Zero means that the sector is used. */
	memset(sector_array, 0, NUMBER_OF_SECTORS);
	
	/* Read the root dir to get the free blocks list. */
	ds_read_sector(0, (void*)&root_dir, SECTOR_SIZE);
	
	next = root_dir.free_sectors_list;

	while(next){
		/* The sector is in the free list, mark with 1. */
		sector_array[next] = 1;
		
		/* move to the next free sector. */
		ds_read_sector(next, (void*)&sector, SECTOR_SIZE);
		
		next = sector.next_sector;
		
		free_space += SECTOR_SIZE;
	}

	/* Create a log file. */
	if( (log = fopen(log_f, "w")) == NULL){
		perror("fopen()");
		free(sector_array);
		ds_stop();
		return 1;
	}
	
	/* Write the the sector map to the log file. */
	for(i=0;i<NUMBER_OF_SECTORS;i++){
		if(i%32==0) fprintf(log, "%s", "\n");
		fprintf(log, " %d", sector_array[i]);
	}
	
	fclose(log);
	
	/* Execute gnuplot to generate the sector's free map. */
	pid = fork();
	if(pid==0){
		execvp("gnuplot", exec_params);
	}
	
	wait(&status);
	
	free(sector_array);
	
	ds_stop();
	
	printf("Free space %d kbytes.\n", free_space/1024);
	
	return 0;
}

