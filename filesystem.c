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
	
	/* Variables to use in functioNextSectorCreate  */
	int ret, i;

	/* Filesystem structures. */
	struct root_table_directory root_dir;
	struct sector_data sector;
	
	if ( (ret = ds_init(FILENAME, SECTOR_SIZE, NUMBER_OF_SECTORS, 1)) != 0 ){
		return ret;
	}
	
	/* Create a list of free sectors in root table. */
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
	
	/* Variables to use in function  */
	int ret, tam, tamaux=0, i=0,k,NextSCreate,j,dirAddr;
    
    /* Filesystem structures. */
    struct root_table_directory root_dir;
	struct sector_data sector;
	struct table_directory directory = {0};
	
	char name[50];
	char * str = (char *) malloc(100);
	strcpy(str,simul_file);
	char* pathAux = strtok(simul_file, "/");
    FILE *ptr;

    /* Initialize Virtual Memory */
	if ( (ret = ds_init(FILENAME, SECTOR_SIZE, NUMBER_OF_SECTORS, 0)) != 0 ){
		return ret;
	}
    
    /* move to the next free sector. */
    ds_read_sector(0,(void*)&root_dir, SECTOR_SIZE);

    ptr = fopen (input_file,"rb");

	/* Verify if write on root*/
	while(pathAux != NULL) {
		pathAux =  strtok(NULL, "/");
		i++;
	}

    /* Get file size */
  	ftell(ptr);
	fseek(ptr, 0L, SEEK_END);
	tam = ftell(ptr);
	fseek(ptr, 0, SEEK_SET);
		
	/* Get name of file to create */
	get_name(name, str);
    
	/* The first Block */
	if(i == 1){
		for (j = 0; j < 15; j++){
			if (root_dir.entries[j].sector_start == 0){
				root_dir.entries[j].dir = 0;
				strcpy(root_dir.entries[j].name, name);
				root_dir.entries[j].size_bytes = tam; 
				root_dir.entries[j].sector_start = root_dir.free_sectors_list;
				NextSCreate = root_dir.free_sectors_list;
				ds_read_sector(NextSCreate,(void*)&sector, SECTOR_SIZE);
				break;
			}	
		}		
	} else {
		dirAddr = navigate(str);
		ds_read_sector(dirAddr,(void*)&directory, SECTOR_SIZE);
		for (j = 0; j < 15; j++){
			if(directory.entries[j].sector_start == 0) {
				directory.entries[j].dir = 0;
				strcpy(directory.entries[j].name, name);
				directory.entries[j].size_bytes = tam;
				directory.entries[j].sector_start = root_dir.free_sectors_list;
				
				/* Save the new directory */
				ds_write_sector(dirAddr, (void*)&directory, SECTOR_SIZE);
				
				NextSCreate = root_dir.free_sectors_list;
				ds_read_sector(NextSCreate,(void*)&sector, SECTOR_SIZE);
				break;
			}
		}
	}

	int readnext, willread;
    unsigned int NextSectorCreate, NextSSector; /* Is the next sector free */
	readnext = tam;								
    while(tamaux < tam){
            fflush(stdout);   
		    if(readnext > 508){
	 			willread = 508;
                NextSectorCreate = sector.next_sector;       
            }
			else{
				willread = readnext;
                NextSectorCreate = 0;                        
            }
            
			char *data2;
			data2 = malloc (willread * sizeof (char));

			fseek(ptr, tamaux, SEEK_SET);
			fread(data2, sizeof(char), willread ,ptr);		
			for(k=0; k<508; k++)
				sector.data[k] = data2[k];
          
          	/* NextSSector recieve the next sector to refresh the sector.next_sector */
			NextSSector = sector.next_sector;                   
            sector.next_sector = NextSectorCreate;
            
        fflush(stdout);
		ds_write_sector(NextSCreate,(void*)&sector, SECTOR_SIZE);
        fflush(stdout);
		ds_read_sector(NextSectorCreate,(void*)&sector, SECTOR_SIZE);
		NextSCreate = NextSectorCreate;
		readnext = readnext - 508;
		tamaux = tamaux + 508;
    }

    /* refresh root table */
	root_dir.free_sectors_list = NextSSector;
	ds_write_sector(0,(void*)&root_dir, SECTOR_SIZE);

	fclose(ptr);
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
	
	/* Variables to use in function  */
	int ret, test, i=0,j=0, tamm, wread, dirAddr, flag=0, ind,f=0;
	
	/* Filesystem structures. */
	struct root_table_directory root_dir;
	struct sector_data sector;
	struct table_directory directory = {0};

	char name[50];
	char * str = (char *) malloc(100);
	
	/* Function: strcpy and strtok use to copy string pointer (destination,source) and split string slash "/", respectively */
	strcpy(str,simul_file);
	char* pathAux1 = strtok(simul_file, "/");
	FILE *fp;

	char *data;
	data = malloc (508 * sizeof (char));
	/* Initialize Virtual Memory */
	if ( (ret = ds_init(FILENAME, SECTOR_SIZE, NUMBER_OF_SECTORS, 0)) != 0 ){
		return ret;
	}

	/* While the path is unique, next directory */
	while(pathAux1 != NULL) { 
		pathAux1 =  strtok(NULL, "/");
		i++;
	}

	/* move to the next free sector. */
	ds_read_sector(0,(void*)&root_dir, SECTOR_SIZE);
	get_name(name,str);

	fp = fopen (output_file,"file");
	if(i == 1){
		for(j=0; j<15; j++)
		{
			test = strcmp(root_dir.entries[j].name,name);
			if(test == 0){
				ind = root_dir.entries[j].sector_start;
				tamm = root_dir.entries[j].size_bytes;
				while(flag==0){
					ds_read_sector(ind,(void*)&sector, SECTOR_SIZE);		
					for(f=0; f<508; f++)
						 data[f]= sector.data[f];				
					if(tamm>507){
						wread = 508;
						tamm = tamm - 508;
					}
					else
						wread = tamm; 
					fwrite(data,sizeof(char),wread ,fp);
					if(sector.next_sector == 0){
						flag = 1;
						j=15;
					}
					ind = sector.next_sector;
					}
			}
		}
	} else{
		dirAddr = navigate(str);
		ds_read_sector(dirAddr,(void*)&directory, SECTOR_SIZE);
		for(j=0; j<15; j++)
		{
			test = strcmp(directory.entries[j].name,name);
			if(test == 0){
				ind = directory.entries[j].sector_start;
				tamm = directory.entries[j].size_bytes;
				while(flag==0){
					ds_read_sector(ind,(void*)&sector, SECTOR_SIZE);		
					for(f=0; f<508; f++)
						 data[f]= sector.data[f];				
					if(tamm>507){
						wread = 508;
						tamm = tamm - 508;
					}
					else
						wread = tamm; 
					fwrite(data,sizeof(char),wread ,fp);
					if(sector.next_sector == 0)
						flag = 1;
					ind = sector.next_sector;
					}
			}
		}	
		}
	
	printf("\n File copied! \n");
	fclose(fp);
	ds_stop();
	
	return 0;
}

