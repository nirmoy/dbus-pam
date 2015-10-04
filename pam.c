/* Define which PAM interfaces we provide */
#define PAM_SM_ACCOUNT
#define PAM_SM_AUTH
#define PAM_SM_PASSWORD
#define PAM_SM_SESSION

  /* Include PAM headers */
#include <security/pam_appl.h>
#include <security/pam_modules.h>
#include <dbus/dbus.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "logger.h"
#include <stdint.h>

#define CLIENT   "task.method.caller"
#define SERVER   "task.method.server"
#define INTERFCE "task.method.Chat"
#define OBJECT   "/task/method/Task"

#if 1

dbus_uint32_t dbus_check_is_blocked(char *method_name, const char *user_name)
{
    DBusMessage* msg;
    DBusMessageIter args;
    DBusConnection* conn;
    DBusError err;
    DBusPendingCall* pending;


    int ret;
    size_t len;
    dbus_uint32_t login_perm = 0;

    if (!method_name) return;
    dbus_error_init(&err);
    conn = dbus_bus_get(DBUS_BUS_SYSTEM, &err);
    if (dbus_error_is_set(&err)) { 
        log_err("Connection Error (%s)", err.message); 
        dbus_error_free(&err);
    }

    if (NULL == conn) { 
        exit(1); 
    }

    /* Try best to become a primary!*/
    ret = dbus_bus_request_name(conn, CLIENT, DBUS_NAME_FLAG_REPLACE_EXISTING , &err);
    if (dbus_error_is_set(&err)) { 
        log_err("Name Error (%s)", err.message); 
        dbus_error_free(&err);
    }

    if (DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER != ret) { 
        exit(1);
    }

    msg = dbus_message_new_method_call(SERVER, OBJECT, 
            INTERFCE, method_name);

    if (NULL == msg) { 
        log("Message Null");
        exit(1);
    }

    dbus_message_iter_init_append(msg, &args);
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &user_name)){
        log_err("Out Of Memory!"); 
        exit(1);
    }

    if (!dbus_connection_send_with_reply (conn, msg, &pending, -1)) {
        log_err("Out Of Memory!"); 
        exit(1);
    }

    if (NULL == pending) { 
        log_err("Pending Call Null"); 
        exit(1); 
    }

    dbus_connection_flush(conn);
    log_dbg("Request sent to server");
    dbus_message_unref(msg);
    /* Warn! its a blocking call */
    dbus_pending_call_block(pending);
    /* time to retrieve the reply back */
    msg = dbus_pending_call_steal_reply(pending);
    if (NULL == msg) {
        log_err("Reply Null\n"); 
        exit(1); 
    }

    dbus_pending_call_unref(pending);
    if (!dbus_message_iter_init(msg, &args))
        log_err("Message has no arguments!"); 
    else if (DBUS_TYPE_INT32 != dbus_message_iter_get_arg_type(&args)) 
        log_err("Argument is not String!"); 
    else
        dbus_message_iter_get_basic(&args, &login_perm);

    dbus_message_unref(msg);

	return login_perm;

}

#endif

/* PAM entry point for authentication verification */
int pam_sm_authenticate(pam_handle_t *pamh, int flags, int argc, const char **argv) 
{
    const char *user;
    if (pam_get_user(pamh, &user, NULL) != PAM_SUCCESS || !user || !*user)
    {
        log_err("Unable to retrieve the PAM user name.\n");
        return (PAM_AUTH_ERR);
    }
    if (dbus_check_is_blocked("Echo", user))
    		return(PAM_AUTH_ERR);
	else
		return (PAM_SUCCESS);
}

PAM_EXTERN
int pam_sm_setcred(pam_handle_t *pamh,int flags,int argc,
		const char **argv)
{
	return (PAM_SUCCESS);
}

#ifdef PAM_STATIC
struct pam_module _pam_usb_modstruct = {
    "pam_ignore",
    pam_sm_authenticate,
    pam_sm_setcred,
    NULL,
    NULL,
    NULL,
    NULL
};

#endif

