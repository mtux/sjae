#include "disco.h"

DiscoInfo::DiscoInfo(void)
{
}

DiscoInfo::~DiscoInfo(void)
{
}

DiscoItems::DiscoItems(void)
{
}

DiscoItems::~DiscoItems(void)
{
}

void registerDiscoMetaTypes() {
	qRegisterMetaType<DiscoInfo>("DiscoInfo");
	qRegisterMetaType<DiscoItems>("DiscoItems");
}
