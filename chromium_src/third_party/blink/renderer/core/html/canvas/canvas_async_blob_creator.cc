/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/renderer/core/farbling/brave_session_cache.h"
#include "third_party/blink/public/platform/web_content_settings_client.h"

#define BRAVE_CANVAS_ASYNC_BLOB_CREATOR                    \
  brave::BraveSessionCache::From(*context_).PerturbPixels( \
      static_cast<const unsigned char*>(src_data_.addr()), \
      src_data_.computeByteSize());

#include "src/third_party/blink/renderer/core/html/canvas/canvas_async_blob_creator.cc"

#undef BRAVE_CANVAS_ASYNC_BLOB_CREATOR
