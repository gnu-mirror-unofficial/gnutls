/*
 * GnuTLS PKCS#11 support
 * Copyright (C) 2010 Free Software Foundation
 * 
 * Author: Nikos Mavrogiannopoulos
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA
*/

#include <gnutls_int.h>
#include <pakchois/pakchois.h>
#include <gnutls/pkcs11.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <gnutls_errors.h>
#include <gnutls_datum.h>
#include <pkcs11_int.h>
#include <sign.h>

struct gnutls_pkcs11_privkey_st {
	pakchois_session_t *pks;
	ck_object_handle_t obj;
	gnutls_pk_algorithm_t pk_algorithm;
	unsigned int flags;
	struct pkcs11_url_info info;
};

/**
 * gnutls_pkcs11_privkey_init:
 * @key: The structure to be initialized
 *
 * This function will initialize an private key structure.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS is returned, otherwise a
 *   negative error value.
 **/
int gnutls_pkcs11_privkey_init(gnutls_pkcs11_privkey_t * key)
{
	*key = gnutls_calloc(1, sizeof(struct gnutls_pkcs11_privkey_st));
	if (*key == NULL) {
		gnutls_assert();
		return GNUTLS_E_MEMORY_ERROR;
	}
	(*key)->obj = CK_INVALID_HANDLE;
	
	return 0;
}

/**
 * gnutls_pkcs11_privkey_deinit:
 * @key: The structure to be initialized
 *
 * This function will deinitialize a private key structure.
 **/
void gnutls_pkcs11_privkey_deinit(gnutls_pkcs11_privkey_t key)
{
	if (key->pks) {
		pakchois_close_session(key->pks);
    }
	gnutls_free(key);
}

/**
 * gnutls_pkcs11_privkey_get_pk_algorithm:
 * @key: should contain a #gnutls_pkcs11_privkey_t structure
 *
 * This function will return the public key algorithm of a private
 * key.
 *
 * Returns: a member of the #gnutls_pk_algorithm_t enumeration on
 *   success, or a negative value on error.
 **/
int gnutls_pkcs11_privkey_get_pk_algorithm(gnutls_pkcs11_privkey_t key, unsigned int *bits)
{
        if (bits)
          *bits = 0; /* FIXME */
	return key->pk_algorithm;
}

/**
 * gnutls_pkcs11_privkey_get_info:
 * @key: should contain a #gnutls_pkcs11_privkey_t structure
 * @itype: Denotes the type of information requested
 * @output: where output will be stored
 * @output_size: contains the maximum size of the output and will be overwritten with actual
 *
 * This function will return information about the PKCS 11 private key such
 * as the label, id as well as token information where the key is stored. When
 * output is text it returns null terminated string although #output_size contains
 * the size of the actual data only.
 *
 * Returns: zero on success or a negative value on error.
 **/
int gnutls_pkcs11_privkey_get_info(gnutls_pkcs11_privkey_t pkey,
				   gnutls_pkcs11_obj_info_t itype,
				   void *output, size_t * output_size)
{
	return pkcs11_get_info(&pkey->info, itype, output, output_size);
}

#define RETRY_BLOCK_START int retries = 0; retry:


/* the rescan_slots() here is a dummy but if not
 * called my card fails to work when removed and inserted.
 * May have to do with the pkcs11 library I use.
 */
#define RETRY_CHECK(rv, key) { \
		if (token_func && (rv == CKR_SESSION_HANDLE_INVALID||rv==CKR_DEVICE_REMOVED)) { \
			pkcs11_rescan_slots(); \
			pakchois_close_session(key->pks); \
			pkcs11_rescan_slots(); \
			key->pks = NULL; \
			key->obj = CK_INVALID_HANDLE; \
			ret = token_func(token_data, key->info.label, retries++); \
			if (ret == 0) { \
				pkcs11_find_object (&key->pks, &key->obj, &key->info, SESSION_LOGIN); \
				goto retry; \
			} \
		} \
	}

