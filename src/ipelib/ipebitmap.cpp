// --------------------------------------------------------------------
// Bitmaps
// --------------------------------------------------------------------
/*

    This file is part of the extensible drawing editor Ipe.
    Copyright (C) 1993-2013  Otfried Cheong

    Ipe is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    As a special exception, you have permission to link Ipe with the
    CGAL library and distribute executables, as long as you follow the
    requirements of the Gnu General Public License in regard to all of
    the software in the executable aside from CGAL.

    Ipe is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
    or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
    License for more details.

    You should have received a copy of the GNU General Public License
    along with Ipe; if not, you can find it at
    "http://www.gnu.org/copyleft/gpl.html", or write to the Free
    Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#include "ipebitmap.h"
#include "ipeutils.h"
#include <zlib.h>

using namespace ipe;

extern bool dctDecode(Buffer dctData, Buffer pixelData);

// --------------------------------------------------------------------

/*! \class ipe::Bitmap::MRenderData
  \ingroup base
  \brief Abstract base class for pixmap data stored by a client.
*/

Bitmap::MRenderData::~MRenderData()
{
  // nothing
}

/*! \class ipe::Bitmap
  \ingroup base
  \brief A bitmap.

  Bitmaps are explicitely shared using reference-counting.  Copying is
  cheap, so Bitmap objects are meant to be passed by value.

  The bitmap can cache data to speed up rendering. This data can be
  set only once (as the bitmap is conceptually immutable).

  The bitmap also provides a slot for short-term storage of an "object
  number".  The PDF embedder, for instance, sets it to the PDF object
  number when embedding the bitmap, and can reuse it when "drawing"
  the bitmap.
*/

//! Default constructor constructs null bitmap.
Bitmap::Bitmap()
{
  iImp = 0;
}

//! Create from XML stream.
Bitmap::Bitmap(const XmlAttributes &attr, String data)
{
  int length = init(attr);
  // decode data
  iImp->iData = Buffer(length);
  char *p = iImp->iData.data();
  if (attr["encoding"] ==  "base64") {
    Buffer dbuffer(data.data(), data.size());
    BufferSource source(dbuffer);
    Base64Source b64source(source);
    while (length-- > 0)
      *p++ = b64source.getChar();
  } else {
    Lex datalex(data);
    while (length-- > 0)
      *p++ = char(datalex.getHexByte());
  }
  computeChecksum();
}

//! Create from XML using external raw data
Bitmap::Bitmap(const XmlAttributes &attr, Buffer data)
{
  int length = init(attr);
  assert(length == data.size());
  iImp->iData = data;
  computeChecksum();
}

int Bitmap::init(const XmlAttributes &attr)
{
  iImp = new Imp;
  iImp->iRefCount = 1;
  iImp->iObjNum = Lex(attr["id"]).getInt();
  iImp->iRender = 0;
  iImp->iWidth = Lex(attr["width"]).getInt();
  iImp->iHeight = Lex(attr["height"]).getInt();
  iImp->iColorKey = -1;
  int length = Lex(attr["length"]).getInt();
  assert(iImp->iWidth > 0 && iImp->iHeight > 0);
  String cs = attr["ColorSpace"];
  if (cs == "DeviceGray") {
    iImp->iComponents = 1;
    iImp->iColorSpace = EDeviceGray;
  } else if (cs == "DeviceCMYK") {
    iImp->iComponents = 4;
    iImp->iColorSpace = EDeviceCMYK;
  } else {
    iImp->iComponents = 3;
    iImp->iColorSpace = EDeviceRGB;
  }
  String cc;
  if (iImp->iColorSpace == EDeviceRGB && attr.has("ColorKey", cc)) {
    iImp->iColorKey = Lex(cc).getHexNumber();
  }
  String fi = attr["Filter"];
  if (fi == "DCTDecode")
    iImp->iFilter = EDCTDecode;
  else if (fi == "FlateDecode")
    iImp->iFilter = EFlateDecode;
  else
    iImp->iFilter = EDirect;
  iImp->iBitsPerComponent = Lex(attr["BitsPerComponent"]).getInt();
  if (length == 0) {
    assert(iImp->iFilter == EDirect);
    int bitsPerRow = width() * components() * bitsPerComponent();
    int bytesPerRow = (bitsPerRow + 7) / 8;
    length = height() * bytesPerRow;
  }
  return length;
}

