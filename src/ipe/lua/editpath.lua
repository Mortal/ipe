----------------------------------------------------------------------
-- editpath.lua
----------------------------------------------------------------------
--[[

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

--]]

EDITTOOL = {}
EDITTOOL.__index = EDITTOOL

local VERTEX = 1
local SPLINECP = 2
local BEZIERCP = 3
local CENTER = 4
local RADIUS = 5
local MINOR = 6
local CURRENT = 7
local SCISSOR = 8

----------------------------------------------------------------------

function revertSubpath(t, shape)
  shape[t.spNum] = t.original
end

-- clone a subpath
-- reuses the same CP (Vector) objects
function cloneSubpath(sp)
  local nsp = {}
  for n in pairs(sp) do
    local t = sp[n]
    if type(t) == "table" then
      local nt = {}
      for nn in pairs(t) do nt[nn] = t[nn] end
      t = nt
    end
    nsp[n] = t
  end
  return nsp
end

local function recomputeMatrix(seg, cpno)
  local alpha, beta = seg.arc:angles()
  local p = ipe.Direction(alpha)
  local q = ipe.Direction(beta)
  local cl
  if cpno == 1 then
    cl = ipe.LineThrough(V(0,0), q)
    p = seg.arc:matrix():inverse() * seg[cpno]
  else -- cpno == 2
    cl = ipe.LineThrough(V(0,0), p)
    q = seg.arc:matrix():inverse() * seg[cpno]
  end
  local center = ipe.Bisector(p, q):intersects(cl)
  if center then
    local radius = (p - center):len()
    alpha = (p - center):angle()
    beta = (q - center):angle()
    local m = (seg.arc:matrix() *
	     ipe.Matrix(radius, 0, 0, radius, center.x, center.y))
    seg.arc = ipe.Arc(m, alpha, beta)
  end
end

