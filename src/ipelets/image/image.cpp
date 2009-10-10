// --------------------------------------------------------------------
// Ipelet to insert an image
// --------------------------------------------------------------------
/*

    This file is part of the extensible drawing editor Ipe.
    Copyright (C) 1993-2009  Otfried Cheong

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

#include "ipelib.h"

#include <QFileDialog>
#include <QImage>
#include <QMessageBox>
#include <QFile>
#include <QClipboard>
#include <QApplication>

using namespace ipe;

// --------------------------------------------------------------------

/*
 JPG reading code
 Copyright (c) 1996-2002 Han The Thanh, <thanh@pdftex.org>

 This code is part of pdfTeX.

 pdfTeX is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.
*/

#define JPG_GRAY  1     /* Gray color space, use /DeviceGray  */
#define JPG_RGB   3     /* RGB color space, use /DeviceRGB    */
#define JPG_CMYK  4     /* CMYK color space, use /DeviceCMYK  */

enum JPEG_MARKER {      /* JPEG marker codes                    */
  M_SOF0  = 0xc0,       /* baseline DCT                         */
  M_SOF1  = 0xc1,       /* extended sequential DCT              */
  M_SOF2  = 0xc2,       /* progressive DCT                      */
  M_SOF3  = 0xc3,       /* lossless (sequential)                */

  M_SOF5  = 0xc5,       /* differential sequential DCT          */
  M_SOF6  = 0xc6,       /* differential progressive DCT         */
  M_SOF7  = 0xc7,       /* differential lossless                */

  M_JPG   = 0xc8,       /* JPEG extensions                      */
  M_SOF9  = 0xc9,       /* extended sequential DCT              */
  M_SOF10 = 0xca,       /* progressive DCT                      */
  M_SOF11 = 0xcb,       /* lossless (sequential)                */

  M_SOF13 = 0xcd,       /* differential sequential DCT          */
  M_SOF14 = 0xce,       /* differential progressive DCT         */
  M_SOF15 = 0xcf,       /* differential lossless                */

  M_DHT   = 0xc4,       /* define Huffman tables                */

  M_DAC   = 0xcc,       /* define arithmetic conditioning table */

  M_RST0  = 0xd0,       /* restart                              */
  M_RST1  = 0xd1,       /* restart                              */
  M_RST2  = 0xd2,       /* restart                              */
  M_RST3  = 0xd3,       /* restart                              */
  M_RST4  = 0xd4,       /* restart                              */
  M_RST5  = 0xd5,       /* restart                              */
  M_RST6  = 0xd6,       /* restart                              */
  M_RST7  = 0xd7,       /* restart                              */

  M_SOI   = 0xd8,       /* start of image                       */
  M_EOI   = 0xd9,       /* end of image                         */
  M_SOS   = 0xda,       /* start of scan                        */
  M_DQT   = 0xdb,       /* define quantization tables           */
  M_DNL   = 0xdc,       /* define number of lines               */
  M_DRI   = 0xdd,       /* define restart interval              */
  M_DHP   = 0xde,       /* define hierarchical progression      */
  M_EXP   = 0xdf,       /* expand reference image(s)            */

  M_APP0  = 0xe0,       /* application marker, used for JFIF    */
  M_APP1  = 0xe1,       /* application marker                   */
  M_APP2  = 0xe2,       /* application marker                   */
  M_APP3  = 0xe3,       /* application marker                   */
  M_APP4  = 0xe4,       /* application marker                   */
  M_APP5  = 0xe5,       /* application marker                   */
  M_APP6  = 0xe6,       /* application marker                   */
  M_APP7  = 0xe7,       /* application marker                   */
  M_APP8  = 0xe8,       /* application marker                   */
  M_APP9  = 0xe9,       /* application marker                   */
  M_APP10 = 0xea,       /* application marker                   */
  M_APP11 = 0xeb,       /* application marker                   */
  M_APP12 = 0xec,       /* application marker                   */
  M_APP13 = 0xed,       /* application marker                   */
  M_APP14 = 0xee,       /* application marker, used by Adobe    */
  M_APP15 = 0xef,       /* application marker                   */

  M_JPG0  = 0xf0,       /* reserved for JPEG extensions         */
  M_JPG13 = 0xfd,       /* reserved for JPEG extensions         */
  M_COM   = 0xfe,       /* comment                              */

  M_TEM   = 0x01,       /* temporary use                        */

