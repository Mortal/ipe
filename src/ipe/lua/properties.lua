----------------------------------------------------------------------
-- properties.lua
----------------------------------------------------------------------
--[[

    This file is part of the extensible drawing editor Ipe.
    Copyright (C) 1993-2012  Otfried Cheong

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

--]]

local function color_icon(sheets, name)
  local abs = sheets:find("color", name)
  if abs then
    return abs.r, abs.g, abs.b
  end
  return 0.0, 0.0, 0.0
end

function MODEL:propertiesPopup()
  self:updateCloseSelection(false)
  local sel = self:selection()
  if #sel > 1 then
    self:multiPopup(sel)
  elseif #sel == 1 then
    self:singlePopup(self:page():primarySelection())
  else
    self:emptyPopup()
  end
end

function MODEL:emptyPopup()
  local m = ipeui.Menu(self.ui:win())
  m:add("action_paste", "Paste")
  m:add("action_pan_here", "Pan here")
  m:add("grid", "Toggle grid")
  local gp = self.ui:globalPos()
  local item, num, value = m:execute(gp.x, gp.y)
  if item then
    if item:sub(1,7) == "action_" then
      self:action(item:sub(8))
    elseif item == "grid" then
      self:toggleGrid()
    end
  end
end

function MODEL:singlePopup(prim)
  local obj = self:page()[prim]
  local m = ipeui.Menu(self.ui:win())
  self["properties_" .. obj:type()](self, obj, m)
  m:add("action_cut", "Cut")
  m:add("action_copy", "Copy")
  m:add("action_paste", "Paste")
  m:add("action_pan_here", "Pan here")
  m:add("grid", "Toggle grid")
  local gp = self.ui:globalPos()
  local item, num, value = m:execute(gp.x, gp.y)
  if item then
    if item == "layer" then
      self:changeLayerOfPrimary(prim, value)
    elseif item:sub(1,7) == "action_" then
      self:action(item:sub(8))
    elseif item == "comment" then
      -- nothing to do
    elseif item == "grid" then
      self:toggleGrid()
    else
      self:setAttributeOfPrimary(prim, item, value)
    end
  end
end

--------------------------------------------------------------------

function MODEL:toggleGrid()
  local t = not self.ui:actionState("grid_visible");
  self.ui:setActionState("grid_visible", t)
  self:action_grid_visible()
end

function MODEL:changeLayerOfPrimary(prim, layer)
  local p = self:page()
  local t = { label="change layer to " .. layer,
	      pno=self.pno,
	      vno=self.vno,
	      primary=prim,
	      original=p:layerOf(prim),
	      toLayer=layer,
	    }
  t.undo = function (t, doc)
	     doc[t.pno]:setLayerOf(t.primary, t.original)
	   end
  t.redo = function (t, doc)
	     doc[t.pno]:setLayerOf(t.primary, t.toLayer)
	   end
  self:register(t)
end

function MODEL:setAttributeOfPrimary(prim, prop, value)
  self:deselectSecondary()
  local p = self:page()
  local t = { label="set attribute " .. prop .. " to " .. tostring(value),
	      pno=self.pno,
	      vno=self.vno,
	      primary=prim,
	      original=p[prim]:clone(),
	      property=prop,
	      value=value,
	      stroke=self.attributes.stroke,
	      fill=self.attributes.fill,
	    }
  t.undo = function (t, doc)
	     doc[t.pno]:replace(t.primary, t.original)
	   end
  t.redo = function (t, doc)
	     doc[t.pno]:setAttribute(t.primary, t.property, t.value,
				     t.stroke, t.fill)
	   end
  self:register(t)
end

----------------------------------------------------------------------

function MODEL:insertBasic(m, obj)
  local prim = self:page():primarySelection()
  local layer = self:page():layerOf(prim)
  local layers = self:page():layers()
  local pinned = { "none", "horizontal", "vertical", "fixed" }
  local transformations = { "translations", "rigid", "affine" }
  m:add("layer", "Layer: " .. layer, layers)
  m:add("pinned", "Pinned: " .. obj:get("pinned"), pinned)
  m:add("transformations", "Transformations: " .. obj:get("transformations"),
	transformations)
