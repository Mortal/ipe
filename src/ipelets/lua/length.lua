----------------------------------------------------------------------
-- compute lengths ipelet
----------------------------------------------------------------------
--[[
--[[

    This file is part of the extensible drawing editor Ipe.
    Copyright (C) 1993-2010  Otfried Cheong

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

    This file is part of the extensible drawing editor Ipe.
    Copyright (C) 2010

    Ipe is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Ipe is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
    or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
    License for more details.

    You should have received a copy of the GNU General Public License
    along with Ipe; if not, you can find it at
    "http://www.gnu.org/copyleft/gpl.html", or write to the Free
    Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

    written by Günter Rote, 2010.

--]]

label = "length of a geometric graph"

about = [[
length of a geometric graph

This ipelet is part of the Dagstuhl collection.
]]

----------------------------------------------------------
----------------------------------------------------------
-- This is the "length" function of a single edge.
-- Change it to your needs

local function norm(v) -- v is an ipe.Vector
  return v:len()
end

local objective_function_name = "Euclidean (L2)"

-- For the squared Euclidean length, uncomment the following 4 lines:

-- local function norm(v)
--   return v:sqLen()
-- end
-- local objective_function_name = "squared Euclidean"
----------------------------------------------------------
----------------------------------------------------------

local function c(p,q)
    local dif = p-q
    return norm(dif)
end

local function incorrect_input(model,s)
  model:warning("Cannot compute length", s)
end

function tlength(model)
  local p = model:page()
  local prim = p:primarySelection()
  local sec

  if not prim then model.ui:explain("no selection") return end

--  local seg = p[prim]
--  if seg:type() ~= "path" then
--     incorrect_input(model,"Primary selection is not a segment")
--     return
--  end
--  local shape = seg:shape()
--  if --shape ~= 1 or shape[1].type ~= "curve" or
--    --shape[1] ~= 1 or shape[1][1].type ~= "segment" then
--     incorrect_input(model,"Primary selection is not a segment")
 --    return
 -- end

--  local p0 = seg:matrix() * shape[1][1][1]
--  local p1 = seg:matrix() * shape[1][1][2]
--  local offset = p1-p0
--  local eps = offset:len()

  local w
  local warn = false
  local total = 0
  for i,obj,sel,layer in p:objects() do
   if sel then
     if (obj:type() ~= "path") then
      incorrect_input(model,"Some secondary selection is of type "..
          obj:type()..", not a path")
      return
     end
     shape = obj:shape()
     for ind,subpath in ipairs(shape) do
      if subpath.type ~= "curve" then
       incorrect_input(model,"Some secondary selection is not a polygonal path")
       return
      end
      for i,s in ipairs(subpath) do
       if s.type ~= "segment" then
        incorrect_input(model,"Some secondary selection is not a polygonal path")
        return
       end
       total = total + c(obj:matrix()*s[1],obj:matrix()*s[2])
      end
      if subpath.closed then
       total = total + c(obj:matrix()*subpath[#subpath][2], obj:matrix()*subpath[1][1])
      end
     end
    end
  end
-------- Now we have computed the length

  local outp = objective_function_name .." length="..total
  print (outp)
  model.ui:explain(outp)
end

run = tlength
