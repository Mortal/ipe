// --------------------------------------------------------------------
// AppUi
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

#include "appui.h"
#include "tools.h"

#include "ipelua.h"

#include "ipecairopainter.h"

using namespace ipe;
using namespace ipelua;

// --------------------------------------------------------------------

const char * const AppUiBase::selectorNames[] =
  { "stroke", "fill", "pen", "textsize", "markshape",
    "symbolsize", "gridsize", "anglesize", "view", "page",
    "viewmarked", "pagemarked" };

AppUiBase::AppUiBase(lua_State *L0, int model)
{
  L = L0;
  iModel = model;

  iMouseIn = 0; // points
  iCoordinatesFormat = "%g%s, %g%s";
  lua_getglobal(L, "prefs");
  lua_getfield(L, -1, "coordinates_format");
  if (lua_isstring(L, -1)) {
    iCoordinatesFormat = lua_tostring(L, -1);
  }
  lua_pop(L, 2);
}

AppUiBase::~AppUiBase()
{
  ipeDebug("AppUiBase C++ destructor");
  luaL_unref(L, LUA_REGISTRYINDEX, iModel);
  // collect this model
  lua_gc(L, LUA_GCCOLLECT, 0);
}

// --------------------------------------------------------------------

void AppUiBase::buildMenus()
{
  addRootMenu(EFileMenu, "&File");
  addRootMenu(EEditMenu, "&Edit");
  addRootMenu(EPropertiesMenu, "P&roperties");
  addRootMenu(ESnapMenu, "&Snap");
  addRootMenu(EModeMenu, "&Mode");
  addRootMenu(EZoomMenu, "&Zoom");
  addRootMenu(ELayerMenu, "&Layers");
  addRootMenu(EViewMenu, "&Views");
  addRootMenu(EPageMenu, "&Pages");
  addRootMenu(EIpeletMenu, "&Ipelets");
  addRootMenu(EHelpMenu, "&Help");

  addItem(EFileMenu, "New Window", "new_window");
  addItem(EFileMenu, "New", "new");
  addItem(EFileMenu, "Open", "open");
  addItem(EFileMenu, "Save", "save");
  addItem(EFileMenu, "Save as", "save_as");
  addItem(EFileMenu);
  addItem(EFileMenu, "Automatically run Latex", "*auto_latex");
  addItem(EFileMenu, "Run Latex", "run_latex");
  addItem(EFileMenu);
  addItem(EFileMenu, "Close", "close");
  addItem(EFileMenu, "Exit", "quit");

  addItem(EEditMenu, "Undo", "undo");
  addItem(EEditMenu, "Redo", "redo");
  addItem(EEditMenu);
  addItem(EEditMenu, "Cut", "cut");
  addItem(EEditMenu, "Copy", "copy");
  addItem(EEditMenu, "Paste", "paste");
  addItem(EEditMenu, "Paste at cursor", "paste_at_cursor");
  addItem(EEditMenu, "Delete", "delete");
  addItem(EEditMenu);
  addItem(EEditMenu, "Group", "group");
  addItem(EEditMenu, "Ungroup", "ungroup");
  addItem(EEditMenu, "Front", "front");
  addItem(EEditMenu, "Back", "back");
  addItem(EEditMenu, "Forward", "forward");
  addItem(EEditMenu, "Backward", "backward");
  addItem(EEditMenu, "Just before", "before");
  addItem(EEditMenu, "Just behind", "behind");
  addItem(EEditMenu, "Duplicate", "duplicate");
  addItem(EEditMenu, "Select all", "select_all");
  addItem(EEditMenu);
  addItem(EEditMenu, "Insert text box", "insert_text_box");
  addItem(EEditMenu, "Edit object", "edit");
  addItem(EEditMenu, "Edit object as XML", "edit_as_xml");
  addItem(EEditMenu, "Change text width", "change_width");
  addItem(EEditMenu);
  addItem(EEditMenu, "Document properties", "document_properties");
  addItem(EEditMenu, "Style sheets", "style_sheets");
  addItem(EEditMenu, "Update style sheets", "update_style_sheets");
  addItem(EEditMenu, "Check symbolic attributes", "check_style");

  startSubMenu(EPropertiesMenu, "Pinned");
  addSubItem("none", "pinned|none");
  addSubItem("horizontal", "pinned|horizontal");
  addSubItem("vertical", "pinned|vertical");
  addSubItem("fixed", "pinned|fixed");
  endSubMenu();

  startSubMenu(EPropertiesMenu, "Transformations");
  addSubItem("translations", "transformations|translations");
  addSubItem("rigid motions", "transformations|rigid");
  addSubItem("affine", "transformations|affine");
  endSubMenu();

  addItem(EPropertiesMenu);

  startSubMenu(EPropertiesMenu, "Text style");
  iTextStyleMenu = endSubMenu();

  startSubMenu(EPropertiesMenu, "Horizontal alignment");
  addSubItem("left", "horizontalalignment|left");
  addSubItem("center", "horizontalalignment|hcenter");
  addSubItem("right", "horizontalalignment|right");
  endSubMenu();

  startSubMenu(EPropertiesMenu, "Vertical alignment");
  addSubItem("bottom", "verticalalignment|bottom");
  addSubItem("baseline", "verticalalignment|baseline");
  addSubItem("center", "verticalalignment|vcenter");
  addSubItem("top", "verticalalignment|top");
  endSubMenu();

  startSubMenu(EPropertiesMenu, "Transformable text");
  addSubItem("Yes", "transformabletext|true");
  addSubItem("No", "transformabletext|false");
  endSubMenu();

  addItem(EModeMenu, "Select objects (with Shift: non-destructive)",
	  "mode_select");
  addItem(EModeMenu, "Translate objects (with Shift: horizontal/vertical)",
	  "mode_translate");
  addItem(EModeMenu, "Rotate objects", "mode_rotate");
  addItem(EModeMenu, "Stretch objects (with Shift: scale objects)",
	  "mode_stretch");
  addItem(EModeMenu, "Pan the canvas", "mode_pan");
  addItem(EModeMenu, "Shred objects", "mode_shredder");
  addItem(EModeMenu);
  // TODO iObjectTools->addSeparator();
  addItem(EModeMenu, "Text labels", "mode_label");
  addItem(EModeMenu, "Mathematical symbols", "mode_math");
  addItem(EModeMenu, "Paragraphs", "mode_paragraph");
  addItem(EModeMenu, "Marks", "mode_marks");
  addItem(EModeMenu, "Rectangles (with Shift: squares)", "mode_rectangles");
  addItem(EModeMenu, "Lines and polylines", "mode_lines");
  addItem(EModeMenu, "Polygons", "mode_polygons");
  addItem(EModeMenu, "Splines", "mode_splines");
  addItem(EModeMenu, "Splinegons", "mode_splinegons");
  addItem(EModeMenu, "Circular arcs (by center, right and left point)",
    "mode_arc1");
  addItem(EModeMenu, "Circular arcs (by center, left and right point)",
    "mode_arc2");
  addItem(EModeMenu, "Circular arcs (by 3 points)", "mode_arc3");
  addItem(EModeMenu, "Circles (by center and radius)", "mode_circle1");
  addItem(EModeMenu, "Circles (by diameter)", "mode_circle2");
  addItem(EModeMenu, "Circles (by 3 points)", "mode_circle3");
  addItem(EModeMenu, "Ink", "mode_ink");

  addItem(ESnapMenu, "Snap to vertex", "@snapvtx");
  addItem(ESnapMenu, "Snap to boundary", "@snapbd");
  addItem(ESnapMenu, "Snap to intersection", "@snapint");
  addItem(ESnapMenu, "Snap to grid", "@snapgrid");
  addItem(ESnapMenu, "Angular snap", "@snapangle");
  addItem(ESnapMenu, "Automatic snap", "@snapauto");
  addItem(ESnapMenu);
  addItem(ESnapMenu, "Set origin && snap", "@set_origin_snap");
  addItem(ESnapMenu, "Hide axes", "@hide_axes");
  addItem(ESnapMenu, "Set direction", "@set_direction");
  addItem(ESnapMenu, "Reset direction", "@reset_direction");
  addItem(ESnapMenu, "Set line", "@set_line");
  addItem(ESnapMenu, "Set line && snap", "@set_line_snap");

#ifndef IPEUI_WIN32
  addItem(EZoomMenu, "Fullscreen", "@*fullscreen");
#endif
  addItem(EZoomMenu, "Grid visible", "@*grid_visible");

  startSubMenu(EZoomMenu, "Coordinates");
  addSubItem("points", "@coordinates|points");
  addSubItem("mm", "@coordinates|mm");
  addSubItem("inch", "@coordinates|inch");
  endSubMenu();

  addItem(EZoomMenu);
  addItem(EZoomMenu, "Zoom in", "@zoom_in");
  addItem(EZoomMenu, "Zoom out", "@zoom_out");
  addItem(EZoomMenu, "Normal size", "@normal_size");
  addItem(EZoomMenu, "Fit page", "@fit_page");
  addItem(EZoomMenu, "Fit width", "@fit_width");
  addItem(EZoomMenu, "Fit page top", "@fit_top");
  addItem(EZoomMenu, "Fit objects", "@fit_objects");
  addItem(EZoomMenu, "Fit selection", "@fit_selection");
  addItem(EZoomMenu);
  addItem(EZoomMenu, "Pan here", "@pan_here");

  addItem(ELayerMenu, "New layer", "new_layer");
  addItem(ELayerMenu, "Rename active layer", "rename_active_layer");
  addItem(ELayerMenu);
  addItem(ELayerMenu, "Select all in active  layer",
    "select_in_active_layer");
  startSubMenu(ELayerMenu, "Select all in layer");
  iSelectLayerMenu = endSubMenu();
  addItem(ELayerMenu, "Move to active layer", "move_to_active_layer");
  startSubMenu(ELayerMenu, "Move to layer");
  iMoveToLayerMenu = endSubMenu();

  addItem(EViewMenu, "Next view", "next_view");
  addItem(EViewMenu, "Previous view", "previous_view");
  addItem(EViewMenu, "First view", "first_view");
  addItem(EViewMenu, "Last view", "last_view");
  addItem(EViewMenu);
  addItem(EViewMenu, "New layer, new view", "new_layer_view");
  addItem(EViewMenu, "New view", "new_view");
  addItem(EViewMenu, "Delete view", "delete_view");
  addItem(EViewMenu);
  addItem(EViewMenu, "Jump to view", "jump_view");
  addItem(EViewMenu, "Edit effects", "edit_effects");

  addItem(EPageMenu, "Next page", "next_page");
  addItem(EPageMenu, "Previous page", "previous_page");
  addItem(EPageMenu, "First page", "first_page");
  addItem(EPageMenu, "Last page", "last_page");
  addItem(EPageMenu);
  addItem(EPageMenu, "New page", "new_page");
  addItem(EPageMenu, "Cut page", "cut_page");
  addItem(EPageMenu, "Copy page", "copy_page");
  addItem(EPageMenu, "Paste page", "paste_page");
  addItem(EPageMenu, "Delete page", "delete_page");
  addItem(EPageMenu);
  addItem(EPageMenu, "Jump to page", "jump_page");
  addItem(EPageMenu, "Edit title && sections", "edit_title");
  addItem(EPageMenu, "Edit notes", "edit_notes");
  addItem(EPageMenu, "Page sorter", "page_sorter");
  addItem(EPageMenu);
#ifdef IPEUI_WIN32
  addItem(EPageMenu, "Notes", "toggle_notes");
  addItem(EPageMenu, "Bookmarks", "toggle_bookmarks");
#endif

  addItem(EHelpMenu, "Ipe &manual", "manual");
  addItem(EHelpMenu, "Onscreen keyboard", "@keyboard");
  addItem(EHelpMenu, "Show &configuration", "show_configuration");
  addItem(EHelpMenu, "About the &ipelets", "about_ipelets");
  addItem(EHelpMenu, "&About Ipe", "about");

  // build Ipelet menu
  lua_getglobal(L, "ipelets");
  int n = lua_objlen(L, -1);
  for (int i = 1; i <= n; ++i) {
    lua_rawgeti(L, -1, i);
    lua_getfield(L, -1, "label");
    String label(lua_tostring(L, -1));
    lua_pop(L, 1);
    lua_getfield(L, -1, "name");
    String name(lua_tostring(L, -1));
    lua_pop(L, 1);
    lua_getfield(L, -1, "methods");
    char buf[20];
    if (lua_isnil(L, -1)) {
      String action("ipelet_1_");
      action += name;
      addItem(EIpeletMenu, label.z(), action.z());
    } else {
      int m = lua_objlen(L, -1);
      startSubMenu(EIpeletMenu, label.z());
      for (int j = 1; j <= m; ++j) {
	lua_rawgeti(L, -1, j);
	lua_getfield(L, -1, "label");
	sprintf(buf, "ipelet_%d_", j);
	String action(buf);
	action += name;
	addSubItem(lua_tostring(L, -1), action.z());
	lua_pop(L, 2); // sublabel, method
      }
      endSubMenu();
    }
    lua_pop(L, 2); // methods, ipelet
  }
  lua_pop(L, 1);
}