/**
 * gnutls_pkcs11_privkey_sign_data:
 * @signer: Holds the key
 * @digest: should be MD5 or SHA1
 * @flags: should be 0 for now
 * @data: holds the data to be signed
 * @signature: will contain the signature allocated with gnutls_malloc()
 *
 * This function will sign the given data using a signature algorithm
 * supported by the private key. Signature algorithms are always used
 * together with a hash functions.  Different hash functions may be
 * used for the RSA algorithm, but only SHA-1 for the DSA keys.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS is returned, otherwise a
 *   negative error value.
 **/
int
gnutls_pkcs11_privkey_sign_data(gnutls_pkcs11_privkey_t signer,
				gnutls_digest_algorithm_t hash,
				unsigned int flags,
				const gnutls_datum_t * data,
				gnutls_datum_t * signature)
{
	int ret;
	gnutls_datum_t digest;

	switch (signer->pk_algorithm) {
	case GNUTLS_PK_RSA:
		ret = pk_pkcs1_rsa_hash(hash, data, &digest);
		if (ret < 0) {
			gnutls_assert();
			return ret;
		}
		break;
	case GNUTLS_PK_DSA:
		ret = pk_dsa_hash(data, &digest);
		if (ret < 0) {
			gnutls_assert();
			return ret;
		}

		break;
	default:
		gnutls_assert();
		return GNUTLS_E_INTERNAL_ERROR;
	}

	ret = gnutls_pkcs11_privkey_sign_hash(signer, &digest, signature);
	_gnutls_free_datum(&digest);

	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	return 0;

}

/**
 * gnutls_pkcs11_privkey_sign_hash:
 * @key: Holds the key
 * @hash: holds the data to be signed (should be output of a hash)
 * @signature: will contain the signature allocated with gnutls_malloc()
 *
 * This function will sign the given data using a signature algorithm
 * supported by the private key. It is assumed that the given data
 * are the output of a hash function.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS is returned, otherwise a
 *   negative error value.
 **/
int gnutls_pkcs11_privkey_sign_hash(gnutls_pkcs11_privkey_t key,
				    const gnutls_datum_t * hash,
				    gnutls_datum_t * signature)
{
	ck_rv_t rv;
	int ret;
	struct ck_mechanism mech;
	unsigned long siglen;

	RETRY_BLOCK_START;

	if (key->obj == CK_INVALID_HANDLE || key->pks == NULL) {
		gnutls_assert();
		return GNUTLS_E_PKCS11_ERROR;
	}

	mech.mechanism =
	    key->pk_algorithm == GNUTLS_PK_DSA ? CKM_DSA : CKM_RSA_PKCS;
	mech.parameter = NULL;
	mech.parameter_len = 0;

	/* Initialize signing operation; using the private key discovered
	 * earlier. */
	rv = pakchois_sign_init(key->pks, &mech, key->obj);
	if (rv != CKR_OK) {
		RETRY_CHECK(rv, key);
		gnutls_assert();
		return GNUTLS_E_PK_SIGN_FAILED;
	}

	/* Work out how long the signature must be: */
	rv = pakchois_sign(key->pks, hash->data, hash->size, NULL,
			   &siglen);
	if (rv != CKR_OK) {
		RETRY_CHECK(rv, key);
		gnutls_assert();
		return GNUTLS_E_PK_SIGN_FAILED;
	}

	signature->data = gnutls_malloc(siglen);
	signature->size = siglen;

	rv = pakchois_sign(key->pks, hash->data, hash->size,
			   signature->data, &siglen);
	if (rv != CKR_OK) {
		gnutls_free(signature->data);
		RETRY_CHECK(rv, key);
		gnutls_assert();
		return GNUTLS_E_PK_SIGN_FAILED;
	}

	signature->size = siglen;

	return 0;
}


