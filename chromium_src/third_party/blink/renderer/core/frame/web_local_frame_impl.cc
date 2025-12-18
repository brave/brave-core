/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/core/frame/web_local_frame_impl.h"

#include <third_party/blink/renderer/core/frame/web_local_frame_impl.cc>

namespace blink {

void WebLocalFrameImpl::SetOriginForClearWindowNameCheck(
    const url::Origin& origin) {
  CHECK(IsProvisional());
  GetFrame()->origin_for_clear_window_name_check_ = origin;
}

}  // namespace blink
