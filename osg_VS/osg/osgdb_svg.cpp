﻿/* -*-c++-*- Copyright (C) 2008 Miguel Escriva Gregori
 *
 * This library is open source and may be redistributed and/or modified under
 * the terms of the OpenSceneGraph Public License (OSGPL) version 0.0 or
 * (at your option) any later version.  The full license is in LICENSE file
 * included with this distribution, and on the openscenegraph.org website.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * OpenSceneGraph Public License for more details.
*/

extern "C" {
#include <librsvg/rsvg.h>

#ifndef LIBRSVG_CHECK_VERSION
#include <librsvg/rsvg-cairo.h>
#endif
}

#include <Windows.h>
#ifdef _DEBUG
#pragma comment(lib, "osgd.lib")
#pragma comment(lib, "osgDBd.lib")
#pragma comment(lib, "osgViewerd.lib")
#pragma comment(lib, "OpenThreadsd.lib")
#pragma comment(lib, "osgGAd.lib")
#pragma comment(lib, "osgUtild.lib")
#else
#pragma comment(lib, "osg.lib")
#pragma comment(lib, "osgDB.lib")
#pragma comment(lib, "osgViewer.lib")
#pragma comment(lib, "OpenThreads.lib")
#pragma comment(lib, "osgGA.lib")
#pragma comment(lib, "osgUtil.lib")
#endif

#include <iostream>
#include <sstream>
#include <osg/Image>
#include <osg/Notify>

#include <osgDB/Registry>
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <osgDB/ImageOptions>

class ReaderWriterSVG : public osgDB::ReaderWriter
{
public:

	ReaderWriterSVG()
	{
		supportsExtension("svg", "Scalar Vector Graphics format");
		supportsOption("1024x512", "Set resolution of the image generated by SVG");
		supportsOption("rs=2.0", "Scale the resolution of the image generated by SVG");
	}

	virtual const char* className() const { return "SVG Image Reader"; }

	virtual ReadResult readObject(const std::string& file, const osgDB::ReaderWriter::Options* options) const
	{
		return readImage(file, options);
	}

	virtual ReadResult readImage(const std::string& file, const osgDB::ReaderWriter::Options* options) const
	{
		std::string ext = osgDB::getLowerCaseFileExtension(file);
		if (!acceptsExtension(ext)) return ReadResult::FILE_NOT_HANDLED;

		std::string fileName = osgDB::findDataFile(file, options);
		if (fileName.empty()) return ReadResult::FILE_NOT_FOUND;

		RsvgDimensionData dimensionData;
		RsvgHandle* handle = rsvg_handle_new_from_file(fileName.c_str(), NULL);
		rsvg_handle_get_dimensions(handle, &dimensionData);

		osg::Image *image;
		if (options)
		{
			std::string op = options->getOptionString();

			size_t i = op.find("x");
			if (i != std::string::npos)
			{

				std::stringstream ss1(op.substr(0, i));
				std::stringstream ss2(op.substr(i + 1, op.size()));
				unsigned int w = 0, h = 0;
				ss1 >> w;
				ss2 >> h;
				if (w != 0 && h != 0)
				{
					dimensionData.width = w;
					dimensionData.height = h;
				}
			}

			i = op.find("rs=");
			if (i != std::string::npos)
			{

				std::stringstream ss(op.substr(i + 3, op.size()));
				double scale;
				ss >> scale;
				if (scale != 0.0)
				{
					dimensionData.width = dimensionData.width * scale;
					dimensionData.height = dimensionData.height * scale;
				}
			}
		}

		image = createImage(handle, dimensionData.width, dimensionData.height);

		g_object_unref(handle);
		image->setFileName(file);
		return image;
	}

	osg::Image* createImage(RsvgHandle *handle, unsigned int width, unsigned int height) const
	{
		RsvgDimensionData dimensionData;
		rsvg_handle_get_dimensions(handle, &dimensionData);
		// If image resollution < 128, cairo produces some artifacts.
		// I don't know why, but we check the size...
		if (width < 128) width = 128;
		if (height < 128) height = 128;
		//width = osg::Image::computeNearestPowerOfTwo(width);
		//height = osg::Image::computeNearestPowerOfTwo(height);
		osg::Image *image = new osg::Image();
		image->allocateImage(width, height, 1, GL_RGBA, GL_UNSIGNED_BYTE);
		image->setPixelFormat(GL_BGRA);

		// make sure the image data is cleared before we write to it.
		memset(image->data(), 0, image->getTotalSizeInBytesIncludingMipmaps());

		cairo_surface_t *cairo_surface = cairo_image_surface_create_for_data(image->data(),
			CAIRO_FORMAT_ARGB32, width, height, image->getRowSizeInBytes());
		cairo_t *cr = cairo_create(cairo_surface);
		cairo_scale(cr, ((float)width) / dimensionData.width, ((float)height) / dimensionData.height);
		rsvg_handle_render_cairo(handle, cr);

		cairo_destroy(cr);
		cairo_surface_destroy(cairo_surface);

		image->flipVertical();
		return image;
	}
protected:
	virtual ~ReaderWriterSVG()
	{
		rsvg_cleanup();
	}
};

// now register with Registry to instantiate the above
// reader/writer.
REGISTER_OSGPLUGIN(SVG, ReaderWriterSVG)
