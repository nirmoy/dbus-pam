#include <dbus/dbus.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <pthread.h>
#include "logger.h"

/* make sure we have proper permission */
#define CLIENT   "task.method.caller"
#define SERVER   "task.method.server"
#define INTERFCE "task.method.Chat"
#define OBJECT   "/task/method/Task"
#define MAX_USER 100
#define CONF_DIR "./"
#define CONF_FILE "blacklist"

void *watcher(void *data);

pthread_t config_file_watcher;
pthread_mutex_t lock;
int thread_run = 1;
char *blacklist_users[MAX_USER];
unsigned int total_user;

char *get_filename_from_path(char *path)
{
	char *ssc;
	int l = 0;

	if (!path) return NULL;
	
	ssc = strstr(path, "/");
	if (!ssc)
		return path;

	do{
		l = strlen(ssc) + 1;
		path = &path[strlen(path)-l+2];
		ssc = strstr(path, "/");
	}while(ssc);

	return path;
}

void print_user(void)
{
	int i = 0;
	
	while(i < total_user) {
		printf("blocked user %d %s\n", total_user, blacklist_users[i]);
		i++;
	}
}


char *trimwhitespace(char *str)
{
	char *end;

	// Trim leading space
	while(isspace(*str)) str++;

	if(*str == 0)  // All spaces?
		return str;

	// Trim trailing space
	end = str + strlen(str) - 1;
	while(end > str && isspace(*end)) end--;

	// Write new null terminator
	*(end+1) = 0;

	return str;
}

int is_blocked(char *name) 
{
	int i = 0;

	while (i < total_user) {
		if (strcmp(blacklist_users[i++], name) == 0)
			return 1;
	}
	return 0;
}
int populate_blacklist (char *blacklist)
{
	FILE *file = fopen ( blacklist, "r" );
	total_user = 0;

	if (file != NULL) {
		pthread_mutex_lock(&lock);
		char line [1000];
		while(fgets(line,sizeof line,file)!= NULL) {
			blacklist_users[total_user++] = trimwhitespace(strdup(line));
		}

		fclose(file);
		pthread_mutex_unlock(&lock);
	}
	else {
		perror(blacklist);
		return -1;
	}
	print_user();
	return 0;
}


void reply_to_method_call(DBusMessage* msg, DBusConnection* conn)
{
    DBusMessage* reply;
    DBusMessageIter args;
    dbus_uint32_t serial = 0;
    char* param = "";
    dbus_uint32_t login_perm = 0;

    if (!dbus_message_iter_init(msg, &args))
        log_err("Message has no arguments!"); 
    else if (DBUS_TYPE_STRING != dbus_message_iter_get_arg_type(&args)) 
        log_err("Argument is not string!"); 
    else 
        dbus_message_iter_get_basic(&args, &param);

    log("Got request from sshd for user[ %s ]", param);
    reply = dbus_message_new_method_return(msg);
    dbus_message_iter_init_append(reply, &args);
	login_perm = is_blocked(param);
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_INT32, &login_perm)) { 
        log_err("Out Of Memory!"); 
        exit(1);
    }
    if (!dbus_connection_send(conn, reply, &serial)) {
        log_err("Out Of Memory!"); /* The only reason to Fail */
        exit(1);
    }

    dbus_connection_flush(conn);
    dbus_message_unref(reply);
}

void serve(char *method_name) 
{
    DBusMessage* msg;
    DBusConnection* conn;
    DBusError err;
    int ret;

    if (!method_name) return; 
    log("Listening for method calls [%s]", method_name);
    dbus_error_init(&err);
    conn = dbus_bus_get(DBUS_BUS_SYSTEM, &err);
    if (dbus_error_is_set(&err)) { 
        log("Connection Error (%s)", err.message); 
        dbus_error_free(&err); 
    }

    if (NULL == conn) {
        log_err("Connection Null"); 
        exit(1); 
    }

    ret = dbus_bus_request_name(conn, SERVER, DBUS_NAME_FLAG_REPLACE_EXISTING , &err);
    if (dbus_error_is_set(&err)) { 
        log_err("Name Error (%s)", err.message); 
        dbus_error_free(&err);
    }

    if (DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER != ret) { 
        log_err("Not Primary Owner (%d)", ret);
        exit(1); 
    }

    while (true) {
        dbus_connection_read_write(conn, 0);
        msg = dbus_connection_pop_message(conn);

        if (NULL == msg) { 
            sleep(1); 
            continue; 
        }

        log_dbg("Received\n");
        if (dbus_message_is_method_call(msg, INTERFCE, method_name)) 
            reply_to_method_call(msg, conn);

        dbus_message_unref(msg);
    }

}

int main(int argc, char** argv)
{ 
	if (2 > argc) {
        printf ("Syntax: %s [conf_file_path]\n", argv[0]);
        return 1;
    }

	if( access( argv[1], F_OK ) == -1 ) {
		log_err("Specify correct path");
		return 1;
	}

	if (populate_blacklist(get_filename_from_path(argv[1])) == -1) {
		log_err("Failed to read %s", get_filename_from_path(argv[1]));
		return 1;
	}
	
	if (pthread_mutex_init(&lock, NULL) != 0)
	{
		log_err("\n mutex init failed\n");
		return 1;
	}
	
	if(pthread_create(&config_file_watcher, NULL, watcher, argv[1])) {

		log_err("Error creating thread\n");
		return 1;

	}
	/* Meat */
    serve("Echo");
	thread_run = 0;
	if(pthread_join(config_file_watcher, NULL)) {

		log_err("Error joining thread\n");
		return 1;
	}

	pthread_mutex_destroy(&lock);

    return 0;
}
