/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_FRAME_LOCAL_FRAME_H_
#define BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_FRAME_LOCAL_FRAME_H_

class SkBitmap;

namespace blink {
class PageGraph;
}  // namespace blink

#define CopyImageAtViewportPoint                                      \
  CopyImageAtViewportPoint_UnUsed() {}                                \
  SkBitmap GetImageAtViewportPoint(const gfx::Point& viewport_point); \
  void CopyImageAtViewportPoint

#define ScriptEnabled                    \
  ScriptEnabled(const KURL& script_url); \
  bool ScriptEnabled_ChromiumImpl

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

#include <third_party/blink/renderer/core/frame/local_frame.h>  // IWYU pragma: export

#undef link_preview_triggerer_
#undef PerformSpellCheck
#undef ScriptEnabled
#undef CopyImageAtViewportPoint

#endif  // BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_FRAME_LOCAL_FRAME_H_