// --------------------------------------------------------------------

void AppUiBase::canvasObserverWheelMoved(int degrees)
{
  if (degrees > 0)
    action("wheel_zoom_in");
  else
    action("wheel_zoom_out");
}

void AppUiBase::canvasObserverToolChanged(bool hasTool)
{
  setActionsEnabled(!hasTool);
}

static void adjust(double &x, int mode)
{
  if (ipe::abs(x) < 1e-12)
    x = 0.0;
  switch (mode) {
  case 1: // mm
    x = (x / 72.0) * 25.4;
    break;
  case 2: // in
    x /= 72;
    break;
  default:
    break;
  }
}

static const char * const mouse_units[] = { "", " mm", " in" };

void AppUiBase::canvasObserverPositionChanged()
{
  Vector v = iCanvas->CanvasBase::pos();
  const Snap &snap = iCanvas->snap();
  if (snap.iWithAxes) {
    v = v - snap.iOrigin;
    v = Linear(-snap.iDir) * v;
  }
  adjust(v.x, iMouseIn);
  adjust(v.y, iMouseIn);
  const char *units = mouse_units[iMouseIn];
  char s[32];
  sprintf(s, iCoordinatesFormat.z(), v.x, units, v.y, units);
  /* TODO: if tool active, show pos relative to origin
  if (!iFileTools->isEnabled()) {
    IpeVector u = pos - iMouseBase;
    if (iSnapData.iWithAxes)
      u = IpeLinear(-iSnapData.iDir) * u;
      Adjust(u.iX, iMouseIn);
      Adjust(u.iY, iMouseIn);
      QString r;
      r.sprintf(" (%+g%s,%+g%s)", u.iX, units, u.iY, units);
      s += r;
    }
  */
  setMouseIndicator(s);
}

