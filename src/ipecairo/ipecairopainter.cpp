// --------------------------------------------------------------------
// Painter using Cairo library
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

#include "ipetext.h"
#include "ipepdfparser.h"
#include "ipecairopainter.h"
#include "ipefonts.h"

using namespace ipe;

// --------------------------------------------------------------------

/*! \defgroup cairo Ipe Cairo interface
  \brief Drawing Ipe objects using the Cairo library.

  This module contains the classes needed to render Ipe objects using
  the Cairo and Freetype libraries.

  These classes are not in Ipelib, but in a separate library
  libipecairo.
*/

// --------------------------------------------------------------------

/*! \class ipe::CairoPainter
  \ingroup cairo
  \brief Ipe Painter using Cairo and Freetype as a backend.

  This painter draws to a Cairo surface.
*/

//! Construct a painter.
/*! \a zoom one means 72 pixels per inch.  Set \a pretty to true
  to avoid drawing text without Latex. */
CairoPainter::CairoPainter(const Cascade *sheet, Fonts *fonts,
			   cairo_t *cc, double zoom, bool pretty)
  : Painter(sheet), iFonts(fonts),
    iCairo(cc), iZoom(zoom), iPretty(pretty)
{
  iDimmed = false;
  iFont = 0;
}

void CairoPainter::doPush()
{
  cairo_save(iCairo);
}

void CairoPainter::doPop()
{
  cairo_restore(iCairo);
}

void CairoPainter::doMoveTo(const Vector &u)
{
  cairo_move_to(iCairo, u.x, u.y);
}

void CairoPainter::doLineTo(const Vector &u)
{
  cairo_line_to(iCairo, u.x, u.y);
}

void CairoPainter::doCurveTo(const Vector &u1, const Vector &u2,
			     const Vector &u3)
{
  cairo_curve_to(iCairo, u1.x, u1.y, u2.x, u2.y, u3.x, u3.y);
}

void CairoPainter::doClosePath()
{
  cairo_close_path(iCairo);
}

void CairoPainter::doDrawArc(const Arc &arc)
{
  cairo_save(iCairo);
  Matrix m = matrix() * arc.iM;
  cairo_matrix_t matrix;
  matrix.xx = m.a[0];
  matrix.xy = m.a[2];
  matrix.yx = m.a[1];
  matrix.yy = m.a[3];
  matrix.x0 = m.a[4];
  matrix.y0 = m.a[5];
  cairo_transform(iCairo, &matrix);
  if (arc.isEllipse()) {
    cairo_new_sub_path(iCairo);
    cairo_arc(iCairo, 0.0, 0.0, 1.0, 0.0, IpeTwoPi);
    cairo_close_path(iCairo);
  } else
    cairo_arc(iCairo, 0.0, 0.0, 1.0, arc.iAlpha, arc.iBeta);
  cairo_restore(iCairo);
}

