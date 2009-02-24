#ifndef _GLOBAL_STATUS_H
#define _GLOBAL_STATUS_H

typedef enum {ST_OFFLINE, ST_INVISIBLE, ST_DND, ST_LONGAWAY, ST_SHORTAWAY, ST_OUTTOLUNCH, ST_ONTHEPHONE, ST_FREETOCHAT, ST_ONLINE, ST_CONNECTING} GlobalStatus;

static const char *status_name[] = {
	"status_offline",
	"status_invisible",
	"status_dnd",
	"status_long_away",
	"status_short_away",
	"status_out_to_lunch",
	"status_on_the_phone",
	"status_free_to_chat",
	"status_online",
	"status_connecting"
};

static const char *hr_status_name[] = {
	"Offline",
	"Invisible",
	"DND",
	"Long Away",
	"Short Away",
	"Out to Lunch",
	"On the Phone",
	"Free to Chat",
	"Online",
	"Connecting"
};

#define each_status(x)				for(GlobalStatus x = ST_OFFLINE; x <= ST_CONNECTING; x = (GlobalStatus)((int)x + 1))
#define each_contact_status(x)		for(GlobalStatus x = ST_OFFLINE; x <= ST_ONLINE; x = (GlobalStatus)((int)x + 1))

#endif
