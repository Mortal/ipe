----------------------------------------------------------------------
-- model.lua
----------------------------------------------------------------------
--[[

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

--]]

MODEL = {}
MODEL.__index = MODEL

function MODEL:new(fname)
  local model = {}
  setmetatable(model, MODEL)
  model:init(fname)
  return model
end

function MODEL:init(fname)

  self.attributes = {
    pathmode = "stroked",
    stroke = "black",
    fill = "white",
    pen = "normal",
    dashstyle = "normal",
    farrowshape = "arrow/normal(spx)",
    rarrowshape = "arrow/normal(spx)",
    farrowsize = "normal",
    rarrowsize = "normal",
    farrow = false,
    rarrow = false,
    symbolsize = "normal",
    textsize = "normal",
    textstyle = "normal",
    transformabletext = prefs.transformable_text,
    horizontalalignment = "left",
    verticalalignment = "baseline",
    pinned = "none",
    transformations = "affine",
    linejoin = "normal",
    linecap = "normal",
    fillrule = "normal",
    markshape = "mark/disk(sx)",
    tiling = "normal",
    gradient = "normal",
    opacity = "opaque",
  }

  self.snap = {
    snapvtx = false,
    snapbd = false,
    snapint = false,
    snapgrid = false,
    snapangle = false,
    snapauto = false,
    grid_visible = prefs.grid_visible,
    gridsize = prefs.grid_size,
    anglesize = 45,  -- degrees
    snap_distance = prefs.snap_distance,
    with_axes = false,
    origin = ipe.Vector(0,0),
    orientation = 0
  }

  self.save_timestamp = 0
  self.ui = AppUi(self)
  self.pristine = false
  self.first_show = true

  if fname then
    if ipe.fileExists(fname) then
      self:loadDocument(fname)
    else
      self:newDocument()
      self.file_name = fname
      self:setCaption()
    end
  end
  if not self.doc then
    self:newDocument()
    self.pristine = true
  end

  self.mode = "select"
  self.ui:setActionState("mode_select", true)
  self.ui:setActionState("grid_visible", self.snap.grid_visible)

  if #self:getBookmarks() == 0 then
    self.ui:showTool("bookmarks", false)
  end
  if self:page():notes() == "" then
    self.ui:showTool("notes", false)
  end

  if prefs.autosave_interval then
    self.timer = ipeui.Timer(self, "autosave")
    self.timer:setInterval(1000 * prefs.autosave_interval) -- millisecs
    self.timer:start()
  end
end

function MODEL:sizeChanged()
  if self.first_show then
    self:action_fit_top()
  end
  self.first_show = false
end

----------------------------------------------------------------------

function MODEL:resetGridSize()
  self.snap.gridsize = prefs.grid_size
  self.snap.anglesize = prefs.angle_size
  local gridsizes = allValues(self.doc:sheets(), "gridsize")
  if #gridsizes == 0 then gridsizes = { 16 } end
  if not indexOf(self.snap.gridsize, gridsizes) then
    self.snap.gridsize = gridsizes[1]
  end
  local anglesizes = allValues(self.doc:sheets(), "anglesize")
  if #anglesizes == 0 then anglesizes = { 45 } end
  if not indexOf(self.snap.anglesize, anglesizes) then
    self.snap.anglesize = anglesizes[1]
  end
  self.ui:setGridAngleSize(self.snap.gridsize, self.snap.anglesize)
end

function MODEL:print_attributes()
  print("----------------------------------------")
  for k in pairs(self.attributes) do
    print(k, self.attributes[k])
  end
  print("----------------------------------------")
end

function MODEL:getString(msg, caption, start)
  if caption == nil then caption = "Ipe" end
  local d = ipeui.Dialog(self.ui:win(), caption)
  d:add("label", "label", {label=msg}, 1, 1)
  d:add("text", "input", {}, 2, 1)
  d:addButton("cancel", "Cancel", "reject")
  d:addButton("ok", "Ok", "accept")
  if start then d:set("text", start) end
  if d:execute() then
    return d:get("text")
  end
