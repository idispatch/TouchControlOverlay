/*
 * Copyright (c) 2011 Research In Motion Limited.
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

#include "labelwindow.h"
#include "pngreader.h"

LabelWindow *LabelWindow::create(screen_context_t context, int width, int height, int alpha)
{
	const int zOrder = 6;
	LabelWindow *window = new LabelWindow(context, width, height, alpha);
	if (!window->m_valid) {
		delete window;
		return 0;
	}

	if (!window->setZOrder(zOrder) ||
		!window->setTouchSensitivity(false)) {
		delete window;
		return 0;
	}

	return window;
}

void LabelWindow::draw(PNGReader &reader)
{
	screen_buffer_t buffer;
	unsigned char *pixels;
	int stride;
	if (!getPixels(&buffer, &pixels, &stride)) {
#ifdef _DEBUG
		fprintf(stderr, "Unable to get label window buffer\n");
#endif
		return;
	}
	screen_buffer_t pixmapBuffer;
	screen_get_pixmap_property_pv(reader.m_pixmap, SCREEN_PROPERTY_RENDER_BUFFERS, (void**)&pixmapBuffer);

	int attribs[] = {
			SCREEN_BLIT_SOURCE_X, 0,
			SCREEN_BLIT_SOURCE_Y, 0,
			SCREEN_BLIT_SOURCE_WIDTH, reader.m_width,
			SCREEN_BLIT_SOURCE_HEIGHT, reader.m_height,
			SCREEN_BLIT_DESTINATION_X, 0,
			SCREEN_BLIT_DESTINATION_Y, 0,
			SCREEN_BLIT_DESTINATION_WIDTH, m_size[0],
			SCREEN_BLIT_DESTINATION_HEIGHT, m_size[1],
			SCREEN_BLIT_TRANSPARENCY, SCREEN_TRANSPARENCY_SOURCE_OVER,
			SCREEN_BLIT_GLOBAL_ALPHA, m_alpha,
			SCREEN_BLIT_END
	};
	screen_blit(m_context, buffer, pixmapBuffer, attribs);
	this->post(buffer);
	int visible = 0;
	int rc = screen_set_window_property_iv(m_window, SCREEN_PROPERTY_VISIBLE, &visible);
}

void LabelWindow::showAt(screen_window_t parent, int x, int y)
{
	move(x, y);
	if (!setParent(parent))
		return;

	int visible = 1;
	int rc = screen_set_window_property_iv(m_window, SCREEN_PROPERTY_VISIBLE, &visible);
#ifdef _DEBUG
	if (rc) {
		perror("set label window visible: ");
	}
#endif
}

void LabelWindow::move(int x, int y)
{
	int position[] = {x, y};
	int rc = screen_set_window_property_iv(m_window, SCREEN_PROPERTY_POSITION, position);
#ifdef _DEBUG
	if (rc) {
		perror("LabelWindow set position: ");
		return;
	}
#endif
}
