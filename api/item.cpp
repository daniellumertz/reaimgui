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

API_SECTION("Item & Status");

API_FUNC(0_1, void, SetItemAllowOverlap, (ImGui_Context*,ctx),
R"(Allow last item to be overlapped by a subsequent item. sometimes useful with
invisible buttons, selectables, etc. to catch unused area.)")
{
  FRAME_GUARD;
  ImGui::SetItemAllowOverlap();
}

API_FUNC(0_5_5, void, BeginDisabled, (ImGui_Context*,ctx)
(bool*,API_RO(disabled),true),
R"(Disable all user interactions and dim items visuals
(applying StyleVar_DisabledAlpha over current colors).

BeginDisabled(false) essentially does nothing useful but is provided to
facilitate use of boolean expressions.
If you can avoid calling BeginDisabled(false)/EndDisabled() best to avoid it.)")
{
  FRAME_GUARD;
  ImGui::BeginDisabled(API_RO_GET(disabled));
}

API_FUNC(0_5_5, void, EndDisabled, (ImGui_Context*,ctx),
"See BeginDisabled.")
{
  FRAME_GUARD;
  ImGui::EndDisabled();
}

API_SUBSECTION("Focus & Activation",
R"~(Prefer using "SetItemDefaultFocus()" over
"if(IsWindowAppearing()) SetScrollHereY()" when applicable to signify
"this is the default item".)~");

API_FUNC(0_1, void, SetItemDefaultFocus, (ImGui_Context*,ctx),
"Make last item the default focused item of a window.")
{
  FRAME_GUARD;
  ImGui::SetItemDefaultFocus();
}

API_FUNC(0_1, void, SetKeyboardFocusHere, (ImGui_Context*,ctx)
(int*,API_RO(offset),0),
R"(Focus keyboard on the next widget. Use positive 'offset' to access sub
components of a multiple component widget. Use -1 to access previous widget.)")
{
  FRAME_GUARD;
  ImGui::SetKeyboardFocusHere(API_RO_GET(offset));
}

API_FUNC(0_8_5, void, PushTabStop, (ImGui_Context*,ctx)
(bool,tab_stop),
R"(Allow focusing using TAB/Shift-TAB, enabled by default but you can disable it
for certain widgets)")
{
  FRAME_GUARD;
  ImGui::PushTabStop(tab_stop);
}

API_FUNC(0_8_5, void, PopTabStop, (ImGui_Context*,ctx),
"See PushTabStop")
{
  FRAME_GUARD;
  ImGui::PopTabStop();
}

API_SUBSECTION("Dimensions");

API_FUNC(0_1, void, GetItemRectMin, (ImGui_Context*,ctx)
(double*,API_W(x))(double*,API_W(y)),
"Get upper-left bounding rectangle of the last item (screen space)")
{
  FRAME_GUARD;
  const ImVec2 &rect { ImGui::GetItemRectMin() };
  if(API_W(x)) *API_W(x) = rect.x;
  if(API_W(y)) *API_W(y) = rect.y;
}

API_FUNC(0_1, void, GetItemRectMax, (ImGui_Context*,ctx)
(double*,API_W(x))(double*,API_W(y)),
"Get lower-right bounding rectangle of the last item (screen space)")
{
  FRAME_GUARD;
  const ImVec2 &rect { ImGui::GetItemRectMax() };
  if(API_W(x)) *API_W(x) = rect.x;
  if(API_W(y)) *API_W(y) = rect.y;
}

API_FUNC(0_1, void, GetItemRectSize, (ImGui_Context*,ctx)
(double*,API_W(w))(double*,API_W(h)),
"Get size of last item")
{
  FRAME_GUARD;
  const ImVec2 &rect { ImGui::GetItemRectSize() };
  if(API_W(w)) *API_W(w) = rect.x;
  if(API_W(h)) *API_W(h) = rect.y;
}

API_FUNC(0_1, void, PushItemWidth, (ImGui_Context*,ctx)
(double,item_width),
R"(Push width of items for common large "item+label" widgets.

- \>0.0: width in pixels
- <0.0 align xx pixels to the right of window
  (so -FLT_MIN always align width to the right side)
- 0.0 = default to ~2/3 of windows width.)")
{
  FRAME_GUARD;
  ImGui::PushItemWidth(item_width);
}

