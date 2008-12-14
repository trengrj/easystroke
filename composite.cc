/*
 * Copyright (c) 2008, Thomas Jaeger <ThJaeger@gmail.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#include "prefdb.h"
#include "composite.h"
#include <gdkmm.h>

Composite::Composite() : Gtk::Window(Gtk::WINDOW_POPUP) {
	if (!is_composited())
		throw std::runtime_error("composite not available");
	Glib::RefPtr<Gdk::Screen> screen = Gdk::Display::get_default()->get_default_screen();

	set_position(Gtk::WIN_POS_CENTER_ALWAYS);
	set_default_size(screen->get_width(), screen->get_height());
	set_decorated(false);
	set_app_paintable(true);

	Glib::RefPtr<Gdk::Colormap> colormap = get_screen()->get_rgba_colormap();
	if (colormap)
		set_colormap(colormap);
	signal_expose_event().connect(sigc::mem_fun(*this, &Composite::on_expose));
	realize();
	get_window()->input_shape_combine_region(Gdk::Region(), 0, 0);
#if BRAVE
	show();
#endif
}

void Composite::draw(Point p, Point q) {
	if (!points.size()) {
		points.push_back(p);
#if BRAVE
		minx = maxx = p.x;
		miny = maxy = p.y;
#endif
	}
	points.push_back(q);
#if BRAVE
	if (q.x < minx)
		minx = q.x;
	if (q.x > maxx)
		maxx = q.x;
	if (q.y < miny)
		miny = q.y;
	if (q.y > maxy)
		maxy = q.y;
#endif
	int x = p.x < q.x ? p.x : q.x;
	int w = abs(q.x - p.x);
	int y = p.y < q.y ? p.y : q.y;
	int h = abs(q.y - p.y);
	get_window()->invalidate_rect(Gdk::Rectangle(x-10, y-10, w+20, h+20), false);
}

void Composite::start_() {
	RGBA rgba = prefs.color.get();
	red = rgba.color.get_red_p();
	green = rgba.color.get_green_p();
	blue = rgba.color.get_blue_p();
	alpha = ((double)rgba.alpha)/65535.0;
	show();
}

void Composite::draw_line(Cairo::RefPtr<Cairo::Context> ctx) {
	if (!points.size())
		return;
	std::list<Point>::iterator i = points.begin();
	ctx->move_to (i->x, i->y);
	for (; i != points.end(); i++)
		ctx->line_to (i->x, i->y);
	ctx->set_source_rgba((red+0.5)/2.0, (green+0.5)/2.0, (blue+0.5)/2.0, alpha/2.0);
	ctx->set_line_width(prefs.trace_width.get());
	ctx->set_line_cap(Cairo::LINE_CAP_ROUND);
	ctx->stroke_preserve();

	ctx->set_source_rgba(red, green, blue, alpha);
	ctx->set_line_width(((double)prefs.trace_width.get())*0.67);
	ctx->set_line_cap(Cairo::LINE_CAP_ROUND);
	ctx->stroke();

}

bool Composite::on_expose(GdkEventExpose* event) {
	Cairo::RefPtr<Cairo::Context> ctx = get_window()->create_cairo_context();
	ctx->set_operator(Cairo::OPERATOR_SOURCE);

	Gdk::Region region(event->region, true);
	Gdk::Cairo::add_region_to_path(ctx, region);
	ctx->clip();

	Gdk::Cairo::add_region_to_path(ctx, region);
	ctx->set_source_rgba(0.0, 0.0, 0.0, 0.0);
	ctx->fill();

	draw_line(ctx);

	return false;
}

void Composite::end_() {
	points.clear();
#if BRAVE
	get_window()->invalidate_rect(Gdk::Rectangle(minx-10, miny-10, maxx-minx+20, maxy-miny+20), false);
#else
	hide();
#endif
}