//! Create a new image
Bitmap::Bitmap(int width, int height,
	       TColorSpace colorSpace, int bitsPerComponent,
	       Buffer data, TFilter filter, bool deflate)
{
  iImp = new Imp;
  iImp->iRefCount = 1;
  iImp->iObjNum = -1;
  iImp->iRender = 0;
  iImp->iWidth = width;
  iImp->iHeight = height;
  iImp->iColorKey = -1;
  assert(iImp->iWidth > 0 && iImp->iHeight > 0);
  iImp->iColorSpace = colorSpace;
  switch (colorSpace) {
  case EDeviceGray:
    iImp->iComponents = 1;
    break;
  case EDeviceRGB:
    iImp->iComponents = 3;
    break;
  case EDeviceCMYK:
    iImp->iComponents = 4;
    break;
  }
  iImp->iFilter = filter;
  iImp->iBitsPerComponent = bitsPerComponent;
  if (deflate && filter == EDirect) {
    // optional deflation
    int deflatedSize;
    Buffer deflated = DeflateStream::deflate(data.data(), data.size(),
					     deflatedSize, 9);
    iImp->iData = Buffer(deflated.data(), deflatedSize);
    iImp->iFilter = EFlateDecode;
  } else
    iImp->iData = data;
  computeChecksum();
}

//! Copy constructor.
/*! Since Bitmaps are reference counted, this is very fast. */
Bitmap::Bitmap(const Bitmap &rhs)
{
  iImp = rhs.iImp;
  if (iImp)
    iImp->iRefCount++;
}

//! Destructor.
Bitmap::~Bitmap()
{
  if (iImp && --iImp->iRefCount == 0) {
    delete iImp->iRender;
    delete iImp;
  }
}

//! Assignment operator (takes care of reference counting).
/*! Very fast. */
Bitmap &Bitmap::operator=(const Bitmap &rhs)
{
  if (this != &rhs) {
    if (iImp && --iImp->iRefCount == 0)
      delete iImp;
    iImp = rhs.iImp;
    if (iImp)
      iImp->iRefCount++;
  }
  return *this;
}

//! Return rgb representation of the transparent color.
/*! Returns -1 if the bitmap is not color keyed. */
int Bitmap::colorKey() const
{
  return iImp->iColorKey;
}

//! Set transparent color.
/*! Use \a key == -1 to disable color key. */
void Bitmap::setColorKey(int key)
{
  iImp->iColorKey = key;
}

//! Save bitmap in XML stream.
void Bitmap::saveAsXml(Stream &stream, int id, int pdfObjNum) const
{
  assert(iImp);
  stream << "<bitmap";
  stream << " id=\"" << id << "\"";
  stream << " width=\"" << width() << "\"";
  stream << " height=\"" << height() << "\"";
  stream << " length=\"" << size() << "\"";
  switch (colorSpace()) {
  case EDeviceGray:
    stream << " ColorSpace=\"DeviceGray\"";
    break;
  case EDeviceRGB:
    stream << " ColorSpace=\"DeviceRGB\"";
    break;
  case EDeviceCMYK:
    stream << " ColorSpace=\"DeviceCMYK\"";
    break;
  }
  switch (filter()) {
  case EFlateDecode:
    stream << " Filter=\"FlateDecode\"";
    break;
  case EDCTDecode:
    stream << " Filter=\"DCTDecode\"";
    break;
  default:
    // no filter
    break;
  }
  stream << " BitsPerComponent=\"" << bitsPerComponent() << "\"";

  if (iImp->iColorKey >= 0) {
    char buf[10];
    sprintf(buf, "%x", iImp->iColorKey);
    stream << " ColorKey=\"" << buf << "\"";
  }

  if (pdfObjNum >= 0) {
    stream << " pdfObject=\"" << pdfObjNum << "\"/>\n";
  } else {
    // save data
    stream << " encoding=\"base64\">\n";
    const char *p = data();
    const char *fin = p + size();
    Base64Stream b64(stream);
    while (p != fin)
      b64.putChar(*p++);
    b64.close();
    stream << "</bitmap>\n";
  }
}