void AppUiBase::canvasObserverMouseAction(int button)
{
  lua_rawgeti(L, LUA_REGISTRYINDEX, iModel);
  lua_getfield(L, -1, "mouseButtonAction");
  lua_insert(L, -2); // model
  push_button(L, button);
  lua_call(L, 3, 0);
}

void AppUiBase::canvasObserverSizeChanged()
{
  lua_rawgeti(L, LUA_REGISTRYINDEX, iModel);
  lua_getfield(L, -1, "sizeChanged");
  lua_insert(L, -2); // model
  lua_call(L, 1, 0);
}

// --------------------------------------------------------------------

static void call_selector(lua_State *L, int model, String name)
{
  // calls model selector
  lua_rawgeti(L, LUA_REGISTRYINDEX, model);
  lua_getfield(L, -1, "selector");
  lua_pushvalue(L, -2); // model
  lua_remove(L, -3);
  push_string(L, name);
}

void AppUiBase::luaSelector(String name, String value)
{
  call_selector(L, iModel, name);
  if (value == "true")
    lua_pushboolean(L, true);
  else if (value == "false")
    lua_pushboolean(L, false);
  else
    push_string(L, value);
  lua_call(L, 3, 0);
}

void AppUiBase::luaAbsoluteButton(const char *s)
{
  // calls model selector
  lua_rawgeti(L, LUA_REGISTRYINDEX, iModel);
  lua_getfield(L, -1, "absoluteButton");
  lua_insert(L, -2); // method, model
  lua_pushstring(L, s);
  lua_call(L, 2, 0);
}

