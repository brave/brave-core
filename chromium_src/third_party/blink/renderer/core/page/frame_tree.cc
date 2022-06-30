/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/core/page/frame_tree.h"

#define ExperimentalSetNulledName ExperimentalSetNulledName_ChromiumImpl

#include "src/third_party/blink/renderer/core/page/frame_tree.cc"

#undef ExperimentalSetNulledName

namespace blink {

void FrameTree::ExperimentalSetNulledName() {
  SetName(g_null_atom, kReplicate);
  ExperimentalSetNulledName_ChromiumImpl();
}

}  // namespace blink
