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

#ifndef REAIMGUI_DRAWLIST_HPP
#define REAIMGUI_DRAWLIST_HPP

#include "helper.hpp"

#include "../src/resource_proxy.hpp"
#include "../src/context.hpp"

struct DrawListProxy
    : public ResourceProxy<DrawListProxy, Context, ImDrawList> {
  using GetterFuncType = ImDrawList*(*)();
  template<Key KeyValue, GetterFuncType GetterFunc>
  struct Getter {
    static constexpr Key key { KeyValue };
    static auto get(Context *ctx)
    {
      assertFrame(ctx);
      return GetterFunc();
    }
  };

  using Window     = Getter<'WNDL', ImGui::GetWindowDrawList>;
  using Foreground = Getter<'FGDL', ImGui::GetForegroundDrawList>;
  using Background = Getter<'BGDL', ImGui::GetBackgroundDrawList>;

  using Decoder = MakeDecoder<Window, Foreground, Background>;
};

using ImGui_DrawList = DrawListProxy;
API_REGISTER_TYPE(DrawListProxy*, "ImGui_DrawList*");

class DrawListSplitter : public Resource {
public:
  DrawListSplitter(ImGui_DrawList *);
  ImDrawList *drawList() const;
  ImDrawListSplitter *operator->();

  bool attachable(const Context *) const override { return true; }

protected:
  bool isValid() const override;

private:
  ImGui_DrawList *m_drawlist;
  ImDrawList *m_lastList;
  ImDrawListSplitter m_splitter;
};

using ImGui_DrawListSplitter = DrawListSplitter;
API_REGISTER_OBJECT_TYPE(DrawListSplitter);

#endif
