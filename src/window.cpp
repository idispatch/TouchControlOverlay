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

#include "window.h"
#include <errno.h>
#include <string.h>
#include <screen/screen.h>

EmulationWindow::EmulationWindow(screen_context_t screenContext, screen_window_t parent)
	: m_context(screenContext)
	, m_parent(0)
{
	m_valid = false;
	int size[2] = {0,0};
	int rc = screen_get_window_property_iv(parent, SCREEN_PROPERTY_BUFFER_SIZE, size);
	if (rc) {
#ifdef _DEBUG
		perror("screen_get_window_property_iv(size)");
#endif
		return;
	}
	m_size[0] = size[0];
	m_size[1] = size[1];
	m_alpha = 0xFF;
	init(parent);
}

EmulationWindow::EmulationWindow(screen_context_t screenContext, int width, int height, int alpha, screen_window_t parent)
	: m_context(screenContext)
	, m_parent(0)
{
	m_valid = false;
	m_size[0] = width;
	m_size[1] = height;
	m_alpha = alpha;
	init(parent);
}

void EmulationWindow::init(screen_window_t parent)
{
	int rc;
	int format = SCREEN_FORMAT_RGBA8888;
	int usage = SCREEN_USAGE_NATIVE | SCREEN_USAGE_READ | SCREEN_USAGE_WRITE;

	rc = screen_create_window_type(&m_window, m_context, SCREEN_CHILD_WINDOW);
	if (rc) {
#ifdef _DEBUG
		perror("screen_create_window");
#endif
		return;
	}

	rc = screen_set_window_property_iv(m_window, SCREEN_PROPERTY_FORMAT, &format);
	if (rc) {
#ifdef _DEBUG
		perror("screen_set_window_property_iv(SCREEN_PROPERTY_FORMAT)");
#endif
		screen_destroy_window(m_window);
		m_window = 0;
		return;
	}

	rc = screen_set_window_property_iv(m_window, SCREEN_PROPERTY_USAGE, &usage);
	if (rc) {
#ifdef _DEBUG
		perror("screen_set_window_property_iv(SCREEN_PROPERTY_USAGE)");
#endif
		screen_destroy_window(m_window);
		m_window = 0;
		return;
	}

	rc = screen_set_window_property_iv(m_window, SCREEN_PROPERTY_SIZE, m_size);
	if (rc) {
#ifdef _DEBUG
		perror("screen_set_window_property_iv(SCREEN_PROPERTY_SIZE)");
#endif
		screen_destroy_window(m_window);
		m_window = 0;
		return;
	}

	rc = screen_create_window_buffers(m_window, 1); /* Do we need 2 buffers?*/
	if (rc) {
#ifdef _DEBUG
		perror("screen_create_window_buffers");
#endif
		screen_destroy_window(m_window);
		m_window = 0;
		return;
	}

	if (!setParent(parent))
	{
		screen_destroy_window(m_window);
		m_window = 0;
		return;
	}
	m_valid = true;
}

bool EmulationWindow::setParent(screen_window_t parent)
{
	int rc;
	if (parent == m_parent)
		return true;

	if (parent != 0) {
		char buffer[256] = {0};

		rc = screen_get_window_property_cv(parent, SCREEN_PROPERTY_GROUP, 256, buffer);
		if (rc) {
#ifdef _DEBUG
			perror("screen_get_window_property_cv(SCREEN_PROPERTY_GROUP)");
#endif
			return false;
		}

		rc = screen_join_window_group(m_window, buffer);
		if (rc) {
#ifdef _DEBUG
			perror("screen_join_window_group");
#endif
			return false;
		}
		m_parent = parent;
	} else if (m_parent) {
		rc = screen_leave_window_group(m_window);
		if (rc) {
#ifdef _DEBUG
			perror("screen_leave_window_group");
#endif
			return false;
		}
		m_parent = 0;
	}
	return true;
}

bool EmulationWindow::getPixels(screen_buffer_t *buffer, unsigned char **pixels, int *stride) const
{
	screen_buffer_t buffers[2];
	int rc = screen_get_window_property_pv(m_window,
			SCREEN_PROPERTY_RENDER_BUFFERS, (void**)buffers);
	if (rc) {
#ifdef _DEBUG
		fprintf(stderr, "Cannot get window render buffers: %s", strerror(errno));
#endif
		return false;
	}
	*buffer = buffers[0];

	rc = screen_get_buffer_property_pv(*buffer, SCREEN_PROPERTY_POINTER, (void **)pixels);
	if (rc) {
#ifdef _DEBUG
		fprintf(stderr, "Cannot get buffer pointer: %s", strerror(errno));
#endif
		return false;
	}

	rc = screen_get_buffer_property_iv(*buffer, SCREEN_PROPERTY_STRIDE, stride);
	if (rc) {
#ifdef _DEBUG
		fprintf(stderr, "Cannot get stride: %s", strerror(errno));
#endif
		return false;
	}
	return true;
}

bool EmulationWindow::setZOrder(int zOrder) const
{
	int rc = screen_set_window_property_iv(m_window,
	                                       SCREEN_PROPERTY_ZORDER,
	                                       &zOrder);
	if (rc) {
#ifdef _DEBUG
		fprintf(stderr, "Cannot set z-order: %s", strerror(errno));
#endif
		return false;
	}
	return true;
}

bool EmulationWindow::setTouchSensitivity(bool isSensitive) const
{
	int sensitivity = (isSensitive)?SCREEN_SENSITIVITY_ALWAYS:SCREEN_SENSITIVITY_NEVER;
	int rc = screen_set_window_property_iv(m_window,
	                                       SCREEN_PROPERTY_SENSITIVITY,
	                                       &sensitivity);
	if (rc) {
#ifdef _DEBUG
		fprintf(stderr, "Cannot set screen sensitivity: %s", strerror(errno));
#endif
		return false;
	}
	return true;
}

void EmulationWindow::post(screen_buffer_t buffer) const
{
	int dirtyRects[4] = {0, 0, m_size[0], m_size[1]};
	int rc = screen_post_window(m_window, buffer, 1, dirtyRects, 0);
    if (rc) {
#ifdef _DEBUG
        fprintf(stderr, "screen_post_window: %s", strerror(errno));
#endif
    }
}

EmulationWindow::~EmulationWindow()
{
	if (m_window) {
		screen_destroy_window(m_window);
		m_window = 0;
	}
}
