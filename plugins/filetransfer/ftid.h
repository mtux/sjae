#ifndef FTID_H
#define FTID_H

#include <contact_info_i.h>

class FTId
{
public:
    FTId();

	QString id;
	Contact *contact;
	bool incoming;

	bool operator==(const FTId &other) const;
	bool operator<(const FTId &other) const;
};

#endif // FTID_H
