// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include <lib/assert.h>
#include <lib/string.h>
#include <lib/stdlib.h>

static const char *digits = "0123456789abcdef";

static
int u64toan_10(uint64_t val, char *str, size_t size)
{
	int i;
	uint64_t t, r;
	size_t req_size;

	t = val;
	req_size = 0;
	do {
		++req_size;
		t = divmod(t, 10, NULL);
	} while (t);

	if (size < req_size)
		return 0;

	t = val;
	for (i = req_size - 1; i >= 0; --i) {
		t = divmod(t, 10, &r);
		str[i] = digits[r];
	}
	return req_size;
}

static
int u64toan_16(uint64_t val, char *str, size_t size)
{
	int i;
	uint64_t t;
	size_t req_size;

	t = val;

	req_size = 1;
	while (t >>= 4)
		++req_size;

	if (size < req_size)
		return 0;

	for (i = req_size - 1; i >= 0; --i, val >>= 4)
		str[i] = digits[val & 0xf];
	return req_size;
}

static
int u64toan(uint64_t val, char *str, size_t size, int radix)
{
	if (radix > 16)
		return 0;
	if (radix == 16)
		return u64toan_16(val, str, size);
	if (radix == 10)
		return u64toan_10(val, str, size);
	return 0;
}

static
int do_vsnprintf(char *str, size_t size, va_list *ap, char len_mod,
		 char conv_spec)
{
	size_t i;
	const char *p;
	uint64_t val;

	if (conv_spec == 'x') {
		if (len_mod == 'l')
			val = va_arg(*ap, uint64_t);
		else
			val = va_arg(*ap, uint32_t);
		return u64toan(val, str, size, 16);
	} else if (conv_spec == 'd') {
		if (len_mod == 'l')
			val = (int64_t)va_arg(*ap, int64_t);
		else
			val = (int64_t)va_arg(*ap, int32_t);
		return u64toan(val, str, size, 10);
	} else if (conv_spec == 's') {
		p = va_arg(*ap, const char *);
		if (p == NULL) return 0;
		for (i = 0; i < size && p[i]; ++i)
			str[i] = p[i];
		return i;
	}
	return 0;
}

int vsnprintf(char *str, size_t size, const char *fmt, va_list ap)
{
	size_t i;
	int j, fmt_len;
	char len_mod, conv_spec;

	fmt_len = strlen(fmt);
	str[0] = 0;

	for (i = 0, j = 0; i < size;) {
		// Copy ordinary characters (not %) unchanged.
		for (; i < size && j < fmt_len && fmt[j] != '%'; ++j, ++i)
			str[i] = fmt[j];

		// Reached end of the inputs.
		if (i == size || j == fmt_len)
			break;
		// Conversion specification:
		// %[flags][field width][precision][length modifier]specifier
		++j;	// Skip the initial %.

		if (j == fmt_len)	// End of fmt string?
			break;

		// Read length modifier.
		len_mod = 0;
		switch (fmt[j]) {
		case 'l':
			len_mod = fmt[j];
			++j;
			break;
		}

		if (j == fmt_len)	// End of fmt string?
			break;

		// Read conversion specifier.
		conv_spec = 0;
		switch (fmt[j]) {
		case 'x':	// x is an unsigned int.
		case 's':
		case 'd':
			conv_spec = fmt[j];
			++j;
			break;
		}

		if (conv_spec == 0)
			continue;
		i += do_vsnprintf(str + i, size - i, &ap, len_mod, conv_spec);
	}

	if (i == size)
		--i;

	str[i] = 0;
	return i;
}

int snprintf(char *str, size_t size, const char *fmt, ...)
{
	int ret;
	va_list ap;

	va_start(ap, fmt);
	ret = vsnprintf(str, size, fmt, ap);
	va_end(ap);
	return ret;
}
