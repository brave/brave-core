/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/core/dom/document.h"

#define BRAVE_CANVAS_ASYNC_BLOB_CREATOR                               \
  Document* document = Document::From(context);                       \
  if (document) {                                                     \
    image_ = brave::BraveSessionCache::From(*document).PerturbPixels( \
        document->GetFrame(), image_);                                \
  }

#include "../../../../../../../third_party/blink/renderer/core/html/canvas/canvas_async_blob_creator.cc"  // NOLINT

#undef BRAVE_CANVAS_ASYNC_BLOB_CREATOR