/**
 * @brief Delete file from file system.
 * @param simul_file Source file path.
 * @return 0 on success.
 */
int fs_del(char* directory_path){
	
	/* Variables to use in function  */
	int ret, i=0, dirAddr, success, occupiedSector;
	char* nameDir = (char *) malloc(100);
	char* pathAux;
	char * str = (char *) malloc(100);

	/* Filesystem structures. */
	struct root_table_directory root_dir;
	struct table_directory directory;
	struct sector_data sector, sectorDel;

	/* Initialize Virtual Memory */
	if ( (ret = ds_init(FILENAME, SECTOR_SIZE, NUMBER_OF_SECTORS, 0)) != 0 ){
		return ret;
	}
	
	/* Function strcpy and strtok use to copy string pointer (destination,source) and split string slash "/", respectively */
	strcpy(str,directory_path);
	pathAux = strtok(directory_path, "/");

	/* move to the next free sector. */
	ds_read_sector(0,(void*)&root_dir, SECTOR_SIZE); 

	/* While the path is unique, next directory */
	while(pathAux != NULL) { 
		nameDir = pathAux;
		pathAux =  strtok(NULL, "/");
		i++;
	}
	if(i > 1) {

		dirAddr = navigate(str);
		ds_read_sector(dirAddr,(void*)&directory, SECTOR_SIZE);
		for(i= 0;i<16;i++) {
			if(!strcmp(directory.entries[i].name,nameDir)) {

				memset(&sector, 0, SECTOR_SIZE);
				/* Read the first sector of delete */
				ds_read_sector(directory.entries[i].sector_start,(void*)&sectorDel, SECTOR_SIZE);
				/* save the first sector of queue */
				occupiedSector = directory.entries[i].sector_start;
				while(sectorDel.next_sector != 0){ 
					sector.next_sector = root_dir.free_sectors_list;
					root_dir.free_sectors_list = occupiedSector;
					occupiedSector = sectorDel.next_sector;
					ds_write_sector(root_dir.free_sectors_list,(void*)&sector, SECTOR_SIZE);
					ds_read_sector(sectorDel.next_sector,(void*)&sectorDel, SECTOR_SIZE);
				}
				sector.next_sector = root_dir.free_sectors_list;
				root_dir.free_sectors_list = occupiedSector;

				/* Write the sector empty and delete */
				ds_write_sector(root_dir.free_sectors_list,(void*)&sector, SECTOR_SIZE);
				directory.entries[i].sector_start = 0;
				ds_write_sector(dirAddr, (void*)&directory, SECTOR_SIZE);
				success = 1;
				break;
			}
		}

	} else { 
		for(i= 0;i<15;i++) {
			if(!strcmp(root_dir.entries[i].name,nameDir)) {
				memset(&sector, 0, SECTOR_SIZE);
				ds_read_sector(root_dir.entries[i].sector_start,(void*)&sectorDel, SECTOR_SIZE);
				occupiedSector = root_dir.entries[i].sector_start; 
				while(sectorDel.next_sector != 0){
					sector.next_sector = root_dir.free_sectors_list;
					root_dir.free_sectors_list = occupiedSector;
					occupiedSector = sectorDel.next_sector;
					ds_write_sector(root_dir.free_sectors_list,(void*)&sector, SECTOR_SIZE);
					ds_read_sector(sectorDel.next_sector,(void*)&sectorDel, SECTOR_SIZE);
				}
				sector.next_sector = root_dir.free_sectors_list;
				root_dir.free_sectors_list = occupiedSector;
				ds_write_sector(root_dir.free_sectors_list,(void*)&sector, SECTOR_SIZE);
				root_dir.entries[i].sector_start = 0;
				success = 1;
				break;
			}
		}
	}

	if(success){
		printf("File removed!\n");
	} else {
		printf("Definition Error");
	}

	/* locate the sector and load to memory buffer .*/
	ds_write_sector(0, (void*)&root_dir, SECTOR_SIZE);

	ds_stop();
	
	return 0;
}