/**
 * gnutls_pkcs11_privkey_import_url:
 * @pkey: The structure to store the parsed key
 * @url: a PKCS 11 url identifying the key
 * @flags: sequence of GNUTLS_PKCS_PRIVKEY_*
 *
 * This function will "import" a PKCS 11 URL identifying a private
 * key to the #gnutls_pkcs11_privkey_t structure. In reality since
 * in most cases keys cannot be exported, the private key structure
 * is being associated with the available operations on the token.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS is returned, otherwise a
 *   negative error value.
 **/
int gnutls_pkcs11_privkey_import_url(gnutls_pkcs11_privkey_t pkey,
				     const char *url, unsigned int flags)
{
	int ret;

	ret = pkcs11_url_to_info(url, &pkey->info);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	if (pkey->info.id[0] == 0) {
		gnutls_assert();
		return GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE;
	}

	ret = pkcs11_find_object (&pkey->pks, &pkey->obj, &pkey->info, SESSION_LOGIN);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}
	pkey->flags = flags;

	return 0;
}

/**
 * gnutls_pkcs11_privkey_decrypt_data:
 * @key: Holds the key
 * @flags: should be 0 for now
 * @ciphertext: holds the data to be signed
 * @plaintext: will contain the plaintext, allocated with gnutls_malloc()
 *
 * This function will decrypt the given data using the public key algorithm
 * supported by the private key. 
 *
 * Returns: On success, %GNUTLS_E_SUCCESS is returned, otherwise a
 *   negative error value.
 **/
int
gnutls_pkcs11_privkey_decrypt_data(gnutls_pkcs11_privkey_t key,
  unsigned int flags, const gnutls_datum_t * ciphertext,
				gnutls_datum_t * plaintext)
{
	ck_rv_t rv;
	int ret;
	struct ck_mechanism mech;
	unsigned long siglen;

	RETRY_BLOCK_START;

	if (key->obj == CK_INVALID_HANDLE) {
		gnutls_assert();
		return GNUTLS_E_PKCS11_ERROR;
	}

	mech.mechanism =
	    key->pk_algorithm == GNUTLS_PK_DSA ? CKM_DSA : CKM_RSA_PKCS;
	mech.parameter = NULL;
	mech.parameter_len = 0;

	/* Initialize signing operation; using the private key discovered
	 * earlier. */
	rv = pakchois_decrypt_init(key->pks, &mech, key->obj);
	if (rv != CKR_OK) {
		RETRY_CHECK(rv, key);
		gnutls_assert();
		return GNUTLS_E_PK_DECRYPTION_FAILED;
	}

	/* Work out how long the plaintext must be: */
	rv = pakchois_decrypt(key->pks, ciphertext->data, ciphertext->size, NULL,
			   &siglen);
	if (rv != CKR_OK) {
		RETRY_CHECK(rv, key);
		gnutls_assert();
		return GNUTLS_E_PK_DECRYPTION_FAILED;
	}

	plaintext->data = gnutls_malloc(siglen);
	plaintext->size = siglen;

	rv = pakchois_decrypt(key->pks, ciphertext->data, ciphertext->size,
			   plaintext->data, &siglen);
	if (rv != CKR_OK) {
		gnutls_free(plaintext->data);
		gnutls_assert();
		return GNUTLS_E_PK_DECRYPTION_FAILED;
	}

	plaintext->size = siglen;

	return 0;
}

/**
 * gnutls_pkcs11_privkey_export_url:
 * @key: Holds the PKCS 11 key
 * @url: will contain an allocated url
 *
 * This function will export a URL identifying the given key.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS is returned, otherwise a
 *   negative error value.
 **/
int gnutls_pkcs11_privkey_export_url (gnutls_pkcs11_privkey_t key, char ** url)
{
int ret;

    ret = pkcs11_info_to_url(&key->info, url);
    if (ret < 0) {
        gnutls_assert();
        return ret;
    }

    return 0;
}