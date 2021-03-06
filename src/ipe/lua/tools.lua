----------------------------------------------------------------------
-- tools.lua
----------------------------------------------------------------------
--[[

    This file is part of the extensible drawing editor Ipe.
    Copyright (C) 1993-2014  Otfried Cheong

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

function externalEditor(d, field)
  local text = d:get(field)
  local fname = os.tmpname()
  local f = io.open(fname, "w")
  f:write(text)
  f:close()
  ipeui.WaitDialog(string.format(prefs.external_editor, fname))
  f = io.open(fname, "r")
  text = f:read("*all")
  f:close()
  os.remove(fname)
  d:set(field, text)
  if prefs.editor_closes_dialog then
    d:accept(true)
  end
end

function addEditorField(d, field)
  if prefs.external_editor then
    d:addButton("editor", "&Editor", function (d) externalEditor(d, field) end)
  end
end

----------------------------------------------------------------------

local function circleshape(center, radius)
  return { type="ellipse";
	   ipe.Matrix(radius, 0, 0, radius, center.x, center.y) }
end

local function segmentshape(v1, v2)
  return { type="curve", closed=false; { type="segment"; v1, v2 } }
end

local function boxshape(v1, v2)
  return { type="curve", closed=true;
	   { type="segment"; v1, V(v1.x, v2.y) },
	   { type="segment"; V(v1.x, v2.y), v2 },
	   { type="segment"; v2, V(v2.x, v1.y) } }
end

local function arcshape(center, radius, alpha, beta)
  local a = ipe.Arc(ipe.Matrix(radius, 0, 0, radius, center.x, center.y),
		    alpha, beta)
  local v1 = center + radius * ipe.Direction(alpha)
  local v2 = center + radius * ipe.Direction(beta)
  return { type="curve", closed=false; { type="arc", arc=a; v1, v2 } }
end

local function rarcshape(center, radius, alpha, beta)
  local a = ipe.Arc(ipe.Matrix(radius, 0, 0, -radius, center.x, center.y),
		    alpha, beta)
  local v1 = center + radius * ipe.Direction(alpha)
  local v2 = center + radius * ipe.Direction(beta)
  return { type="curve", closed=false; { type="arc", arc=a; v1, v2 } }
end

----------------------------------------------------------------------

local VERTEX = 1
local SPLINE = 2
local QUAD = 3
local BEZIER = 4
local ARC = 5

LINESTOOL = {}
LINESTOOL.__index = LINESTOOL

-- mode is one of "lines", "polygons", "splines"
function LINESTOOL:new(model, mode)
  local tool = {}
  setmetatable(tool, LINESTOOL)
  tool.model = model
  tool.mode = mode
  local v = model.ui:pos()
  tool.v = { v, v }
  model.ui:setAutoOrigin(v)
  if mode == "splines" then
    tool.t = { VERTEX, SPLINE }
  else
    tool.t = { VERTEX, VERTEX }
  end
  model.ui:shapeTool(tool)
  tool.setColor(1.0, 0, 0)
  return tool
end

function LINESTOOL:last()
  return self.t[#self.t]
end

function LINESTOOL:has_segs(count)
  if #self.t < count then return false end
  local result = true
  for i = #self.t - count + 1,#self.t do
    if self.t[i] ~= VERTEX then return false end
  end
  return true
end

local function compute_arc(p0, pmid, p1)
  if p0 == p1 then return { type="segment", p0, p1 } end
  local l1 = ipe.Line(p0, (pmid- p0):normalized())
  local l2 = ipe.Line(p0, l1:normal())
  local bi1 = ipe.Bisector(p0, p1)
  local center = l2:intersects(bi1)
  local side = l1:side(p1)
  if side == 0.0 or not center then return { type="segment", p0, p1 } end
  local u = p0 - center
  local alpha = side * u:angle()
  local beta = side * ipe.normalizeAngle((p1 - center):angle(), alpha)
  local radius = u:len()
  local m = { radius, 0, 0, side * radius, center.x, center.y }
  return { type = "arc", p0, p1, arc = ipe.Arc(ipe.Matrix(m), alpha, beta) }
end

-- compute orientation of tangent to previous segment at its final point
function LINESTOOL:compute_orientation()
  if self.t[#self.t - 2] ~= ARC then
    return (self.v[#self.v - 1] - self.v[#self.v - 2]):angle()
  else
    -- only arc needs special handling
    local arc = compute_arc(self.v[#self.v - 3], self.v[#self.v - 2],
			    self.v[#self.v - 1])
    if arc.type == "arc" then
      local alpha, beta = arc.arc:angles()
      local dir = ipe.Direction(beta + math.pi / 2)
      return (arc.arc:matrix():linear() * dir):angle()
    else
      return (self.v[#self.v - 1] - self.v[#self.v - 3]):angle()
    end
  end
end

function LINESTOOL:compute()
  self.shape = { type="curve", closed=(self.mode == "polygons") }
  local i = 2
  while i <= #self.v do
    -- invariant: t[i-1] == VERTEX
    local seg
    if self.t[i] == VERTEX then
      seg = { type="segment", self.v[i-1], self.v[i] }
      i = i + 1
    elseif self.t[i] == SPLINE then
      local j = i
      while j <= #self.v and self.t[j] == SPLINE do j = j + 1 end
      if j > #self.v then j = #self.v end
      seg = { type="spline" }
      for k = i-1,j do seg[#seg+1] = self.v[k] end
      i = j + 1
    elseif self.t[i] == QUAD then
      seg = { type="quad", self.v[i-1], self.v[i], self.v[i+1] }
      i = i + 2
    elseif self.t[i] == BEZIER then
      seg = { type="bezier", self.v[i-1], self.v[i], self.v[i+1], self.v[i+2] }
      i = i + 3
    elseif self.t[i] == ARC then
      seg = compute_arc(self.v[i-1], self.v[i], self.v[i+1])
      i = i + 2
    end
    self.shape[#self.shape + 1] = seg
  end
  self.setShape( { self.shape } )
end

function LINESTOOL:mouseButton(button, modifiers, press)
  if not press then return end
  local v = self.model.ui:pos()
  self.v[#self.v] = v
  if button == 2 then
    if self:last() == SPLINE then self.t[#self.t] = VERTEX end
    self:compute()
    self.model.ui:finishTool()
    local obj = ipe.Path(self.model.attributes, { self.shape }, true)
    -- if it is just a point, force line cap to round
    if #self.shape == 1 and self.shape[1].type == "segment" and
      self.shape[1][1] == self.shape[1][2] then
      obj:set("linecap", "round")
    end
    self.model:creation("create path", obj)
    return
  end
  self.v[#self.v + 1] = v
  self.model.ui:setAutoOrigin(v)
  if self:last() == SPLINE then
    local typ = modifiers.shift and VERTEX or SPLINE
    self.t[#self.t] = typ
    self.t[#self.t + 1] = typ
  else
    self.t[#self.t + 1] = VERTEX
  end
  self:compute()
  self.model.ui:update(false) -- update tool
  self:explain()
end

function LINESTOOL:mouseMove()
  self.v[#self.v] = self.model.ui:pos()
  self:compute()
  self.model.ui:update(false) -- update tool
  self:explain()
end

function LINESTOOL:explain()
  local s
  if self:last() == SPLINE then
    s = ("Left: add ctrl point | Shift+Left: switch to line mode" ..
	 " | Del: delete ctrl point")
  else
    s = "Left: add vtx | Del: delete vtx"
  end
  s = s .. " | Right: final vtx"
  if self:has_segs(2) then
    s = s .. " | " .. shortcuts_linestool.spline .. ": spline mode"
  end
  if self:has_segs(3) then
    s = s .. " | " .. shortcuts_linestool.quadratic_bezier .. ": quadratic Bezier"
    s = s .. " | " .. shortcuts_linestool.arc .. ": circle arc"
  end
  if self:has_segs(4) then
    s = s .. " | " .. shortcuts_linestool.cubic_bezier ..": cubic Bezier"
  end
  if #self.v > 2 and self.t[#self.t - 1] == VERTEX then
    s = s .. " | " .. shortcuts_linestool.set_axis ..": set axis"
  end
  self.model.ui:explain(s, 0)
end

function LINESTOOL:key(code, modifiers, text)
  if text == prefs.delete_key then  -- Delete
    if #self.v > 2 then
      table.remove(self.v)
      table.remove(self.t)
      if self:last() == QUAD or self:last() == ARC then
	self.t[#self.t] = VERTEX
      end
      if self:last() == BEZIER then
	self.t[#self.t] = VERTEX
	self.t[#self.t-1] = VERTEX
      end
      self:compute()
      self.model.ui:update(false)
      self:explain()
    else
      self.model.ui:finishTool()
    end
    return true
  elseif text == "\027" then
    self.model.ui:finishTool()
    return true
  elseif self:has_segs(2) and text == shortcuts_linestool.spline then
    self.t[#self.t] = SPLINE
    self:compute()
    self.model.ui:update(false)
    self:explain()
  elseif self:has_segs(3) and text == shortcuts_linestool.quadratic_bezier then
    self.t[#self.t - 1] = QUAD
    self:compute()
    self.model.ui:update(false)
    self:explain()
  elseif self:has_segs(3) and text == shortcuts_linestool.arc then
    self.t[#self.t - 1] = ARC
    self:compute()
    self.model.ui:update(false)
    self:explain()
  elseif self:has_segs(4) and text == shortcuts_linestool.cubic_bezier then
    self.t[#self.t - 1] = BEZIER
    self.t[#self.t - 2] = BEZIER
    self:compute()
    self.model.ui:update(false)
    self:explain()
  elseif #self.v > 2 and self.t[#self.t - 1] == VERTEX and
    text == shortcuts_linestool.set_axis then
    -- set axis
    self.model.snap.with_axes = true
    self.model.snap.snapangle = true
    self.model.snap.origin = self.v[#self.v - 1]
    self.model.snap.orientation = self:compute_orientation()
    self.model.ui:setSnap(self.model.snap)
    self.model.ui:setActionState("snapangle", true)
    self.model.ui:update(true)   -- redraw coordinate system
    self:explain()
  else
    return false
  end
end

----------------------------------------------------------------------

BOXTOOL = {}
BOXTOOL.__index = BOXTOOL

function BOXTOOL:new(model, square)
  local tool = {}
  setmetatable(tool, BOXTOOL)
  tool.model = model
  tool.square = square
  local v = model.ui:pos()
  tool.v = { v, v }
  model.ui:shapeTool(tool)
  tool.setColor(1.0, 0, 0)
  return tool
end

function BOXTOOL:compute()
  self.v[2] = self.model.ui:pos()
  if self.square then
    local d = self.v[2] - self.v[1]
    local sign = V(d.x > 0 and 1 or -1, d.y > 0 and 1 or -1)
    local dd = math.max(math.abs(d.x), math.abs(d.y))
    self.v[2] = self.v[1] + dd * sign
  end
  self.shape = boxshape(self.v[1], self.v[2])
end

function BOXTOOL:mouseButton(button, modifiers, press)
  if not press then return end
  self:compute()
  self.model.ui:finishTool()
  local obj = ipe.Path(self.model.attributes, { self.shape })
  self.model:creation("create box", obj)
end

function BOXTOOL:mouseMove()
  self:compute()
  self.setShape( { self.shape } )
  self.model.ui:update(false) -- update tool
end

function BOXTOOL:key(code, modifiers, text)
  if text == "\027" then
    self.model.ui:finishTool()
    return true
  else
    return false
  end
end

----------------------------------------------------------------------

SPLINEGONTOOL = {}
SPLINEGONTOOL.__index = SPLINEGONTOOL

function SPLINEGONTOOL:new(model)
  local tool = {}
  setmetatable(tool, SPLINEGONTOOL)
  tool.model = model
  local v = model.ui:pos()
  tool.v = { v, v }
  model.ui:shapeTool(tool)
  tool.setColor(1.0, 0, 0)
  return tool
end

function SPLINEGONTOOL:compute()
  if #self.v == 2 then
    self.shape = segmentshape(self.v[1], self.v[2])
  else
    self.shape = { type="closedspline" }
    for _,v in ipairs(self.v) do self.shape[#self.shape + 1] = v end
  end
  self.setShape( { self.shape } )
end

function SPLINEGONTOOL:explain()
  local s = "Left: Add vertex | Right: Add final vertex | Del: Delete vertex"
  self.model.ui:explain(s, 0)
end

function SPLINEGONTOOL:mouseButton(button, modifiers, press)
  if not press then return end
  local v = self.model.ui:pos()
  self.v[#self.v] = v
  if button == 2 then
    self.model.ui:finishTool()
    local obj = ipe.Path(self.model.attributes, { self.shape })
    self.model:creation("create splinegon", obj)
    return
  end
  self.v[#self.v + 1] = v
  self:compute()
  self.model.ui:update(false) -- update tool
  self:explain()
end

function SPLINEGONTOOL:mouseMove()
  self.v[#self.v] = self.model.ui:pos()
  self:compute()
  self.model.ui:update(false) -- update tool
end

function SPLINEGONTOOL:key(code, modifiers, text)
  if text == prefs.delete_key then  -- Delete
    if #self.v > 2 then
      table.remove(self.v)
      self:compute()
      self.model.ui:update(false)
      self:explain()
    else
      self.model.ui:finishTool()
    end
    return true
  elseif text == "\027" then
    self.model.ui:finishTool()
    return true
  else
    self:explain()
    return false
  end
end

----------------------------------------------------------------------

CIRCLETOOL = {}
CIRCLETOOL.__index = CIRCLETOOL

function CIRCLETOOL:new(model, mode)
  local tool = {}
  setmetatable(tool, CIRCLETOOL)
  tool.model = model
  tool.mode = mode
  local v = model.ui:pos()
  tool.v = { v, v, v }
  tool.cur = 2
  model.ui:shapeTool(tool)
  tool.setColor(1.0, 0, 0)
  return tool
end

function CIRCLETOOL:compute()
  if self.mode == "circle1" then
    self.shape = circleshape(self.v[1], (self.v[2] - self.v[1]):len())
  elseif self.mode == "circle2" then
    self.shape = circleshape(0.5 * (self.v[1] + self.v[2]),
			     (self.v[2] - self.v[1]):len() / 2.0)
  elseif self.mode == "circle3" then
    if self.cur == 2 or self.v[3] == self.v[2] then
      self.shape = circleshape(0.5 * (self.v[1] + self.v[2]),
			       (self.v[2] - self.v[1]):len() / 2.0)
    else
      local l1 = ipe.LineThrough(self.v[1], self.v[2])
      if math.abs(l1:side(self.v[3])) < 1e-9 then
	self.shape = segmentshape(self.v[1], self.v[2])
      else
	local l2 = ipe.LineThrough(self.v[2], self.v[3])
	local bi1 = ipe.Bisector(self.v[1], self.v[2])
	local bi2 = ipe.Bisector(self.v[2], self.v[3])
	local center = bi1:intersects(bi2)
	self.shape = circleshape(center, (self.v[1] - center):len())
      end
    end
  end
end

function CIRCLETOOL:mouseButton(button, modifiers, press)
  if not press then return end
  local v = self.model.ui:pos()
  -- refuse point identical to previous
  if v == self.v[self.cur - 1] then return end
  self.v[self.cur] = v
  self:compute()
  if self.cur == 3 or (self.mode ~= "circle3" and self.cur == 2) then
    self.model.ui:finishTool()
    local obj = ipe.Path(self.model.attributes, { self.shape })
    self.model:creation("create circle", obj)
  else
    self.cur = self.cur + 1
    self.model.ui:update(false)
  end
end

function CIRCLETOOL:mouseMove()
  self.v[self.cur] = self.model.ui:pos()
  self:compute()
  self.setShape({ self.shape })
  self.model.ui:update(false) -- update tool
end

function CIRCLETOOL:key(code, modifiers, text)
  if text == "\027" then
    self.model.ui:finishTool()
    return true
  else
    return false
  end
end

----------------------------------------------------------------------

ARCTOOL = {}
ARCTOOL.__index = ARCTOOL

function ARCTOOL:new(model, mode)
  local tool = {}
  setmetatable(tool, ARCTOOL)
  tool.model = model
  tool.mode = mode
  local v = model.ui:pos()
  tool.v = { v, v, v }
  tool.cur = 2
  model.ui:shapeTool(tool)
  tool.setColor(1.0, 0, 0)
  return tool
end

function ARCTOOL:compute()
  local u = self.v[2] - self.v[1]
  if self.cur == 2 then
    if self.mode == "arc3" then
      self.shape = circleshape(0.5 * (self.v[1] + self.v[2]), u:len() / 2.0)
    else
      self.shape = circleshape(self.v[1], u:len())
    end
    return
  end
  local alpha = u:angle()
  local beta = (self.v[3] - self.v[1]):angle()
  if self.mode == "arc1" then
    self.shape = arcshape(self.v[1], u:len(), alpha, beta)
  elseif self.mode == "arc2" then
    self.shape = rarcshape(self.v[1], u:len(), alpha, beta)
  else
    local l1 = ipe.LineThrough(self.v[1], self.v[2])
    if math.abs(l1:side(self.v[3])) < 1e-9 or self.v[3] == self.v[2] then
      self.shape = segmentshape(self.v[1], self.v[2])
    else
      local l2 = ipe.LineThrough(self.v[2], self.v[3])
      local bi1 = ipe.Line(0.5 * (self.v[1] + self.v[2]), l1:normal())
      local bi2 = ipe.Line(0.5 * (self.v[2] + self.v[3]), l2:normal())
      local center = bi1:intersects(bi2)
      u = self.v[1] - center
      alpha = u:angle()
      beta = ipe.normalizeAngle((self.v[2] - center):angle(), alpha)
      local gamma = ipe.normalizeAngle((self.v[3] - center):angle(), alpha)
      if gamma > beta then
	self.shape = arcshape(center, u:len(), alpha, gamma)
      else
	self.shape = rarcshape(center, u:len(), alpha, gamma)
      end
    end
  end
end

function ARCTOOL:mouseButton(button, modifiers, press)
  if not press then return end
  local v = self.model.ui:pos()
  -- refuse point identical to previous
  if v == self.v[self.cur - 1] then return end
  self.v[self.cur] = v
  self:compute()
  if self.cur == 3 then
    self.model.ui:finishTool()
    local obj = ipe.Path(self.model.attributes, { self.shape }, true)
    self.model:creation("create arc", obj)
  else
    self.cur = self.cur + 1
    self.model.ui:update(false)
  end
end

function ARCTOOL:mouseMove()
  self.v[self.cur] = self.model.ui:pos()
  self:compute()
  self.setShape({ self.shape })
  self.model.ui:update(false) -- update tool
end

function ARCTOOL:key(code, modifiers, text)
  if text == "\027" then
    self.model.ui:finishTool()
    return true
  else
    return false
  end
end

----------------------------------------------------------------------

INKTOOL = {}
INKTOOL.__index = INKTOOL

function INKTOOL:new(model)
  local tool = {}
  setmetatable(tool, INKTOOL)
  tool.model = model
  local v = model.ui:pos()
  tool.v = { v }
  model.ui:shapeTool(tool)
  local s = model.doc:sheets():find("color", model.attributes.stroke)
  tool.setColor(s.r, s.g, s.b)
  tool.w = model.doc:sheets():find("pen", model.attributes.pen)
  model.ui:setCursor(tool.w, s.r, s.g, s.b)
  return tool
end

function INKTOOL:compute()
  self.shape = { type="curve", closed=false }
  for i = 2, #self.v do
    self.shape[#self.shape + 1] = { type="segment", self.v[i-1], self.v[i] }
  end
  self.setShape( { self.shape }, 0, self.w * self.model.ui:zoom())
end

function INKTOOL:mouseButton(button, modifiers, press)
  self.model.ui:finishTool()
  if self.shape then
     local obj = ipe.Path(self.model.attributes, { self.shape })
     -- round linecaps are prettier for handwriting
     obj:set("linecap", "round")
     self.model:creation("create ink path", obj)
  else
     -- mouse was pressed and released without movement
     local shape = { type="curve", closed=false,
		     { type="segment", self.v[1], self.v[1] } }
     local obj = ipe.Path(self.model.attributes, { shape })
     -- must make round linecap, otherwise it will be invisible
     obj:set("linecap", "round")
     self.model:creation("create ink dot", obj)
  end
end

function INKTOOL:mouseMove()
  self.v[#self.v + 1] = self.model.ui:pos()
  self:compute()
  self.model.ui:update(false) -- update tool
end

function INKTOOL:key(code, modifiers, text)
  if text == "\027" then
    self.model.ui:finishTool()
    return true
  else
    return false
  end
end

----------------------------------------------------------------------

function MODEL:shredObject()
  local bound = prefs.close_distance
  local pos = self.ui:unsnappedPos()
  local p = self:page()

  local closest
  for i,obj,sel,layer in p:objects() do
     if p:visible(self.vno, i) and not p:isLocked(layer) then
	local d = p:distance(i, pos, bound)
	if d < bound then closest = i; bound = d end
     end
  end

  if closest then
    local t = { label="shred object",
		pno = self.pno,
		vno = self.vno,
		num = closest,
		original=self:page():clone(),
		undo=revertOriginal,
	      }
    t.redo = function (t, doc)
	       local p = doc[t.pno]
	       p:remove(t.num)
	       p:ensurePrimarySelection()
	     end
    self:register(t)
  else
    self.ui:explain("No object found to shred")
  end
end

----------------------------------------------------------------------

function MODEL:createMark()
  local obj = ipe.Reference(self.attributes, self.attributes.markshape,
			    self.ui:pos())
  self:creation("create mark", obj)
end

----------------------------------------------------------------------

function MODEL:createText(mode)
  local prompt
  local pos = self.ui:pos()
  if mode == "math" then
    prompt = "Enter Latex source for math formula"
  else
    prompt = "Enter Latex source"
  end
  local d = ipeui.Dialog(self.ui:win(), "Create text object")
  d:add("label", "label", { label=prompt }, 1, 1)
  d:add("text", "text", { syntax="latex" }, 2, 1)
  addEditorField(d, "text")
  d:addButton("cancel", "&Cancel", "reject")
  d:addButton("ok", "&Ok", "accept")
  d:setStretch("row", 2, 1)
  d:set("ignore-cancel", true)
  if prefs.auto_external_editor then
    externalEditor(d, "text")
  end
  if ((prefs.auto_external_editor and prefs.editor_closes_dialog)
    or d:execute(prefs.editor_size)) then
    local t = d:get("text")
    if mode == "math" then
      t = "$" .. t .. "$"
    end
    local obj = ipe.Text(self.attributes, t, pos)
    self:creation("create text", obj)
    self:autoRunLatex()
  end
end

----------------------------------------------------------------------

function MODEL:createParagraph(pos, width, pinned)
  local styles = self.doc:sheets():allNames("textstyle")
  local sizes = self.doc:sheets():allNames("textsize")
  local d = ipeui.Dialog(self.ui:win(), "Create text object")
  d:add("label", "label", { label="Enter latex source" }, 1, 1)
  d:add("style", "combo", styles, 1, 3)
  d:add("size", "combo", sizes, 1, 4)
  d:add("text", "text", { syntax="latex" }, 2, 1, 1, 4)
  addEditorField(d, "text")
  d:addButton("cancel", "&Cancel", "reject")
  d:addButton("ok", "&Ok", "accept")
  d:setStretch("row", 2, 1)
  d:setStretch("column", 2, 1)
  d:set("ignore-cancel", true)
  local style = indexOf(self.attributes.textstyle, styles)
  if not style then style = indexOf("normal", styles) end
  local size = indexOf(self.attributes.textsize, sizes)
  if not size then size = indexOf("normal", sizes) end
  d:set("style", style)
  d:set("size", size)
  if prefs.auto_external_editor then
    externalEditor(d, "text")
  end
  if ((prefs.auto_external_editor and prefs.editor_closes_dialog)
    or d:execute(prefs.editor_size)) then
    local t = d:get("text")
    local style = styles[d:get("style")]
    local size = sizes[d:get("size")]
    local obj = ipe.Text(self.attributes, t, pos, width)
    obj:set("textsize", size)
    obj:set("textstyle", style)
    if pinned then
      obj:set("pinned", "horizontal")
    end
    self:creation("create text paragraph", obj)
    self:autoRunLatex()
  end
end

function MODEL:action_insert_text_box()
  local layout = self.doc:sheets():find("layout")
  local p = self:page()
  local r = ipe.Rect()
  local m = ipe.Matrix()
  for i, obj, sel, layer in p:objects() do
    if p:visible(self.vno, i) and obj:type() == "text" then
      obj:addToBBox(r, m)
    end
  end
  local y = layout.framesize.y
  if not r:isEmpty() and r:bottom() < layout.framesize.y then
    y = r:bottom() - layout.paragraph_skip
  end
  self:createParagraph(ipe.Vector(0, y), layout.framesize.x, true)
end

----------------------------------------------------------------------

PARAGRAPHTOOL = {}
PARAGRAPHTOOL.__index = PARAGRAPHTOOL

function PARAGRAPHTOOL:new(model)
  local tool = {}
  setmetatable(tool, PARAGRAPHTOOL)
  tool.model = model
  local v = model.ui:pos()
  tool.v = { v, v }
  model.ui:shapeTool(tool)
  tool.setColor(1.0, 0, 1.0)
  return tool
end

function PARAGRAPHTOOL:compute()
  self.v[2] = V(self.model.ui:pos().x, self.v[1].y - 20)
  self.shape = boxshape(self.v[1], self.v[2])
end

function PARAGRAPHTOOL:mouseButton(button, modifiers, press)
  if not press then return end
  self:compute()
  self.model.ui:finishTool()
  local pos = V(math.min(self.v[1].x, self.v[2].x), self.v[1].y)
  local wid = math.abs(self.v[2].x - self.v[1].x)
  self.model:createParagraph(pos, wid)
end

function PARAGRAPHTOOL:mouseMove()
  self:compute()
  self.setShape( { self.shape } )
  self.model.ui:update(false) -- update tool
end

function PARAGRAPHTOOL:key(code, modifiers, text)
  if text == "\027" then
    self.model.ui:finishTool()
    return true
  else
    return false
  end
end

----------------------------------------------------------------------

CHANGEWIDTHTOOL = {}
CHANGEWIDTHTOOL.__index = CHANGEWIDTHTOOL

function CHANGEWIDTHTOOL:new(model, prim, obj)
  local tool = {}
  setmetatable(tool, CHANGEWIDTHTOOL)
  tool.model = model
  tool.prim = prim
  tool.obj = obj
  tool.pos = obj:matrix() * obj:position()
  tool.wid = obj:get("width")
  model.ui:shapeTool(tool)
  tool.setColor(1.0, 0, 1.0)
  tool.setShape( { boxshape(tool.pos, tool.pos + V(tool.wid, -20)) } )
  model.ui:update(false) -- update tool
  return tool
end

function CHANGEWIDTHTOOL:compute()
  local w = self.model.ui:pos()
  self.nwid = self.wid + (w.x - self.v.x)
  self.shape = boxshape(self.pos, self.pos + V(self.nwid, -20))
end

function CHANGEWIDTHTOOL:mouseButton(button, modifiers, press)
  if press then
    if not self.v then self.v = self.model.ui:pos() end
  else
    self:compute()
    self.model.ui:finishTool()
    self.model:setAttributeOfPrimary(self.prim, "width", self.nwid)
    self.model:autoRunLatex()
  end
end

function CHANGEWIDTHTOOL:mouseMove()
  if self.v then
    self:compute()
    self.setShape( { self.shape } )
    self.model.ui:update(false) -- update tool
  end
end

function CHANGEWIDTHTOOL:key(code, modifiers, text)
  if text == "\027" then
    self.model.ui:finishTool()
    return true
  else
    return false
  end
end

function MODEL:action_change_width()
  local p = self:page()
  local prim = p:primarySelection()
  if not prim or p[prim]:type() ~= "text" or not p[prim]:get("minipage") then
    self.ui:explain("no selection or not a minipage object")
    return
  end
  CHANGEWIDTHTOOL:new(self, prim, p[prim])
end

----------------------------------------------------------------------

function MODEL:startTransform(mode, withShift)
  self:updateCloseSelection(false)
  if mode == "stretch" and withShift then mode = "scale" end
  self.ui:transformTool(self:page(), self.vno, mode, withShift,
			function (m) self:transformation(mode, m) end)
end

function MODEL:startModeTool(modifiers)
  if self.mode == "select" then
    self.ui:selectTool(self:page(), self.vno,
		       prefs.select_distance, modifiers.shift)
  elseif (self.mode == "translate" or self.mode == "stretch"
	  or self.mode == "rotate") then
    self:startTransform(self.mode, modifiers.shift)
  elseif self.mode == "pan" then
    self.ui:panTool(self:page(), self.vno)
  elseif self.mode == "shredder" then
    self:shredObject()
  elseif self.mode == "rectangles" then
    BOXTOOL:new(self, modifiers.shift)
  elseif self.mode == "splinegons" then
    SPLINEGONTOOL:new(self)
  elseif (self.mode == "lines" or self.mode == "polygons" or
	  self.mode == "splines") then
    LINESTOOL:new(self, self.mode)
  elseif self.mode:sub(1,6) == "circle" then
    CIRCLETOOL:new(self, self.mode)
  elseif self.mode:sub(1,3) == "arc" then
    ARCTOOL:new(self, self.mode)
  elseif self.mode == "ink" then
    INKTOOL:new(self)
  elseif self.mode == "marks" then
    self:createMark()
  elseif self.mode == "label" or self.mode == "math" then
    self:createText(self.mode)
  elseif self.mode == "paragraph" then
    PARAGRAPHTOOL:new(self)
  else
    print("start mode tool:", self.mode)
  end
end

local mouse_mappings = {
  select = function (m, mo)
	     m.ui:selectTool(m:page(), m.vno, prefs.select_distance, mo.shift)
	   end,
  translate = function (m, mo) m:startTransform("translate", mo.shift) end,
  rotate = function (m, mo) m:startTransform("rotate", mo.shift) end,
  stretch = function (m, mo) m:startTransform("stretch", mo.shift) end,
  scale = function (m, mo) m:startTransform("stretch", true) end,
  pan = function (m, mo) m.ui:panTool(m:page(), m.vno) end,
  menu = function (m, mo) m:propertiesPopup() end,
  shredder = function (m, mo) m:shredObject() end,
}

function MODEL:mouseButtonAction(button, modifiers)
  -- print("Mouse button", button, modifiers.alt, modifiers.control)
  if button == 1 and not modifiers.alt and
    not modifiers.control and not modifiers.meta then
    self:startModeTool(modifiers)
  else
    local s = ""
    if button == 1 then s = "left"
    elseif button == 2 then s = "right"
    elseif button == 4 then s = "middle"
    elseif button == 8 then s = "button8"
    elseif button == 16 then s = "button9"
    elseif button == 0 then
      -- This is a hack because of the Qt limitation.
      -- It really means any other button.
      s = "button10"
    end
    if modifiers.shift then s = s .. "_shift" end
    if modifiers.control then s = s .. "_control" end
    if modifiers.alt then s = s .. "_alt" end
    if modifiers.meta then s = s .. "_meta" end
    local r = mouse[s]
    if type(r) == "string" then r = mouse_mappings[r] end
    if r then
      r(self, modifiers)
    else
      print("No mouse action defined for " .. s)
    end
  end
end

----------------------------------------------------------------------

function apply_text_edit(d, data, run_latex)
  -- refuse to do anything with empty text
  if string.match(d:get("text"), "^%s*$") then return end
  local model = data.model
  local final = data.obj:clone()
  final:setText(d:get("text"))
  if data.style then final:set("textstyle", data.styles[d:get("style")]) end
  if data.size then final:set("textsize", data.sizes[d:get("size")]) end
  local t = { label="edit text",
	      pno=model.pno,
	      vno=model.vno,
	      original=data.obj:clone(),
	      primary=data.prim,
	      final=final,
	    }
  t.undo = function (t, doc)
	     doc[t.pno]:replace(t.primary, t.original)
	   end
  t.redo = function (t, doc)
	     doc[t.pno]:replace(t.primary, t.final)
	   end
  model:register(t)
  -- need to update data.obj for the next run!
  data.obj = final
  if run_latex then model:runLatex() end
end

function MODEL:action_edit_text(prim, obj)
  local mp = obj:get("minipage")
  local d = ipeui.Dialog(self.ui:win(), "Edit text object")
  local data = { model=self,
		 styles = self.doc:sheets():allNames("textstyle"),
		 sizes = self.doc:sheets():allNames("textsize"),
		 prim=prim,
		 obj=obj,
	       }
  d:add("label", "label", { label="Edit latex source" }, 1, 1)
  d:add("text", "text", { syntax="latex" }, 2, 1, 1, 4)
  d:addButton("apply", "&Apply",
	      function (d) apply_text_edit(d, data, true) end)
  addEditorField(d, "text")
  d:addButton("cancel", "&Cancel", "reject")
  d:addButton("ok", "&Ok", "accept")
  d:setStretch("row", 2, 1)
  d:setStretch("column", 2, 1)
  d:set("text", obj:text())
  d:set("ignore-cancel", true)
  local style = nil
  local size = nil
  if mp then
    data.style = indexOf(obj:get("textstyle"), data.styles)
    if data.style then
      d:add("style", "combo", data.styles, 1, 3)
      d:set("style", data.style)
    end
    data.size = indexOf(obj:get("textsize"), data.sizes)
    if data.size then
      d:add("size", "combo", data.sizes, 1, 4)
      d:set("size", data.size)
    end
  end
  if prefs.auto_external_editor then
    externalEditor(d, "text")
  end
  if ((prefs.auto_external_editor and prefs.editor_closes_dialog)
    or d:execute(prefs.editor_size)) then
    if string.match(d:get("text"), "^%s*$") then return end
    apply_text_edit(d, data, self.auto_latex)
  end
end

function MODEL:action_edit()
  local p = self:page()
  local prim = p:primarySelection()
  if not prim then self.ui:explain("no selection") return end
  local obj = p[prim]
  if obj:type() == "text" then
    self:action_edit_text(prim, obj)
  elseif obj:type() == "path" then
    self:action_edit_path(prim, obj)
  else
    self:warning("Cannot edit " .. obj:type() .. " object",
		 "Only text objects and path objects can be edited")
  end
end

----------------------------------------------------------------------

PASTETOOL = {}
PASTETOOL.__index = PASTETOOL

function PASTETOOL:new(model, elements, pos)
  local tool = {}
  _G.setmetatable(tool, PASTETOOL)
  tool.model = model
  tool.elements = elements
  tool.start = model.ui:pos()
  if pos then tool.start = pos end
  local obj = ipe.Group(elements)
  tool.pinned = obj:get("pinned")
  model.ui:pasteTool(obj, tool)
  tool.setColor(1.0, 0, 0)
  tool:computeTranslation()
  tool.setMatrix(ipe.Translation(tool.translation))
  return tool
end

function PASTETOOL:computeTranslation()
  self.translation = self.model.ui:pos() - self.start
  if self.pinned == "horizontal" or self.pinned == "fixed" then
    self.translation = V(0, self.translation.y)
  end
  if self.pinned == "vertical" or self.pinned == "fixed" then
    self.translation = V(self.translation.x, 0)
  end
end

function PASTETOOL:mouseButton(button, modifiers, press)
  self:computeTranslation()
  self.model.ui:finishTool()
  local t = { label="paste objects at cursor",
	      pno = self.model.pno,
	      vno = self.model.vno,
	      elements = self.elements,
	      layer = self.model:page():active(self.model.vno),
	      translation = ipe.Translation(self.translation),
	    }
  t.undo = function (t, doc)
	     local p = doc[t.pno]
	     for i = 1,#t.elements do p:remove(#p) end
	   end
  t.redo = function (t, doc)
	     local p = doc[t.pno]
	     for i,obj in ipairs(t.elements) do
	       p:insert(nil, obj, 2, t.layer)
	       p:transform(#p, t.translation)
	     end
	     p:ensurePrimarySelection()
	   end
  self.model:page():deselectAll()
  self.model:register(t)
end

function PASTETOOL:mouseMove()
  self:computeTranslation()
  self.setMatrix(ipe.Translation(self.translation))
  self.model.ui:update(false) -- update tool
end

function PASTETOOL:key(code, modifiers, text)
  if text == "\027" then
    self.model.ui:finishTool()
    return true
  else
    return false
  end
end

function MODEL:action_paste_at_cursor()
  local data = ipeui.clipboard()
  if data:sub(1,13) ~= "<ipeselection" then
    self:warning("No Ipe selection to paste")
    return
  end
  local pos
  local px, py = data:match('^<ipeselection pos="([%d%.]+) ([%d%.]+)"')
  if px then pos = V(tonumber(px), tonumber(py)) end
  local elements = ipe.Object(data)
  if not elements then
    self:warning("Could not parse Ipe selection on clipboard")
    return
  end
  PASTETOOL:new(self, elements, pos)
end

----------------------------------------------------------------------