void CairoPainter::doDrawPath(TPathMode mode)
{
  cairo_save(iCairo);
  if (mode >= EStrokedAndFilled) {
    Color fillColor = fill();
    // DimColor(qfill);

    cairo_set_fill_rule(iCairo, (fillRule() == EEvenOddRule) ?
			CAIRO_FILL_RULE_EVEN_ODD : CAIRO_FILL_RULE_WINDING);

    const Tiling *t = 0;
    if (!tiling().isNormal())
      t = cascade()->findTiling(tiling());

    if (t == 0) {
      // no tiling, or tiling not found
      cairo_set_source_rgba(iCairo, fillColor.iRed.toDouble(),
			    fillColor.iGreen.toDouble(),
			    fillColor.iBlue.toDouble(),
			    opacity().toDouble());
      if (mode == EStrokedAndFilled)
	cairo_fill_preserve(iCairo);
      else
	cairo_fill(iCairo);

    } else {
      // tiling
      cairo_surface_t *s =
	cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 32, 32);
      uchar *data = cairo_image_surface_get_data(s);
      memset(data, 0, 4 * 32 * 32);

      cairo_t *cc = cairo_create(s);
      cairo_set_source_rgba(cc,
			    fillColor.iRed.toDouble(),
			    fillColor.iGreen.toDouble(),
			    fillColor.iBlue.toDouble(),
			    1.0);

      cairo_rectangle(cc, 0, 0, 32, 32 * t->iWidth / t->iStep);
      cairo_fill(cc);
      cairo_destroy(cc);
      cairo_pattern_t *p = cairo_pattern_create_for_surface(s);
      cairo_pattern_set_extend(p, CAIRO_EXTEND_REPEAT);

      cairo_matrix_t m;
      cairo_matrix_init_scale(&m, 1.0, 32.0 / t->iStep);
      cairo_matrix_rotate(&m, -double(t->iAngle));
      cairo_pattern_set_matrix(p, &m);

      cairo_set_source(iCairo, p);

      if (mode == EStrokedAndFilled)
	cairo_fill_preserve(iCairo);
      else
	cairo_fill(iCairo);

      cairo_pattern_destroy(p);
      cairo_surface_destroy(s);
    }
  }

  if (mode <= EStrokedAndFilled) {
    Color strokeColor = stroke();
    // DimColor(qstroke);
    cairo_set_source_rgba(iCairo, strokeColor.iRed.toDouble(),
			  strokeColor.iGreen.toDouble(),
			  strokeColor.iBlue.toDouble(),
			  opacity().toDouble());

    cairo_set_line_width(iCairo, pen().toDouble());

    switch (lineJoin()) {
    case EMiterJoin:
      cairo_set_line_join(iCairo, CAIRO_LINE_JOIN_MITER);
      break;
    case ERoundJoin:
    case EDefaultJoin:
      cairo_set_line_join(iCairo, CAIRO_LINE_JOIN_ROUND);
      break;
    case EBevelJoin:
      cairo_set_line_join(iCairo, CAIRO_LINE_JOIN_BEVEL);
      break;
    }
    switch (lineCap()) {
    case EButtCap:
      cairo_set_line_cap(iCairo, CAIRO_LINE_CAP_BUTT);
      break;
    case ERoundCap:
    case EDefaultCap:
      cairo_set_line_cap(iCairo, CAIRO_LINE_CAP_ROUND);
      break;
    case ESquareCap:
      cairo_set_line_cap(iCairo, CAIRO_LINE_CAP_SQUARE);
      break;
    }
    if (dashStyle() != "[]0") {
      std::vector<double> dashes;
      double offset;
      dashStyle(dashes, offset);
      cairo_set_dash(iCairo, &dashes[0], dashes.size(), offset);
    }
    cairo_stroke(iCairo);
  }
  cairo_restore(iCairo);
}

void CairoPainter::doAddClipPath()
{
  cairo_clip(iCairo);
}

void CairoPainter::doDrawGradient(Attribute gradient)
{
  const Gradient *g = cascade()->findGradient(gradient);
  if (!g) return;
  cairo_pattern_t *p;
  if (g->iType == Gradient::ERadial)
    p = cairo_pattern_create_radial(g->iV[0].x, g->iV[0].y, g->iRadius[0],
				    g->iV[1].x, g->iV[1].y, g->iRadius[1]);
  else
    p = cairo_pattern_create_linear(g->iV[0].x, g->iV[0].y,
				    g->iV[1].x, g->iV[1].y);
  cairo_pattern_set_extend(p, g->iExtend ?
			   CAIRO_EXTEND_PAD : CAIRO_EXTEND_NONE);
  cairo_pattern_add_color_stop_rgb(p, 0.0,
				   g->iColor[0].iRed.toDouble(),
				   g->iColor[0].iGreen.toDouble(),
				   g->iColor[0].iBlue.toDouble());
  cairo_pattern_add_color_stop_rgb(p, 1.0,
				   g->iColor[1].iRed.toDouble(),
				   g->iColor[1].iGreen.toDouble(),
				   g->iColor[1].iBlue.toDouble());

  cairo_save(iCairo);
  const Matrix &m0 = matrix();
  cairo_matrix_t m;
  m.xx = m0.a[0];
  m.yx = m0.a[1];
  m.xy = m0.a[2];
  m.yy = m0.a[3];
  m.x0 = m0.a[4];
  m.y0 = m0.a[5];
  cairo_transform(iCairo, &m);
  cairo_set_source(iCairo, p);
  cairo_paint(iCairo);
  cairo_restore(iCairo);
  cairo_pattern_destroy(p);
}

// --------------------------------------------------------------------

class RenderData : public Bitmap::MRenderData {
public:
  virtual ~RenderData() { /* Nothing */ }
  Buffer pixels;
};