API_FUNC(0_1, void, PopItemWidth, (ImGui_Context*,ctx),
"See PushItemWidth")
{
  FRAME_GUARD;
  ImGui::PopItemWidth();
}

API_FUNC(0_1, void, SetNextItemWidth, (ImGui_Context*,ctx)
(double,item_width),
R"(Set width of the _next_ common large "item+label" widget.

- \>0.0: width in pixels
- <0.0 align xx pixels to the right of window
  (so -FLT_MIN always align width to the right side))")
{
  FRAME_GUARD;
  ImGui::SetNextItemWidth(item_width);
}

API_FUNC(0_1, double, CalcItemWidth, (ImGui_Context*,ctx),
R"(Width of item given pushed settings and current cursor position.
NOT necessarily the width of last item unlike most 'Item' functions.)")
{
  FRAME_GUARD;
  return ImGui::CalcItemWidth();
}

API_SUBSECTION("Item/Widgets Utilities and Query Functions",
R"(Most of the functions are referring to the previous Item that has been submitted.

See Demo Window under "Widgets->Querying Item Status" for an interactive
visualization of most of those functions.)");

API_FUNC(0_1, bool, IsItemHovered, (ImGui_Context*,ctx)
(int*,API_RO(flags),ImGuiHoveredFlags_None),
R"(Is the last item hovered? (and usable, aka not blocked by a popup, etc.).
See HoveredFlags_* for more options.)")
{
  FRAME_GUARD;
  return ImGui::IsItemHovered(API_RO_GET(flags));
}

