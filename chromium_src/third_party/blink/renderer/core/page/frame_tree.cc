/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/core/page/frame_tree.h"

#define CrossSiteCrossBrowsingContextGroupSetNulledName \
  CrossSiteCrossBrowsingContextGroupSetNulledName_ChromiumImpl

#include "src/third_party/blink/renderer/core/page/frame_tree.cc"

#undef CrossSiteCrossBrowsingContextGroupSetNulledName

namespace blink {

void FrameTree::CrossSiteCrossBrowsingContextGroupSetNulledName() {
  SetName(g_null_atom, kReplicate);
  CrossSiteCrossBrowsingContextGroupSetNulledName_ChromiumImpl();
}

}  // namespace blink
