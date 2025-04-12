/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "content/public/test/fake_local_frame.h"

#include "src/content/public/test/fake_local_frame.cc"

namespace content {

void FakeLocalFrame::GetImageAt(const ::gfx::Point& window_point,
                                GetImageAtCallback callback) {
  std::move(callback).Run(SkBitmap());
}

}  // namespace content
