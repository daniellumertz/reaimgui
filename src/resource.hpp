/* ReaImGui: ReaScript binding for Dear ImGui
 * Copyright (C) 2021-2023  Christian Fillion
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

#ifndef REAIMGUI_RESOURCE_HPP
#define REAIMGUI_RESOURCE_HPP

#include <memory>

#include "flat_set.hpp"

class Context;

class Resource {
public:
  static constexpr const char *api_type_name { "ImGui_Resource" };

  Resource();
  Resource(const Resource &) = delete;
  virtual ~Resource();

  void keepAlive();

  virtual bool attachable(const Context *) const = 0;

  template<typename T>
  static bool isValid(T *userData)
  {
    if constexpr(std::is_same_v<Resource, T>)
      return g_rsx.contains(userData) && userData->isValid();

    auto resource { static_cast<Resource *>(userData) };
    return isValid(resource) && resource->isInstanceOf<T>();
  }

  template<typename T, typename Fn>
  static void foreach(const Fn &&callback) // O(n) of all types, not per type
  {
    for(Resource *rs : g_rsx) {
      if(rs->isInstanceOf<T>())
        callback(static_cast<T *>(rs));
    }
  }

  static void destroyAll();

  template<typename T>
  bool isInstanceOf() const
  {
    // short-circuiting dynamic_cast for faster exact type match
    return typeid(*this) == typeid(T) || dynamic_cast<const T *>(this);
  }

protected:
  virtual bool heartbeat();
  virtual bool isValid() const;

private:
  class Timer;

  static FlatSet<Resource *> g_rsx;

  std::shared_ptr<Timer> m_timer;
  unsigned int m_keepAlive;
};

using ImGui_Resource = Resource;

#endif
