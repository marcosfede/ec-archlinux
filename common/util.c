/* Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Utility functions for Chrome EC */

#include "util.h"

int strlen(const char *s)
{
	int len = 0;

	while (*s++)
		len++;

	return len;
}


int isspace(int c)
{
	return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}


int isdigit(int c)
{
	return c >= '0' && c <= '9';
}


int isalpha(int c)
{
	return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

int isprint(int c)
{
	return c >= ' ' && c <= '~';
}

int tolower(int c)
{
	return c >= 'A' && c <= 'Z' ? c + 'a' - 'A' : c;
}


int strcasecmp(const char *s1, const char *s2)
{
	int diff;
	do {
		diff = tolower(*s1) - tolower(*s2);
		if (diff)
			return diff;
	} while (*(s1++) && *(s2++));
	return 0;
}


int strncasecmp(const char *s1, const char *s2, int size)
{
	int diff;

	if (!size)
		return 0;

	do {
		diff = tolower(*s1) - tolower(*s2);
		if (diff)
			return diff;
	} while (*(s1++) && *(s2++) && --size);
	return 0;
}


int atoi(const char *nptr)
{
	int result = 0;
	int neg = 0;
	char c = '\0';

	while ((c = *nptr++) && isspace(c)) {}

	if (c == '-') {
		neg = 1;
		c = *nptr++;
	}

	while (isdigit(c)) {
		result = result * 10 + (c - '0');
		c = *nptr++;
	}

	return neg ? -result : result;
}


/* Like strtol(), but for integers */
int strtoi(const char *nptr, char **endptr, int base)
{
	int result = 0;
	int neg = 0;
	int c = '\0';

	if (endptr)
		*endptr = (char *)nptr;

	while((c = *nptr++) && isspace(c)) {}

	if (c == '0' && *nptr == 'x') {
		base = 16;
		c = nptr[1];
		nptr += 2;
	} else if (base == 0) {
		base = 10;
		if (c == '-') {
			neg = 1;
			c = *nptr++;
		}
	}

	while (c) {
		if (c >= '0' && c < '0' + MIN(base, 10))
			result = result * base + (c - '0');
		else if (c >= 'A' && c < 'A' + base - 10)
			result = result * base + (c - 'A' + 10);
		else if (c >= 'a' && c < 'a' + base - 10)
			result = result * base + (c - 'a' + 10);
		else
			break;

		if (endptr)
			*endptr = (char *)nptr;
		c = *nptr++;
	}

	return neg ? -result : result;
}


int memcmp(const void *s1, const void *s2, int len)
{
	const char *sa = s1;
	const char *sb = s2;

	int diff = 0;
	while (len-- > 0) {
		diff = *(sa++) - *(sb++);
		if (diff)
			return diff;
	}

	return 0;
}


void *memcpy(void *dest, const void *src, int len)
{
	/* TODO: optimized version using LDM/STM would be much faster */
	char *d = (char *)dest;
	const char *s = (const char *)src;
	while (len > 0) {
		*(d++) = *(s++);
		len--;
	}
	return dest;
}


void *memset(void *dest, int c, int len)
{
	/* TODO: optimized version using STM would be much faster */
	char *d = (char *)dest;
	while (len > 0) {
		*(d++) = c;
		len--;
	}
	return dest;
}


void *memmove(void *dest, const void *src, int len)
{
	if ((uint32_t)dest <= (uint32_t)src ||
	    (uint32_t)dest >= (uint32_t)src + len) {
		/* Start of destination doesn't overlap source, so just use
		 * memcpy(). */
		return memcpy(dest, src, len);
	} else {
		/* Copy from end, so we don't overwrite the source */
		char *d = (char *)dest + len;
		const char *s = (const char *)src + len;
		/* TODO: optimized version using LDM/STM would be much faster */
		while (len > 0) {
			*(--d) = *(--s);
			len--;
		}
		return dest;
	}
}


char *strzcpy(char *dest, const char *src, int len)
{
	char *d = dest;
	if (len <= 0)
		return dest;
	while (len > 1 && *src) {
		*(d++) = *(src++);
		len--;
	}
	*d = '\0';
	return dest;
}


int uint64divmod(uint64_t *n, int d)
{
	uint64_t q = 0, mask;
	int r = 0;

	/* Divide-by-zero returns zero */
	if (!d) {
		*n = 0;
		return 0;
	}

	/* Common powers of 2 = simple shifts */
	if (d == 2) {
		r = *n & 1;
		*n >>= 1;
		return r;
	} else if (d == 16) {
		r = *n & 0xf;
		*n >>= 4;
		return r;
	}

	/* If v fits in 32-bit, we're done. */
	if (*n <= 0xffffffff) {
		uint32_t v32 = *n;
		r = v32 % d;
		*n = v32 / d;
		return r;
	}

	/* Otherwise do integer division the slow way. */
	for (mask = (1ULL << 63); mask; mask >>= 1) {
		r <<= 1;
		if (*n & mask)
			r |= 1;
		if (r >= d) {
			r -= d;
			q |= mask;
		}
	}
	*n = q;
	return r;
}
