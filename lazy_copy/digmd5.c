#include<openssl/evp.h>
#include<stdio.h>

#define ALGO EVP_md5()

/** get a bufffer and calculate md5 sum of the buffer
 @buffer to find checksum
 @dgst an array with at least EVP_MD_size(md) bytes to put the result
 @n size of the buffer
*/
int digmd5(const char *buffer,  char *dgst, int n)
{
	size_t r;
	int m = 0;
	int ds;

	EVP_MD_CTX ctx;
	const EVP_MD *md;

	md = ALGO ;
	if (md == NULL) /* invalid ALGO */
		return -2;

	EVP_DigestInit(&ctx, md);
	ds = EVP_MD_size(md);

	EVP_DigestUpdate( &ctx, buffer, n);

	EVP_DigestFinal(&ctx, dgst, &ds);

	return ds;
}