function moveCP(shape, spno, segno, cpno, v)
  local sp = shape[spno]
  if sp.type == "closedspline" then
    sp[cpno] = v
  elseif sp.type == "ellipse" then
    if cpno == 0 then -- center
      local m = sp[1]:elements()
      m[5] = v.x
      m[6] = v.y
      sp[1] = ipe.Matrix(m)
    elseif cpno == 1 then -- radius
      local u = sp[1]:inverse() * v
      local radius = u:len()
      local alpha = u:angle()
      local m = (sp[1] * ipe.Matrix(radius, 0, 0, radius, 0, 0)
	       * ipe.Rotation(alpha))
      sp[1] = m
    elseif cpno == 2 then -- minor
      local u = sp[1]:inverse() * v
      local m = sp[1] * ipe.Matrix(1, 0, u.x, u.y, 0, 0)
      sp[1] = m
    end
  else -- curve
    local seg = sp[segno]
    if cpno == 0 then
      -- center of arc
      local v1 = seg.arc:matrix():inverse() * v
      local alpha, beta = seg.arc:angles()
      local p, q = ipe.Direction(alpha), ipe.Direction(beta)
      local l = ipe.Bisector(p, q)
      local center = l:project(v1)
      local radius = (p - center):len()
      local m = ipe.Matrix(radius, 0, 0, radius, center.x, center.y)
      m = seg.arc:matrix() * m
      seg.arc = ipe.Arc(m, (p - center):angle(), (q - center):angle())
    else
      seg[cpno] = v
      if cpno == 1 and segno > 1 then
	local pseg = sp[segno - 1]
	pseg[#pseg] = v
      elseif cpno == #seg and segno < #sp then
	sp[segno + 1][1] = v
      end
      if seg.type == "arc" then recomputeMatrix(seg, cpno) end
      if segno > 1 and cpno == 1 and sp[segno - 1].type == "arc" then
	recomputeMatrix(sp[segno - 1], 2)
      end
      if segno < #sp and cpno == #seg and sp[segno + 1].type == "arc" then
	recomputeMatrix(sp[segno + 1], 1)
      end
    end
  end
end

----------------------------------------------------------------------

function EDITTOOL:new(model, prim, obj)
  local tool = {}
  setmetatable(tool, EDITTOOL)
  tool.model = model
  tool.undo = {}
  tool.primary = prim
  tool.obj = obj
  tool.shape = obj:shape()
  transformShape(obj:matrix(), tool.shape)
  model.ui:shapeTool(tool)
  tool.setColor(1.0, 0, 0)
  tool:resetCP()
  tool:setShapeMarks()
  return tool
end

----------------------------------------------------------------------

function EDITTOOL:resetCP()
  self.spNum = 1
  self.segNum = 1
  self.cpNum = 1
  if self.shape[1].type == "ellipse" then self.cpNum = 0 end
end

function EDITTOOL:type()
  local cur = self.shape[self.spNum]
  if cur.type == "closedspline" then return SPLINECP end
  if self.cpNum == 0 then return CENTER end
  if cur.type == "ellipse" then
    if self.cpNum == 1 then return RADIUS else return MINOR end
  end
  cur = cur[self.segNum]
  if self.cpNum == 1 or self.cpNum == #cur then return VERTEX end
  if cur.type == "spline" then return SPLINECP end
  if cur.type == "bezier" or cur.type == "quad" then return BEZIERCP end
  -- must be center of arc
  return CENTER
end

function EDITTOOL:current()
  local cur = self.shape[self.spNum]
  if cur.type == "ellipse" then
    if self.cpNum == 0 then
      return cur[1]:translation()
    elseif self.cpNum == 1 then
      return cur[1] * V(1,0)
    elseif self.cpNum == 2 then
      return cur[1] * V(0,1)
    end
  end
  if cur.type == "curve" then
    cur = cur[self.segNum]
  end
  if self.cpNum == 0 then
    cur = cur.arc:matrix():translation()
  else
    cur = cur[self.cpNum]
  end
  return cur
end

function EDITTOOL:canDelete()
  local t = self:type()
  if t == SPLINECP then return true end
  if t == VERTEX then
    local sp = self.shape[self.spNum]
    if sp[self.segNum].type ~= "segment" then return false end
    if self.cpNum == 1 then
      return self.segNum == 1 or sp[self.segNum - 1].type == "segment"
    end
    if self.cpNum == 2 then
      return self.segNum == #sp or sp[self.segNum + 1].type == "segment"
    end
  end
  return false
end

----------------------------------------------------------------------

function EDITTOOL:setShapeMarks()
  local m = {}
  local aux = {}
  for _, sp in ipairs(self.shape) do
    if sp.type == "closedspline" then
      for _,cp in ipairs(sp) do
	m[#m + 1] = cp
	m[#m + 1] = SPLINECP
      end
      local as = { type="curve", closed=true }
      for i = 2,#sp do
	as[#as + 1] = { type="segment", sp[i-1], sp[i] }
      end
      aux[#aux + 1] = as
    elseif sp.type == "curve" then
      m[#m + 1] = sp[1][1]
      m[#m + 1] = VERTEX
      for _,seg in ipairs(sp) do
	m[#m + 1] = seg[#seg]
	m[#m + 1] = VERTEX
	if seg.type=="quad" or seg.type=="bezier" or seg.type=="spline" then
	  for i = 2,#seg-1 do
	    m[#m + 1] = seg[i]
	    m[#m + 1] = seg.type=="spline" and SPLINECP or BEZIERCP
	  end
	  local as = { type="curve", closed=false }
	  for i = 2,#seg do
	    as[#as + 1] = { type="segment", seg[i-1], seg[i] }
	  end
	  aux[#aux + 1] = as
	elseif seg.type == "arc" then
	  m[#m + 1] = seg.arc:matrix():translation()
	  m[#m + 1] = CENTER
	end
      end
    else -- ellipse
      m[#m + 1] = sp[1]:translation()
      m[#m + 1] = CENTER
      m[#m + 1] = sp[1] * V(1, 0)
      m[#m + 1] = RADIUS
      m[#m + 1] = sp[1] * V(0, 1)
      m[#m + 1] = MINOR
    end
  end
  m[#m + 1] = self:current()
  m[#m + 1] = CURRENT
  if self.scissor then
    m[#m + 1] = self.scissorPos
    m[#m + 1] = SCISSOR
  end
  self.setShape(self.shape)
  self.setShape(aux, 1)
  self.setMarks(m)
end

function EDITTOOL:find(v)
  local bd = 1000000  -- maybe init with current vertex
  local spnum, segnum, cpnum
  for i,sp in ipairs(self.shape) do
    if sp.type == "closedspline" then
      for j,cp in ipairs(sp) do
	if (v-cp):sqLen() < bd then
	  bd = (v-cp):sqLen()
	  spnum = i
	  segnum = 1
	  cpnum = j
	end
      end
    elseif sp.type == "ellipse" then
      local cp = sp[1]:translation()
      if (v-cp):sqLen() < bd then
	bd = (v-cp):sqLen()
	spnum = i
	segnum = 1
	cpnum = 0
      end
      cp = sp[1] * V(1,0)
      if (v-cp):sqLen() < bd then
	bd = (v-cp):sqLen()
	spnum = i
	segnum = 1
	cpnum = 1
      end
      cp = sp[1] * V(0,1)
      if (v-cp):sqLen() < bd then
	bd = (v-cp):sqLen()
	spnum = i
	segnum = 1
	cpnum = 2
      end
    else -- curve
      for j,seg in ipairs(sp) do
	for k,cp in ipairs(seg) do
	  if (v-cp):sqLen() < bd then
	    bd = (v-cp):sqLen()
	    spnum = i
	    segnum = j
	    cpnum = k
	  end
	end
	if seg.type == "arc" then
	  local cp = seg.arc:matrix():translation()
	  if (v-cp):sqLen() < bd then
	    bd = (v-cp):sqLen()
	    spnum = i
	    segnum = j
	    cpnum = 0
	  end
	end
      end
    end
  end
  return spnum, segnum, cpnum
end

----------------------------------------------------------------------

function EDITTOOL:action_delete_subpath()
  local t = { label = "delete subpath",
	      spNum = self.spNum,
	      original = table.remove(self.shape, self.spNum),
	      undo = function (t, shape)
		       table.insert(shape, t.spNum, t.original)
		     end
	    }
  self.undo[#self.undo + 1] = t
  self:resetCP()
end

function EDITTOOL:action_delete()
  local sp = self.shape[self.spNum]
  local t = { label = "delete control point",
	      spNum = self.spNum,
	      original = cloneSubpath(sp),
	      undo = revertSubpath }
  if sp.type == "closedspline" then
    -- cannot have closed spline with less than 3 cp
    if #sp < 4 then return end
    table.remove(sp, self.cpNum)
  elseif sp.type == "curve" then
    local seg = sp[self.segNum]
    if 1 < self.cpNum and self.cpNum < #seg then
      -- must be SPLINECP
      table.remove(seg, self.cpNum)
      if #seg == 2 then seg.type = "segment" end
    else -- VERTEX
      assert(seg.type == "segment")
      -- cannot delete from last segment or from last triangle
      if #sp == 1 or sp.closed and #sp == 2 then return end
      if self.cpNum == 1 and self.segNum == 1 then
	table.remove(sp, 1)
      elseif self.cpNum == 2 and self.segNum == #sp then
	table.remove(sp, #sp)
      else
	-- not first vertex, not last vertex
	if self.cpNum == 1 then
	  self.segNum = self.segNum - 1
	  seg = sp[self.segNum]
	  assert(seg.type == "segment")
	  self.cpNum = 2
	end
	assert(self.cpNum == 2)
	sp[self.segNum + 1][1] = seg[1]
	table.remove(sp, self.segNum)
      end
    end
  end
  self.undo[#self.undo + 1] = t
  self:resetCP()
end

function EDITTOOL:action_duplicate()
  local sp = self.shape[self.spNum]
  local t = { label = "duplicate control point",
	      spNum = self.spNum,
	      original = cloneSubpath(sp),
	      undo = revertSubpath }
  if sp.type == "closedspline" then
    local cp = sp[self.cpNum]
    table.insert(sp, self.cpNum, cp)
    local pcp, ncp
    if self.cpNum > 1 then pcp = sp[self.cpNum - 1] else pcp = sp[#sp] end
    if self.cpNum < #sp-1 then ncp = sp[self.cpNum + 2] else ncp = sp[1] end
    sp[self.cpNum] = 0.9 * cp + 0.1 * pcp
    sp[self.cpNum + 1] = 0.9 * cp + 0.1 * ncp
  else
    assert(sp.type == "curve")
    local seg = sp[self.segNum]
    local cp = seg[self.cpNum]
    if 1 < self.cpNum and self.cpNum < #seg then
      assert(seg.type == "spline")
      table.insert(seg, self.cpNum, cp)
      local pcp = seg[self.cpNum - 1]
      local ncp = seg[self.cpNum + 2]
      seg[self.cpNum] = 0.9 * cp + 0.1 * pcp
      seg[self.cpNum + 1] = 0.9 * cp + 0.1 * ncp
    else
      if self.cpNum == #sp[self.segNum] then
	self.segNum = self.segNum + 1
	self.cpNum = 1
      end
      assert(self.cpNum == 1)
      local seg = { type="segment", cp, cp }
      table.insert(sp, self.segNum, seg)
      if self.segNum > 1 then
	local pseg = sp[self.segNum - 1]
	seg[1] = 0.9 * cp + 0.1 * pseg[#pseg - 1]
	pseg[#pseg] = seg[1]
      end
      if self.segNum < #sp then
	local nseg = sp[self.segNum + 1]
	seg[2] = 0.9 * cp + 0.1 * nseg[2]
	nseg[1] = seg[2]
      end
    end
  end
  self.undo[#self.undo + 1] = t
end

function EDITTOOL:action_cut()
  local sp = self.shape[self.spNum]
  local t = { label = "cut at vertex",
	      spNum = self.spNum,
	      original = cloneSubpath(sp),
	      undo = function (t, shape)
		       shape[t.spNum] = t.original
		       if t.newSpNum then
			 table.remove(shape, t.newSpNum)
		       end
		     end
	    }
  assert(sp.type == "curve")
  if not sp.closed then
    -- cannot cut at first or last vertex
    if self.segNum == 1 and self.cpNum == 1 then return end
    if self.segNum == #sp and self.cpNum ~= 1 then return end
    if self.cpNum ~= 1 then
      self.segNum = self.segNum + 1
      self.cpNum = 1
    end
    local nsp = { type = "curve", closed = false }
    for i = self.segNum,#sp do nsp[#nsp + 1] = sp[i] end
    for i = #sp,self.segNum,-1 do table.remove(sp, i) end
    self.spNum = self.spNum + 1
    t.newSpNum = self.spNum
    table.insert(self.shape, t.newSpNum, nsp)
    self.segNum = 1
  else -- sp is closed
    sp.closed = false
    local lastseg = sp[#sp]
    local newseg = { type = "segment", lastseg[#lastseg], sp[1][1] }
    if self.segNum == 1 and self.cpNum == 1 then
      sp[#sp + 1] = newseg
    elseif self.segNum == #sp and self.cpNum ~= 1 then
      table.insert(sp, 1, newseg)
    else -- cutting somewhere else
      if self.cpNum ~= 1 then
	self.segNum = self.segNum + 1
	self.cpNum = 1
      end
      sp[#sp + 1] = newseg
      -- move self.segNum - 1 segments from front to back
      for i=1,self.segNum - 1 do
	local seg = table.remove(sp, 1)
	sp[#sp + 1] = seg
      end
    end
    self.segNum = 1
    self.cpNum = 1
  end
end

function EDITTOOL:action_convert_spline()
  local sp = self.shape[self.spNum]
  local t = { label = "convert spline to Beziers",
	      spNum = self.spNum,
	      original = cloneSubpath(sp),
	      undo = revertSubpath }
  if sp.type == "closedspline" then
    local beziers = ipe.splineToBeziers(sp, true)
    beziers.type = "curve"
    beziers.closed = false
    self.shape[self.spNum] = beziers
  else
    local beziers = ipe.splineToBeziers(sp[self.segNum], false)
    table.remove(sp, self.segNum)
    for i=1,#beziers do
      table.insert(sp, self.segNum + i - 1, beziers[i])
    end
  end
  self.undo[#self.undo + 1] = t
end

----------------------------------------------------------------------

-- subdivide at parameter t (0 <= t <= 1)
local function subdivideBezier(bez, t)
  local tt = 1 - t
  local l = {}
  local r = {}
  l[1] = bez[1];
  l[2] = tt * bez[1] + t * bez[2]
  local h = tt * bez[2] + t * bez[3]
  l[3] = tt * l[2] + t * h
  r[3] = tt * bez[3] + t * bez[4]
  r[2] = tt * h + t * r[3]
  r[1] = tt * l[3] + t * r[2]
  l[4] = r[1]
  r[4] = bez[4]
  return l, r
end

-- subdivide at parameter t (0 <= t <= 1)
local function subdivideQuad(bez, t)
  local tt = 1 - t
  local l = {}
  local r = {}
  l[1] = bez[1];
  l[2] = tt * bez[1] + t * bez[2]
  r[2] = tt * bez[2] + t * bez[3]
  r[1] = tt * l[2] + t * r[2]
  l[3] = r[1]
  r[3] = bez[3]
  return l, r
end

function EDITTOOL:cutter_ellipse()
  local sp = self.shape[self.spNum]
  local t = { label = "cut ellipse",
	      spNum = self.spNum,
	      original = cloneSubpath(sp),
	      undo = revertSubpath,
	    }
  local alpha, _ = self.scissor(self.model.ui:pos())
  sp.type = "curve"
  sp.closed = false
  local arc = ipe.Arc(sp[1], alpha, alpha - 0.2)
  local cp1, cp2 = arc:endpoints()
  sp[1] = { type = "arc", arc = arc, cp1, cp2 }
  self.undo[#self.undo + 1] = t
end

function EDITTOOL:cutter_arc()
  local sp = self.shape[self.spNum]
  local t = { label = "cut arc",
	      spNum = self.spNum,
	      original = cloneSubpath(sp),
	      undo = function (t, shape)
		       shape[t.spNum] = t.original
		       if t.newSpNum then
			 table.remove(shape, t.newSpNum)
		       end
		     end
	    }
  local nalpha, _ = self.scissor(self.model.ui:pos())
  local seg = sp[self.segNum]
  local alpha, beta = seg.arc:angles()
  local narc1 = ipe.Arc(seg.arc:matrix(), alpha, nalpha)
  local narc2 = ipe.Arc(seg.arc:matrix(), nalpha, beta)
  local npos, _ = narc2:endpoints()
  local nseg1 = { type = "arc", arc = narc1, seg[1], npos }
  local nseg2 = { type = "arc", arc = narc2, npos, seg[2] }
  if not sp.closed then
    local nsp = { type = "curve", closed = false, nseg2 }
    for i = self.segNum+1,#sp do nsp[#nsp + 1] = sp[i] end
    for i = #sp,self.segNum+1,-1 do table.remove(sp, i) end
    sp[self.segNum] = nseg1
    t.newSpNum = self.spNum + 1
    table.insert(self.shape, t.newSpNum, nsp)
  else -- sp is closed
    sp.closed = false
    local lastseg = sp[#sp]
    local nsp = { type = "curve", closed = false, nseg2 }
    for i = self.segNum+1,#sp do nsp[#nsp + 1] = sp[i] end
    nsp[#nsp + 1] = { type = "segment", lastseg[#lastseg], sp[1][1] }
    for i = 1,self.segNum-1 do nsp[#nsp + 1] = sp[i] end
    nsp[#nsp + 1] = nseg1
    self.shape[self.spNum] = nsp
    self.segNum = #nsp
  end
  self.undo[#self.undo + 1] = t
end

function EDITTOOL:cutter_bezier()
  local sp = self.shape[self.spNum]
  local t = { label = "cut bezier",
	      spNum = self.spNum,
	      original = cloneSubpath(sp),
	      undo = function (t, shape)
		       shape[t.spNum] = t.original
		       if t.newSpNum then
			 table.remove(shape, t.newSpNum)
		       end
		     end
	    }
  local param, _ = self.scissor(self.model.ui:pos())
  local seg = sp[self.segNum]
  local nseg1, nseg2
  if seg.type == "bezier" then
    nseg1, nseg2 = subdivideBezier(seg, param)
    nseg1.type = "bezier"
    nseg2.type = "bezier"
  else
    nseg1, nseg2 = subdivideQuad(seg, param)
    nseg1.type = "quad"
    nseg2.type = "quad"
  end
  if not sp.closed then
    local nsp = { type = "curve", closed = false, nseg2 }
    for i = self.segNum+1,#sp do nsp[#nsp + 1] = sp[i] end
    for i = #sp,self.segNum+1,-1 do table.remove(sp, i) end
    sp[self.segNum] = nseg1
    t.newSpNum = self.spNum + 1
    table.insert(self.shape, t.newSpNum, nsp)
  else -- sp is closed
    sp.closed = false
    local lastseg = sp[#sp]
    local nsp = { type = "curve", closed = false, nseg2 }
    for i = self.segNum+1,#sp do nsp[#nsp + 1] = sp[i] end
    nsp[#nsp + 1] = { type = "segment", lastseg[#lastseg], sp[1][1] }
    for i = 1,self.segNum-1 do nsp[#nsp + 1] = sp[i] end
    nsp[#nsp + 1] = nseg1
    self.shape[self.spNum] = nsp
    self.segNum = #nsp
  end
  self.undo[#self.undo + 1] = t
end

----------------------------------------------------------------------

local function scissor_arc(pos, arc)
  local t, p = arc:snap(pos)
  return t, p
end

local function scissor_bezier(pos, bezier)
  local t, p = bezier:snap(pos)
  return t, p
end

function EDITTOOL:action_cut_bezier()
  local sp = self.shape[self.spNum]
  local seg = sp[self.segNum]
  local bezier
  if seg.type == "bezier" then
    bezier = ipe.Bezier(seg[1], seg[2], seg[3], seg[4])
  else
    bezier = ipe.Quad(seg[1], seg[2], seg[3])
  end
  self.scissor = function (pos) return scissor_bezier(pos, bezier) end
  self.scissorPos = sp[self.segNum][1]
  self.cutter = "cutter_bezier"
end

function EDITTOOL:action_cut_arc()
  local sp = self.shape[self.spNum]
  local arc = sp[self.segNum].arc
  self.scissor = function (pos) return scissor_arc(pos, arc) end
  self.scissorPos = sp[self.segNum][1]
  self.cutter = "cutter_arc"
end

function EDITTOOL:action_cut_ellipse()
  local sp = self.shape[self.spNum]
  local arc = ipe.Arc(sp[1])
  self.scissor = function (pos) return scissor_arc(pos, arc) end
  self.scissorPos = arc:matrix() * V(1,0)
  self.cutter = "cutter_ellipse"
end

----------------------------------------------------------------------

function EDITTOOL:action_undo()
  if #self.undo == 0 then
    self.model:warning("Nothing to undo")
    return
  end
  local t = self.undo[#self.undo]
  table.remove(self.undo)
  t.undo(t, self.shape)
  self:resetCP()
  self.model.ui:explain("Undo " .. t.label)
end

----------------------------------------------------------------------

function EDITTOOL:showMenu()
  local t = self:type()
  local m = ipeui.Menu()
  local gp = self.model.ui:globalPos()
  if self:canDelete() then
    m:add("action_delete", "Delete vertex")
  end
  if t == VERTEX or t == SPLINECP then
    m:add("action_duplicate", "Duplicate vertex")
  end
  if t == VERTEX then
    m:add("action_cut", "Cut at vertex")
  end
  if #self.shape > 1 then
    m:add("action_delete_subpath", "Delete subpath")
  end
  if t == SPLINECP then
    m:add("action_convert_spline", "Convert spline to Beziers")
  end
  if t == BEZIERCP then
    m:add("action_cut_bezier", "Cut Bezier spline")
  end
  if t == CENTER then
    if self.shape[self.spNum].type == "ellipse" then
      m:add("action_cut_ellipse", "Cut ellipse")
    else
      m:add("action_cut_arc", "Cut arc")
    end
  end
  m:add("action_undo", "Undo")
  local item = m:execute(gp.x, gp.y)
  if item then
    self[item](self)
    self:setShapeMarks()
    self.model.ui:update(false) -- update tool
  end
end

----------------------------------------------------------------------

function EDITTOOL:mouseButton(button, modifiers, press)
  self.moving = false
  if self.scissor and press and button == 1 then
    self[self.cutter](self)
    self.scissor = nil
    self:setShapeMarks()
    self.model.ui:update(false) -- update tool
    self:explain()
    return
  end
  self.scissor = nil
  if not press then
    if button == 2 then
      self.spNum, self.segNum, self.cpNum =
	self:find(self.model.ui:unsnappedPos())
      self:showMenu()
      return
    end
  else -- press
    if button == 1 then
      self.spNum, self.segNum, self.cpNum =
	self:find(self.model.ui:unsnappedPos())
      self.moving = true
      local t = { label = "move",
		  original = cloneSubpath(self.shape[self.spNum]),
		  spNum = self.spNum,
		  undo = revertSubpath }
      self.undo[#self.undo + 1] = t
    end
  end
  self:setShapeMarks()
  self.model.ui:update(false) -- update tool
  self:explain()
end

function EDITTOOL:mouseMove(button, modifiers)
  if self.moving then
    local pos = self.model.ui:pos()
    moveCP(self.shape, self.spNum, self.segNum, self.cpNum, pos)
    self:setShapeMarks()
    self.model.ui:update(false) -- update tool
  elseif self.scissor then
    local t
    t, self.scissorPos = self.scissor(self.model.ui:pos())
    self:setShapeMarks()
    self.model.ui:update(false) -- update tool
  end
  self:explain()
end

function EDITTOOL:explain()
  if self.scissor then
    self.model.ui:explain("Left: cut | Right: menu | Ctrl+Z: undo | "
			  .. "Space: accept")
  else
    self.model.ui:explain("Left: move | Right: menu | Ctrl+Z: undo | "
			  .. "Space: accept")
  end
end

function EDITTOOL:acceptEdit()
  local t = { label="edit path",
	      pno=self.model.pno,
	      vno=self.model.vno,
	      primary=self.primary,
	      original=self.obj:clone(),
	      final=self.obj:clone(),
	    }
  t.final:setShape(self.shape)
  t.final:setMatrix(ipe.Matrix()) -- already in shape
  t.undo = function (t, doc)
	     doc[t.pno]:replace(t.primary, t.original)
	   end
  t.redo = function (t, doc)
	     doc[t.pno]:replace(t.primary, t.final)
	   end
  self.model:register(t)
end

function EDITTOOL:key(code, modifiers, text)
  print("Key:", code, text)
  if text == "\027" then  -- Esc
    self.model.ui:finishTool()
    return true
  elseif code == 90 and modifiers.control then  -- Ctrl+Z
    self.moving = false
    self.scissor = false
    self:action_undo()
    self:setShapeMarks()
    self.model.ui:update(false) -- update tool
    return true
  elseif code == 0x20 then -- Space: accept edit
    self.moving = false
    self:acceptEdit()
    self.model.ui:finishTool()
    return true
  else -- not consumed
    return false
  end
end

----------------------------------------------------------------------

function MODEL:action_edit_path(prim, obj)
  EDITTOOL:new(self, prim, obj)
end

----------------------------------------------------------------------

local join_threshold = 1e-9

local function flip(sub)
  sub[1], sub[2] = sub[2], sub[1]
  sub.flip = not sub.flip
end

local function findPartner(l, src, beg)
  local v = l[src][2]
  -- print("FindPartner for", src, v)
  while beg <= #l do
    local w1 = l[beg][1]
    -- print("   Considering", beg, w1)
    if (v - w1):sqLen() < join_threshold then return beg end
    local w2 = l[beg][2]
    -- print("   Considering", beg, w2)
    if (v - w2):sqLen() < join_threshold then flip(l[beg]) return beg end
    beg = beg + 1
  end
  return nil
end

local function reverse_segment(seg)
  local rseg = { type = seg.type }
  for i=#seg,1,-1 do
    rseg[#rseg + 1] = seg[i]  -- copy control points in reverse order
  end
  if rseg.type == "arc" then  -- need to do something special
    local alpha, beta = seg.arc:angles()
    rseg.arc = ipe.Arc(seg.arc:matrix() * ipe.Matrix(1, 0, 0, -1), beta, alpha)
  end
  return rseg
end

function MODEL:saction_join()
  local p = self:page()
  -- check that all objects consist of open subpaths only
  local l = {}
  for i,obj,sel,lay in p:objects() do
    if sel then
      if obj:type() ~= "path" then
	self:warning("Selection contains objects that are not paths.")
	return
      end
      local shape = obj:shape()
      transformShape(obj:matrix(), shape)
      for _,sp in ipairs(shape) do
	if sp.type ~= "curve" or sp.closed then
	  self:warning("A selected object does not consist of open curves.")
	  return
	end
	l[#l + 1] = { sp=sp, sp[1][1], sp[#sp][#sp[#sp]] }
      end
    end
  end

  if #l < 2 then
    self.ui:explain("only one path, nothing to join")
    return
  end

  -- match up endpoints
  if not findPartner(l, 1, 2) then
    -- not found, flip l[1] and try again
    flip(l[1])
  end
  for i = 1,#l - 1 do
    -- invariant: l[1] .. l[i] form a chain
    local j = findPartner(l, i, i+1)
    if not j then
      self:warning("Cannot join paths",
		   "Unable to join paths to a single curve")
      return
    end
    -- flip i+1 and j
    if j ~= i+1 then
      l[i+1], l[j] = l[j], l[i+1]
    end
  end

  -- now have a chain
  local nsp = { type = "curve", closed = false }
  for _, sp in ipairs(l) do
    if sp.flip then
      for i=#sp.sp,1,-1 do
	nsp[#nsp + 1] = reverse_segment(sp.sp[i])
	-- because of rounding errors:
	if #nsp > 1 then nsp[#nsp][1] = nsp[#nsp - 1][#nsp[#nsp - 1]] end
      end
    else
      for i=1,#sp.sp do
	nsp[#nsp + 1] = sp.sp[i]
	-- because of rounding errors:
	if #nsp > 1 then nsp[#nsp][1] = nsp[#nsp - 1][#nsp[#nsp - 1]] end
      end
    end
  end

  -- check if it is closed
  if (nsp[1][1] - nsp[#nsp][#nsp[#nsp]]):sqLen() < join_threshold then
    nsp.closed = true
    if nsp[#nsp].type == "segment" then
      table.remove(nsp)
    end
  end

  local prim = p:primarySelection()
  local obj = p[prim]:clone()
  obj:setMatrix( ipe.Matrix() )
  obj:setShape( { nsp } )

  local t = { label = "join paths",
	      pno = self.pno,
	      vno = self.vno,
	      selection = self:selection(),
	      original = p:clone(),
	      undo = revertOriginal,
	      object = obj,
	      layer = p:active(self.vno),
	    }
  t.redo = function (t, doc)
	     local p = doc[t.pno]
	     for i = #t.selection,1,-1 do
	       p:remove(t.selection[i])
	     end
	     p:insert(nil, t.object, 1, t.layer)
	   end
  self:register(t)
end

----------------------------------------------------------------------

function MODEL:saction_compose()
  local p = self:page()
  local prim = p:primarySelection()
  local supershape = {}
  for i,obj,sel,lay in p:objects() do
    if sel then
      if obj:type() ~= "path" then
	self:warning("Cannot compose objects",
		     "One of the selected objects is not a path object")
	return
      end
      local shape = obj:shape()
      if i ~= prim then
	transformShape(obj:matrix(), shape)
	transformShape(p[prim]:matrix():inverse(), shape)
      end
      for _,path in ipairs(shape) do
	supershape[#supershape + 1] = path
      end
    end
  end

  local obj = p[prim]:clone()
  obj:setShape(supershape)

  local t = { label = "compose paths",
	      pno = self.pno,
	      vno = self.vno,
	      selection = self:selection(),
	      original = p:clone(),
	      undo = revertOriginal,
	      object = obj,
	      layer = p:active(self.vno),
	    }
  t.redo = function (t, doc)
	     local p = doc[t.pno]
	     for i = #t.selection,1,-1 do
	       p:remove(t.selection[i])
	     end
	     p:insert(nil, t.object, 1, t.layer)
	   end
  self:register(t)
end

----------------------------------------------------------------------

function MODEL:saction_decompose()
  local p = self:page()
  local prim = p:primarySelection()
  if p[prim]:type() ~= "path" then
    self:warning("Cannot decompose objects",
		 "Primary selection is not a path object")
    return
  end
  local shape = p[prim]:shape()
  if #shape <= 1 then
    self:warning("Cannot decompose objects",
		 "Path object has only one subpath")
    return
  end
  transformShape(p[prim]:matrix(), shape)
  local objects = {}
  for _,path in ipairs(shape) do
    objects[#objects + 1] = ipe.Path(self.attributes, { path })
  end

  local t = { label = "decompose path",
	      pno = self.pno,
	      vno = self.vno,
	      primary = prim,
	      original = p:clone(),
	      undo = revertOriginal,
	      objects = objects,
	      layer = p:active(self.vno),
	    }
  t.redo = function (t, doc)
	     local p = doc[t.pno]
	     p:remove(t.primary)
	     for _,obj in ipairs(t.objects) do
	       p:insert(nil, obj, 2, t.layer)
	     end
	     p:ensurePrimarySelection()
	   end
  p:deselectAll()
  self:register(t)
end

----------------------------------------------------------------------