// --------------------------------------------------------------------

void AppUiBase::luaAction(String name)
{
  if (name.left(12) == "coordinates|") {
    if (name.right(2) == "mm")
      iMouseIn = 1;
    else if (name.right(4) == "inch")
      iMouseIn = 2;
    else
      iMouseIn = 0;
  } else if (name.find('|') >= 0) {
    // calls model selector
    int i = name.find('|');
    luaSelector(name.left(i), name.substr(i+1));
  } else {
    // calls model action
    lua_rawgeti(L, LUA_REGISTRYINDEX, iModel);
    lua_getfield(L, -1, "action");
    lua_insert(L, -2); // before model
    push_string(L, name);
    lua_call(L, 2, 0);
  }
}

void AppUiBase::luaShowPathStylePopup(Vector v)
{
  lua_rawgeti(L, LUA_REGISTRYINDEX, iModel);
  lua_getfield(L, -1, "showPathStylePopup");
  lua_insert(L, -2); // before model
  push_vector(L, v);
  lua_call(L, 2, 0);
}

void AppUiBase::luaShowLayerBoxPopup(Vector v, String layer)
{
  lua_rawgeti(L, LUA_REGISTRYINDEX, iModel);
  lua_getfield(L, -1, "showLayerBoxPopup");
  lua_insert(L, -2); // before model
  push_vector(L, v);
  push_string(L, layer);
  lua_call(L, 3, 0);
}

