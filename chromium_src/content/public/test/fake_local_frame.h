/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CONTENT_PUBLIC_TEST_FAKE_LOCAL_FRAME_H_
#define BRAVE_CHROMIUM_SRC_CONTENT_PUBLIC_TEST_FAKE_LOCAL_FRAME_H_

#include "third_party/blink/public/mojom/frame/frame.mojom.h"

#define UpdatePrerenderURL                                                  \
  GetImageAt(const ::gfx::Point& window_point, GetImageAtCallback callback) \
      override;                                                             \
  void UpdatePrerenderURL

#include "src/content/public/test/fake_local_frame.h"  // IWYU pragma: export
#undef UpdatePrerenderURL

#endif  // BRAVE_CHROMIUM_SRC_CONTENT_PUBLIC_TEST_FAKE_LOCAL_FRAME_H_
