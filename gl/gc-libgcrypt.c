/* gc-libgcrypt.c --- Crypto wrappers around Libgcrypt for GC.
 * Copyright (C) 2002, 2003, 2004, 2005  Simon Josefsson
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2.1, or (at your
 * option) any later version.
 *
 * This file is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this file; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 */

/* Note: This file is only built if GC uses Libgcrypt. */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

/* Get prototype. */
#include "gc.h"

/* Get libgcrypt API. */
#include <gcrypt.h>

#include <assert.h>

/* Initialization. */

Gc_rc
gc_init (void)
{
  gcry_error_t err;

  err = gcry_control (GCRYCTL_ANY_INITIALIZATION_P);
  if (err == GPG_ERR_NO_ERROR)
    {
      if (gcry_check_version (GCRYPT_VERSION) == NULL)
	return GC_INIT_ERROR;

      err = gcry_control (GCRYCTL_INITIALIZATION_FINISHED, NULL, 0);
      if (err != GPG_ERR_NO_ERROR)
	return GC_INIT_ERROR;
    }

  return GC_OK;
}

void
gc_done (void)
{
  return;
}

/* Randomness. */

Gc_rc
gc_nonce (char *data, size_t datalen)
{
  gcry_create_nonce ((unsigned char *) data, datalen);
  return GC_OK;
}

Gc_rc
gc_pseudo_random (char *data, size_t datalen)
{
  gcry_randomize ((unsigned char *) data, datalen, GCRY_STRONG_RANDOM);
  return GC_OK;
}

Gc_rc
gc_random (char *data, size_t datalen)
{
  gcry_randomize ((unsigned char *) data, datalen, GCRY_VERY_STRONG_RANDOM);
  return GC_OK;
}

/* Memory allocation. */

void
gc_set_allocators (gc_malloc_t func_malloc,
		   gc_malloc_t secure_malloc,
		   gc_secure_check_t secure_check,
		   gc_realloc_t func_realloc, gc_free_t func_free)
{
  gcry_set_allocation_handler (func_malloc, secure_malloc, secure_check,
			       func_realloc, func_free);
}

/* Ciphers. */

Gc_rc
gc_cipher_open (Gc_cipher alg, Gc_cipher_mode mode,
		gc_cipher_handle * outhandle)
{
  int gcryalg, gcrymode;
  gcry_error_t err;

  switch (alg)
    {
    case GC_AES128:
      gcryalg = GCRY_CIPHER_RIJNDAEL;
      break;

    case GC_AES192:
      gcryalg = GCRY_CIPHER_RIJNDAEL;
      break;

    case GC_AES256:
      gcryalg = GCRY_CIPHER_RIJNDAEL256;
      break;

    case GC_3DES:
      gcryalg = GCRY_CIPHER_3DES;
      break;

    case GC_DES:
      gcryalg = GCRY_CIPHER_DES;
      break;

    case GC_ARCFOUR128:
    case GC_ARCFOUR40:
      gcryalg = GCRY_CIPHER_ARCFOUR;
      break;

    case GC_ARCTWO40:
      gcryalg = GCRY_CIPHER_RFC2268_40;
      break;

    default:
      return GC_INVALID_CIPHER;
    }

  switch (mode)
    {
    case GC_CBC:
      gcrymode = GCRY_CIPHER_MODE_CBC;
      break;

    case GC_STREAM:
      gcrymode = GCRY_CIPHER_MODE_STREAM;
      break;

    default:
      return GC_INVALID_CIPHER;
    }

  err = gcry_cipher_open ((gcry_cipher_hd_t *) outhandle,
			  gcryalg, gcrymode, 0);
  if (gcry_err_code (err))
    return GC_INVALID_CIPHER;

  return GC_OK;
}

Gc_rc
gc_cipher_setkey (gc_cipher_handle handle, size_t keylen, const char *key)
{
  gcry_error_t err;

  err = gcry_cipher_setkey ((gcry_cipher_hd_t) handle, key, keylen);
  if (gcry_err_code (err))
    return GC_INVALID_CIPHER;

  return GC_OK;
}

