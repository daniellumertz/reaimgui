/* ReaImGui: ReaScript binding for Dear ImGui
 * Copyright (C) 2021  Christian Fillion
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

#ifndef REAIMGUI_PLATFORM_HPP
#define REAIMGUI_PLATFORM_HPP

class DockerHost;
class Window;
struct ImGuiViewport;
struct ImVec2;

namespace Platform {
  void install();
  Window *createWindow(ImGuiViewport *, DockerHost * = nullptr);
  void updateMonitors();
  ImGuiViewport *viewportUnder(ImVec2);
  void translatePosition(ImVec2 *, bool toHiDpi = false);
};

#endif