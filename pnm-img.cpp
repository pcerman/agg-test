/*
* pnm-img.cpp
*
* Copyright (c) 2016 Peter Cerman (https://github.com/pcerman)
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
* MA 02110-1301, USA.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


using uchar = unsigned char;


#pragma warning(disable : 4996)


bool write_ppm(const uchar * buf, int width, int height, const char * filename)
{
	if (width <= 0 || height <= 0)
		return false;

	FILE * fd = fopen(filename, "wb");
	if (fd == nullptr)
		return false;

	fprintf(fd, "P6\n# NitroPDF 11.1\n%d %d\n255\n", width, height);
	fwrite(buf, 1, width * height * 3, fd);
	fclose(fd);

	return true;
}


inline bool is_whitespace(char ch)
{
	switch (ch)
	{
	case '\t':
	case '\n':
	case '\r':
	case ' ':
		return true;
	}

	return false;
}


inline const char * skip_to_eol(const char * ptr)
{
	while (*ptr && *ptr != '\r' && *ptr != '\n')
		ptr++;

	if (*ptr == '\r')
		ptr++;
	if (*ptr == '\n')
		ptr++;

	return ptr;
}


const char * skip_whitespace(const char * ptr)
{
	for (;;)
	{
		while (is_whitespace(*ptr))
			ptr++;

		if (*ptr == '#')
			ptr = skip_to_eol(ptr);
		else
			break;
	}

	return ptr;
}


inline const char * get_number(const char * ptr, int & num)
{
	ptr = skip_whitespace(ptr);
	if (!isdigit(*ptr))
		return nullptr;

	num = atoi(ptr);
	while (isdigit(*ptr))
		ptr++;

	return ptr;
}


uchar * read_ppm(const char * filename, int & width, int & height)
{
	char buf[1024];
	int len;

	FILE * fd = fopen(filename, "rb");
	if (fd == nullptr)
		return nullptr;

	if ((len = (int)fread(buf, 1, sizeof(buf) - 2, fd)) < 14)
	{
		fclose(fd);
		return nullptr;
	}

	buf[len] = 0;

	if (buf[0] != 'P' || buf[1] != '6' || !(is_whitespace(buf[2]) || buf[2] == '#'))
	{
		fclose(fd);
		return nullptr;
	}

	int _width = 0;
	int _height = 0;
	int _maxval = 0;

	const char * ptr = get_number(buf + 2, _width);
	if (ptr == nullptr || _width < 1 || _width > 8192)
	{
		fclose(fd);
		return nullptr;
	}

	ptr = get_number(ptr, _height);
	if (ptr == nullptr || _height < 1 || _height > 8192)
	{
		fclose(fd);
		return nullptr;
	}

	ptr = get_number(ptr, _maxval);
	if (ptr == nullptr || _maxval != 255)
	{
		fclose(fd);
		return nullptr;
	}

	if (*ptr == '#')
		ptr = skip_to_eol(ptr);

	ptr++;
	fseek(fd, long(ptr - buf), SEEK_SET);

	int stride = _width * 3;
	int size = stride * _height;

	uchar * buf_img = new uchar[size];

	len = fread(buf_img, 1, size, fd);
	fclose(fd);

	if (len != size)
		return nullptr;

	width = _width;
	height = _height;

	return buf_img;
}
