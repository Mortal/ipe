----------------------------------------------------------------------
-- symbols ipelet
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

label = "Symbols"

revertOriginal = _G.revertOriginal

about = [[
Functions for creating, using, and editing symbols.

This ipelet is part of Ipe.
]]

V = ipe.Vector

----------------------------------------------------------------------

PASTETOOL = {}
PASTETOOL.__index = PASTETOOL

function PASTETOOL:new(model, name)
  local tool = {}
  _G.setmetatable(tool, PASTETOOL)
  tool.model = model
  tool.pos = model.ui:pos()
  tool.name = name
  tool.size = 1
  if name:match("%(s?f?p?x%)$") then
    tool.size = model.doc:sheets():find("symbolsize",
					model.attributes.symbolsize)
  end
  local obj = model.doc:sheets():find("symbol", name)
  model.ui:pasteTool(obj, tool)
  tool.setColor(1.0, 0, 0)
  return tool
end

function PASTETOOL:mouseButton(button, modifiers, press)
  self.model.ui:finishTool()
  local obj = ipe.Reference(self.model.attributes, self.name, self.pos)
  self.model:creation("create symbol", obj)
end

function PASTETOOL:mouseMove(button, modifiers)
  self.pos = self.model.ui:pos()
  self.setMatrix(ipe.Matrix(self.size, 0, 0, self.size, self.pos.x, self.pos.y))
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

----------------------------------------------------------------------

function use_symbol(model, num)
  local s = model.doc:sheets():allNames("symbol")
  local d = ipeui.Dialog(model.ui, "Use symbol")
  d:add("label", "label", { label = "Select symbol" }, 1, 1, 1, 3)
  d:add("select", "combo", s, 2, 1, 1, 3)
  d:add("ok", "button", { label="&Ok", action="accept" }, 3, 3)
  d:add("cancel", "button", { label="&Cancel", action="reject" }, 3, 2)
  d:setStretch("column", 1, 1)
  if not d:execute() then return end
  local name = s[d:get("select")]
  if num == 1 then
    PASTETOOL:new(model, name)
  else -- clone symbol
    local obj = model.doc:sheets():find("symbol", name)
    self.model:creation("clone symbol", obj)
  end
end

----------------------------------------------------------------------

function create_symbol(model, num)
  local p = model:page()
  local prim = p:primarySelection()
  if not prim then model.ui:explain("no selection") return end
  local str = ipeui.getString(model.ui, "Enter name of new symbol")
  if not str or str:match("^%s*$") then return end
  local name = str:match("^%s*%S+%s*$")
  local old = model.doc:sheets():find("symbol", name)
  if old then
    local r = ipeui.messageBox(model.ui, "question",
				    "Symbol '" .. name .. "' already exists",
				    "Do you want to proceed?",
				    "okcancel")
    if r <= 0 then return end
  end

  if num == 2 then -- new stylesheet
    local sheet = ipe.Sheet()
    sheet:add("symbol", name, p[prim])
    local t = { label = methods[num].label,
		sheet = sheet,
	      }
    t.redo = function (t, doc)
	       doc:sheets():insert(1, t.sheet:clone())
	     end
    t.undo = function (t, doc)
	       doc:sheets():remove(1)
	     end
    model:register(t)
  else  -- top stylesheet
    local sheet = model.doc:sheets():sheet(1)
    local t = { label = methods[num].label,
		original = sheet:clone(),
		final = sheet:clone(),
	      }
    t.final:add("symbol", name, p[prim])
    t.redo = function (t, doc)
	       doc:sheets():remove(1)
	       doc:sheets():insert(1, t.final:clone())
	     end
    t.undo = function (t, doc)
	       doc:sheets():remove(1)
	       doc:sheets():insert(1, t.original:clone())
	     end
    model:register(t)
  end
end

----------------------------------------------------------------------

methods = {
  { label = "use symbol", run = use_symbol },
  { label = "create symbol (in new style sheet)", run = create_symbol },
  { label = "create symbol (in top style sheet)", run = create_symbol },
  { label = "clone symbol", run = use_symbol },
}

----------------------------------------------------------------------