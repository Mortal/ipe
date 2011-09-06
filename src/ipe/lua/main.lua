----------------------------------------------------------------------
-- Ipe
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

-- order is important
require "prefs"
require "model"
require "actions"
require "tools"
require "editpath"
require "properties"
require "shortcuts"
require "mouse"

----------------------------------------------------------------------

-- short names
V = ipe.Vector

-- store the model for the first window
-- Used only by MacOS file_open_event
local first_model = nil

----------------------------------------------------------------------

function formatFromFileName(fname)
  local s = string.lower(fname:sub(-4))
  if s == ".xml" or s == ".ipe" then return "xml" end
  if s == ".pdf" then return "pdf" end
  if s == ".eps" then return "eps" end
  return nil
end

function revertOriginal(t, doc)
  doc:set(t.pno, t.original)
end

function revertFinal(t, doc)
  doc:set(t.pno, t.final)
end

function indexOf(el, list)
  for i,n in ipairs(list) do
    if n == el then return i end
  end
  return nil
end

function symbolNames(sheet, prefix, postfix)
  local list = sheet:allNames("symbol")
  local result = {}
  for _, n in ipairs(list) do
    if n:sub(1, #prefix) == prefix and n:sub(-#postfix) == postfix then
      result[#result + 1] = n
    end
  end
  return result
end

function stripPrefixPostfix(list, m, mm)
  local result = {}
  for _, n in ipairs(list) do
    result[#result + 1] = n:sub(m, -mm-1)
  end
  return result
end

function arrowshapeToName(i, s)
  return s:match("^arrow/(.+)%(s?f?p?x%)$")
end

function colorString(color)
  if type(color) == "string" then return color end
  -- else must be table
  return string.format("(%g,%g,%g)", color.r, color.g, color.b)
end

function extractElements(p, selection)
  local r = {}
  local l = {}
  for i,j in ipairs(selection) do
    r[#r + 1] = p[j-i+1]:clone()
    l[#l + 1] = p:layerOf(j-i+1)
    p:remove(j-i+1)
  end
  return r, l
end

-- make a list of all values from stylesheets
function allValues(sheets, kind)
  local syms = sheets:allNames(kind)
  local values = {}
  for _,sym in ipairs(syms) do
    values[#values + 1] = sheets:find(kind, sym)
  end
  return values
end

-- apply transformation to a shape
function transformShape(matrix, shape)
  local result = {}
  for _,path in ipairs(shape) do
    if path.type == "ellipse" or path.type == "closedspline" then
      for i = 1,#path do
	path[i] = matrix * path[i]
      end
    else -- must be "curve"
      for _,seg in ipairs(path) do
	for i = 1,#seg do
	  seg[i] = matrix * seg[i]
	end
	if seg.type == "arc" then
	  seg.arc = matrix * seg.arc
	end
      end
    end
  end
end

function findStyle(w, dir)
  if dir and ipe.fileExists(dir .. "/" .. w) then
    return dir .. "/" .. w
  end
  for _, d in ipairs(config.styleDirs) do
    local s = d .. "/" .. w
    if ipe.fileExists(s) then return s end
  end
end

----------------------------------------------------------------------
-- This function is called to launch a file on MacOS X

function file_open_event(fname)
  if first_model and first_model.pristine then
    first_model:loadDocument(fname)
  else
    MODEL:new(fname)
  end
end

----------------------------------------------------------------------

local win32_conversions = {
  PgDown=0x22, PgUp=0x21, Home=0x24, End=0x23,
  Left=0x25, Up=0x26, Right=0x27, Down=0x28,
  insert=0x2d, delete=0x2e,
  ["/"]= 0xbf, ["\\"]= 0xdc, ["-"]= 0xbd, ["="]= 0xbb,
  ["@"]= 0x40032, -- actually Shift+2
}

function win32_shortcut_convert(s)
  local k = 0
  local done = false
  while not done do
    if s:sub(1,5) == "Ctrl+" then
      s = s:sub(6)
      k = k + 0x20000
    elseif s:sub(1,6) == "Shift+" then
      s = s:sub(7)
      k = k + 0x40000
    elseif s:sub(1,4) == "Alt+" then
      s = s:sub(5)
      k = k + 0x10000
    else
      done = true
    end
  end
  if s:sub(1,1) == "F" and tonumber(s:sub(2)) then
    return k + 0x6f + tonumber(s:sub(2))
  elseif win32_conversions[s] then
    return k + win32_conversions[s]
  else
    return k + string.byte(s:sub(1,1))
  end
end

function win32_shortcuts(ui)
  if config.toolkit ~= "win32" then return end
  local accel = {}
  for i in pairs(shortcuts) do
    local id = ui:actionId(i)
    local s = shortcuts[i]
    if type(s) == "table" then
      for j,s1 in ipairs(s) do
	accel[#accel+1] = win32_shortcut_convert(s1)
	accel[#accel+1] = id
      end
    elseif s then
      accel[#accel+1] = win32_shortcut_convert(s)
      accel[#accel+1] = id
    end
  end
  return accel
end

----------------------------------------------------------------------

local function show_configuration()
  local s = config.version
  s = s .. "\nLua code: " .. package.path
  s = s .. "\nStyle directories: " .. table.concat(config.styleDirs, ", ")
  s = s .. "\nStyles for new documents: " .. table.concat(prefs.styles, ", ")
  s = s .. "\nAutosave file: " .. prefs.autosave_filename
  s = s .. "\nDocumentation: " .. config.docdir
  s = s .. "\nIpelets: " .. table.concat(config.ipeletDirs, ", ")
  s = s .. "\nLatex directory: " .. config.latexdir
  s = s .. "\nFontmap: " .. config.fontmap
  s = s .. "\nIcons: " .. config.icons
  s = s .. "\n"
  io.stdout:write(s)
end

local function usage()
  io.stderr:write("Usage: ipe { -sheet <filename.isy> } [ <filename> ]\n")
  io.stderr:write("or:    ipe -show-configuration\n")
  io.stderr:write("or:    ipe --help\n")
end

-- check locale
test1 = string.format("%g", 1.5)
test2 = string.format("%s", tonumber("1.5"))
if test1 ~= "1.5" or test2 ~= "1.5" then
  m = "<qt>Formatting the number <code>1.5</code> results in '"
    .. test1 .. "'. "
    .. "Reading '1.5' results in <code>" .. test2 .. "</code><br />"
    .. "Therefore Ipe will not work correctly when loading or saving files. "
    .. "<em>Please report this problem.</em><br />"
    .. "As a workaround, you can start Ipe from the commandline like this: "
    .. "<pre>export LANG=C\nexport LC_NUMERIC=C\nipe</pre></qt>"
  ipeui.messageBox(nil, "critical",
		   "Ipe is running with an incorrect locale", m)
  return
end

--------------------------------------------------------------------

local home = os.getenv("HOME")
local ipeletpath = os.getenv("IPELETPATH")
if ipeletpath then
  config.ipeletDirs = {}
  for w in string.gmatch(ipeletpath, "[^:;]+") do
    if w == "_" then w = config.system_ipelets end
    config.ipeletDirs[#config.ipeletDirs + 1] = w
  end
else
  config.ipeletDirs = { config.system_ipelets }
  if config.platform ~= "win" then
    table.insert(config.ipeletDirs, 1, home .. "/.ipe/ipelets")
    if config.platform == "apple" then
      table.insert(config.ipeletDirs, 2, home .. "/Library/Ipe/Ipelets")
    end
  end
end

local ipestyles = os.getenv("IPESTYLES")
if ipestyles then
  config.styleDirs = {}
  for w in string.gmatch(ipestyles, "[^:;]+") do
    if w == "_" then w = config.system_styles end
    config.styleDirs[#config.styleDirs + 1] = w
  end
else
  config.styleDirs = { config.system_styles }
  if config.platform ~= "win" then
    table.insert(config.styleDirs, 1, home .. "/.ipe/styles")
    if config.platform == "apple" then
      table.insert(config.styleDirs, 2, home .. "/Library/Ipe/Styles")
    end
  end
end

--------------------------------------------------------------------

-- look for ipelets
ipelets = {}
for _,w in ipairs(config.ipeletDirs) do
  if ipe.fileExists(w) then
    for f in lfs.dir(w) do
      if f:sub(-4) == ".lua" then
	ff = assert(loadfile(w .. "/" .. f))
	ft = {}
	ft._G = _G
	ft.ipe = ipe
	ft.ipeui = ipeui
	ft.math = math
	ft.string = string
	ft.table = table
	ft.assert = assert
	ft.shortcuts = shortcuts
	ft.prefs = prefs
	ft.ipairs = ipairs
	ft.pairs = pairs
	ft.print = print
	ft.tonumber = tonumber
	ft.tostring = tostring
	ft.name = f:sub(1,-5)
	ft.dllname = w .. "/" .. ft.name
	setfenv(ff, ft)
	ff()
	-- if it has no label, then running it once was all
	if ft.label then
	  ipelets[#ipelets + 1] = ft
	end
      end
    end
  end
end

--------------------------------------------------------------------

-- Windows provides the command line as one long string
if config.toolkit == "win32" then
  local t = {}
  for w in string.gmatch(argv, "%S+") do
    t[#t+1] = w
  end
  argv = t
end

if #argv == 1 and argv[1] == "-show-configuration" then
  show_configuration()
  return
end

if #argv == 1 and (argv[1] == "--help" or argv[1] == "-h") then
  usage()
  return
end

--------------------------------------------------------------------

local first_file = nil
local i = 1
local style_sheets = {}

while i <= #argv do
  if argv[i] == "-sheet" then
    if i == #argv then usage() return end
    style_sheets[#style_sheets + 1] = argv[i+1]
    i = i + 2
  else
    if i ~= #argv then usage() return end
    first_file = argv[i]
    i = i + 1
  end
end

if #style_sheets > 0 then prefs.styles = style_sheets end

config.styleList = {}
for _,w in ipairs(prefs.styles) do
  if w:sub(-4) ~= ".isy" then w = w .. ".isy" end
  if not w:find("/") then w = findStyle(w) end
  config.styleList[#config.styleList + 1] = w
end

first_model = MODEL:new(first_file)

mainloop(win32_shortcuts(first_model.ui))

----------------------------------------------------------------------
