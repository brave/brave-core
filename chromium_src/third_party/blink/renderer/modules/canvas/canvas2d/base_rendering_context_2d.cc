/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/core/dom/document.h"

#define BRAVE_GET_IMAGE_DATA                                         \
  LocalDOMWindow* window = LocalDOMWindow::From(script_state);       \
  if (window) {                                                      \
    snapshot = brave::BraveSessionCache::From(*(window->document())) \
                   .PerturbPixels(snapshot);                         \
  }

#include "../../../../../../../third_party/blink/renderer/modules/canvas/canvas2d/base_rendering_context_2d.cc"

#undef BRAVE_GET_IMAGE_DATA
