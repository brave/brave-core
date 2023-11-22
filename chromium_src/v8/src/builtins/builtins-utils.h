/* Copyright (c) 2016 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_V8_SRC_BUILTINS_BUILTINS_UTILS_H_
#define BRAVE_CHROMIUM_SRC_V8_SRC_BUILTINS_BUILTINS_UTILS_H_

#include "src/v8/src/builtins/builtins-utils.h"  // IWYU pragma: export

#include "brave/components/brave_page_graph/common/buildflags.h"

namespace v8 {
namespace internal {

#if BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH_WEBAPI_PROBES)

constexpr bool IsBuiltinTrackedInPageGraph(const char* name) {
  constexpr const char* const kBuiltinsToTrack[] = {
      "Date",
      "Json",
  };
  for (const char* builtin_to_track : kBuiltinsToTrack) {
    // Check if |name| starts with |builtin_to_track|.
    if (strstr(name, builtin_to_track) == name)
      return true;
  }
  return false;
}

// Replace BUILTIN macro with a similar one, but with Page Graph-tracking call.
#if !defined(BUILTIN)
static_assert(false, "BUILTIN macro is expected to be defined");
#endif
#undef BUILTIN
#define BUILTIN(name)                                                      \
  V8_WARN_UNUSED_RESULT static Tagged<Object> Builtin_Impl_##name(         \
      BuiltinArguments args, Isolate* isolate);                            \
                                                                           \
  V8_WARN_UNUSED_RESULT Address Builtin_##name(                            \
      int args_length, Address* args_object, Isolate* isolate) {           \
    DCHECK(isolate->context().is_null() || IsContext(isolate->context())); \
    BuiltinArguments args(args_length, args_object);                       \
    Tagged<Object> result(Builtin_Impl_##name(args, isolate));             \
    if (V8_UNLIKELY(IsBuiltinTrackedInPageGraph(#name)) &&                 \
        V8_UNLIKELY(isolate->page_graph_delegate())) {                     \
      ReportBuiltinCallAndResponse(isolate, #name, args, result);          \
    }                                                                      \
    return BUILTIN_CONVERT_RESULT(result);                                 \
  }                                                                        \
                                                                           \
  V8_WARN_UNUSED_RESULT static Tagged<Object> Builtin_Impl_##name(         \
      BuiltinArguments args, Isolate* isolate)

#endif  // BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH_WEBAPI_PROBES)

}  // namespace internal
}  // namespace v8

#endif  // BRAVE_CHROMIUM_SRC_V8_SRC_BUILTINS_BUILTINS_UTILS_H_
