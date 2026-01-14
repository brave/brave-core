/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_FRAME_LOCAL_FRAME_H_
#define BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_FRAME_LOCAL_FRAME_H_

#include "brave/components/brave_page_graph/common/buildflags.h"

class SkBitmap;

#if BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH)
namespace blink {
class PageGraph;
}  // namespace blink
#endif

#define CopyImageAtViewportPoint                                      \
  CopyImageAtViewportPoint_UnUsed() {}                                \
  SkBitmap GetImageAtViewportPoint(const gfx::Point& viewport_point); \
  void CopyImageAtViewportPoint

#define ScriptEnabled                    \
  ScriptEnabled(const KURL& script_url); \
  bool ScriptEnabled_ChromiumImpl

#if BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH)
#define PerformSpellCheck                                          \
  PerformSpellCheck();                                             \
  ForwardDeclaredMember<PageGraph> GetPageGraph() const {          \
    return page_graph_;                                            \
  }                                                                \
  void SetPageGraph(ForwardDeclaredMember<PageGraph> page_graph) { \
    page_graph_ = page_graph;                                      \
  }                                                                \
  void Unused

#define link_preview_triggerer_ \
  link_preview_triggerer_;      \
  ForwardDeclaredMember<PageGraph> page_graph_
#endif

#define IsSameOrigin(...)    \
  IsSameOrigin(__VA_ARGS__); \
  url::Origin origin_for_clear_window_name_check_

#include <third_party/blink/renderer/core/frame/local_frame.h>  // IWYU pragma: export

#if BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH)
#undef link_preview_triggerer_
#undef PerformSpellCheck
#endif
#undef ScriptEnabled

#undef CopyImageAtViewportPoint
#undef IsSameOrigin

#endif  // BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_FRAME_LOCAL_FRAME_H_