API_FUNC(0_1, bool, IsItemActive, (ImGui_Context*,ctx),
R"(Is the last item active? (e.g. button being held, text field being edited.
This will continuously return true while holding mouse button on an item.
Items that don't interact will always return false.)")
{
  FRAME_GUARD;
  return ImGui::IsItemActive();
}

API_FUNC(0_1, bool, IsItemFocused, (ImGui_Context*,ctx),
"Is the last item focused for keyboard/gamepad navigation?")
{
  FRAME_GUARD;
  return ImGui::IsItemFocused();
}

API_FUNC(0_1, bool, IsItemClicked, (ImGui_Context*,ctx)
(int*,API_RO(mouse_button),ImGuiMouseButton_Left),
R"(Is the last item clicked? (e.g. button/node just clicked on)
== IsMouseClicked(mouse_button) && IsItemHovered().

This is NOT equivalent to the behavior of e.g. Button.
Most widgets have specific reactions based on mouse-up/down state, mouse position etc.)")
{
  FRAME_GUARD;
  return ImGui::IsItemClicked(API_RO_GET(mouse_button));
}

API_FUNC(0_1, bool, IsItemVisible, (ImGui_Context*,ctx),
"Is the last item visible? (items may be out of sight because of clipping/scrolling)")
{
  FRAME_GUARD;
  return ImGui::IsItemVisible();
}

API_FUNC(0_1, bool, IsItemEdited, (ImGui_Context*,ctx),
R"(Did the last item modify its underlying value this frame? or was pressed?
This is generally the same as the "bool" return value of many widgets.)")
{
  FRAME_GUARD;
  return ImGui::IsItemEdited();
}

API_FUNC(0_1, bool, IsItemActivated, (ImGui_Context*,ctx),
"Was the last item just made active (item was previously inactive).")
{
  FRAME_GUARD;
  return ImGui::IsItemActivated();
}

API_FUNC(0_1, bool, IsItemDeactivated, (ImGui_Context*,ctx),
R"(Was the last item just made inactive (item was previously active).
Useful for Undo/Redo patterns with widgets that require continuous editing.)")
{
  FRAME_GUARD;
  return ImGui::IsItemDeactivated();
}

API_FUNC(0_1, bool, IsItemDeactivatedAfterEdit, (ImGui_Context*,ctx),
R"(Was the last item just made inactive and made a value change when it was
active? (e.g. Slider/Drag moved).

Useful for Undo/Redo patterns with widgets that require continuous editing. Note
that you may get false positives (some widgets such as Combo/ListBox/Selectable
will return true even when clicking an already selected item).)")
{
  FRAME_GUARD;
  return ImGui::IsItemDeactivatedAfterEdit();
}

API_FUNC(0_1, bool, IsAnyItemHovered, (ImGui_Context*,ctx),
"")
{
  FRAME_GUARD;
  return ImGui::IsAnyItemHovered();
}

API_FUNC(0_1, bool, IsAnyItemActive, (ImGui_Context*,ctx),
"")
{
  FRAME_GUARD;
  return ImGui::IsAnyItemActive();
}

API_FUNC(0_1, bool, IsAnyItemFocused, (ImGui_Context*,ctx),
"")
{
  FRAME_GUARD;
  return ImGui::IsAnyItemFocused();
}

API_SECTION_DEF(hoveredFlags, ROOT_SECTION,
               "Hovered Flags", "For IsItemHovered(), IsWindowHovered() etc.");
API_ENUM(0_1, ImGui, HoveredFlags_None,
  R"(Return true if directly over the item/window, not obstructed by another
  window, not obstructed by an active popup or modal blocking inputs under them.)");
API_ENUM(0_1, ImGui, HoveredFlags_AllowWhenBlockedByPopup,
  "Return true even if a popup window is normally blocking access to this item/window.");
API_ENUM(0_1, ImGui, HoveredFlags_AllowWhenBlockedByActiveItem,
  R"(Return true even if an active item is blocking access to this item/window.
  Useful for Drag and Drop patterns.)");
API_ENUM(0_7, ImGui, HoveredFlags_NoNavOverride,
  "Disable using gamepad/keyboard navigation state when active, always query mouse.");

API_ENUM(0_8, ImGui, HoveredFlags_DelayNormal,
  "Return true after ConfigVar_HoverDelayNormal elapsed (~0.30 sec)");
API_ENUM(0_8, ImGui, HoveredFlags_DelayShort,
  "Return true after ConfigVar_HoverDelayShort elapsed (~0.10 sec)");
API_ENUM(0_8, ImGui, HoveredFlags_NoSharedDelay,
  R"(Disable shared delay system where moving from one item to the next keeps
  the previous timer for a short time (standard for tooltips with long delays)");

API_SECTION_P(hoveredFlags, "IsItemHovered only");
API_ENUM(0_1, ImGui, HoveredFlags_AllowWhenOverlapped,
  R"(IsItemHovered only: Return true even if the position is obstructed or
  overlapped by another window.)");
API_ENUM(0_1, ImGui, HoveredFlags_AllowWhenDisabled,
  "IsItemHovered only: Return true even if the item is disabled.");
API_ENUM(0_1, ImGui, HoveredFlags_RectOnly,
  R"(HoveredFlags_AllowWhenBlockedByPopup |
  HoveredFlags_AllowWhenBlockedByActiveItem | HoveredFlags_AllowWhenOverlapped)");

API_SECTION_P(hoveredFlags, "IsWindowHovered only");
API_ENUM(0_1, ImGui, HoveredFlags_ChildWindows,
  "IsWindowHovered only: Return true if any children of the window is hovered.");
API_ENUM(0_1, ImGui, HoveredFlags_RootWindow,
  "IsWindowHovered only: Test from root window (top most parent of the current hierarchy).");
API_ENUM(0_1, ImGui, HoveredFlags_AnyWindow,
  "IsWindowHovered only: Return true if any window is hovered.");
API_ENUM(0_5_10, ImGui, HoveredFlags_NoPopupHierarchy,
  R"(IsWindowHovered only: Do not consider popup hierarchy (do not treat popup
  emitter as parent of popup) (when used with _ChildWindows or _RootWindow).)");
API_ENUM(0_5_10, ImGui, HoveredFlags_DockHierarchy,
  R"(IsWindowHovered only: Consider docking hierarchy (treat dockspace host as
  parent of docked window) (when used with _ChildWindows or _RootWindow).)");
API_ENUM(0_1, ImGui, HoveredFlags_RootAndChildWindows,
  "HoveredFlags_RootWindow | HoveredFlags_ChildWindows");
