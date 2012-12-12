/*
 * Copyright (c) 2011-2012 Research In Motion Limited.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "pngreader.h"
#include <errno.h>

PNGReader::PNGReader(FILE *file, const char * file_name, screen_context_t context)
	: m_context(context)
	, m_read(0)
	, m_info(0)
	, m_data(0)
	, m_rows(0)
	, m_width(0)
	, m_height(0)
	, m_file(file)
	, m_pixmap(0)
	, m_buffer(0)
{
	m_file_name = new char[strlen(file_name) + 1];
	strcpy(m_file_name, file_name);
}

PNGReader::~PNGReader()
{
	delete [] m_file_name;
	delete [] m_rows;
	delete [] m_data;

	if (m_read) {
		png_destroy_read_struct(&m_read, m_info ? &m_info : (png_infopp) 0, (png_infopp) 0);
	}
	m_read = 0;

	if (m_file)
		fclose(m_file);
	m_file = 0;

	if (m_pixmap)
		screen_destroy_pixmap(m_pixmap);
	else if (m_buffer)
		screen_destroy_buffer(m_buffer);

	m_pixmap = 0;
	m_buffer = 0;
}

bool PNGReader::doRead()
{
	png_byte header[8];
	if(fread(header, 1, sizeof(header), m_file) < sizeof(header)) {
#ifdef _DEBUG
		fprintf(stderr, "Could not read PNG header in file %s\n", m_file_name);
#endif
		return false;
	}

	if (png_sig_cmp(header, 0, sizeof(header))) {
#ifdef _DEBUG
		fprintf(stderr, "Invalid PNG header in file %s\n", m_file_name);
#endif
		return false;
	}

	m_read = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!m_read) {
#ifdef _DEBUG
		fprintf(stderr, "Failed to create PNG read struct\n");
#endif
		return false;
	}

	m_info = png_create_info_struct(m_read);
	if (!m_info) {
#ifdef _DEBUG
		fprintf(stderr, "Failed to create PNG info struct\n");
#endif
		return false;
	}

	if (setjmp(png_jmpbuf(m_read))) {
#ifdef _DEBUG
		fprintf(stderr, "PNG jumped to failure\n");
#endif
		return false;
	}

	png_init_io(m_read, m_file);
	png_set_sig_bytes(m_read, sizeof(header));
	png_read_info(m_read, m_info);

	m_width = png_get_image_width(m_read, m_info);
	if (m_width <= 0 || m_width > 1024) {
#ifdef _DEBUG
		fprintf(stderr, "Invalid PNG width: %d (in file %s)\n", m_width, m_file_name);
#endif
		return false;
	}

	m_height = png_get_image_height(m_read, m_info);
	if (m_height <= 0 || m_height > 600) {
#ifdef _DEBUG
		fprintf(stderr, "Invalid PNG height: %d (in file %s)\n", m_height, m_file_name);
#endif
		return false;
	}

	png_byte color_type = png_get_color_type(m_read, m_info);
	if(PNG_COLOR_TYPE_RGBA != color_type) {
#ifdef _DEBUG
		fprintf(stderr,
				"Invalid PNG color type %d, must be %d (in file %s)\n",
				color_type, PNG_COLOR_TYPE_RGBA, m_file_name);
#endif
		return false;
	}
	png_byte bit_depth = png_get_bit_depth(m_read, m_info);
	if(8 != bit_depth) {
#ifdef _DEBUG
		fprintf(stderr,
				"Invalid PNG bit depth %d, must be %d (in file %s)\n",
				bit_depth, 8, m_file_name);
#endif
		return false;
	}

	png_read_update_info(m_read, m_info);

	const int channels = 4;
	png_set_palette_to_rgb(m_read);
	png_set_tRNS_to_alpha(m_read);
	png_set_bgr(m_read);
	png_set_expand(m_read);
	png_set_strip_16(m_read);
	png_set_gray_to_rgb(m_read);

	if (png_get_channels(m_read, m_info) < channels)
		png_set_filler(m_read, 0xff, PNG_FILLER_AFTER);

	m_data = new unsigned char[m_width * m_height * channels];
	memset(m_data, 0, m_width * m_height * channels * sizeof(unsigned char));
	int png_stride = m_width * channels;
	m_rows = new png_bytep[m_height];
	memset(m_rows, 0, m_height * sizeof(png_bytep));

	for (int i = m_height - 1; i >= 0; --i) {
		m_rows[i] = (png_bytep)(m_data + i * png_stride);
	}
	png_read_image(m_read, m_rows);

	int rc;
	int format = SCREEN_FORMAT_RGBA8888;
	int size[2] = {m_width, m_height};

	rc = screen_create_pixmap(&m_pixmap, m_context);
	if(rc != 0) {
#ifdef _DEBUG
		fprintf(stderr, "Failed to create pixmap - %s (for file %s)\n",
				strerror(errno), m_file_name);
#endif
		return false;
	}
	rc = screen_set_pixmap_property_iv(m_pixmap, SCREEN_PROPERTY_FORMAT, &format);
	if(rc != 0) {
#ifdef _DEBUG
		fprintf(stderr, "Failed to set pixmap property %d - %s (for file %s)\n",
				SCREEN_PROPERTY_FORMAT, strerror(errno), m_file_name);
#endif
		return false;
	}
	rc = screen_set_pixmap_property_iv(m_pixmap, SCREEN_PROPERTY_BUFFER_SIZE, size);
	if(rc != 0) {
#ifdef _DEBUG
		fprintf(stderr, "Failed to set pixmap property %d - %s (for file %s)\n",
				SCREEN_PROPERTY_BUFFER_SIZE, strerror(errno), m_file_name);
#endif
		return false;
	}
	rc = screen_create_pixmap_buffer(m_pixmap);
	if(rc != 0) {
#ifdef _DEBUG
		fprintf(stderr, "Failed to create pixmap buffer - %s (for file %s)\n",
				strerror(errno), m_file_name);
#endif
		return false;
	}

	unsigned char *realPixels;
	int realStride;
	rc = screen_get_pixmap_property_pv(m_pixmap, SCREEN_PROPERTY_RENDER_BUFFERS, (void**)&m_buffer);
	if(rc != 0) {
#ifdef _DEBUG
		fprintf(stderr, "Failed to get pixmap property %d - %s (for file %s)\n",
				SCREEN_PROPERTY_RENDER_BUFFERS, strerror(errno), m_file_name);
#endif
		return false;
	}
	rc = screen_get_buffer_property_pv(m_buffer, SCREEN_PROPERTY_POINTER, (void **)&realPixels);
	if(rc != 0) {
#ifdef _DEBUG
		fprintf(stderr, "Failed to get buffer property %d - %s (for file %s)\n",
				SCREEN_PROPERTY_POINTER, strerror(errno), m_file_name);
#endif
		return false;
	}
	rc = screen_get_buffer_property_iv(m_buffer, SCREEN_PROPERTY_STRIDE, &realStride);
	if(rc != 0) {
#ifdef _DEBUG
		fprintf(stderr, "Failed to get buffer property %d - %s (for file %s)\n",
				SCREEN_PROPERTY_STRIDE, strerror(errno), m_file_name);
#endif
		return false;
	}

	memset(realPixels, 0, realStride * m_height * sizeof(unsigned char));

	unsigned char * buffer_pixel_row = realPixels;
	unsigned char * png_pixel_row = m_data;
	for(int i = 0; i < m_height; ++i) {
		memcpy(buffer_pixel_row, png_pixel_row, realStride);
		png_pixel_row += png_stride;
		buffer_pixel_row += realStride;
	}

	delete [] m_rows;
    m_rows = 0;
	delete [] m_data; /* we copied the data - no need to keep the copy in memory */
	m_data = 0;

	return true;
}
