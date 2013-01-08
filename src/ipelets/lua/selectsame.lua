----------------------------------------------------------------------
-- Select objects with same attribute as primary selection
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

-- To assign a shortcut to first method (stroke), uncomment this:
-- shortcuts.ipelet_1_selectsame = "Alt+E"

label = "Select same"

about = [[
Select objects that have the same attribute value as the primary selection.

This ipelet is part of Ipe.
]]

methods = {
  { label = "stroke" },
  { label = "fill" },
  { label = "pen" },
  { label = "dashstyle" },
}

type = _G.type

function equal(a, b)
  if type(a) == "table" and type(b) == "table" then
    -- must be colors
    return a.r == b.r and a.g == b.g and a.b == b.b
  else
    return a == b
  end
end

function run(model, num)
  local p = model:page()
  if not p:hasSelection() then
    model.ui:explain("no selection")
    return
  end
  local att = methods[num].label
  local prim = p:primarySelection()
  local val = p[prim]:get(att)

  p:deselectAll()

  for i = 1,#p do
    if equal(p[i]:get(att), val) then p:setSelect(i, 2) end
  end

  p:setSelect(prim, 1) -- select primary again
end

----------------------------------------------------------------------