void CairoPainter::doDrawBitmap(Bitmap bitmap)
{
  // make caching optional, or cache only most recent bitmaps?
#if 1
  // The original data in the bitmap may be deflated or dct encoded
  // cache the decoded data for faster rendering
  if (!bitmap.renderData()) {
    RenderData *render = new RenderData;
    render->pixels = bitmap.pixelData(); // empty if failed
    // may need to scale down?
    bitmap.setRenderData(render);
  }
  if (!bitmap.renderData())
    return;
  RenderData *render = static_cast<RenderData *>(bitmap.renderData());
  Buffer data = render->pixels;
#else
  Buffer data = bitmap.pixelData();
#endif
  if (!data.size())
    return;
  // is this legal?  I don't want cairo to modify my bitmap temporarily.
  cairo_surface_t *image =
    cairo_image_surface_create_for_data((uchar *) data.data(),
					CAIRO_FORMAT_RGB24,
					bitmap.width(), bitmap.height(),
					4 * bitmap.width());
  cairo_save(iCairo);
  Matrix tf = matrix() * Matrix(1.0 / bitmap.width(), 0.0,
				0.0, -1.0 / bitmap.height(),
				0.0, 1.0);
  cairo_matrix_t matrix;
  matrix.xx = tf.a[0];
  matrix.yx = tf.a[1];
  matrix.xy = tf.a[2];
  matrix.yy = tf.a[3];
  matrix.x0 = tf.a[4];
  matrix.y0 = tf.a[5];
  cairo_transform(iCairo, &matrix);
  cairo_set_source_surface(iCairo, image, 0, 0);
  cairo_paint(iCairo);
  cairo_restore(iCairo);
}

void CairoPainter::doDrawText(const Text *text)
{
  // Current origin is lower left corner of text box

  // Draw bounding box rectangle
  if (!iPretty && !iDimmed && !text->isInternal()) {
    cairo_save(iCairo);
    cairo_set_source_rgb(iCairo, 0.0, 1.0, 0.0);
    cairo_set_line_width(iCairo, 1.0 / iZoom);
    double dash = 3.0 / iZoom;
    cairo_set_dash(iCairo, &dash, 1, 0.0);
    Vector u0 = matrix() * Vector::ZERO;
    Vector u1 = matrix() * Vector(0, text->totalHeight());
    Vector u2 = matrix() * Vector(text->width(), text->totalHeight());
    Vector u3 = matrix() * Vector(text->width(), 0);
    cairo_move_to(iCairo, u0.x, u0.y);
    cairo_line_to(iCairo, u1.x, u1.y);
    cairo_line_to(iCairo, u2.x, u2.y);
    cairo_line_to(iCairo, u3.x, u3.y);
    cairo_close_path(iCairo);
    cairo_stroke(iCairo);

    Vector ref = matrix() * text->align();
    cairo_rectangle(iCairo, ref.x - 3.0/iZoom, ref.y - 3.0/iZoom,
		    6.0/iZoom, 6.0/iZoom);
    cairo_fill(iCairo);
    cairo_restore(iCairo);
  }

  Color col = stroke();
  iTextRgb[0] = col.iRed.toDouble();
  iTextRgb[1] = col.iGreen.toDouble();
  iTextRgb[2] = col.iBlue.toDouble();

  // DimColor(iTextRgb);

  const Text::XForm *xf = text->getXForm();
  if (!xf || !iFonts) {
    if (!text->isInternal()) {
      String s = text->text();
      int i = s.find('\n');
      if (i < 0 || i > 30)
	i = 30;
      if (i < s.size())
	s = s.left(i) + "...";

      Vector pt = matrix().translation();
      // pt.y = pt.y - iPainter->fontMetrics().descent();
      cairo_font_face_t *font = Fonts::screenFont();
      if (font) {
	cairo_save(iCairo);
	cairo_set_font_face(iCairo, font);
	cairo_set_font_size(iCairo, 9.0);
	cairo_set_source_rgba(iCairo, iTextRgb[0], iTextRgb[1], iTextRgb[2],
			      opacity().toDouble());
	cairo_translate(iCairo, pt.x, pt.y);
	cairo_scale(iCairo, 1.0, -1.0);
	cairo_show_text(iCairo, s.z());
	cairo_restore(iCairo);
      }
    }
  } else {
    transform(Matrix(xf->iStretch, 0, 0, xf->iStretch, 0, 0));
    execute(xf->iStream);
  }
}

// --------------------------------------------------------------------

#if 0
//! If dimming, replace color by dimmed version.
void CairoPainter::DimColor(QColor &col)
{
  if (iDimmed) {
    int h, s, v;
    col.getHsv(&h, &s, &v);
    v += 150;
    if (v > 255)
      col = col.light(175);
    else
      col.setHsv(h, s, v);
  }
}
#endif

// --------------------------------------------------------------------

//! Clear PDF argument stack
void CairoPainter::clearArgs()
{
  while (!iArgs.empty()) {
    delete iArgs.back();
    iArgs.pop_back();
  }
}

