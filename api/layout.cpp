/* ReaImGui: ReaScript binding for Dear ImGui
 * Copyright (C) 2021-2024  Christian Fillion
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "helper.hpp"

API_SECTION("Layout");

API_FUNC(0_1, void, Separator, (ImGui_Context*,ctx),
R"(Separator, generally horizontal. inside a menu bar or in horizontal layout
mode, this becomes a vertical separator.)")
{
  FRAME_GUARD;
  ImGui::Separator();
}

API_FUNC(0_8_4, void, SeparatorText, (ImGui_Context*,ctx)
(const char*,label),
"Text formatted with an horizontal line")
{
  FRAME_GUARD;
  ImGui::SeparatorText(label);
}

API_SUBSECTION("Cursor",
R"(By "cursor" we mean the current output position.
The typical widget behavior is to output themselves at the current cursor
position, then move the cursor one line down.

You can call SameLine() between widgets to undo the last carriage return and
output at the right of the preceding widget.)");

API_FUNC(0_1, void, SameLine, (ImGui_Context*,ctx)
(double*,API_RO(offset_from_start_x),0.0)(double*,API_RO(spacing),-1.0),
R"(Call between widgets or groups to layout them horizontally.
X position given in window coordinates.)")
{
  FRAME_GUARD;
  ImGui::SameLine(API_RO_GET(offset_from_start_x), API_RO_GET(spacing));
}

API_FUNC(0_1, void, NewLine, (ImGui_Context*,ctx),
"Undo a SameLine() or force a new line when in a horizontal-layout context.")
{
  FRAME_GUARD;
  ImGui::NewLine();
}

API_FUNC(0_1, void, Spacing, (ImGui_Context*,ctx),
"Add vertical spacing.")
{
  FRAME_GUARD;
  ImGui::Spacing();
}

API_FUNC(0_1, void, Dummy, (ImGui_Context*,ctx)(double,size_w)(double,size_h),
R"(Add a dummy item of given size. unlike InvisibleButton, Dummy() won't take the
mouse click or be navigable into.)")
{
  FRAME_GUARD;
  ImGui::Dummy(ImVec2(size_w, size_h));
}

API_FUNC(0_1, void, Indent, (ImGui_Context*,ctx)(double*,API_RO(indent_w),0.0),
R"(Move content position toward the right, by 'indent_w', or
StyleVar_IndentSpacing if 'indent_w' <= 0. See Unindent.)")
{
  FRAME_GUARD;
  ImGui::Indent(API_RO_GET(indent_w));
}

API_FUNC(0_1, void, Unindent, (ImGui_Context*,ctx)(double*,API_RO(indent_w),0.0),
R"(Move content position back to the left, by 'indent_w', or
StyleVar_IndentSpacing if 'indent_w' <= 0)")
{
  FRAME_GUARD;
  ImGui::Unindent(API_RO_GET(indent_w));
}

API_FUNC(0_1, void, BeginGroup, (ImGui_Context*,ctx),
"Lock horizontal starting position. See EndGroup.")
{
  FRAME_GUARD;
  ImGui::BeginGroup();
}

API_FUNC(0_8, void, EndGroup, (ImGui_Context*,ctx),
R"(Unlock horizontal starting position + capture the whole group bounding box
into one "item" (so you can use IsItemHovered or layout primitives such as
SameLine on whole group, etc.).

See BeginGroup.)")
{
  FRAME_GUARD;
  ImGui::EndGroup();
}

API_FUNC(0_1, void, GetCursorPos, (ImGui_Context*,ctx)
(double*,API_W(x))(double*,API_W(y)),
"Cursor position in window")
{
  FRAME_GUARD;
  const ImVec2 &pos { ImGui::GetCursorPos() };
  if(API_W(x)) *API_W(x) = pos.x;
  if(API_W(y)) *API_W(y) = pos.y;
}

API_FUNC(0_1, double, GetCursorPosX, (ImGui_Context*,ctx),
"Cursor X position in window")
{
  FRAME_GUARD;
  return ImGui::GetCursorPosX();
}

API_FUNC(0_1, double, GetCursorPosY, (ImGui_Context*,ctx),
"Cursor Y position in window")
{
  FRAME_GUARD;
  return ImGui::GetCursorPosY();
}

API_FUNC(0_1, void, SetCursorPos, (ImGui_Context*,ctx)
(double,local_pos_x)(double,local_pos_y),
"Cursor position in window")
{
  FRAME_GUARD;
  ImGui::SetCursorPos(ImVec2(local_pos_x, local_pos_y));
}

API_FUNC(0_1, void, SetCursorPosX, (ImGui_Context*,ctx)
(double,local_x),
"Cursor X position in window")
{
  FRAME_GUARD;
  ImGui::SetCursorPosX(local_x);
}

API_FUNC(0_1, void, SetCursorPosY, (ImGui_Context*,ctx)
(double,local_y),
"Cursor Y position in window")
{
  FRAME_GUARD;
  ImGui::SetCursorPosY(local_y);
}

API_FUNC(0_1, void, GetCursorStartPos, (ImGui_Context*,ctx)
(double*,API_W(x))(double*,API_W(y)),
"Initial cursor position in window coordinates.")
{
  FRAME_GUARD;
  const ImVec2 &pos { ImGui::GetCursorStartPos() };
  if(API_W(x)) *API_W(x) = pos.x;
  if(API_W(y)) *API_W(y) = pos.y;
}

API_FUNC(0_1, void, GetCursorScreenPos, (ImGui_Context*,ctx)
(double*,API_W(x))(double*,API_W(y)),
"Cursor position in absolute screen coordinates (useful to work with the DrawList API).")
{
  FRAME_GUARD;
  const ImVec2 &pos { ImGui::GetCursorScreenPos() };
  if(API_W(x)) *API_W(x) = pos.x;
  if(API_W(y)) *API_W(y) = pos.y;
}

API_FUNC(0_1, void, SetCursorScreenPos, (ImGui_Context*,ctx)
(double,pos_x)(double,pos_y),
"Cursor position in absolute screen coordinates.")
{
  FRAME_GUARD;
  ImGui::SetCursorScreenPos(ImVec2(pos_x, pos_y));
}

API_SUBSECTION("Clipping",
R"(Mouse hovering is affected by PushClipRect() calls, unlike direct calls to
DrawList_PushClipRect() which are render only. Coordinates are in screen space.)");

API_FUNC(0_1, void, PushClipRect, (ImGui_Context*,ctx)
(double,clip_rect_min_x)(double,clip_rect_min_y)
(double,clip_rect_max_x)(double,clip_rect_max_y)
(bool,intersect_with_current_clip_rect),
"")
{
  FRAME_GUARD;
  ImGui::PushClipRect(
    ImVec2(clip_rect_min_x, clip_rect_min_y),
    ImVec2(clip_rect_max_x, clip_rect_max_y),
    intersect_with_current_clip_rect
  );
}

API_FUNC(0_1, void, PopClipRect, (ImGui_Context*,ctx),
"See PushClipRect")
{
  FRAME_GUARD;
  ImGui::PopClipRect();
}

API_FUNC(0_1, bool, IsRectVisible, (ImGui_Context*,ctx)
(double,size_w)(double,size_h),
R"(Test if rectangle (of given size, starting from cursor position) is
visible / not clipped.)")
{
  FRAME_GUARD;
  return ImGui::IsRectVisible(ImVec2(size_w, size_h));
}

API_FUNC(0_1, bool, IsRectVisibleEx, (ImGui_Context*,ctx)
(double,rect_min_x)(double,rect_min_y)(double,rect_max_x)(double,rect_max_y),
R"(Test if rectangle (in screen space) is visible / not clipped. to perform
coarse clipping on user's side.)")
{
  FRAME_GUARD;
  return ImGui::IsRectVisible(
    ImVec2(rect_min_x, rect_min_y), ImVec2(rect_max_x, rect_max_y));
}