void AppUiBase::luaLayerAction(String name, String layer)
{
  lua_rawgeti(L, LUA_REGISTRYINDEX, iModel);
  lua_getfield(L, -1, "layerAction");
  lua_insert(L, -2); // before model
  push_string(L, name);
  push_string(L, layer);
  lua_call(L, 3, 0);
}

void AppUiBase::luaBookmarkSelected(int index)
{
  lua_rawgeti(L, LUA_REGISTRYINDEX, iModel);
  lua_getfield(L, -1, "bookmark");
  lua_insert(L, -2); // method, model
  lua_pushnumber(L, index + 1);
  lua_call(L, 2, 0);
}

// --------------------------------------------------------------------

static String stripMark(Attribute mark)
{
  String s = mark.string();
  if (s.left(5) == "mark/") {
    int i = s.rfind('(');
    return s.substr(5, i > 0 ? i-5 : -1);
  } else
    return String();
}

void AppUiBase::showInCombo(const Cascade *sheet, Kind kind,
			    int sel, const char *deflt)
{
  AttributeSeq seq;
  sheet->allNames(kind, seq);
  if (!seq.size() && deflt != 0) {
    addCombo(sel, deflt);
    iComboContents[sel].push_back(deflt);
  }
  for (uint i = 0; i < seq.size(); ++i) {
    String s = seq[i].string();
    addCombo(sel, s);
    iComboContents[sel].push_back(s);
  }
}

void AppUiBase::showMarksInCombo(const Cascade *sheet)
{
  AttributeSeq seq;
  sheet->allNames(ESymbol, seq);
  for (uint i = 0; i < seq.size(); ++i) {
    String s = stripMark(seq[i]);
    if (!s.isEmpty()) {
      addCombo(EUiMarkShape, s);
      iComboContents[EUiMarkShape].push_back(s);
    }
  }
}

