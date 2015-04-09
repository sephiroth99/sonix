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
#include <stdlib.h>
#include <string.h>
#include <direct.h> /* for _mkdir */
#include <errno.h>

enum section_type {
	audio = 6,
};

#pragma pack(push, 1)
struct ste {
	unsigned int datalen;
	unsigned int type:8;
	unsigned int dummy1:24;
	unsigned int flags:8;
	unsigned int hdrlen:24;
	unsigned int id;
	unsigned int dummy2;
};
#pragma pack(pop)

#define BLOCKSIZ 36
#define VERSION 3

extern void decode_block(unsigned char *, FILE *);
extern void wav_skip_header(FILE *);
extern int wav_write_header(FILE *, unsigned int, unsigned short, unsigned int, unsigned short);

void usage(char * name)
{
	char basename[FILENAME_MAX];
	char ext[FILENAME_MAX];
	_splitpath_s(name, NULL, 0, NULL, 0, basename, FILENAME_MAX, ext, FILENAME_MAX);

	printf("sonix - extract sounds from DRM files\n");
	printf("Version %d\n", VERSION);
	printf("Copyright (c) 2013-2015 sephiroth99\n");
	printf("\n");
	printf("Usage:\n");
	printf("  %s%s FILE [FILE ...]\n", basename, ext);
	printf("  Extract all sounds from each FILE, where FILE is a valid DRM file.\n");
	exit(0);
}

int xcs(FILE * src, FILE * dst, unsigned int len)
{
	unsigned char data[BLOCKSIZ];

	if (len % BLOCKSIZ)
		return -1;

	int blocks = len / BLOCKSIZ;

	for (int i = 0; i < blocks; i++) {
		if (fread(data, BLOCKSIZ, 1, src) != 1)
			return -1;

		decode_block(data, dst);
	}

	return 0;
}

char * create_output_dir(char * name)
{
	const char * append = "_wav";
	int destdir_len = strlen(name) + strlen(append) + 1;

	char * destdir = malloc(destdir_len);
	if (!destdir)
		return NULL;

	strcpy_s(destdir, destdir_len, name);

	int last = strlen(name) - 1;
	if (destdir[last] == '/' || destdir[last] == '\\')
		destdir[last] = '\0';

	strcat_s(destdir, destdir_len, append);

	if (_mkdir(destdir)) {
		if (errno != EEXIST) {
			free(destdir);
			return NULL;
		}
	}

	return destdir;
}

void extract_wavs_from_drm(char * name)
{
	FILE * drmfile;
	FILE * wavfile;
	size_t len;

	unsigned int version;
	unsigned int count;

	char * destdir = create_output_dir(name);
	if (!destdir) {
		fprintf(stderr, "Error creating output directory for file %s.\n", name);
		return;
	}

	if (fopen_s(&drmfile, name, "rb")) {
		fprintf(stderr, "Error opening file %s.\n", name);
		free(destdir);
		return;
	}

	len = fread(&version, sizeof(version), 1, drmfile);
	if (len != 1)
		goto read_err;

	if (version != 14) {
		fprintf(stderr, "DRM version 0x%02x is not supported.\n", version);
		goto bail;
	}

	len = fread(&count, sizeof(count), 1, drmfile);
	if (len != 1)
		goto read_err;

	struct ste * section_table = calloc(count, sizeof(struct ste));
	if (!section_table) {
		fprintf(stderr, "Error allocating section table.\n");
		goto bail;
	}

	len = fread((void *)section_table, sizeof(struct ste), count, drmfile);
	if (len != count)
		goto read_err;

	for (unsigned int i = 0; i < count; i++) {
		if (section_table[i].type != audio) {
			fseek(drmfile, (section_table[i].hdrlen * 8) + section_table[i].datalen, SEEK_CUR);
			continue;
		}
		
		char wavname[1024] = {0};
		if (_snprintf_s(wavname, 1024, _TRUNCATE, "%s/%u.wav", destdir, section_table[i].id) == -1) {
			fprintf(stderr, "Error printing wav file name.\n");
			goto bail;
		}
		if (fopen_s(&wavfile, wavname, "wb")) {
			fprintf(stderr, "Error creating wav file %s.\n", wavname);
			goto bail;
		}

		/* Leave space for the WAV header */
		wav_skip_header(wavfile);

		/* Prepare DRM file */
		fseek(drmfile, (section_table[i].hdrlen * 8), SEEK_CUR);

		unsigned int freq;
		len = fread(&freq, sizeof(freq), 1, drmfile);
		if (len != 1)
			goto read_err;

		/* Skip over loop points */
		fseek(drmfile, 8, SEEK_CUR);

		if (xcs(drmfile, wavfile, section_table[i].datalen - 12) < 0)
			fprintf(stderr, "Error converting audio section %d.\n", i);

		/* Write WAV header */
		unsigned int extract_size = ((section_table[i].datalen - 12) / 36) * 128;
		wav_write_header(wavfile, extract_size, 1, freq, 16);

		fclose(wavfile);
	}

bail:
	free(destdir);
	fclose(drmfile);
	return;

read_err:
	fprintf(stderr, "Read error in file %s.\n", name);
	free(destdir);
	fclose(drmfile);
	return;
}

int main(int argc, char * argv[])
{
	if (argc <= 1)
		usage(argv[0]);

	argc--;
	argv++;

	for (int i = 0; i < argc; i++)
		extract_wavs_from_drm(argv[i]);

	return 0;
}
