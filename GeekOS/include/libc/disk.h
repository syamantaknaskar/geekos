#include <string.h> 

#define NO_PLATTERS 3
#define NO_TRACKS 100
#define NO_SECTORS_MIN 100
#define NO_SECTORS_MAX 200
#define DATA_SIZE 512
#define PLATTER_SIZE (NO_SECTORS_MIN+NO_SECTORS_MAX)*NO_TRACKS/2
#define DISK_SIZE NO_PLATTERS*PLATTER_SIZE
#define TOTAL_DISK 4
/*****************************
******** Sector **************
*****************************/
typedef struct
{
	int sector_no;
	size_t size; //size of data it can store
	char filename[100];
	char * data;
} Sector;

int write_data_sector(Sector *,char *);
int write_data_sector(Sector *s,char *d)
{
	if(strlen(d)>s->size) return 0;
	int i;
	for ( i = 0; i <(int) strlen(d); i++) 
	{
		s->data[i] = d[i];
	}
	return 1;
}
int read_data_sector(Sector *,char *);
int read_data_sector(Sector *s,char *d)
{
	int i;
	for (i = 0; i < (int)strlen(d); i++) 
	{
		d[i] = s->data[i];
	}
	return 1;
}

/*****************************
******** Track **************
*****************************/
typedef struct 
{
	// Track number
	int track_no;

	// Number of Sectors in the track
	int size;

	// List of Sectors in the Track
	Sector * sectors;
} Track;
int write_data_track(Track *t,char *d, int s)
{
	return write_data_sector(&t->sectors[s],d);
}
int read_data_track(Track *t,char *d, int s)
{
	return read_data_sector(&t->sectors[s],d);
}

/*****************************
******** Platter **************
*****************************/
typedef struct 
{
	// Platter number
	int platter_no;

	// Number of tracks in the platter
	int size;
	
	// List of Tracks in the Platter
	Track * tracks;
}Platter;
int write_data_platter(Platter *p,char *d,int t,int s)
{
	return write_data_track(&p->tracks[t],d,s);
}
int read_data_platter(Platter *p,char *d,int t,int s)
{
	return read_data_track(&p->tracks[t],d,s);
}

/*****************************
******** Hard_Disk **************
*****************************/

struct buffer_entry{
	int time;
	int read_or_write;//0 for read and 1 for write
	int platter_no;
	int track_no;
	int sector_no;
	char * data;

	//buffer_entry();
	//buffer_entry(int tm, int rw, int p, int t, int s, char * d);
};

typedef struct 
{

	// HEAD FOR DISK
	int trackno;

	// Direction of motion of the Arm
	int arm_dir;

	// Position of arm - wrt track number
	int arm_pos;

	int * timer;

	// time for the next operation
	int next_op_time;

	Platter *platters;	
} HardDisk;




int write_data_disk(HardDisk *H, char*d, int p, int t, int s)
{
	return write_data_platter(&H->platters[p],d,t,s);
}

int read_data_disk(HardDisk *H ,char*d, int p, int t, int s)
{
	return read_data_platter(&H->platters[p],d,t,s);
}

/*****************************
****** Disk Controller *******
*****************************/

typedef struct{
	/// Implementation of RAID 1
	HardDisk h[TOTAL_DISK];
	//HardDisk h_copy[TOTAL_DISK];
} DiskController;

int read_data(DiskController *disk, long long int addr,char data[DATA_SIZE],int time)
{
	int _diskno;
	_diskno=(int)addr/DISK_SIZE;
	if(_diskno<0 || _diskno >= TOTAL_DISK){
		//Print("%lld Error:Read Operation Failed",addr);
		return 0;
	}
	
	int address_in_disk;
	address_in_disk=(int)addr%DISK_SIZE;
	int platter_no=address_in_disk/PLATTER_SIZE;
	int address_in_platter=address_in_disk%PLATTER_SIZE;
	int track_no;
	int i;
	for (i = 0; i < NO_TRACKS; ++i)
	{
		if(address_in_platter>= (NO_SECTORS_MIN+i))
		{
			address_in_platter-=i;
			address_in_platter-=NO_SECTORS_MIN;
		}
		else{
			track_no=i;
			break;
		}
	}
	int sector_no=address_in_platter;

	return read_data_disk(&disk->h[_diskno],data,platter_no,track_no,sector_no);

}

int write_data(DiskController *disk,long long int addr,char data[DATA_SIZE],int time)
{
	int disk_no=(int)addr/DISK_SIZE;
	if(disk_no<0 || disk_no >= TOTAL_DISK){
		//Print("%lld Error:Read Operation Failed",addr);
		return 0;
	}
	
	int address_in_disk=(int)addr%DISK_SIZE;
	int platter_no=address_in_disk/PLATTER_SIZE;
	int address_in_platter=address_in_disk%PLATTER_SIZE;
	int track_no;
	int i;
	for (i = 0; i < NO_TRACKS; ++i)
	{
		if(address_in_platter>= (NO_SECTORS_MIN+i))
		{
			address_in_platter-=i;
			address_in_platter-=NO_SECTORS_MIN;
		}
		else{
			track_no=i;
			break;
		}
	}
	int sector_no=address_in_platter;

	return write_data_disk(&disk->h[disk_no],data,platter_no,track_no,sector_no);

}