void AppUiBase::setupSymbolicNames(const Cascade *sheet)
{
  resetCombos();
  for (int i = 0; i < EUiView; ++i)
    iComboContents[i].clear();
  AttributeSeq seq;
  sheet->allNames(EColor, seq);
  for (uint i = 0; i < seq.size(); ++i) {
    Attribute abs = sheet->find(EColor, seq[i]);
    Color color = abs.color();
    String s = seq[i].string();
    addComboColor(EUiStroke, s, color);
    addComboColor(EUiFill, s, color);
    iComboContents[EUiStroke].push_back(s);
    iComboContents[EUiFill].push_back(s);
  }
  showInCombo(sheet, EPen, EUiPen);
  showInCombo(sheet, ETextSize, EUiTextSize);
  showInCombo(sheet, ESymbolSize, EUiSymbolSize);
  showMarksInCombo(sheet);
  showInCombo(sheet, EGridSize, EUiGridSize, "16pt");
  showInCombo(sheet, EAngleSize, EUiAngleSize, "45 deg");
}

void AppUiBase::setGridAngleSize(Attribute abs_grid, Attribute abs_angle)
{
  AttributeSeq seq;
  iCascade->allNames(EGridSize, seq);
  for (uint i = 0; i < seq.size(); ++i) {
    if (iCascade->find(EGridSize, seq[i]) == abs_grid) {
      setComboCurrent(EUiGridSize, i);
      break;
    }
  }
  seq.clear();
  iCascade->allNames(EAngleSize, seq);
  for (uint i = 0; i < seq.size(); ++i) {
    if (iCascade->find(EAngleSize, seq[i]) == abs_angle) {
      setComboCurrent(EUiAngleSize, i);
      break;
    }
  }
}

// --------------------------------------------------------------------

void AppUiBase::setAttribute(int sel, Attribute a)
{
  String s = a.string();
  for (int i = 0; i < int(iComboContents[sel].size()); ++i) {
    if (iComboContents[sel][i] == s) {
      setComboCurrent(sel, i);
      return;
    }
  }
}

void AppUiBase::setAttributes(const AllAttributes &all, Cascade *sheet)
{
  iAll = all;
  iCascade = sheet;

  setPathView(all, sheet);

  setAttribute(EUiStroke, iAll.iStroke);
  setAttribute(EUiFill, iAll.iFill);
  Color stroke = iCascade->find(EColor, iAll.iStroke).color();
  Color fill = iCascade->find(EColor, iAll.iFill).color();
  setButtonColor(EUiStroke, stroke);
  setButtonColor(EUiFill, fill);
  // iButton[]->setIcon(prefsColorIcon(stroke));
  setAttribute(EUiPen, iAll.iPen);
  setAttribute(EUiTextSize, iAll.iTextSize);
  setAttribute(EUiSymbolSize, iAll.iSymbolSize);

  String s = stripMark(iAll.iMarkShape);
  for (int i = 0; i < int(iComboContents[EUiMarkShape].size()); ++i) {
    if (iComboContents[EUiMarkShape][i] == s) {
      setComboCurrent(EUiMarkShape, i);
      break;
    }
  }

  setCheckMark("horizontalalignment", Attribute(iAll.iHorizontalAlignment));
  setCheckMark("verticalalignment", Attribute(iAll.iVerticalAlignment));
  setCheckMark("pinned", Attribute(iAll.iPinned));
  setCheckMark("transformabletext",
	       Attribute::Boolean(iAll.iTransformableText));
  setCheckMark("transformations", Attribute(iAll.iTransformations));
  setCheckMark("linejoin", Attribute(iAll.iLineJoin));
  setCheckMark("linecap", Attribute(iAll.iLineCap));
  setCheckMark("fillrule", Attribute(iAll.iFillRule));
}

// --------------------------------------------------------------------
