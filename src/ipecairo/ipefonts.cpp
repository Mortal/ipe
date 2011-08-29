// --------------------------------------------------------------------
// Rendering fonts onto the canvas
// --------------------------------------------------------------------
/*

    This file is part of the extensible drawing editor Ipe.
    Copyright (C) 1993-2011  Otfried Cheong

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

#include "ipefonts.h"
#include "ipefontpool.h"
#include "ipepdfparser.h"
#include "ipexml.h"

#include <ft2build.h>
#include FT_FREETYPE_H

#include <cairo-ft.h>

#ifdef WIN32XX
extern "C" cairo_font_face_t *
_cairo_font_face_twin_create(cairo_font_slant_t slant,
			     cairo_font_weight_t weight);
#endif

using namespace ipe;

extern bool getStdFontWidths(String fontName,
			     const String encoding[0x100],
			     int width[0x100]);

// --------------------------------------------------------------------

static const char * const stdFontMap[] = {
  "Times-Roman",
  "Times-Italic",
  "Times-Bold",
  "Times-BoldItalic",
  "Helvetica",
  "Helvetica-Oblique",
  "Helvetica-Bold",
  "Helvetica-BoldOblique",
  "Courier",
  "Courier-Oblique",
  "Courier-Bold",
  "Courier-BoldOblique",
  "Symbol",
  "ZapfDingbats",
};

static const char * const stdFontMapAlias[] = {
  "NimbusRomNo9L-Regu",
  "NimbusRomNo9L-ReguItal",
  "NimbusRomNo9L-Medi",
  "NimbusRomNo9L-MediItal",
  "NimbusSanL-Regu",
  "NimbusSanL-ReguItal",
  "NimbusSanL-Bold",
  "NimbusSanL-BoldItal",
  "NimbusMonL-Regu",
  "NimbusMonL-ReguObli",
  "NimbusMonL-Bold",
  "NimbusMonL-BoldObli",
  "StandardSymL",
  "Dingbats",
};

// --------------------------------------------------------------------

class FontmapParser : public XmlParser {
public:
  explicit FontmapParser(DataSource &source) :
    XmlParser(source) { /* nothing */ }
  bool parseFontmap();

  String iGlyphs[14];
};

// Parse a fontmap.
bool FontmapParser::parseFontmap()
{
  XmlAttributes att;
  String tag = parseToTag();

  if (tag == "?xml") {
    if (!parseAttributes(att, true))
      return false;
    tag = parseToTag();
  }

  if (tag != "fontmap")
    return false;
  if (!parseAttributes(att))
    return false;
  tag = parseToTag();

  while (tag == "font") {
    if (!parseAttributes(att))
      return false;
    if (att["format"] == "type1") {
      String name = att["name"];
      String glyphs = att["glyphs"];
      int i = 0;
      while (i < 14 && name != String(stdFontMap[i])
	     && name != String(stdFontMapAlias[i]))
	++i;
      if (i < 14)
	iGlyphs[i] = glyphs;
    }
    tag = parseToTag();
  }

  if (tag != "/fontmap")
    return false;
  return true;
}


// --------------------------------------------------------------------

struct Engine {
public:
  Engine();
  ~Engine();
  Buffer standardFont(String name);
  cairo_font_face_t *screenFont();

private:
  void findStandardFonts();
  bool iFontMapLoaded;
  bool iScreenFontLoaded;
  String iStandardFont[14];
  Buffer iStandardFontBuffer[14];
  cairo_font_face_t *iScreenFont;

public:
  bool iOk;
  FT_Library iLib;
  int iFacesLoaded;
  int iFacesUnloaded;
  int iFacesDiscarded;
};

// Auto-constructed and destructed Freetype engine.
static Engine engine;

// --------------------------------------------------------------------

Engine::Engine()
{
  iOk = false;
  iFontMapLoaded = false;
  iScreenFont = 0;
  iScreenFontLoaded = false;
  iFacesLoaded = 0;
  iFacesUnloaded = 0;
  iFacesDiscarded = 0;
  if (FT_Init_FreeType(&iLib))
    return;
  iOk = true;
}

Engine::~Engine()
{
  ipeDebug("Freetype engine: %d faces loaded, %d faces unloaded, "
	   "%d faces discarded",
	   iFacesLoaded, iFacesUnloaded, iFacesDiscarded);
  if (iScreenFont)
    cairo_font_face_destroy(iScreenFont);
  if (iOk)
    FT_Done_FreeType(iLib);
  // causes an assert in cairo to fail:
  // cairo_debug_reset_static_data();
  ipeDebug("Freetype engine: %d faces discarded", iFacesDiscarded);
}

