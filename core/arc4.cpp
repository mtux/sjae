#include "arc4.h"

void swap(char *b1, char *b2) {
 char b = *b1;
 *b1 = *b2;
 *b2 = b;
}

void arc4_init(arc4_ctx *ctx, char *key, int keylen) {
	int i;
	for(i = 0; i < 256; i++)
		ctx->lookup[i] = i;

	ctx->x = 0;	
	for(i = 0; i < 256; i++) {
        ctx->x = (key[i % keylen] + ctx->lookup[i] + ctx->x) & 0xFF;
        swap(&ctx->lookup[ctx->x], &ctx->lookup[i]);
	}
	ctx->x = 0;
	ctx->y = 0;
}

void arc4_crypt(arc4_ctx *ctx, char *dataIn, char *dataOut, int datalen) {
	for(int i = 0; i < datalen; i++) {
		ctx->x = (ctx->x + 1) & 0xFF;
		ctx->y = (ctx->lookup[ctx->x] + ctx->y) & 0xFF;
		swap(&ctx->lookup[ctx->x], &ctx->lookup[ctx->y]);
		dataOut[i] = (dataIn[i] ^ ctx->lookup[(ctx->lookup[ctx->x] + ctx->lookup[ctx->y]) & 0xFF]);
	}
}