  M_ERROR = 0x100       /* dummy marker, internal use only      */
};

inline int read2bytes(QFile &f)
{
  char c1, c2;
  f.getChar(&c1);
  f.getChar(&c2);
  return (uchar(c1) << 8) + uchar(c2);
}

// --------------------------------------------------------------------

class ImageIpelet : public Ipelet {
public:
  virtual int ipelibVersion() const { return IPELIB_VERSION; }
  virtual bool run(int fn, IpeletData *data, IpeletHelper *helper);

private:
  bool insertBitmap(QString name);
  bool insertJpeg(QString name);
  void fail(QString msg);
  void fail(const char *msg) { fail(QLatin1String(msg)); }
  bool readJpegInfo(QFile &file);
  Rect computeRect();

private:
  IpeletData *iData;
  int iWidth;
  int iHeight;
  Bitmap::TColorSpace iColorSpace;
  int iBitsPerComponent;
  Vector iDotsPerInch;
};

// --------------------------------------------------------------------

bool ImageIpelet::run(int fn, IpeletData *data, IpeletHelper *helper)
{
  iData = data;
  QString name;
  if (fn != 2) {
    name = QFileDialog::getOpenFileName();
    if (name.isNull())
      return false;
  }
  switch (fn) {
  case 0: // bitmap
    return insertBitmap(name);
  case 1: // JPEG
    return insertJpeg(name);
  case 2: // bitmap from clipboard
    return insertBitmap(QString::null);
  default:
    return false;
  }
}

void ImageIpelet::fail(QString msg)
{
  QMessageBox::warning(0, QLatin1String("Insert image ipelet"),
		       QLatin1String("<qt>") + msg
		       + QLatin1String("</qt>"),
		       QLatin1String("Dismiss"));
}

// --------------------------------------------------------------------

Rect ImageIpelet::computeRect()
{
  Vector frame = iData->iDoc->cascade()->findLayout()->iFrameSize;

  double dx = (iWidth * 72.0) / iDotsPerInch.x;
  double dy = (iHeight * 72.0) / iDotsPerInch.y;

  double xfactor = 1.0;
  if (dx > frame.x)
    xfactor = frame.x / dx;
  double yfactor = 1.0;
  if (dy > frame.y)
    yfactor = frame.y / dy;
  double factor = (xfactor < yfactor) ? xfactor : yfactor;
  Rect rect(Vector::ZERO, factor * Vector(dx, dy));
  Vector v = 0.5 * Vector(frame.x - (rect.left() + rect.right()),
			  frame.y - (rect.bottom() + rect.top()));
  return Rect(rect.bottomLeft() + v, rect.topRight() + v);
}

// --------------------------------------------------------------------

// 72 points per inch
const double InchPerMeter = (1000.0 / 25.4);

bool ImageIpelet::insertBitmap(QString name)
{
  ipeDebug("insertBitmap");
  QImage im;
  if (name.isNull()) {
    QClipboard *cb = QApplication::clipboard();
    ipeDebug("about to retrieve image");
    im = cb->image();
    ipeDebug("image retrieved %d", im.width());
    if (im.isNull()) {
      fail("The clipboard contains no image, or perhaps\n"
	   "an image in a format not supported by Qt.");
      return false;
    }
  } else {
    if (!im.load(name)) {
      fail("The image could not be loaded.\n"
	   "Perhaps the format is not supported by Qt.");
      return false;
    }
  }
  QImage im1 = im.convertToFormat(QImage::Format_RGB32);
  iWidth = im1.width();
  iHeight = im1.height();
  iDotsPerInch = Vector(72, 72);

  if (im1.dotsPerMeterX())
    iDotsPerInch.x = double(im1.dotsPerMeterX()) / InchPerMeter;
  if (im1.dotsPerMeterY())
    iDotsPerInch.y = double(im1.dotsPerMeterY()) / InchPerMeter;

  bool isGray = im1.allGray();
  iColorSpace = isGray ? Bitmap::EDeviceGray : Bitmap::EDeviceRGB;
  int datalen = iWidth * iHeight * (isGray ? 1 : 3);
  Buffer data(datalen);
  char *d = data.data();
  for (int y = 0; y < iHeight; ++y) {
    uint *p = (uint *) im1.scanLine(y);
    for (int x = 0; x < iWidth; ++x) {
      if (isGray) {
	*d++ = qRed(*p++);
      } else {
	*d++ = qRed(*p);
	*d++ = qGreen(*p);
	*d++ = qBlue(*p++);
      }
    }
  }

  Bitmap bitmap(iWidth, iHeight, iColorSpace, 8, data, Bitmap::EDirect, true);
  Image *obj = new Image(computeRect(), bitmap);
  iData->iPage->append(ESecondarySelected, iData->iLayer, obj);
  return true;
}