// --------------------------------------------------------------------

//! Parse the fontmap and store absolute paths for standard fonts.
void Engine::findStandardFonts()
{
  iFontMapLoaded = true;
  String fm = Platform::fontmapFile();
  int i = fm.rfind(Platform::pathSeparator());
  String fmb = fm.left(i+1);
  std::FILE *file = std::fopen(fm.z(), "rb");
  if (!file) {
    ipeDebug("Could not open fontmap '%s'", fm.z());
    return;
  }

  FileSource source(file);
  FontmapParser parser(source);
  if (parser.parseFontmap()) {
    for (int i = 0; i < 14; ++i) {
      if (!parser.iGlyphs[i].empty()) {
	String fname = parser.iGlyphs[i];
	// make relative paths based on fmb
#ifdef __MINGW32__
	if (fname[0] != '\\' && fname[1] != ':')
	  fname = fmb + fname;
#else
	if (fname[0] != '/')
	  fname = fmb + fname;
#endif
	if (Platform::fileExists(fname))
	  iStandardFont[i] = fname;
      }
    }
  } else
    ipeDebug("Could not parse fontmap.");
  std::fclose(file);

  for (int i = 0; i < 14; ++i) {
    if (iStandardFont[i].empty()) {
      ipeDebug("No font file found for '%s'", stdFontMap[i]);
    } else {
      ipeDebug("Standard font '%s' uses font file '%s",
	       stdFontMap[i], iStandardFont[i].z());
    }
  }
}

//! Return contents of font file for standard font \a name.
Buffer Engine::standardFont(String name)
{
  if (!iFontMapLoaded)
    findStandardFonts();
  for (int i = 0; i < 14; ++i) {
    if (name == stdFontMap[i]) {
      // if it's known and not yet loaded, load it
      if (!iStandardFont[i].empty() && iStandardFontBuffer[i].size() == 0) {
	String s = Platform::readFile(iStandardFont[i]);
	iStandardFontBuffer[i] = Buffer(s.data(), s.size());
      }
      return iStandardFontBuffer[i];
    }
  }
  return Buffer();
}

cairo_font_face_t *Engine::screenFont()
{
  if (!iScreenFontLoaded) {
    iScreenFontLoaded = true;
#ifdef WIN32XX
    iScreenFont = _cairo_font_face_twin_create(CAIRO_FONT_SLANT_NORMAL,
					       CAIRO_FONT_WEIGHT_BOLD);
#else
    iScreenFont = cairo_toy_font_face_create("Sans", CAIRO_FONT_SLANT_NORMAL,
					     CAIRO_FONT_WEIGHT_BOLD);
    // Alternative (in Cairo 1.6):
    // FcPattern *pat = FcFontMatch(NULL, FcNameParse((FcChar8 *) "Sans"), 0);
    // iScreenFont = cairo_ft_font_face_create_for_pattern(pat);
#endif
  }
  return iScreenFont;
}

// --------------------------------------------------------------------

/*! \class ipe::Fonts
  \ingroup cairo
  \brief Provides the fonts used to render text.
*/

Fonts *Fonts::New(const FontPool *fontPool)
{
  if (!engine.iOk)
    return 0;
  return new Fonts(fontPool);
}

//! Private constructor
Fonts::Fonts(const FontPool *fontPool) : iFontPool(fontPool)
{
  // nothing
}

//! Delete all the loaded Faces.
Fonts::~Fonts()
{
  for (FaceSeq::iterator it = iFaces.begin(); it != iFaces.end(); ++it)
    delete *it;
}

String Fonts::freetypeVersion()
{
  int major, minor, patch;
  FT_Library_Version(engine.iLib, &major, &minor, &patch);
  char buf[128];
  sprintf(buf, "Freetype %d.%d.%d / %d.%d.%d",
	  FREETYPE_MAJOR, FREETYPE_MINOR, FREETYPE_PATCH,
	  major, minor, patch);
  ipeDebug("Using %s", buf);
  return String(buf);
}

//! Return a Cairo font to render to the screen w/o Latex font.
cairo_font_face_t *Fonts::screenFont()
{
  return engine.screenFont();
}

//! Get a typeface.
/*! Corresponds to a Freetype "face", or a PDF font resource.  A Face
  can be loaded at various sizes (transformations), resulting in
  individual FaceSize's. */