end

function MODEL:insertOpacity(m, obj)
  local opacity = self.doc:sheets():allNames("opacity")
  m:add("opacity", "Opacity: " .. obj:get("opacity"), opacity)
end

----------------------------------------------------------------------

function MODEL:properties_group(obj, m)
  m:add("comment", "Group object")
  local s = string.format("%d elements", obj:count())
  local clip = obj:clip()
  if clip then
    s = s .. ", with clipping"
  end
  m:add("comment", s)
  self:insertBasic(m, obj)
  if clip then
    m:add("action_remove_clipping", "Remove clipping")
    m:add("action_extract_clipping", "Extract clipping path")
  else
    m:add("action_add_clipping", "Add clipping path")
  end
  m:add("action_edit_as_xml", "Edit as XML")
  m:add("action_ungroup", "Ungroup")
end

function MODEL:properties_path(obj, m)
  local sheet = self.doc:sheets()
  local colors = sheet:allNames("color")
  local pathmodes = { "stroke only", "stroke && fill", "fill only" }
  local pathmode = { "stroked", "strokedfilled", "filled" }
  local pen = sheet:allNames("pen")
  local dashstyle = sheet:allNames("dashstyle")
  local arrowsizes = sheet:allNames("arrowsize")
  local arrowshapes = symbolNames(sheet, "arrow/", "(spx)")
  local tilings = sheet:allNames("tiling")
  table.insert(tilings, 1, "normal")
  local gradients = sheet:allNames("gradient")
  table.insert(gradients, 1, "normal")
  local linecap = { "normal", "butt", "round", "square", }
  local linejoin = { "normal", "miter", "round", "bevel", }
  local fillrule = { "normal", "wind", "evenodd", }

  local shape = obj:shape()

  m:add("comment", "Path object")
  m:add("comment", string.format("%d subpaths", #shape))
  self:insertBasic(m, obj)

  local pm = obj:get("pathmode")
  m:add("pathmode", "Stroke && Fill: " .. pathmodes[indexOf(pm, pathmode)],
	pathmode, pathmodes)
  if pm ~= "filled"  then
    m:add("stroke", "Stroke: " .. colorString(obj:get("stroke")), colors, nil,
	  function (i, name) return color_icon(self.doc:sheets(), name) end)
  end
  if pm ~= "stroked" then
    m:add("fill", "Fill: " .. colorString(obj:get("fill")), colors, nil,
	  function (i, name) return color_icon(self.doc:sheets(), name) end)
  end
  m:add("pen", "Pen: " .. obj:get("pen"), pen)
  m:add("dashstyle", "Dash style: " .. obj:get("dashstyle"), dashstyle)

  local farr = obj:get("farrow")
  local rarr = obj:get("rarrow")
  local boolnames = { "yes", "no" }
  local boolmap = { "true", "false" }
  local bool_to_yesno = { [false]="no", [true]="yes" }
  m:add("farrow", "Forward arrow: " .. bool_to_yesno[farr], boolmap, boolnames)
  if farr then
    m:add("farrowsize", "Forward arrow size", arrowsizes)
    m:add("farrowshape", "Forward arrow shape", arrowshapes, arrowshapeToName)
  end
  m:add("rarrow", "Reverse arrow: " .. bool_to_yesno[rarr], boolmap, boolnames)
  if rarr then
    m:add("rarrowsize", "Reverse arrow size", arrowsizes)
    m:add("rarrowshape", "Reverse arrow shape", arrowshapes, arrowshapeToName)
  end

  m:add("linecap", "Line cap: " .. obj:get("linecap"), linecap)
  m:add("linejoin", "Line join: " .. obj:get("linejoin"), linejoin)
  m:add("fillrule", "Fill rule: " .. obj:get("fillrule"), fillrule)

  if pm ~= "stroked" then
    m:add("tiling", "Tiling pattern: " .. obj:get("tiling"), tilings)
    m:add("gradient", "Gradient: " .. obj:get("gradient"), gradients)
  end
  self:insertOpacity(m, obj)

  if #shape > 1 then
    m:add("action_decompose", "Decompose path")
  end
  m:add("action_join", "Join paths")
  m:add("action_compose", "Compose paths")
  m:add("action_edit_as_xml", "Edit as XML")
  m:add("action_edit", "Edit path")
end

function MODEL:properties_text(obj, m)
  local sheet = self.doc:sheets()
  local minipage = obj:get("minipage")
  local stroke = sheet:allNames("color")
  local textsize = sheet:allNames("textsize")
  local textstyle = sheet:allNames("textstyle")
  m:add("comment", "Text object")
  self:insertBasic(m, obj)
  local t = minipage and "minipage" or "label"
  m:add("minipage", "Type: " .. t, {"true", "false"}, {"minipage", "label"})
  m:add("stroke", "Stroke: " .. colorString(obj:get("stroke")), stroke,
	nil,
	function (i, name) return color_icon(self.doc:sheets(), name) end)
  m:add("textsize", "Size: " .. obj:get("textsize"), textsize)
  if minipage then
    m:add("textstyle", "Style: " .. obj:get("textstyle"), textstyle)
  else
    m:add("horizontalalignment", "Horizontal alignment: " ..
	  obj:get("horizontalalignment"), {"left", "right", "hcenter"})
  end
  m:add("verticalalignment", "Vertical alignment: " ..
	obj:get("verticalalignment"), {"bottom", "baseline", "top", "vcenter"})

  self:insertOpacity(m, obj)

  if minipage then
    m:add("action_change_width", "Change width")
  end
  m:add("action_edit_as_xml", "Edit as XML")
  m:add("action_edit", "Edit text")
end

function MODEL:properties_image(obj, m)
  m:add("comment", "Image object")
  local info = obj:info()
  m:add("comment", string.format("%d x %d pixels",
				  info.width, info.height))
  m:add("comment", string.format("%d components with %d bits",
				  info.components, info.bitsPerComponent))
  m:add("comment", "Filter " .. info.filter)
  self:insertBasic(m, obj)
end

function MODEL:properties_reference(obj, m)
  local sheet = self.doc:sheets()
  local colors = sheet:allNames("color")
  local sizes = sheet:allNames("symbolsize")
  local pens = sheet:allNames("pen")
  m:add("comment", "Reference object")
  m:add("comment", "Symbol name: " .. obj:symbol())
  self:insertBasic(m, obj)
  m:add("stroke", "Stroke: " .. colorString(obj:get("stroke")), colors, nil,
  	function (i, name) return color_icon(self.doc:sheets(), name) end)
  m:add("fill", "Fill: " .. colorString(obj:get("fill")), colors, nil,
	function (i, name) return color_icon(self.doc:sheets(), name) end)
  m:add("symbolsize", "Size: " .. obj:get("symbolsize"), sizes)
  m:add("pen", "Pen: " .. obj:get("pen"), pens)
  m:add("action_edit_as_xml", "Edit as XML")
end

----------------------------------------------------------------------

local object_name = { text = "Text", path = "Path", image = "Image",
		       reference = "Reference", group = "Group", }

function MODEL:multiAttributes(m, tm)
  local layers = self:page():layers()
  local pinned = { "none", "horizontal", "vertical", "fixed" }
  local transformations = { "translations", "rigid", "affine" }
  m:add("layer", "Layer", layers)
  m:add("pinned", "Pinned", pinned)
  m:add("transformations", "Transformations", transformations)

  local sheet = self.doc:sheets()
  local colors = sheet:allNames("color")
  local sizes = sheet:allNames("symbolsize")
  local pens = sheet:allNames("pen")
  local opacity = sheet:allNames("opacity")
  local pathmodes = { "stroke only", "stroke && fill", "fill only" }
  local pathmode = { "stroked", "strokedfilled", "filled" }
  local dashstyle = sheet:allNames("dashstyle")
  local arrowsizes = sheet:allNames("arrowsize")
  local arrowshapes = symbolNames(sheet, "arrow/", "(spx)")
  local tilings = sheet:allNames("tiling")
  table.insert(tilings, 1, "normal")
  local gradients = sheet:allNames("gradient")
  table.insert(gradients, 1, "normal")
  local linecap = { "normal", "butt", "round", "square", }
  local linejoin = { "normal", "miter", "round", "bevel", }
  local fillrule = { "normal", "wind", "evenodd", }
  local textsize = sheet:allNames("textsize")
  local textstyle = sheet:allNames("textstyle")

  if tm.path or tm.text or tm.reference then
    m:add("stroke", "Stroke", colors, nil,
	  function (i, name) return color_icon(sheet, name) end)
  end
  if tm.path or tm.reference then
    m:add("fill", "Fill", colors, nil,
	  function (i, name) return color_icon(sheet, name) end)
  end
  if tm.path or tm.reference then m:add("pen", "Pen", pens) end

  if tm.path then
    m:add("pathmode", "Stroke && Fill", pathmode, pathmodes)
    m:add("dashstyle", "Dash style", dashstyle)
    local boolnames = { "yes", "no" }
    local boolmap = { "true", "false" }
    m:add("farrow", "Forward arrow", boolmap, boolnames)
    m:add("farrowsize", "Forward arrow size", arrowsizes)
    m:add("farrowshape", "Forward arrow shape", arrowshapes, arrowshapeToName)
    m:add("rarrow", "Reverse arrow", boolmap, boolnames)
    m:add("rarrowsize", "Reverse arrow size", arrowsizes)
    m:add("rarrowshape", "Reverse arrow shape", arrowshapes, arrowshapeToName)
    m:add("linecap", "Line cap", linecap)
    m:add("linejoin", "Line join", linejoin)
    m:add("fillrule", "Fill rule", fillrule)
    m:add("tiling", "Tiling pattern", tilings)
    m:add("gradient", "Gradient", gradients)
  end

  if tm.text then
    m:add("textsize", "Text size", textsize)
    m:add("textstyle", "Text style", textstyle)
    m:add("horizontalalignment", "Horizontal alignment",
	  {"left", "right", "hcenter"})
    m:add("verticalalignment", "Vertical alignment",
	  {"bottom", "baseline", "top", "vcenter"})
  end

  if tm.reference then m:add("symbolsize", "Size", sizes) end
  if tm.text or tm.path then m:add("opacity", "Opacity", opacity) end
end

function MODEL:multiPopup()
  -- collect types of selected objects
  local p = self:page()
  local typmap = {}
  local count = 0
  for i,obj,sel,layer in self:page():objects() do
    if sel then typmap[obj:type()] = true; count = count + 1 end
  end
  local typcount = 0
  local ttype = nil
  for t in pairs(typmap) do typcount = typcount + 1; ttype = t end

  local m = ipeui.Menu(self.ui:win())
  if typcount == 1 then
    m:add("comment", string.format("%d %s objects", count, object_name[ttype]))
  else
    m:add("comment", string.format("%d objects", count))
  end

  self:multiAttributes(m, typmap)

  m:add("action_cut", "Cut")
  m:add("action_copy", "Copy")
  m:add("action_paste", "Paste")
  m:add("action_pan_here", "Pan here")
  m:add("grid", "Toggle grid")
  local gp = self.ui:globalPos()
  local item, num, value = m:execute(gp.x, gp.y)
  if item then
    if item == "layer" then
      self:action_move_to_layer(value)
    elseif item:sub(1,7) == "action_" then
      self:action(item:sub(8))
    elseif item == "comment" then
      -- nothing to do
    elseif item == "grid" then
      self:toggleGrid()
    else
      self:setAttribute(item, value)
    end
  end
end

--------------------------------------------------------------------