Gc_rc
gc_cipher_setiv (gc_cipher_handle handle, size_t ivlen, const char *iv)
{
  gcry_error_t err;

  err = gcry_cipher_setiv ((gcry_cipher_hd_t) handle, iv, ivlen);
  if (gcry_err_code (err))
    return GC_INVALID_CIPHER;

  return GC_OK;
}

Gc_rc
gc_cipher_encrypt_inline (gc_cipher_handle handle, size_t len, char *data)
{
  if (gcry_cipher_encrypt ((gcry_cipher_hd_t) handle,
			   data, len, NULL, len) != 0)
    return GC_INVALID_CIPHER;

  return GC_OK;
}

Gc_rc
gc_cipher_decrypt_inline (gc_cipher_handle handle, size_t len, char *data)
{
  if (gcry_cipher_decrypt ((gcry_cipher_hd_t) handle,
			   data, len, NULL, len) != 0)
    return GC_INVALID_CIPHER;

  return GC_OK;
}

Gc_rc
gc_cipher_close (gc_cipher_handle handle)
{
  gcry_cipher_close (handle);

  return GC_OK;
}

/* Hashes. */

Gc_rc
gc_hash_open (Gc_hash hash, Gc_hash_mode mode, gc_hash_handle * outhandle)
{
  int gcryalg, gcrymode;
  gcry_error_t err;

  switch (hash)
    {
    case GC_MD5:
      gcryalg = GCRY_MD_MD5;
      break;

    case GC_SHA1:
      gcryalg = GCRY_MD_SHA1;
      break;

    case GC_RMD160:
      gcryalg = GCRY_MD_RMD160;
      break;

    default:
      return GC_INVALID_HASH;
    }

  switch (mode)
    {
    case 0:
      gcrymode = 0;
      break;

    case GC_HMAC:
      gcrymode = GCRY_MD_FLAG_HMAC;
      break;

    default:
      return GC_INVALID_HASH;
    }

  err = gcry_md_open ((gcry_md_hd_t *) outhandle, gcryalg, gcrymode);
  if (gcry_err_code (err))
    return GC_INVALID_HASH;

  return GC_OK;
}

Gc_rc
gc_hash_clone (gc_hash_handle handle, gc_hash_handle * outhandle)
{
  int err;

  err = gcry_md_copy ((gcry_md_hd_t *) outhandle, (gcry_md_hd_t) handle);
  if (err)
    return GC_INVALID_HASH;

  return GC_OK;
}

size_t
gc_hash_digest_length (Gc_hash hash)
{
  int gcryalg;

  switch (hash)
    {
    case GC_MD5:
      gcryalg = GCRY_MD_MD5;
      break;

    case GC_SHA1:
      gcryalg = GCRY_MD_SHA1;
      break;

    case GC_RMD160:
      gcryalg = GCRY_MD_RMD160;
      break;

    default:
      return 0;
    }

  return gcry_md_get_algo_dlen (gcryalg);
}

void
gc_hash_hmac_setkey (gc_hash_handle handle, size_t len, const char *key)
{
  gcry_md_setkey ((gcry_md_hd_t) handle, key, len);
}

void
gc_hash_write (gc_hash_handle handle, size_t len, const char *data)
{
  gcry_md_write ((gcry_md_hd_t) handle, data, len);
}

const char *
gc_hash_read (gc_hash_handle handle)
{
  const char *digest;

  gcry_md_final ((gcry_md_hd_t) handle);
  digest = gcry_md_read ((gcry_md_hd_t) handle, 0);

  return digest;
}

void
gc_hash_close (gc_hash_handle handle)
{
  gcry_md_close ((gcry_md_hd_t) handle);
}

Gc_rc
gc_hash_buffer (Gc_hash hash, const void *in, size_t inlen, char *resbuf)
{
  int gcryalg;

  switch (hash)
    {
#ifdef GC_USE_MD5
    case GC_MD5:
      gcryalg = GCRY_MD_MD5;
      break;
#endif

#ifdef GC_USE_SHA1
    case GC_SHA1:
      gcryalg = GCRY_MD_SHA1;
      break;
#endif

#ifdef GC_USE_RMD160
    case GC_RMD160:
      gcryalg = GCRY_MD_RMD160;
      break;
#endif

    default:
      return GC_INVALID_HASH;
    }

  gcry_md_hash_buffer (gcryalg, resbuf, in, inlen);

  return GC_OK;
}