Face *Fonts::getFace(int id)
{
  for (FaceSeq::iterator it = iFaces.begin(); it != iFaces.end(); ++it) {
    if ((*it)->matches(id))
      return *it;
  }
  // need to create it
  std::vector<Font>::const_iterator it = iFontPool->begin();
  while (it != iFontPool->end() && it->iLatexNumber != id)
    ++it;
  if (it == iFontPool->end())
    return 0;
  Face *face = new Face(id, *it);
  iFaces.push_back(face);
  return face;
}

// --------------------------------------------------------------------

struct FaceData {
  Buffer iData;
  FT_Face iFace;
};

static void face_data_destroy(FaceData *face_data)
{
  ++engine.iFacesDiscarded;
  FT_Done_Face(face_data->iFace);  // discard Freetype face
  delete face_data;  // discard memory buffer
}

static const cairo_user_data_key_t key = { 0 };

/*! \class ipe::Face
  \ingroup cairo
  \brief A typeface (aka font), actually loaded (from a font file or PDF file).
*/

Face::Face(int id, const Font &font)
{
  ipeDebug("Loading face '%s' for id %d", font.iName.z(), id);
  iId = id;
  iType = font.iType;
  iCairoFont = 0;

  for (int i = 0; i < 0x100; ++i) {
    iGlyphIndex[i] = 0;
    iWidth[i] = 0;
  }

  Buffer data;
  FT_Face face;

  if (!font.iStandardFont) {
    for (int i = 0; i < 0x100; ++i)
      iWidth[i] = font.iWidth[i];
    data = font.iStreamData;
    if (FT_New_Memory_Face(engine.iLib,
			   (const uchar *) data.data(), data.size(), 0, &face))
      return;
  } else {
    if (!getStdFontWidths(font.iName, font.iEncoding, iWidth))
      return;
    data = engine.standardFont(font.iName);
    if (data.size() == 0 ||
	FT_New_Memory_Face(engine.iLib,
			   (const uchar *) data.data(), data.size(), 0, &face))
      return;
  }

  FaceData *face_data = new FaceData;
  face_data->iData = data;
  face_data->iFace = face;

  // see cairo_ft_font_face_create_for_ft_face docs
  iCairoFont = cairo_ft_font_face_create_for_ft_face(face, 0);
  cairo_status_t status =
    cairo_font_face_set_user_data(iCairoFont, &key, face_data,
				  (cairo_destroy_func_t) face_data_destroy);
  if (status) {
    ipeDebug("Failed to set user data for Cairo font");
    cairo_font_face_destroy(iCairoFont);
    FT_Done_Face(face);
    delete face_data;
    return;
  }
  ++engine.iFacesLoaded;

  if (iType == Font::EType1) {
    if (font.iHasEncoding) {
      for (int i = 0; i < 0x100; ++i)
	iGlyphIndex[i] =
	  FT_Get_Name_Index(face,
			    const_cast<char *>(font.iEncoding[i].z()));
    } else {
      for (int k = 0; k < face->num_charmaps; ++k) {
	if (face->charmaps[k]->encoding == FT_ENCODING_ADOBE_CUSTOM) {
	  FT_Set_Charmap(face, face->charmaps[k]);
	  break;
	}
      }
      // Otherwise need to check FT_Has_PS_Glyph_Names(iFace)?
      for (int i = 0; i < 0x100; ++i)
	iGlyphIndex[i] = FT_Get_Char_Index(face, i);
    }
  } else {
    // Truetype font, don't use /Encoding (shouldn't have one)
    FT_Set_Charmap(face, face->charmaps[0]);
    if (face->charmaps[0]->platform_id != 1 ||
	face->charmaps[0]->encoding_id != 0) {
      ipeDebug("TrueType face %d has strange first charmap (of %d)",
	       iId, face->num_charmaps);
      for (int i = 0; i < face->num_charmaps; ++i) {
	ipeDebug("Map %d has platform %d, encoding %d",
		 i, face->charmaps[i]->platform_id,
		 face->charmaps[i]->encoding_id);
      }
    }
  }
}

Face::~Face()
{
  ipeDebug("Done with Cairo face %d (%d references left)", iId,
	   cairo_font_face_get_reference_count(iCairoFont));
  if (iCairoFont) {
    ++engine.iFacesUnloaded;
    cairo_font_face_destroy(iCairoFont);
  }
}

int Face::getGlyph(int ch)
{
  FaceData *face_data = (FaceData *)
    cairo_font_face_get_user_data(iCairoFont, &key);
  if (!face_data->iFace)  // face is not available
    return 0;
  if (type() == Font::EType1)
    return iGlyphIndex[ch];
  else
    return FT_Get_Char_Index(face_data->iFace, (FT_ULong) ch);
}

// --------------------------------------------------------------------