void CairoPainter::execute(const Buffer &buffer)
{
  BufferSource source(buffer);
  PdfParser parser(source);
  iFont = 0;
  iFillRgb[0] = iFillRgb[1] = iFillRgb[2] = 0.0;
  iStrokeRgb[0] = iStrokeRgb[1] = iStrokeRgb[2] = 0.0;
  while (!parser.eos()) {
    PdfToken tok = parser.token();
    if (tok.iType != PdfToken::EOp) {
      const PdfObj *obj = parser.getObject();
      if (!obj)
	break; // no further parsing attempted
      iArgs.push_back(obj);
    } else {
      // its an operator, execute it
      String op = tok.iString;
      parser.getToken();
      if (op == "cm")
	opcm();
      else if (op == "q")
	opq();
      else if (op == "Q")
	opQ();
      else if (op == "Tf")
	opTf();
      else if (op == "Td")
	opTd();
      else if (op == "TJ")
	opTJ();
      else if (op == "rg")
	oprg(false);
      else if (op == "RG")
	oprg(true);
      else if (op == "g")
	opg(false);
      else if (op == "G")
	opg(true);
      else if (op == "k")
	opk(false);
      else if (op == "K")
	opk(true);
      else if (op == "w")
	opw();
      else if (op == "m")
	opm();
      else if (op == "l")
	opl();
      else if (op == "re")
	opre();
      else if (op == "sym")
	opsym();
      else if (op == "BT")
	opBT();
      else if (op != "ET") {
	String a;
	for (uint i = 0; i < iArgs.size(); ++i)
	  a += iArgs[i]->repr() + " ";
	ipeDebug("op %s (%s)", op.z(), a.z());
      }
      clearArgs();
    }
  }
  clearArgs();
}

void CairoPainter::opg(bool stroke)
{
  if (iArgs.size() != 1 || !iArgs[0]->number())
    return;
  double gr = iArgs[0]->number()->value();
  if (stroke)
    iStrokeRgb[0] = iStrokeRgb[1] = iStrokeRgb[2] = gr;
  else
    iFillRgb[0] = iFillRgb[1] = iFillRgb[2] = gr;
}


void CairoPainter::oprg(bool stroke)
{
  if (iArgs.size() != 3 || !iArgs[0]->number() || !iArgs[1]->number()
      || !iArgs[2]->number())
    return;
  double *col = (stroke ? iStrokeRgb : iFillRgb);
  for (int i = 0; i < 3; ++i)
    col[i] = iArgs[i]->number()->value();
  // Dimcolor?
}

void CairoPainter::opk(bool stroke)
{
  if (iArgs.size() != 4 || !iArgs[0]->number() || !iArgs[1]->number()
      || !iArgs[2]->number() || !iArgs[3]->number())
    return;
  // ignore values, but use text object stroke color
  double *col = (stroke ? iStrokeRgb : iFillRgb);
  for (int i = 0; i < 3; ++i)
    col[i] = iTextRgb[i];
}

void CairoPainter::opcm()
{
  if (iArgs.size() != 6)
    return;
  Matrix m;
  for (int i = 0; i < 6; ++i) {
    if (!iArgs[i]->number())
      return;
    m.a[i] = iArgs[i]->number()->value();
  }
  transform(m);
}

void CairoPainter::opw()
{
  if (iArgs.size() != 1 || !iArgs[0]->number())
    return;
  iLineWid = iArgs[0]->number()->value();

}

void CairoPainter::opq()
{
  if (iArgs.size() != 0)
    return;
  push();
  pushMatrix();
}

void CairoPainter::opQ()
{
  if (iArgs.size() != 0)
    return;
  popMatrix();
  pop();
}

void CairoPainter::opm()
{
  if (iArgs.size() != 2 || !iArgs[0]->number() || !iArgs[1]->number())
    return;
  Vector t(iArgs[0]->number()->value(), iArgs[1]->number()->value());
  iMoveTo = t;
}

void CairoPainter::opl()
{
  if (iArgs.size() != 2 || !iArgs[0]->number() || !iArgs[1]->number())
    return;
  Vector t(iArgs[0]->number()->value(), iArgs[1]->number()->value());

  cairo_set_source_rgb(iCairo, iStrokeRgb[0], iStrokeRgb[1], iStrokeRgb[2]);
  cairo_set_line_width(iCairo, iLineWid);
  Vector u0 = matrix() * iMoveTo;
  Vector u1 = matrix() * t;
  cairo_move_to(iCairo, u0.x, u0.y);
  cairo_line_to(iCairo, u1.x, u1.y);
  cairo_stroke(iCairo);
}

