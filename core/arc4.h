#ifndef _ARC4_INC
#define _ARC4_INC

typedef struct {
	char lookup[256];
	int x, y;
} arc4_ctx;

void arc4_init(arc4_ctx *ctx, char *key, int keylen);
void arc4_crypt(arc4_ctx *ctx, char *dataIn, char *dataOut, int datalen);

#endif
