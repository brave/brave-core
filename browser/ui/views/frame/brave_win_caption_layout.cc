/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/brave_win_caption_layout.h"

#include "base/check.h"
#include "base/check_op.h"

namespace brave {

namespace {

thread_local int g_win_caption_geometry_tabstrip_overlap_depth = 0;

}  // namespace

// static
int ScopedWinCaptionLayoutUsesGeometryTabstripOverlap::
    GetCurrentWinCaptionGeometryTabstripOverlapDepth() {
  return g_win_caption_geometry_tabstrip_overlap_depth;
}

ScopedWinCaptionLayoutUsesGeometryTabstripOverlap::
    ScopedWinCaptionLayoutUsesGeometryTabstripOverlap(bool enable)
    : enabled_(enable) {
  if (enabled_) {
    ++g_win_caption_geometry_tabstrip_overlap_depth;
  }
}

ScopedWinCaptionLayoutUsesGeometryTabstripOverlap::
    ~ScopedWinCaptionLayoutUsesGeometryTabstripOverlap() {
  if (enabled_) {
    DCHECK_GT(g_win_caption_geometry_tabstrip_overlap_depth, 0);
    --g_win_caption_geometry_tabstrip_overlap_depth;
  }
}

}  // namespace brave
