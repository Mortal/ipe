----------------------------------------------------------------------
-- Ipe keyboard shortcuts
----------------------------------------------------------------------
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

shortcuts = {
  open = "Ctrl+O",
  quit = "Ctrl+Q",
  save = "Ctrl+S",
  save_as = nil,
  run_latex = "Ctrl+L",
  absolute = "Ctrl+T",
  undo = "Ctrl+Z",
  redo = "Ctrl+Y",
  copy = "Ctrl+C",
  paste = nil,
  paste_at_cursor = "Ctrl+V",
  cut = "Ctrl+X",
  delete = "Delete",
  group = "Ctrl+G",
  ungroup = "Ctrl+U",
  front = "Ctrl+F",
  back = "Ctrl+B",
  forward = "Shift+Ctrl+F",
  backward = "Shift+Ctrl+B",
  before = nil,
  behind = nil,
  duplicate = "D",
  select_all = "Ctrl+A",
  join_paths = "Ctrl+J",
  insert_text_box = "F10",
  edit = "Ctrl+E",
  change_width = "Ctrl+W",
  document_properties = "Ctrl+Shift+P",
  style_sheets = "Ctrl+Shift+S",
  update_style_sheets = "Ctrl+Shift+U",
  mode_select = "S",
  mode_translate = "T",
  mode_rotate = "R",
  mode_stretch = "E",
  mode_pan = nil,
  mode_label = "L",
  mode_math = "Shift+4",
  mode_paragraph = "G",
  mode_marks = "M",
  mode_rectangles = "B",
  mode_lines = "P",
  mode_polygons = "Shift+P",
  mode_arc1 = "A",
  mode_arc2 = "Shift+A",
  mode_arc3 = "Alt+A",
  mode_circle1 = "O",
  mode_circle2 = "Shift+O",
  mode_circle3 = "Alt+O",
  mode_splines = "I",
  mode_splinegons = "Shift+I",
  mode_ink = "K",
  snapvtx = "F4",
  snapbd = "F5",
  snapint = "F6",
  snapgrid = "F7",
  snapangle = "F8",
  snapauto = "F9",
  set_origin_snap = "F1",
  hide_axes = "Ctrl+F1",
  set_direction = "F2",
  reset_direction = "Ctrl+F2",
  set_line = nil,
  set_line_snap = "F3",
  fullscreen = "F11",
  normal_size = "/",
  grid_visible = "F12",
  zoom_in = "Ctrl+PgUp",
  zoom_out = "Ctrl+PgDown",
  fit_page = "\\",
  fit_objects = "=",
  fit_selection = "@",
  pan_here = "x",
  new_layer = "Ctrl+Shift+N",
  select_in_active_layer = "Ctrl+Shift+A",
  move_to_active_layer = "Ctrl+Shift+M",
  next_view = "PgDown",
  previous_view = "PgUp",
  first_view = "Home",
  last_view = "End",
  new_layer_view = "Ctrl+Shift+I",
  new_view = nil,
  delete_view = nil,
  edit_effects = nil,
  next_page = "Shift+PgDown",
  previous_page = "Shift+PgUp",
  first_page = "Shift+Home",
  last_page = "Shift+End",
  new_page = "Ctrl+I",
  cut_page = "Ctrl+Shift+X",
  copy_page = "Ctrl+Shift+C",
  paste_page = "Ctrl+Shift+V",
  delete_page = nil,
  edit_title = "Ctrl+P",
  ipelet_1_goodies = nil, -- Mirror horizontal
  ipelet_2_goodies = nil, -- Mirror vertical
  ipelet_3_goodies = nil, -- Mirror at x-axis
  ipelet_4_goodies = nil, -- Turn 90 degrees
  ipelet_5_goodies = nil, -- Turn 180 degrees
  ipelet_6_goodies = nil, -- Turn 270 degrees
  ipelet_7_goodies = "Ctrl+R", -- Precise rotate
  ipelet_8_goodies = "Ctrl+K", -- Precise stretch
  ipelet_9_goodies = nil, -- Insert precise box
  ipelet_10_goodies = nil, -- Insert bounding box
  ipelet_11_goodies = nil, -- Insert media box
  ipelet_12_goodies = nil, -- Mark circle center
  ipelet_13_goodies = nil, -- Make parabolas
  ipelet_14_goodies = nil, -- Regular k-gon
}

----------------------------------------------------------------------
