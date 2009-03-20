#include "ftid.h"

FTId::FTId()
{
}

bool FTId::operator==(const FTId &other) const {
	return id == other.id && contact == other.contact && incoming == other.incoming;
}

bool FTId::operator<(const FTId &other) const {
	return id < other.id || contact < other.contact || (incoming ? 1 : 0) < (other.incoming ? 1 : 0);
}
