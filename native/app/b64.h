/*
 * Modified version of http://web.mit.edu/freebsd/head/contrib/wpa/src/utils/base64.c
 * This software may be distributed under the terms of the BSD license.
 */

#ifndef B64_H
#define B64_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Only For GoogleTest
#ifdef __cplusplus
extern "C" {
#endif

unsigned char *base64_encode(const unsigned char *src, size_t len, size_t *out_len);
unsigned char *base64_decode(const unsigned char *src, size_t len, size_t *out_len);

#ifdef __cplusplus
}
#endif

#endif // B64_H
