----------------------------------------------------------------------
-- Translation cover computations
----------------------------------------------------------------------
--[[

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

--]]

label = "Convex hull"

about = [[
Compute the convex hull of polygonal objects, and print its area.
]]

function incorrect(model)
  model:warning("Selection contains non-polygonal objects")
end

function collect_vertices(model)
  local p = model:page()
  local v = {}

  for i,obj,sel,layer in p:objects() do
    if sel then
      local m = obj:matrix()
      if obj:type() ~= "path" then incorrect(model) return end
      local shape = obj:shape()
      for i,s in ipairs(shape) do
	if s.type ~= "curve" then incorrect(model) return end
	for j,sp in ipairs(s) do
	  if sp.type ~= "segment" then incorrect(model) return end
	  for k,u in ipairs(sp) do
	    local um = m * u
	    if #v == 0 or um ~= v[#v] then v[#v+1] = um end
	  end
	end
      end
    end
  end
  return v
end

function comp(u, v)
  return u.x < v.x or (u.x == v.x and u.y < v.y)
end

function show(v)
  for i,u in ipairs(v) do
    print(i, u)
  end
end

function convex_hull(v)
  table.sort(v, comp)
  -- remove duplicates
  local i = 2
  while i <= #v do
    if v[i] == v[i-1] then
      table.remove(v, i)
    else
      i = i + 1
    end
  end
  --show(v)
  local up = { v[1], v[2] }
  for i = 3, #v do
    local w = v[i]
    while #up >= 2 and ipe.LineThrough(up[#up-1], up[#up]):side(w) >= 0 do
      table.remove(up)
    end
    up[#up + 1] = w
  end
  local dn = { v[1], v[2] }
  for i = 3, #v do
    local w = v[i]
    while #dn >= 2 and ipe.LineThrough(dn[#dn-1], dn[#dn]):side(w) <= 0 do
      table.remove(dn)
    end
    dn[#dn + 1] = w
  end
  return up, dn
end

function calculate_below(v)
  local r = 0
  for i = 2, #v do
    local yh = 0.5 * (v[i-1].y + v[i].y)
    local w = v[i].x - v[i-1].x
    r = r + yh * w
  end
  return r
end

function calculate_area(upper, lower)
  return calculate_below(upper) - calculate_below(lower)
end

function make_polygon(model, upper, lower)
  local curve = { type="curve", closed=true }
  for i = 2,#upper do
    curve[#curve+1] = { type="segment"; upper[i-1], upper[i] }
  end
  for i = #lower-1,2,-1 do
    curve[#curve+1] = { type="segment"; lower[i+1], lower[i] }
  end
  local obj = ipe.Path(model.attributes, { curve })
  model:creation("create convex hull", obj)
end

function run(model, num)
  local v = collect_vertices(model)
  if #v < 3 then
    model:warning("I need at least three vertices!")
    return
  end
  local upper, lower = convex_hull(v)
  make_polygon(model, upper, lower)
  local area = calculate_area(upper, lower)
  model.ui:explain("Area is " .. area)
  print("Area of convex hull is " .. area)
end

----------------------------------------------------------------------
