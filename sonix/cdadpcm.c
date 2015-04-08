/*
 * Copyright (c) 2013-2015 sephiroth99
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <limits.h>
#include <stdio.h>

static short step_table[89] = {
	28, 32, 36, 40, 44, 48, 52, 56,
	64, 68, 76, 84, 92, 100, 112, 124,
	136, 148, 164, 180, 200, 220, 240, 264,
	292, 320, 352, 388, 428, 472, 520, 572,
	628, 692, 760, 836, 920, 1012, 1116, 1228,
	1348, 1484, 1632, 1796, 1976, 2176, 2392, 2632,
	2896, 3184, 3504, 3852, 4240, 4664, 5128, 5644,
	6208, 6828, 7512, 8264, 9088, 9996, 10996, 12096,
	13308, 14640, 16104, 17712, 19484, 21432, 23576, 25936,
	28528, 31380, 32764, 32764, 32764, 32764, 32764, 32764,
	32764, 32764, 32764, 32764, 32764, 32764, 32764, 32764,
	32764
};

static char index_table[16] = {
	-1, -1, -1, -1, 2, 4, 6, 8,
	-1, -1, -1, -1, 2, 4, 6, 8
};

static short init_table[16] = {
	0x0800, 0x1800, 0x2800, 0x3800, 0x4800, 0x5800, 0x6800, 0x7800,
	0xF800, 0xE800, 0xD800, 0xC800, 0xB800, 0xA800, 0x9800, 0x8800
};

static short value;
static int index_step;

static short get_sample(int code)
{
	short val1 = step_table[index_step];
	short val2 = init_table[code];
	short delta = (short)(((int)val1 * (int)val2) >> 16);

	index_step += index_table[code];
	if (index_step < 0)
		index_step = 0;
	else if (index_step > 88)
		index_step = 88;

	int tmp = value + delta;
	if (tmp > SHRT_MAX)
		tmp = SHRT_MAX;
	else if (tmp < SHRT_MIN)
		tmp = SHRT_MIN;

	value = (short)tmp;

	return value;
}

void decode_block(unsigned char * data, FILE * out)
{
	int ptr = 0;

	/* Get initial sample */
	value = data[ptr++];
	value += (data[ptr++] << 8);
	fwrite(&value, sizeof(value), 1, out);

	/* Get initial index */
	index_step = data[ptr++];
	ptr++;

	/* First byte has only the upper part used as a code */
	get_sample(data[ptr++] >> 4);
	fwrite(&value, sizeof(value), 1, out);

	/* Decode remaining data */
	for (int i = 0; i < 31; i++)
	{
		get_sample(data[ptr] & 0x0F);
		fwrite(&value, sizeof(value), 1, out);
		get_sample(data[ptr++] >> 4);
		fwrite(&value, sizeof(value), 1, out);
	}
}