end

function MODEL:getDouble(caption, label, value, minv, maxv)
  local s = self:getString(label, caption, tostring(value))
  -- print("getDouble", s)
  if s then
    local n = tonumber(s)
    if n and minv <= n and n <= maxv then
      return n
    end
  end
end

----------------------------------------------------------------------

-- Return current page
function MODEL:page()
  return self.doc[self.pno]
end

-- return table with selected objects on current page
function MODEL:selection()
  local t = {}
  for i,obj,sel,layer in self:page():objects() do
    if sel then t[#t+1] = i end
  end
  return t
end

function MODEL:markAsUnmodified()
  self.save_timestamp = self.save_timestamp + 1
  self.undo[#self.undo].save_timestamp = self.save_timestamp
end

-- is document modified?
function MODEL:isModified()
  return (self.undo[#self.undo].save_timestamp ~= self.save_timestamp)
end

----------------------------------------------------------------------

-- set window caption
function MODEL:setCaption()
  local s = "Ipe "
  if config.toolkit == "qt" then
    s = s .. "[*]"
  elseif self:isModified() then
    s = s .. '*'
  end
  if self.file_name then
    s = s .. '"' .. self.file_name .. '" '
  else
    s = s .. "[unsaved] "
  end
--[[
  if #self.doc > 1 then
    s = s .. string.format("Page %d/%d ", self.pno, #self.doc)
  end
  if self:page():countViews() > 1 then
    s = s .. string.format("(View %d/%d) ", self.vno, self:page():countViews())
  end
--]]
  self.ui:setWindowTitle(self:isModified(), s)
end

-- show a warning messageBox
function MODEL:warning(text, details)
  ipeui.messageBox(self.ui:win(), "warning", text, details)
end

-- Set canvas to current page
function MODEL:setPage()
  local p = self:page()
  self.ui:setPage(p, self.pno, self.vno, self.doc:sheets())
  self.ui:setLayers(p, self.vno)
  self.ui:setNumbering(self.doc:properties().numberpages)
  self.ui:update()
  self:setCaption()
  self:setBookmarks()
  self.ui:setNotes(p:notes())
  local vno
  if p:countViews() > 1 then
    vno = string.format("View %d/%d", self.vno, p:countViews())
  end
  local pno
  if #self.doc > 1 then
    pno = string.format("Page %d/%d", self.pno, #self.doc)
  end
  self.ui:setNumbers(vno, p:markedView(self.vno), pno, p:marked())
end

function MODEL:getBookmarks()
  local b = {}
  for _,p in self.doc:pages() do
    local t = p:titles()
    if t.section then
      if t.section ~= "" then b[#b+1] = t.section end
    elseif t.title ~= "" then b[#b+1] = t.title end
    if t.subsection then
      if t.subsection ~= "" then b[#b+1] = "  " .. t.subsection end
    elseif t.title ~= "" then b[#b+1] = "   " .. t.title end
  end
  return b
end

function MODEL:setBookmarks()
  self.ui:setBookmarks(self:getBookmarks())
end

function MODEL:findPageForBookmark(index)
  local count = 0
  for i,p in self.doc:pages() do
    local t = p:titles()
    if t.section then
      if t.section ~= "" then count = count + 1 end
    elseif t.title ~= "" then count = count + 1 end
    if t.subsection then
      if t.subsection ~= "" then count = count + 1 end
    elseif t.title ~= "" then count = count + 1 end
    if count >= index then return i end
  end
end

----------------------------------------------------------------------

-- Deselect all objects not in current view, or in a locked layer.
function MODEL:deselectNotInView()
  local p = self:page()
  for i,obj,sel,layer in p:objects() do
    if not p:visible(self.vno, i) or p:isLocked(layer) then
      p:setSelect(i, nil)
    end
  end
  p:ensurePrimarySelection()
end

-- Deselect all but primary selection
function MODEL:deselectSecondary()
  local p = self:page()
  for i,obj,sel,layer in p:objects() do
    if sel ~= 1 then p:setSelect(i, nil) end
  end
end

-- If no selected object is close, select closest object.
-- If there is a selected object close enough, returns true.
-- Otherwise, check whether the closest object to the current mouse
-- position is close enough.  If so, unselect everything, make
-- this object the primary selection, and return true.
-- If not, return whether the page has a selection at all.

-- If primaryOnly is true, the primary selection has to be close enough,
-- otherwise it'll be replaced as above.
function MODEL:updateCloseSelection(primaryOnly)
  local bound = prefs.close_distance
  local pos = self.ui:unsnappedPos()
  local p = self:page()

  -- is current selection close enough?
  if primaryOnly then
    local prim = p:primarySelection()
    if prim and p:distance(prim, pos, bound) < bound then return true end
  else
    for i,obj,sel,layer in p:objects() do
      if sel and p:distance(i, pos, bound) < bound then return true end
    end
  end

  -- current selection is not close enough: find closest object
  local closest
  for i,obj,sel,layer in p:objects() do
     if p:visible(self.vno, i) and not p:isLocked(layer) then
	local d = p:distance(i, pos, bound)
	if d < bound then closest = i; bound = d end
     end
  end

  if closest then
    -- deselect all, and select only closest object
    p:deselectAll()
    p:setSelect(closest, 1)
    return true
  else
    return p:hasSelection()
  end
end

----------------------------------------------------------------------

-- Move to next/previous view
function MODEL:nextView(delta)
  if 1 <= self.vno + delta and self.vno + delta <= self:page():countViews() then
    self.vno = self.vno + delta;
    self:deselectNotInView()
  elseif 1 <= self.pno + delta and self.pno + delta <= #self.doc then
    self.pno = self.pno + delta
    if delta > 0 then
      self.vno = 1;
    else
      self.vno = self:page():countViews()
    end
  end
end

-- Move to next/previous page
function MODEL:nextPage(delta)
  if 1 <= self.pno + delta and self.pno + delta <= #self.doc then
    self.pno = self.pno + delta
    if delta > 0 then
      self.vno = 1;
    else
      self.vno = self:page():countViews()
    end
  end
end

-- change zoom and pan to fit box on screen
function MODEL:fitBox(box)
  if box:isEmpty() then return end
  local cs = self.ui:canvasSize()
  local xfactor = 20.0
  local yfactor = 20.0
  if box:width() > 0 then xfactor = cs.x / box:width() end
  if box:height() > 0 then yfactor = cs.y / box:height() end
  local zoom = math.min(xfactor, yfactor)

  if zoom > prefs.max_zoom then zoom = prefs.max_zoom end
  if zoom < prefs.min_zoom then zoom = prefs.min_zoom end

  self.ui:setPan(0.5 * (box:bottomLeft() + box:topRight()))
  self.ui:setZoom(zoom)
  self.ui:update()
end

----------------------------------------------------------------------

function MODEL:latexErrorBox(log)
  local d = ipeui.Dialog(self.ui:win(), "Ipe: error running Latex")
  d:add("label", "label",
	{ label="An error occurred during the Pdflatex run. " ..
	  "Please consult the logfile below." },
	1, 1)
  d:add("text", "text", { read_only=true, syntax="logfile" }, 2, 1)
  d:addButton("ok", "Ok", "accept")
  d:set("text", log)
  d:execute()
end

function MODEL:runLatex()
  local success, errmsg, result, log = self.doc:runLatex()
  if success then
    self.ui:setFontPool(self.doc)
    self.ui:update()
    return true
  elseif result == "latex" then
    self:latexErrorBox(log)
    return false
  else
    self:warning("An error occurred during the Pdflatex run", errmsg)
  end
end

-- checks if document is modified, and asks user if it can be discarded
-- returns true if object is unmodified or user confirmed discarding
function MODEL:checkModified()
  if self:isModified() then
    return ipeui.messageBox(self.ui:win(), "question",
				 "The document has been modified",
				 "Do you wish to discard the current document?",
				 "discardcancel") == 0
  else
    return true
  end
end

----------------------------------------------------------------------

function MODEL:newDocument()
  self.pno = 1
  self.vno = 1
  self.file_name = nil

  self.undo = { {} }
  self.redo = {}
  self:markAsUnmodified()

  self.doc = ipe.Document()
  for _, w in ipairs(config.styleList) do
    local sheet = assert(ipe.Sheet(w))
    self.doc:sheets():insert(1, sheet)
  end

  self:setPage()

  self.ui:setupSymbolicNames(self.doc:sheets())
  self.ui:setAttributes(self.doc:sheets(), self.attributes)
  self:resetGridSize()

  self:action_normal_size()
  self.ui:setSnap(self.snap)

  self.ui:explain("New document")
end

function MODEL:loadDocument(fname)
  local doc, err = ipe.Document(fname)
  if doc then
    self.doc = doc
    self.file_name = fname
    self.pno = 1
    self.vno = 1

    self.undo = { {} }
    self.redo = {}
    self:markAsUnmodified()

    self:setPage()

    self.ui:setupSymbolicNames(self.doc:sheets())
    self.ui:setAttributes(self.doc:sheets(), self.attributes)
    self:resetGridSize()

    self:action_normal_size()
    self.ui:setSnap(self.snap)

    self.ui:explain("Document '" .. fname .. "' loaded")

    local syms = self.doc:checkStyle()
    if #syms > 0 then
      self:warning("The document contains symbolic attributes " ..
		   "that are not defined in the style sheet:",
		 "<qt><ul><li>" .. table.concat(syms, "<li>")
		 .. "</qt>")
    end
  else
    self:warning("Document '" .. fname .. "' could not be opened", err)
  end
end

function MODEL:saveDocument(fname)
  if not fname then
    fname = self.file_name
  end
  local fm = formatFromFileName(fname)
  if not fm then
    self:warning("File not saved!",
		 "You must save as *.xml, *.ipe, *.pdf, or *.eps")
    return
  end

  if fm == "eps" and self.doc:countTotalViews() > 1 then
    self:warning("File not saved!",
		 "Only single-view documents can be saved in EPS format")
    return
  end

  -- run Latex if format is not XML
  if (fm ~= "xml" or prefs.auto_export_xml_to_eps or
      prefs.auto_export_xml_to_pdf) then
    if not self:runLatex() then
      self.ui:explain("Latex error - file not saved")
      return
    end
  end

  if fm == "eps" then
    local ok = true
    local msg = "<qt>The document uses the following features:<ul>"
    if self.doc:has("truetype") then
      ok = false
      msg = msg .. "<li>Truetype fonts"
    end
    if self.doc:has("gradients") then
      ok = false
      msg = msg .. "<li>Gradients"
    end
    if self.doc:has("transparency") then
      ok = false
      msg = msg .. "<li>Transparency"
    end
    if not ok then
      msg = msg .. "</ul><p>These cannot be saved in EPS format.</p>"
	.. "<p>You can save as PDF, and use pdftops to create a "
	.. "Postscript file.</p></qt>"
      self:warning("File not saved!", msg)
      return
    end
  end

  local props = self.doc:properties()
  props.modified = "D:" .. ipeui.currentDateTime()
  if props.created == "" then props.created = props.modified end
  props.creator = config.version
  self.doc:setProperties(props)

  if not self.doc:save(fname, fm) then
    self:warning("File not saved!", "Error saving the document")
    return
  end

  -- auto-exporting
  if fm == "xml" and prefs.auto_export_xml_to_eps then
    self:auto_export("eps", fname)
  end
  if fm == "xml" and prefs.auto_export_xml_to_pdf then
    self:auto_export("pdf", fname)
  end
  if fm == "eps" and prefs.auto_export_eps_to_pdf then
    self:auto_export("pdf", fname)
  end
  if fm == "pdf" and prefs.auto_export_pdf_to_eps then
    self:auto_export("eps", fname)
  end

  self:markAsUnmodified()
  self.ui:explain("Saved document '" .. fname .. "'")
  self.file_name = fname
  self:setCaption()
  return true
end

function MODEL:auto_export(fm, fname)
  local ename = fname:sub(1,-4) .. fm
  if prefs.auto_export_only_if_exists and not ipe.fileExists(ename) then
    return
  end
  local result
  if fm == "eps" then
    result = self.doc:exportView(ename, fm, nil, 1, 1)
  else
    result = self.doc:save(ename, fm, { export=true } )
  end
  if not result then
    self:warning("Auto-exporting failed",
		 "I could not export in format '" .. fm ..
		   "' to file '" .. ename .. "'.")
  end
end

----------------------------------------------------------------------

function MODEL:autosave()
  -- only autosave if document has been modified
  if not self:isModified() then return end
  local f
  if prefs.autosave_filename:find("%%s") then
    if self.file_name then
      f = self.file_name:match("/([^/]+)$") or self.file_name
    else
      f = "unnamed"
    end
    f = string.format(prefs.autosave_filename, f)
  else
    f = prefs.autosave_filename
  end
  self.ui:explain("Autosaving to " .. f .. "...")
  if not self.doc:save(f, "xml") then
    ipeui.messageBox(self.ui:win(), "critical",
		     "Autosaving failed!")
  end
end

----------------------------------------------------------------------

function MODEL:closeEvent()
  if self == first_model then first_model = nil end
  if self:isModified() then
    local r = ipeui.messageBox(self.ui:win(), "question",
				    "The document has been modified",
				    "Do you wish to save the document?",
				    "savediscardcancel")
    if r == 1 then
      return self:action_save()
    else
      return r == 0
    end
  else
    return true
  end
end

----------------------------------------------------------------------

-- TODO:  limit on undo stack size?
function MODEL:registerOnly(t)
  self.pristine = false
  -- store it on undo stack
  self.undo[#self.undo + 1] = t
  -- flush redo stack
  self.redo = {}
  self:setPage()
end

function MODEL:register(t)
  -- store selection
  t.original_selection = self:selection()
  t.original_primary = self:page():primarySelection()
  -- perform action
  t.redo(t, self.doc)
  self:registerOnly(t)
  self.ui:explain(t.label)
  if t.style_sheets_changed then
    self.ui:setupSymbolicNames(self.doc:sheets())
    self.ui:setAttributes(self.doc:sheets(), self.attributes)
  end
end

function MODEL:creation(label, obj)
  local t = { label=label, pno=self.pno, vno=self.vno,
	      layer=self:page():active(self.vno), object=obj }
  t.undo = function (t, doc) doc[t.pno]:remove(#doc[t.pno]) end
  t.redo = function (t, doc)
	     doc[t.pno]:deselectAll()
	     doc[t.pno]:insert(nil, t.object, 1, t.layer)
	   end
  self:register(t)
end

function MODEL:transformation(mode, m)
  local t = { label = mode,
	      pno = self.pno,
	      vno = self.vno,
	      selection = self:selection(),
	      original = self:page():clone(),
	      matrix = m,
	      undo = revertOriginal,
	    }
  t.redo = function (t, doc)
	     local p = doc[t.pno]
	     for _,i in ipairs(t.selection) do
	       p:transform(i, t.matrix)
	     end
	   end
  self:register(t)
end

function MODEL:setAttribute(prop, value)
  local t = { label="set attribute " .. prop .. " to " .. tostring(value),
	      pno=self.pno,
	      vno=self.vno,
	      selection=self:selection(),
	      original=self:page():clone(),
	      property=prop,
	      value=value,
	      undo=revertOriginal,
	      stroke=self.attributes.stroke,
	      fill=self.attributes.fill,
	    }
  t.redo = function (t, doc)
	     local p = doc[t.pno]
	     local changed = false
	     for _,i in ipairs(t.selection) do
	       if p:setAttribute(i, t.property, t.value, t.stroke, t.fill) then
		 changed = true
	       end
	     end
	     return changed
	   end
  if (t.redo(t, self.doc)) then
    t.original_selection = t.selection
    t.original_primary = self:page():primarySelection()
    self:registerOnly(t)
  end
end

----------------------------------------------------------------------

HELPER = {}
HELPER.__index = HELPER

function HELPER.new(model)
  local helper = {}
  setmetatable(helper, HELPER)
  helper.model = model
  return helper
end

function HELPER:message(m)
  self.model.ui:explain(m)
end

function HELPER:messageBox(text, details, buttons)
  return ipeui.messageBox(self.model.ui:win(), nil, text, details, buttons)
end

function HELPER:getString(m)
  return self.model:getString(m)
end

function MODEL:runIpelet(label, ipelet, num)
  local helper = HELPER.new(self)
  local t = { label="ipelet '" .. label .."'",
	      pno=self.pno,
	      vno=self.vno,
	      original=self:page():clone(),
	      undo=revertOriginal,
	      redo=revertFinal,
	      original_selection = self:selection(),
	      original_primary = self:page():primarySelection(),
	    }
  local need_undo = ipelet:run(num or 1, self:page(), self.doc,
			       self.pno, self.vno,
			       self:page():active(self.vno),
			       self.attributes, helper)
  if need_undo then
    t.final = self:page():clone()
    self:registerOnly(t)
  end
  self:setPage()
end

----------------------------------------------------------------------

function MODEL:action_undo()
  if #self.undo <= 1 then
    ipeui.messageBox(self.ui:win(), "information",
			  "No undo information available")
    return
  end
  t = self.undo[#self.undo]
  table.remove(self.undo)
  t.undo(t, self.doc)
  self.ui:explain("Undo '" .. t.label .. "'")
  self.redo[#self.redo + 1] = t
  if t.pno then
    self.pno = t.pno
  elseif t.pno0 then
    self.pno = t.pno0
  end
  if t.vno then
    self.vno = t.vno
  elseif t.vno0 then
    self.vno = t.vno0
  end
  local p = self:page()
  p:deselectAll()
  if t.original_selection then
    for _, no in ipairs(t.original_selection) do
      p:setSelect(no, 2)
    end
  end
  if t.original_primary then
    p:setSelect(t.original_primary, 1)
  end
  self:setPage()
  if t.style_sheets_changed then
    self.ui:setupSymbolicNames(self.doc:sheets())
    self.ui:setAttributes(self.doc:sheets(), self.attributes)
  end
end

function MODEL:action_redo()
  if #self.redo == 0 then
    ipeui.messageBox(self.ui:win(), "information",
		     "No redo information available")
    return
  end
  t = self.redo[#self.redo]
  table.remove(self.redo)
  t.redo(t, self.doc)
  self.ui:explain("Redo '" .. t.label .. "'")
  self.undo[#self.undo + 1] = t
  if t.pno then
    self.pno = t.pno
  elseif t.pno1 then
    self.pno = t.pno1
  end
  if t.vno then
    self.vno = t.vno
  elseif t.vno1 then
    self.vno = t.vno1
  end
  self:page():deselectAll()
  self:setPage()
  if t.style_sheets_changed then
    self.ui:setupSymbolicNames(self.doc:sheets())
    self.ui:setAttributes(self.doc:sheets(), self.attributes)
  end
end

----------------------------------------------------------------------