// --------------------------------------------------------------------

bool ImageIpelet::readJpegInfo(QFile &file)
{
  static char jpg_id[] = "JFIF";
  char units;
  int xres;
  int yres;
  int fpos;
  char ch;

  iDotsPerInch = Vector(72.0, 72.0);

  file.seek(0);
  if (read2bytes(file) != 0xFFD8) {
    fail("The file does not appear to be a JPEG image");
    return false;
  }
  if (read2bytes(file) == 0xFFE0) { /* JFIF APP0 */
    read2bytes(file);
    for (int i = 0; i < 5; i++) {
      file.getChar(&ch);
      if (ch != jpg_id[i]) {
	fail("Reading JPEG image failed");
	return false;
      }
    }
    read2bytes(file);
    file.getChar(&units);
    xres = read2bytes(file);
    yres = read2bytes(file);
    if (xres != 0 && yres != 0) {
      switch (units) {
      case 1: /* pixels per inch */
	iDotsPerInch = Vector(xres, yres);
	break;
      case 2: /* pixels per cm */
	iDotsPerInch = Vector(xres * 2.54, yres * 2.54);
	break;
      default:
	break;
      }
    }
  }

  file.seek(0);
  while(1) {
    if (file.atEnd() || (file.getChar(&ch), ch != char(0xFF))) {
      fail("Reading JPEG image failed");
      return false;
    }
    file.getChar(&ch);
    switch (ch & 0xff) {
    case M_SOF5:
    case M_SOF6:
    case M_SOF7:
    case M_SOF9:
    case M_SOF10:
    case M_SOF11:
    case M_SOF13:
    case M_SOF14:
    case M_SOF15:
      fail("Unsupported type of JPEG compression");
      return false;
    case M_SOF2:
      //if (cfgpar(cfgpdf12compliantcode) > 0)
      //pdftex_fail("cannot use progressive DCT with PDF-1.2");
    case M_SOF0:
    case M_SOF1:
    case M_SOF3:
      read2bytes(file);    /* read segment length  */
      file.getChar(&ch);
      iBitsPerComponent = ch;
      iHeight = read2bytes(file);
      iWidth = read2bytes(file);
      file.getChar(&ch);
      switch (ch & 0xff) {
      case JPG_GRAY:
	iColorSpace = Bitmap::EDeviceGray;
        break;
      case JPG_RGB:
	iColorSpace = Bitmap::EDeviceRGB;
        break;
      case JPG_CMYK:
	iColorSpace = Bitmap::EDeviceCMYK;
        // pdf_puts("/DeviceCMYK\n/Decode [1 0 1 0 1 0 1 0]\n");
	break;
      default:
	fail("Unsupported color space in JPEG image");
	return false;
      }
      file.seek(0);
      return true;
    case M_SOI:        /* ignore markers without parameters */
    case M_EOI:
    case M_TEM:
    case M_RST0:
    case M_RST1:
    case M_RST2:
    case M_RST3:
    case M_RST4:
    case M_RST5:
    case M_RST6:
    case M_RST7:
      break;
    default:           /* skip variable length markers */
      fpos = file.pos();
      file.seek(fpos + read2bytes(file));
      break;
    }
  }
}

bool ImageIpelet::insertJpeg(QString name)
{
  QFile file(name);
  if (!file.open(QIODevice::ReadOnly)) {
    fail(QString(QLatin1String("Could not open file '%1'")).arg(name));
    return false;
  }
  if (!readJpegInfo(file))
    return false;

  QByteArray a = file.readAll();
  file.close();

  Bitmap bitmap(iWidth, iHeight, iColorSpace,
		iBitsPerComponent, Buffer(a.data(), a.size()),
		Bitmap::EDCTDecode);
  Image *obj = new Image(computeRect(), bitmap);
  iData->iPage->append(ESecondarySelected, iData->iLayer, obj);
  return true;
}

// --------------------------------------------------------------------

IPELET_DECLARE Ipelet *newIpelet()
{
  return new ImageIpelet;
}

// --------------------------------------------------------------------