/**
 * @brief List files from a directory.
 * @param simul_file Source file path.
 * @return 0 on success.
 */
int fs_ls(char *dir_path){
	
	/* Variables to use in function  */
	int ret,i;
	int dirAddr;

	/* Filesystem structures. */
	struct root_table_directory root_dir;
	struct table_directory directory;

	/* Initialize Virtual Memory */
	if ( (ret = ds_init(FILENAME, SECTOR_SIZE, NUMBER_OF_SECTORS, 0)) != 0 ){
		return ret;
	}

	/* move to the next free sector. */
	ds_read_sector(0,(void*)&root_dir, SECTOR_SIZE);

	/* Compare if name on root directory is same of slash "/" and add the next slash */
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
		
		/* Check the change directory on table */
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

	/* Variables to use in functioNextSectorCreate  */
	int ret, i = 0, success = 0, dirAddr;

	char* nameDir; 
	char* pathAux;

	/* sector byte representation */
	char * str = (char *) malloc(100);
	
	/* Filesystem structures. */
	struct root_table_directory root_dir;
	struct table_directory directory = {0}, dirEntry  = {0};
	struct sector_data sector;
	
	/* Initialize Virtual Memory */
	if ( (ret = ds_init(FILENAME, SECTOR_SIZE, NUMBER_OF_SECTORS, 0)) != 0 ){
		return ret;
	}
	
	/* Create new directory on simulfs */
	ds_read_sector(0,(void*)&root_dir, SECTOR_SIZE);


	/* FunctioNextSectorCreate: strcpy and strtok use to copy string pointer (destination,source) and split string slash "/", respectively */
	strcpy(str,directory_path);
	pathAux = strtok(directory_path, "/");

	/* While the path is unique, next directory */
	while(pathAux != NULL) { 
		nameDir = pathAux;
		pathAux =  strtok(NULL, "/");
		i++;
	}


	if(i > 1) {
		

		/* Check the change directory on table */
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
				
				/* locate the sector and load to memory buffer .*/
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
				
				/* Set the next sector free on list .*/
				root_dir.entries[i].size_bytes = 0;
				root_dir.entries[i].sector_start = root_dir.free_sectors_list;
				success = 1;
				
				/* Start the next free sector in array */
				ds_read_sector(root_dir.entries[i].sector_start,(void*)&sector, SECTOR_SIZE);
				root_dir.free_sectors_list = sector.next_sector;
				
				/* locate the sector and load to memory buffer .*/
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

	/* locate the sector and load to memory buffer .*/
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

	/* Variables to use in functioNextSectorCreate  */
	int ret, i=0, j=0;
	int dirAddr, success;
	char* pathAux;
	char* PathAbove;
	
	/* sector byte representation */
	char* nameDir = (char *) malloc(100);
	char * str = (char *) malloc(100);

	/* Filesystem structures. */
	struct root_table_directory root_dir;
	struct table_directory directory, rmDir;
	struct sector_data sector;
	
	if ( (ret = ds_init(FILENAME, SECTOR_SIZE, NUMBER_OF_SECTORS, 0)) != 0 ){
		return ret;
	}
	
	/* FunctioNextSectorCreate: strcpy and strtok use to copy string pointer (destination,source) and split string slash "/", respectively */
	strcpy(str,directory_path);
	pathAux = strtok(directory_path, "/");

	/* Read the root dir to get the free blocks list */
	ds_read_sector(0,(void*)&root_dir, SECTOR_SIZE);

	/* While the path is unique, next directory */
	while(pathAux != NULL) { 
		nameDir = pathAux;
		pathAux =  strtok(NULL, "/");
		i++;
	}

	if(i > 1) {
		
		/* Check the path above the directory */
		PathAbove = (char*)malloc(100);
		strncpy( PathAbove, str, (strlen(str) - strlen(nameDir)) -1 );
		
		/* Check the change directory on table */
		dirAddr = navigate(PathAbove);
		
		/* move to the directory adddress sector set in array */
		ds_read_sector(dirAddr,(void*)&directory, SECTOR_SIZE);
		for(i= 0;i<16;i++) {
			
			/* Compare if name on root directory is same of the nameDir */
			if(!strcmp(directory.entries[i].name,nameDir)) {
				
				/* Start the next free sector in array */
				ds_read_sector(directory.entries[i].sector_start,(void*)&rmDir, SECTOR_SIZE);
				for(j= 0;j<16;j++) {
					
					/* Check on sector if the directory of rmdir is not empty! */
					if(rmDir.entries[i].sector_start != 0) {
						printf("Is not empty!\n");
						ds_stop();
						return -1;
					}
				}
				
				/* Create a list of free sectors. */
				memset(&sector, 0, SECTOR_SIZE);

				/* Set the next sector free on list .*/
				sector.next_sector = root_dir.free_sectors_list;
				root_dir.free_sectors_list = directory.entries[i].sector_start;
				
				/* load the NEW sector to memory buffer .*/
				ds_write_sector(directory.entries[i].sector_start,(void*)&sector, SECTOR_SIZE);
				directory.entries[i].sector_start = 0;
				
				/* locate the sector and load to memory buffer .*/
				ds_write_sector(dirAddr, (void*)&directory, SECTOR_SIZE);
				
				success = 1;
				
				break;
			}
		}

	} else { 
		for(i= 0;i<15;i++) {
			
			/* Compare if name on root directory is same of the nameDir */
			if(!strcmp(root_dir.entries[i].name,nameDir)) {
				
				/* Start the next free sector in array */
				ds_read_sector(root_dir.entries[i].sector_start,(void*)&rmDir, SECTOR_SIZE);
				for(j= 0;j<16;j++) {

					/* Check on sector if the directory of rmdir is not empty! */
					if(rmDir.entries[i].sector_start != 0) {
						printf("Is not empty!\n");
						ds_stop();
						return -1;
					}
				}
				
				/* Create a list of free sectors. */
				memset(&sector, 0, SECTOR_SIZE);
				sector.next_sector = root_dir.free_sectors_list;
				
				/* load the NEW sector to memory buffer .*/
				ds_write_sector(root_dir.entries[i].sector_start,(void*)&sector, SECTOR_SIZE);
				
				/* Set the next sector free on list .*/
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

/* Get the directory path */
int navigate(char* directory_path) { 
	
	/* Variables to use in functioNextSectorCreate  */
	int i, sectorAddress;
	char* pathAux = strtok(directory_path, "/");
	
	/* Filesystem structures. */
	struct root_table_directory root_dir;
	struct table_directory directory;
	
	/* Read the root dir to get the free blocks list */
	ds_read_sector(0,(void*)&root_dir, SECTOR_SIZE);

	for(i=0; i<15; i++) {

		/* Compare if name on root directory is same of the pathaux */
		if (!strcmp(root_dir.entries[i].name, pathAux) && root_dir.entries[i].sector_start != 0 && root_dir.entries[i].dir) { 
			
			/* Start the next free sector in array */
			ds_read_sector(root_dir.entries[i].sector_start,(void*)&directory,SECTOR_SIZE);
			sectorAddress = root_dir.entries[i].sector_start;
			while( (pathAux = strtok(NULL, "/")) != NULL) {
				for(i=0; i<16; i++) {
					
					/* Again, compare if name on root directory is same of the pathaux */
					if((!strcmp(directory.entries[i].name,pathAux) && directory.entries[i].sector_start != 0 && directory.entries[i].dir == 1)) {
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

int next_free_sector(int NextSectorCreate){
struct root_table_directory root_dir;
	int got = 0 ,k =1;
	ds_read_sector(0,(void*)&root_dir, SECTOR_SIZE);
	while(got == 0){	
		if(root_dir.entries[k].sector_start == 0){
			return k;
		}
		k++;
	}
	return 0;
}

void get_name(char *name, char* simul_file){
	int i=0, slashs =0, position;
	while(simul_file[i] != NULL){
		if(simul_file[i] == '/'){
			slashs ++;
			position = i + 1;
		}
		i++;
	}

	if(slashs == 0)    /* Tirar e trocar o input_file pelo simul_file , sempre haverá barra no nome */
		position = 0;
	i = 0;

	while(simul_file[i] != NULL){
		name[i] = simul_file[position];
		i++;
		position++;
	}

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

	/* set 0 to all sectors. Zero meNextSectorCreate that the sector is used. */
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