//! Set a cached bitmap for fast rendering.
void Bitmap::setRenderData(MRenderData *data) const
{
  assert(iImp && iImp->iRender == 0);
  iImp->iRender = data;
}

bool Bitmap::equal(Bitmap rhs) const
{
  if (iImp == rhs.iImp)
    return true;
  if (!iImp || !rhs.iImp)
    return false;

  if (iImp->iColorSpace != rhs.iImp->iColorSpace ||
      iImp->iBitsPerComponent != rhs.iImp->iBitsPerComponent ||
      iImp->iWidth != rhs.iImp->iWidth ||
      iImp->iHeight != rhs.iImp->iHeight ||
      iImp->iComponents != rhs.iImp->iComponents ||
      iImp->iColorKey != rhs.iImp->iColorKey ||
      iImp->iFilter != rhs.iImp->iFilter ||
      iImp->iChecksum != rhs.iImp->iChecksum ||
      iImp->iData.size() != rhs.iImp->iData.size())
    return false;
  // check actual data
  int len = iImp->iData.size();
  char *p = iImp->iData.data();
  char *q = rhs.iImp->iData.data();
  while (len--) {
    if (*p++ != *q++)
      return false;
  }
  return true;
}

void Bitmap::computeChecksum()
{
  int s = 0;
  int len = iImp->iData.size();
  char *p = iImp->iData.data();
  while (len--) {
    s = (s & 0x0fffffff) << 3;
    s += *p++;
  }
  iImp->iChecksum = s;
}

// --------------------------------------------------------------------

//! Convert bitmap data to a height x width pixel array in rgb format.
/*! Returns empty buffer if it cannot decode the bitmap information.
  Otherwise, returns a buffer of size Width() * Height() uint's. */
Buffer Bitmap::pixelData() const
{
  ipeDebug("pixelData %d x %d x %d, %d", width(), height(),
	   components(), int(filter()));
  if (bitsPerComponent() != 8)
    return Buffer();
  Buffer stream = iImp->iData;
  Buffer pixels;
  if (filter() == EDirect) {
    pixels = stream;
  } else if (filter() == EFlateDecode) {
    // inflate data
    uLong inflatedSize = width() * height() * components();
    pixels = Buffer(inflatedSize);
    if (uncompress((Bytef *) pixels.data(), &inflatedSize,
		   (const Bytef *) stream.data(), stream.size()) != Z_OK
	|| pixels.size() != int(inflatedSize))
      return Buffer();
  } else if (filter() == EDCTDecode) {
    pixels = Buffer(width() * height() * components());
    if (!dctDecode(stream, pixels))
      return Buffer();
  }
  Buffer data(height() * width() * sizeof(uint));
  // convert pixels to data
  const char *p = pixels.data();
  uint *q = (uint *) data.data();
  if (components() == 3) {
    uint colorKey = (iImp->iColorKey | 0xff000000);
    if (iImp->iColorKey < 0)
      colorKey = 0;
    for (int y = 0; y < iImp->iHeight; ++y) {
      for (int x = 0; x < iImp->iWidth; ++x) {
	uchar r = uchar(*p++);
	uchar g = uchar(*p++);
	uchar b = uchar(*p++);
	uint pixel = 0xff000000 | (r << 16) | (g << 8) | b;
	if (pixel == colorKey)
	  *q++ = 0;
	else
	  *q++ = pixel;
      }
    }
  } else if (components() == 1) {
    for (int y = 0; y < iImp->iHeight; ++y) {
      for (int x = 0; x < iImp->iWidth; ++x) {
	uchar r = uchar(*p++);
	*q++ = 0xff000000 | (r << 16) | (r << 8) | r;
      }
    }
  }
  return data;
}

// --------------------------------------------------------------------
