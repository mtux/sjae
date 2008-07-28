#ifndef _GLOBAL_STATUS_H
#define _GLOBAL_STATUS_H

typedef enum {ST_OFFLINE, ST_ONLINE, ST_INVISIBLE, ST_FREETOCHAT, ST_SHORTAWAY, ST_LONGAWAY, ST_DND, ST_OUTTOLUNCH, ST_ONTHEPHONE, ST_CONNECTING} GlobalStatus;

static char *status_name[] = {
	"status_offline",
	"status_online",
	"status_invisible",
	"status_free_to_chat",
	"status_short_away",
	"status_long_away",
	"status_dnd",
	"status_out_to_lunch",
	"status_on_the_phone",
	"status_connecting"
};

static char *hr_status_name[] = {
	"Offline",
	"Online",
	"Invisible",
	"Free to Chat",
	"Short Away",
	"Long Away",
	"DND",
	"Out to Lunch",
	"On the Phone",
	"Connecting"
};

#define each_status(x)				for(GlobalStatus x = ST_OFFLINE; x <= ST_CONNECTING; x = (GlobalStatus)((int)x + 1))
#define each_contact_status(x)		for(GlobalStatus x = ST_OFFLINE; x <= ST_ONTHEPHONE; x = (GlobalStatus)((int)x + 1))

#endif
