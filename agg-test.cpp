/*
 * agg-test.cpp
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

#include <iostream>
#include <vector>
#include <memory>

#include <agg_rendering_buffer.h>
#include <agg_pixfmt_rgb.h>
#include <agg_color_rgba.h>
#include <agg_renderer_base.h>
#include <agg_rasterizer_scanline_aa.h>
#include <agg_renderer_scanline.h>
#include <agg_path_storage.h>
#include <agg_scanline_u.h>
#include <agg_conv_curve.h>
#include <agg_conv_stroke.h>
#include <agg_trans_affine.h>
#include <agg_conv_transform.h>
#include <agg_image_accessors.h>
#include <agg_span_allocator.h>
#include <agg_image_filters.h>
#include <agg_span_interpolator_linear.h>
#include <agg_span_image_filter_rgb.h>

#include "pnm-img.h"


#define TRUE        1
#define FALSE       0

using int8u = agg::int8u;


enum
{
   eFrame_CX = 640,
   eFrame_CY = 480,
};

const agg::rgba8 clr_black = agg::rgba8(  0,  0,  0);
const agg::rgba8 clr_white = agg::rgba8(255,255,255);

const agg::rgba8 clr_red   = agg::rgba8(255,192,128);
const agg::rgba8 clr_green = agg::rgba8(128,255,192);
const agg::rgba8 clr_blue  = agg::rgba8(128,192,255);


inline bool write_image(const agg::rendering_buffer & rbuf, const char * filename)
{
	if (!write_ppm(rbuf.buf(), rbuf.width(), rbuf.height(), filename))
	{
		std::cerr << "unable to write image: '" << filename << "'\n";
		return false;
	}
	return true;
}


std::unique_ptr<int8u[]> read_image(agg::rendering_buffer & rbuf, const char * filename)
{
	int img_cx = 0;
	int img_cy = 0;

	auto buffer = read_ppm(filename, img_cx, img_cy);
	if (buffer == nullptr)
	{
		std::cerr << "unable to read image: '" << filename << "'\n";
		return nullptr;
	}

	rbuf.attach(buffer, img_cx, img_cy, img_cx * 3);

	return std::unique_ptr<int8u[]>(buffer);
}


std::unique_ptr<int8u[]> create_rendering_buffer(agg::rendering_buffer & rbuf, int cx, int cy)
{
	int stride = cx * 3;
	int size = cy * stride;

	auto buffer = std::make_unique<int8u[]>(size);
	if (buffer)
		rbuf.attach(buffer.get(), cx, cy, stride);

	return buffer;
}


void test_A()
{
	agg::rendering_buffer rbuf;
	auto buffer = create_rendering_buffer(rbuf, eFrame_CX, eFrame_CY);
	rbuf.clear(255);

	agg::rendering_buffer rbuf2(rbuf.row_ptr(rbuf.height() / 2) + (rbuf.stride_abs() / 2),
	                            rbuf.width() / 4,
	                            rbuf.height() / 4,
	                           -rbuf.stride());

	for (unsigned i = 0; i < rbuf2.height(); i++)
		memset(rbuf2.row_ptr(i), 127, (1 + i) * 3);

	memset(rbuf.row_ptr(0), 0, rbuf.stride_abs());
	memset(rbuf.row_ptr(1), 0, rbuf.stride_abs());
	memset(rbuf.row_ptr(rbuf.height() - 1), 0, rbuf.stride_abs());
	memset(rbuf.row_ptr(rbuf.height() - 2), 0, rbuf.stride_abs());

	write_image(rbuf, "test_A.ppm");
}


void test_B()
{
	agg::rendering_buffer rbuf;
	auto buffer = create_rendering_buffer(rbuf, eFrame_CX, eFrame_CY);
	agg::pixfmt_rgb24 pixf(rbuf);
	rbuf.clear(255);

	std::vector<agg::rgba8> span;
	std::vector<int8u> covers;

	int dx = pixf.width();

	for (unsigned i = 0; i < rbuf.width(); i++)
	{
		agg::rgba c(380 + 400.0 * i / dx, 0.8);
		span.emplace_back(c);

		covers.emplace_back((int8u)(128.0 + 126.0 * sin(8.0 * agg::pi * i / rbuf.width())));
	}

	for (unsigned i = 0; i < rbuf.height(); i++)
#if TRUE
		pixf.blend_color_hspan(0, i, rbuf.width(), span.data(), nullptr, i * 255 / rbuf.height());
#else
		pixf.blend_color_hspan(0, i, rbuf.width(), span.data(), covers.data(), 0);
#endif

	write_image(rbuf, "test_B.ppm");
}


void test_C()
{
	agg::rendering_buffer rbuf;
	auto buffer = create_rendering_buffer(rbuf, eFrame_CX, eFrame_CY);
	agg::pixfmt_rgb24 pixf(rbuf);

	using renbase_type = agg::renderer_base<agg::pixfmt_rgb24>;
	renbase_type rbase(pixf);

	rbase.clear(clr_blue);

	int dx = pixf.width();
	int dy = pixf.height();

	rbase.clip_box(dx * 1/8, dy * 1/8,
	               dx * 5/8, dy * 5/8);

	rbase.copy_bar(0, 0, dx / 2, dy / 2, clr_green);
	rbase.blend_bar(dx * 1/4, dy * 1/4,
	                dx * 3/4, dy * 3/4,
	                agg::rgba8(clr_red, 160), 160);

	write_image(rbuf, "test_C.ppm");
}


void test_D()
{
	agg::rendering_buffer rbuf;
	auto buffer = create_rendering_buffer(rbuf, eFrame_CX, eFrame_CY);
	agg::pixfmt_rgb24 pixf(rbuf);

	using renbase_type = agg::renderer_base<agg::pixfmt_rgb24>;
	renbase_type rbase(pixf);

	rbase.clear(clr_white);

	agg::path_storage path;

	path.start_new_path();

	double _CX = (double)pixf.width();
	double _CY = (double)pixf.height();

	path.move_to(_CX * 1/8, _CY * 1/8);
	path.curve3( _CX * 1/2, _CY * 7/8,
	             _CX * 7/8, _CY * 5/8);

	using renderer_type = agg::renderer_scanline_aa_solid<renbase_type>;

	renderer_type renderer(rbase);
	renderer.color(clr_blue);

	agg::rasterizer_scanline_aa<> rasterizer;
	agg::scanline_u8 scanline;

	using curve_type  = agg::conv_curve<agg::path_storage>;
	using stroke_type = agg::conv_stroke<curve_type>;

	curve_type  curve(path);
	stroke_type stroke(curve);

	stroke.line_cap(agg::round_cap);        // butt_cap, square_cap, round_cap
	stroke.line_join(agg::round_join);      // miter_join, miter_join_revert,
	                                        // round_join, bevel_join,
	                                        // miter_join_round
	stroke.miter_limit(_CY / 8);
	stroke.width(_CY / 8);

	rasterizer.add_path(stroke);

	agg::render_scanlines(rasterizer, scanline, renderer);

	write_image(rbuf, "test_D.ppm");
}


void test_E()
{
	agg::rendering_buffer rbuf;
	auto buffer = create_rendering_buffer(rbuf, eFrame_CX, eFrame_CY);
	agg::pixfmt_rgb24 pixf(rbuf);

	using renbase_type = agg::renderer_base<agg::pixfmt_rgb24>;
	renbase_type rbase(pixf);

	rbase.clear(clr_white);

	agg::path_storage path;

	path.start_new_path();

	double _CX = (double)pixf.width();
	double _CY = (double)pixf.height();

	path.move_to(_CX * 1/8, _CY * 1/8);
	path.curve3( _CX * 1/2, _CY * 7/8,
	             _CX * 7/8, _CY * 5/8);

	using renderer_type = agg::renderer_scanline_aa_solid<renbase_type>;

	renderer_type renderer(rbase);
	renderer.color(clr_green);

	agg::rasterizer_scanline_aa<> rasterizer;
	agg::scanline_u8 scanline;

	using curve_type  = agg::conv_curve<agg::path_storage>;
	using stroke_type = agg::conv_stroke<curve_type>;
	using trans_type  = agg::conv_transform<stroke_type>;

	agg::trans_affine matrix(0.8, 0, 0, -0.8, 0, _CY);

	curve_type  curve(path);
	stroke_type stroke(curve);
	trans_type  trans_path(stroke, matrix);

	stroke.line_cap(agg::round_cap);        // butt_cap, square_cap, round_cap
	stroke.line_join(agg::round_join);      // miter_join, miter_join_revert,
	                                        // round_join, bevel_join,
	                                        // miter_join_round
	stroke.miter_limit(_CY * 1/8);
	stroke.width(_CY * 1/8);

	rasterizer.add_path(trans_path);

	agg::render_scanlines(rasterizer, scanline, renderer);

	write_image(rbuf, "test_E.ppm");
}


void test_F()
{
	// --- Image --------------------------------------------------------
	agg::rendering_buffer img_rbuf;
	auto img_buffer = read_image(img_rbuf, "picture.ppm");
	if (!img_buffer)
		return;
	agg::pixfmt_rgb24 img_pixf(img_rbuf);

	using img_source_type = agg::image_accessor_clone<agg::pixfmt_rgb24>;
	img_source_type img_src(img_pixf);
	//-------------------------------------------------------------------

	agg::rendering_buffer rbuf;
	auto buffer = create_rendering_buffer(rbuf, eFrame_CX, eFrame_CY);
	agg::pixfmt_rgb24 pixf(rbuf);

	using renbase_type = agg::renderer_base<agg::pixfmt_rgb24>;
	renbase_type rbase(pixf);
	rbase.clear(clr_white);

	int sx = img_pixf.width();
	int sy = img_pixf.height();
	int dx = pixf.width();
	int dy = pixf.height();

	rbase.copy_bar(dx * 1/5, dy * 1/5,
	               dx * 4/5, dy * 4/5,
	               clr_red);

	double fx = (double)sx / dx;
	double fy = (double)sy / dy;
	double scale = __max(fx, fy);

	agg::trans_affine matrix = agg::trans_affine_translation(-dx / 2.0, -dy / 2.0)
	                         * agg::trans_affine_rotation(-30.0 * agg::pi / 180.0)
	                         * agg::trans_affine_scaling(scale * 11/7)
	                         * agg::trans_affine_translation(sx / 2.0, sy / 2.0);

	using interpolator_type = agg::span_interpolator_linear<>;
	interpolator_type interpolator(matrix);

#if TRUE
	using span_gen_type = agg::span_image_filter_rgb_bilinear<img_source_type, interpolator_type>;
	span_gen_type sg(img_src, interpolator);
#else
	using span_gen_type = agg::span_image_filter_rgb<img_source_type, interpolator_type>;
	#if TRUE
		agg::image_filter<agg::image_filter_spline36> filter;
	#else
		agg::image_filter<agg::image_filter_bicubic> filter;
	#endif
	span_gen_type sg(img_src, interpolator, filter);
#endif

	agg::rasterizer_scanline_aa<> rasterizer;
	agg::scanline_u8 scanline;
	agg::span_allocator<agg::rgba8> sa;

	rasterizer.clip_box(0, 0, dx, dy);

	agg::trans_affine inv_mat = ~matrix;
	double x, y;

	x = 0;  y = 0;  inv_mat.transform(&x, &y); rasterizer.move_to_d(x, y);
	x = sx; y = 0;  inv_mat.transform(&x, &y); rasterizer.line_to_d(x, y);
	x = sx; y = sy; inv_mat.transform(&x, &y); rasterizer.line_to_d(x, y);
	x = 0;  y = sy; inv_mat.transform(&x, &y); rasterizer.line_to_d(x, y);

	agg::render_scanlines_aa(rasterizer, scanline, rbase, sa, sg);

	write_image(rbuf, "test_F.ppm");
}


int main(int argc, char * argv[])
{
	test_A();
	test_B();
	test_C();
	test_D();
	test_E();
	test_F();

	return 0;
}
