----------------------------------------------------------------------
-- Select objects within bounding box of primary selection
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

-- To assign a shortcut to this:
-- shortcuts.ipelet_1_selectbox = "Alt+E"

label = "Select inside"

about = [[
Select objects that are inside the bounding box of the primary selection.

This ipelet is part of Ipe.
]]

function run(model, num)
  local p = model:page()
  if not p:hasSelection() then
    model.ui:explain("no selection")
    return
  end
  local prim = p:primarySelection()
  local box = p:bbox(prim)

  p:deselectAll()

  for i = 1,#p do
    local b = p:bbox(i)
    if i ~= prim and box:contains(b) then p:setSelect(i, 2) end
  end

  p:ensurePrimarySelection()
end

----------------------------------------------------------------------