void CairoPainter::opsym()
{
  if (iArgs.size() != 5) return;
  String s[5];
  for (int i = 0; i < 5; ++i) {
    if (!iArgs[i]->string())
      return;
    s[i] = iArgs[i]->string()->value();
  }
  // ipeDebug("render Symbol %s, %s, %s, %s, %s",
  // s[0].z(), s[1].z(), s[2].z(), s[3].z(), s[4].z());
  const Symbol *symbol = cascade()->findSymbol(Attribute(true, s[0]));
  if (symbol) {
    pushMatrix();
    push();
    untransform(symbol->iTransformations);
    double sa = Lex(s[1]).getDouble();
    Matrix m(sa, 0, 0, sa, 0, 0);
    transform(m);
    setSymStroke(Attribute::makeColor(s[2], Attribute::BLACK()));
    setSymFill(Attribute::makeColor(s[3], Attribute::WHITE()));
    setSymPen(Attribute::makeScalar(s[4], Attribute::NORMAL()));
    symbol->iObject->draw(*this);
    pop();
    popMatrix();
  }
}

void CairoPainter::opre()
{
  if (iArgs.size() != 4 || !iArgs[0]->number() || !iArgs[1]->number()
      || !iArgs[2]->number() || !iArgs[3]->number())
    return;
  Vector t(iArgs[0]->number()->value(), iArgs[1]->number()->value());
  Vector wh(iArgs[2]->number()->value(), iArgs[3]->number()->value());

  cairo_set_source_rgb(iCairo, iFillRgb[0], iFillRgb[1], iFillRgb[2]);
  Vector t1 = matrix() * t;
  Vector wh1 = matrix().linear() * wh;
  cairo_rectangle(iCairo, t1.x, t1.y, wh1.x, wh1.y);
  cairo_fill(iCairo);
}

void CairoPainter::opBT()
{
  iFont = 0;
  iTextPos = iTextLinePos = Vector::ZERO;
}

void CairoPainter::opTf()
{
  if (iArgs.size() != 2 || !iArgs[0]->name() || !iArgs[1]->number())
    return;
  String name = iArgs[0]->name()->value();
  iFontSize = iArgs[1]->number()->value();
  if (name[0] != 'F')
    return;
  int font = Lex(name.substr(1)).getInt();
  iFont = iFonts->getFace(font);
  iFontMatrix = matrix().linear() * Linear(iFontSize, 0, 0, -iFontSize);
}

void CairoPainter::opTd()
{
  if (iArgs.size() != 2 || !iArgs[0]->number() || !iArgs[1]->number())
    return;
  Vector t(iArgs[0]->number()->value(), iArgs[1]->number()->value());
  iTextPos = iTextLinePos = iTextLinePos + t;
}

void CairoPainter::opTJ()
{
  if (!iFont || iArgs.size() != 1 || !iArgs[0]->array())
    return;
  std::vector<cairo_glyph_t> glyphs;
  for (int i = 0; i < iArgs[0]->array()->count(); ++i) {
    const PdfObj *obj = iArgs[0]->array()->obj(i, 0);
    if (obj->number()) {
      iTextPos.x -= 0.001 * iFontSize * obj->number()->value();
    } else if (obj->string()) {
      String s = obj->string()->value();
      for (int j = 0; j < s.size(); ++j) {
	uchar ch = s[j];
	Vector pt = matrix() * iTextPos;
	cairo_glyph_t g;
	g.index = iFont->getGlyph(ch);
	g.x = pt.x;
	g.y = pt.y;
	glyphs.push_back(g);
	iTextPos.x += 0.001 * iFontSize * iFont->width(ch);
      }
    }
  }
  drawGlyphs(glyphs);
}

// --------------------------------------------------------------------

//! Draw a glyph.
/*! Glyph is drawn with hotspot at position pos. */
void CairoPainter::drawGlyphs(std::vector<cairo_glyph_t> &glyphs)
{
  if (!iFont)
    return;

  cairo_matrix_t matrix;
  matrix.xx = iFontMatrix.a[0];
  matrix.yx = iFontMatrix.a[1];
  matrix.xy = iFontMatrix.a[2];
  matrix.yy = iFontMatrix.a[3];
  matrix.x0 = 0.0;
  matrix.y0 = 0.0;

  cairo_save(iCairo);
  cairo_set_font_face(iCairo, iFont->cairoFont());
  cairo_set_font_matrix(iCairo, &matrix);
  cairo_set_source_rgba(iCairo, iFillRgb[0], iFillRgb[1], iFillRgb[2],
			opacity().toDouble());
  cairo_show_glyphs(iCairo, &glyphs[0], glyphs.size());
  cairo_restore(iCairo);
}

// --------------------------------------------------------------------
