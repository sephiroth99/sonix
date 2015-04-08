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

#include <stdio.h>

enum wave_format_tags {
	WAVE_FORMAT_PCM = 0x0001,
};

static const char * container_id_riff  = "RIFF";
static const char * riff_chunk_id_wave = "WAVE";
static const char * wave_chunk_id_fmt  = "fmt ";
static const char * wave_chunk_id_data = "data";

#define WAVHDRSIZ 40

void wav_skip_header(FILE * f)
{
	fseek(f, WAVHDRSIZ, SEEK_CUR);
}

int wav_write_header(FILE * f, unsigned int len, unsigned short chans, unsigned int rate, unsigned short bps)
{
	fseek(f, 0, SEEK_SET);

	/* Write fourcc */
	fwrite(container_id_riff, 1, 4, f);

	/* Write length */
	unsigned int filelen = len + WAVHDRSIZ - 8;
	fwrite(&filelen, sizeof(filelen), 1, f);

	/* Write riff chunk id */
	fwrite(riff_chunk_id_wave, 1, 4, f);

	/* Start of wave format chunk "fmt " */
	fwrite(wave_chunk_id_fmt, 1, 4, f);

	/* Length of format chunk */
	unsigned int fmtlen = 16;
	fwrite(&fmtlen, sizeof(fmtlen), 1, f);

	/* Format tag */
	unsigned short wavefmt = WAVE_FORMAT_PCM;
	fwrite(&wavefmt, sizeof(wavefmt), 1, f);

	/* nb channels */
	fwrite(&chans, sizeof(chans), 1, f);

	/* sampling rate */
	fwrite(&rate, sizeof(rate), 1, f);

	/* average bytes per sec */
	unsigned int avgbps = rate * chans * bps / 8;
	fwrite(&avgbps, sizeof(avgbps), 1, f);

	/* block size of data in bytes */
	unsigned short blksize = chans * bps / 8;
	fwrite(&blksize, sizeof(blksize), 1, f);

	/* Number of bits per sample of mono data */
	fwrite(&bps, sizeof(bps), 1, f);

	/* Start of wave data chunk */
	fwrite(wave_chunk_id_data, 1, 4, f);

	return 0;
}
