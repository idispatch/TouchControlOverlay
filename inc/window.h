/*
 * window.h
 *
 *  Created on: Aug 5, 2011
 *      Author: jnicholl
 */

#ifndef WINDOW_H_
#define WINDOW_H_

#include <screen/screen.h>

class EmulationWindow
{
public:
	virtual ~EmulationWindow();

protected:
	EmulationWindow(screen_context_t screenContext, screen_window_t parent=0);
	EmulationWindow(screen_context_t screenContext, int width, int height, screen_window_t parent=0);

	bool setZOrder(int zOrder) const;
	bool setTouchSensitivity(bool isSensitive) const;
	bool getPixels(screen_buffer_t *buffer, unsigned char **pixels, int *stride) const;
	void post(screen_buffer_t buffer) const;

	bool setParent(screen_window_t parent);

	bool m_valid;
	screen_context_t m_context;
	screen_window_t m_window;
	screen_window_t m_parent;
	int m_size[2];

private:
	void init(screen_window_t parent);
};

#endif /* WINDOW_H_ */