/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/core/frame/local_dom_window.h"

#define BRAVE_CANVAS_ASYNC_BLOB_CREATOR                                 \
  if (LocalDOMWindow* window = DynamicTo<LocalDOMWindow>(context)) {    \
    if (Document* document = window->document()) {                      \
      image_ = brave::BraveSessionCache::From(*document).PerturbPixels( \
          document->GetFrame(), image_);                                \
    }                                                                   \
  }

#include "../../../../../../../../third_party/blink/renderer/core/html/canvas/canvas_async_blob_creator.cc"

#undef BRAVE_CANVAS_ASYNC_BLOB_CREATOR
