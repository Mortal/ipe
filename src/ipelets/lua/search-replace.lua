----------------------------------------------------------------------
-- search-replace ipelet
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

label = "Search && replace"

about = [[
Search and replace text in selected text objects.

This ipelet is part of Ipe.
]]

function run(model)
  local s = [[<qt>Enter pattern and replacement text.<br/>
The pattern will be substituted by the replacement text in all
selected text objects.<br/>
      The substitution does not occur inside groups.</qt>]]
  local d = ipeui.Dialog(model.ui, "Search & replace")
  d:add("label1", "label", {label=s}, 1, 1, 1, 4)
  d:add("label2", "label", {label="Pattern"}, 2, 1)
  d:add("pattern", "input", {}, 2, 2, 1, 3)
  d:add("label3", "label", {label="Replace by"}, 3, 1)
  d:add("replace", "input", {}, 3, 2, 1, 3)
  d:add("regex", "checkbox", {label="Use Lua patterns"}, 4, 1, 1, 4)
  d:add("ok", "button", {label="&Ok", action="accept"}, 6, 4)
  d:add("cancel", "button", {label="&Cancel", action="reject"}, 6, 3)
  d:setStretch("column", 2, 1)
  d:setStretch("row", 5, 1)
  if not d:execute() then return end
  local s1 = d:get("pattern")
  local s2 = d:get("replace")
  if not d:get("regex") then
    -- make strings non-magic
    s1 = string.gsub(s1, "(%W)", "%%%1")
    s2 = string.gsub(s2, "%%", "%%%%")
  end

  local t = { label = label,
	      pno = model.pno,
	      vno = model.vno,
	      original = model:page():clone(),
	      final = model:page():clone(),
	      undo = _G.revertOriginal,
	      redo = _G.revertFinal,
	    }
  for i,obj,sel,layer in t.final:objects() do
    if obj:type() == "text" then
      local text = obj:text()
      text = text:gsub(s1, s2)
      obj:setText(text)
    end
  end
  model:register(t)
end

----------------------------------------------------------------------