/* ReaImGui: ReaScript binding for Dear ImGui
 * Copyright (C) 2021-2022  Christian Fillion
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

#ifndef REAIMGUI_API_HELPER_HPP
#define REAIMGUI_API_HELPER_HPP

#include "context.hpp"

#include <array>
#include <boost/type_index.hpp>
#include <cstring> // strlen

#define REAIMGUI_API __attribute__((annotate("reaimgui_api")))
#define REAIMGUI_CONST(prefix, name, doc)                     \
  REAIMGUI_API __attribute__((annotate("reaimgui_doc=" doc))) \
  int name() { return prefix##name; }

#define API_RO(var)       var##InOptional // read, optional/nullable (except string, use nullIfEmpty)
#define API_RW(var)       var##InOut      // read/write
#define API_RWO(var)      var##InOutOptional // documentation/python only
#define API_W(var)        var##Out        // write
#define API_W_SZ(var)     var##Out_sz     // write
// Not using varInOutOptional because REAPER refuses to pass NULL to them
#define API_RWBIG(var)    var##InOutNeedBig    // read/write, resizable (realloc_cmd_ptr)
#define API_RWBIG_SZ(var) var##InOutNeedBig_sz // size of previous API_RWBIG buffer
#define API_WBIG(var)     var##OutNeedBig
#define API_WBIG_SZ(var)  var##OutNeedBig_sz

#define FRAME_GUARD assertValid(ctx); assertFrame(ctx);

template<typename Output, typename Input>
inline Output valueOr(const Input *ptr, const Output fallback)
{
  return ptr ? static_cast<Output>(*ptr) : fallback;
}

// const char *foobarInOptional from REAPER is never null no matter what
inline void nullIfEmpty(const char *&string)
{
  if(string && !strlen(string))
    string = nullptr;
}

template<typename T>
inline void assertValid(T *ptr)
{
  if constexpr (std::is_base_of_v<Resource, T>) {
    if(Resource::exists(ptr))
      return;
  }
  else if(ptr)
    return;

  std::string type;
  if constexpr (std::is_class_v<T>)
    type = T::api_type_name;
  else
    type = boost::typeindex::type_id<T>().pretty_name();

  char message[255];
  snprintf(message, sizeof(message),
    "expected valid %s*, got %p", type.c_str(), ptr);
  throw reascript_error { message };
}

inline void assertFrame(Context *ctx)
{
  if(!ctx->enterFrame()) {
    delete ctx;
    throw reascript_error { "frame initialization failed" };
  }
}

template <typename PtrType, typename ValType, size_t N>
class ReadWriteArray {
public:
  template<typename... Args,
    typename = typename std::enable_if_t<sizeof...(Args) == N>>
  ReadWriteArray(Args&&... args)
    : m_inputs { std::forward<Args>(args)... }
  {
    size_t i { 0 };
    for(const PtrType *ptr : m_inputs) {
      assertValid(ptr);
      m_values[i++] = *ptr;
    }
  }

  size_t size() const { return N; }
  ValType *data() { return m_values.data(); }
  ValType &operator[](const size_t i) { return m_values[i]; }

  bool commit()
  {
    size_t i { 0 };
    for(const ValType value : m_values)
      *m_inputs[i++] = value;
    return true;
  }

private:
  std::array<PtrType*, N> m_inputs;
  std::array<ValType, N> m_values;
};

// Common behavior for p_open throughout the API.
// When false, set output to true to signal it's open to the caller, but give
// NULL to Dear ImGui to signify not closable.
inline bool *openPtrBehavior(bool *p_open)
{
  if(p_open && !*p_open) {
    *p_open = true;
    return nullptr;
  }

  return p_open;
}

#endif