/* One-call interface. */

#ifdef GC_USE_MD5
Gc_rc
gc_md5 (const void *in, size_t inlen, void *resbuf)
{
  size_t outlen = gcry_md_get_algo_dlen (GCRY_MD_MD5);
  gcry_md_hd_t hd;
  gpg_error_t err;
  unsigned char *p;

  assert (outlen == GC_MD5_DIGEST_SIZE);

  err = gcry_md_open (&hd, GCRY_MD_MD5, 0);
  if (err != GPG_ERR_NO_ERROR)
    return GC_INVALID_HASH;

  gcry_md_write (hd, in, inlen);

  p = gcry_md_read (hd, GCRY_MD_MD5);
  if (p == NULL)
    {
      gcry_md_close (hd);
      return GC_INVALID_HASH;
    }

  memcpy (resbuf, p, outlen);

  gcry_md_close (hd);

  return GC_OK;
}
#endif

#ifdef GC_USE_SHA1
Gc_rc
gc_sha1 (const void *in, size_t inlen, void *resbuf)
{
  size_t outlen = gcry_md_get_algo_dlen (GCRY_MD_SHA1);
  gcry_md_hd_t hd;
  gpg_error_t err;
  unsigned char *p;

  assert (outlen == GC_SHA1_DIGEST_SIZE);

  err = gcry_md_open (&hd, GCRY_MD_SHA1, 0);
  if (err != GPG_ERR_NO_ERROR)
    return GC_INVALID_HASH;

  gcry_md_write (hd, in, inlen);

  p = gcry_md_read (hd, GCRY_MD_SHA1);
  if (p == NULL)
    {
      gcry_md_close (hd);
      return GC_INVALID_HASH;
    }

  memcpy (resbuf, p, outlen);

  gcry_md_close (hd);

  return GC_OK;
}
#endif

#ifdef GC_USE_HMAC_MD5
Gc_rc
gc_hmac_md5 (const void *key, size_t keylen,
	     const void *in, size_t inlen, char *resbuf)
{
  size_t hlen = gcry_md_get_algo_dlen (GCRY_MD_MD5);
  gcry_md_hd_t mdh;
  unsigned char *hash;
  gpg_error_t err;

  assert (hlen == 16);

  err = gcry_md_open (&mdh, GCRY_MD_MD5, GCRY_MD_FLAG_HMAC);
  if (err != GPG_ERR_NO_ERROR)
    return GC_INVALID_HASH;

  err = gcry_md_setkey (mdh, key, keylen);
  if (err != GPG_ERR_NO_ERROR)
    {
      gcry_md_close (mdh);
      return GC_INVALID_HASH;
    }

  gcry_md_write (mdh, in, inlen);

  hash = gcry_md_read (mdh, GCRY_MD_MD5);
  if (hash == NULL)
    {
      gcry_md_close (mdh);
      return GC_INVALID_HASH;
    }

  memcpy (resbuf, hash, hlen);

  gcry_md_close (mdh);

  return GC_OK;
}
#endif

#ifdef GC_USE_HMAC_SHA1
Gc_rc
gc_hmac_sha1 (const void *key, size_t keylen,
	      const void *in, size_t inlen, char *resbuf)
{
  size_t hlen = gcry_md_get_algo_dlen (GCRY_MD_SHA1);
  gcry_md_hd_t mdh;
  unsigned char *hash;
  gpg_error_t err;

  assert (hlen == GC_SHA1_DIGEST_SIZE);

  err = gcry_md_open (&mdh, GCRY_MD_SHA1, GCRY_MD_FLAG_HMAC);
  if (err != GPG_ERR_NO_ERROR)
    return GC_INVALID_HASH;

  err = gcry_md_setkey (mdh, key, keylen);
  if (err != GPG_ERR_NO_ERROR)
    {
      gcry_md_close (mdh);
      return GC_INVALID_HASH;
    }

  gcry_md_write (mdh, in, inlen);

  hash = gcry_md_read (mdh, GCRY_MD_SHA1);
  if (hash == NULL)
    {
      gcry_md_close (mdh);
      return GC_INVALID_HASH;
    }

  memcpy (resbuf, hash, hlen);

  gcry_md_close (mdh);

  return GC_OK;
}
#endif
