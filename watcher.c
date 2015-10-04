#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/inotify.h>
#include <pthread.h>
#include <dirent.h>
#include "logger.h"

#define EVENT_SIZE  ( sizeof (struct inotify_event) )
#define BUF_LEN     ( 1024 * ( EVENT_SIZE + 16 ) )
#define TIMEOUT 2/* 2 sec timeout for select*/
#define CONF_FILE "blacklist"

extern int thread_run;
int populate_blacklist (char *blacklist);
char *get_filename_from_path(char *path);
fd_set descriptors, tempdesc;


void handle_file(char *name, char *full_path)
{
	if (strcmp(name, get_filename_from_path(full_path)) == 0) {
		populate_blacklist(full_path);
	}

}

int watch_dir(int fd, char *_path)
{
	int wd;
	char path[4096] = {0};
	
	strcat(path, _path);

	wd = inotify_add_watch( fd, path,
			IN_MODIFY | IN_CREATE | IN_DELETE );
	if ( wd < 0 ) {
		log_err( "inotify_init" );
		return -1;
	}
	log_dbg("new fd reg %d %s\n", wd, path);
	return wd;
}

void *watcher(void *data) 
{
	int length, i = 0;
	int fd, fd_max;
	int wd;
	int ret;
	char buffer[BUF_LEN];
	char conf_dir[512], *conf_file;
	struct timeval tv = {TIMEOUT, 0};


	if (!data) pthread_exit(NULL);

	conf_file = get_filename_from_path((char *)data);
	if (conf_file == data)
		memcpy(conf_dir, "./", 2);
	else
		memcpy(conf_dir, (char *)data, (conf_file - (char *) data));
	fd = inotify_init();

	if ( fd < 0 ) {
		perror( "inotify_init" );
	}

	wd = watch_dir(fd,  conf_dir);
	
	FD_ZERO ( &descriptors);
	FD_SET (fd, &descriptors);
	fd_max = fd;

	while(1) {
		i = 0;
		tv.tv_sec = TIMEOUT;
		tv.tv_usec = 0;
		memcpy(&tempdesc, &descriptors, sizeof(tempdesc));
		ret = select ( fd_max + 1, &tempdesc, NULL, NULL, &tv);
		if (ret < 0) {
			log_err("select error\n");
			pthread_exit(NULL);
		}

		if (ret == 0) {
			/* Do I need to run */
			if (!thread_run)
				pthread_exit(NULL);
			log_dbg("Timeout\n");
			continue;

		}

		FD_CLR(fd, &tempdesc);
		length = read( fd, buffer, BUF_LEN );  
		if ( length < 0 ) {
			log_err("read error");
			pthread_exit(NULL);
		}  

		while ( i < length ) {
			struct inotify_event *event = ( struct inotify_event * ) &buffer[ i ];
			if ( event->len ) {
				if ( event->mask & IN_MODIFY ) {
					if ( event->mask & IN_ISDIR ) {
						log_dbg( "The directory %s was modified.\n", event->name );
					}
					else {
						log_dbg( "The file %s was modified.\n", event->name );
						handle_file(event->name, (char *) data);
					}
				}
			}
			i += EVENT_SIZE + event->len;
		}
	}
	inotify_rm_watch(fd, wd);
	close(fd);

	pthread_exit(NULL);
}
