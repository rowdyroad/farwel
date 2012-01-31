/*
 * SOURCE:  http://www.aarongifford.com/computers/sha.html
 * FILE:    http://www.aarongifford.com/computers/hmac_sha1.tar.gz
 * DATE:    27 June 2008
 * LICENCE: Public Domain (sha.h)
 * LICENCE: BSD (hmac_sha1.h)
 *
 * Modified to add const specifiers and the SHA1() and HMAC_SHA1()
 * convenience functions.
 */

/*
 * sha.h
 *
 * Originally taken from the public domain SHA1 implementation
 * written by by Steve Reid <steve@edmweb.com>
 *
 * Modified by Aaron D. Gifford <agifford@infowest.com>
 *
 * NO COPYRIGHT - THIS IS 100% IN THE PUBLIC DOMAIN
 *
 * The original unmodified version is available at:
 *    ftp://ftp.funet.fi/pub/crypt/hash/sha/sha1.c
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR(S) OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef __SHA1_H__
#define __SHA1_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Define this if your machine is LITTLE_ENDIAN, otherwise #undef it: */
#ifndef LITTLE_ENDIAN
#define LITTLE_ENDIAN
#endif

/* Make sure you define these types for your architecture: */
typedef unsigned int    sha1_quadbyte;  /* 4 byte type */
typedef unsigned char   sha1_byte;      /* single byte type */

/*
 * Be sure to get the above definitions right.  For instance, on my
 * x86 based FreeBSD box, I define LITTLE_ENDIAN and use the type
 * "unsigned long" for the quadbyte.  On FreeBSD on the Alpha, however,
 * while I still use LITTLE_ENDIAN, I must define the quadbyte type
 * as "unsigned int" instead.
 */

#define SHA1_BLOCK_LENGTH     64
#define SHA1_DIGEST_LENGTH    20

/* The SHA1 structure: */
typedef struct _SHA_CTX
{
    sha1_quadbyte state[5];
    sha1_quadbyte count[2];
    sha1_byte     buffer[SHA1_BLOCK_LENGTH];
} SHA_CTX;

#ifndef NOPROTO
void SHA1_Init(SHA_CTX *context);
void SHA1_Update(SHA_CTX *context, const sha1_byte *data, unsigned int len);
void SHA1_Final(sha1_byte digest[SHA1_DIGEST_LENGTH], SHA_CTX * context);
void SHA1(sha1_byte digest[SHA1_DIGEST_LENGTH], const sha1_byte * data, unsigned int len);
#else
void SHA1_Init();
void SHA1_Update();
void SHA1_Final();
void SHA1();
#endif

#ifdef  __cplusplus
}
#endif

#endif



/*
 * hmac_sha1.h
 *
 * Version 1.0.0
 *
 * Written by Aaron D. Gifford <me@aarongifford.com>
 *
 * Copyright 1998, 2000 Aaron D. Gifford.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR(S) OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 *
 *
 *
 *
 *

