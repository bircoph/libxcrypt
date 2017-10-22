/*-
 * Copyright (c) 2003 Michael Bretterklieber
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "crypt-port.h"
#include "alg-md4.h"
#include "crypt-private.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>

/*
 * NT HASH = md4(str2unicode(phrase))
 */

void
crypt_nthash_rn (const char *phrase,
                 ARG_UNUSED(const char *setting),
                 uint8_t *output,
                 size_t o_size,
                 void *scratch,
                 size_t s_size)
{
  size_t unipwLen;
  int i;
  static const char hexconvtab[] = "0123456789abcdef";
  static const char *magic = "$3$";
  uint16_t unipw[128];
  unsigned char hash[16];
  const char *s;

  if ((o_size < 4 + 32) ||
      (s_size < sizeof (struct md4_ctx)))
  {
    errno = ERANGE;
    return;
  }

  struct md4_ctx *ctx = scratch;

  memset (unipw, 0, sizeof(unipw));
  /* convert to unicode (thanx Archie) */
  unipwLen = 0;
  for (s = phrase; unipwLen < sizeof(unipw) / 2 && *s; s++)
    unipw[unipwLen++] = htons((uint16_t)(*s << 8));

  /* Compute MD4 of Unicode password */
  md4_init_ctx (ctx);
  md4_process_bytes ((unsigned char *)unipw, ctx, unipwLen*sizeof(uint16_t));
  md4_finish_ctx (ctx, hash);

  output = (uint8_t *)stpcpy ((char *)output, magic);
  *output++ = '$';
  for (i = 0; i < 16; i++) {
    *output++ = (uint8_t)hexconvtab[hash[i] >> 4];
    *output++ = (uint8_t)hexconvtab[hash[i] & 0xf];
  }
  *output = '\0';
}

void
gensalt_nthash_rn (unsigned long count,
                   const uint8_t *rbytes,
                   size_t nrbytes,
                   uint8_t *output,
                   size_t o_size)
{
  static const char *salt = "$3$__not_used__";
  struct md4_ctx ctx;
  unsigned char hashbuf[16];
  char hashstr[14];
  unsigned long i;

  /* Minimal O_SIZE to store the fake salt.
     At least 1 byte of RBYTES is needed
     to calculate the MD4 hash used in the
     fake salt.  */
  if ((o_size < 29) || (nrbytes < 1))
  {
    errno = ERANGE;
    return;
  }

  if (count < 20)
    count = 20;

  md4_init_ctx (&ctx);
  for (i = 0; i < count; i++)
  {
    md4_process_bytes (salt, &ctx, (i % 15) + 1);
    md4_process_bytes (rbytes, &ctx, nrbytes);
    md4_process_bytes (salt, &ctx, 15);
    md4_process_bytes (salt, &ctx, 15 - (i % 15));
  }
  md4_finish_ctx (&ctx, &hashbuf);

  for (i = 0; i < 7; i++)
    sprintf (&(hashstr[i * 2]), "%02x", hashbuf[i]);

  memcpy (output, salt, 15);
  memcpy (output + 15, hashstr, 14);
}
