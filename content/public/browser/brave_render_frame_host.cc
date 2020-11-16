/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "content/public/browser/render_frame_host.h"

#if defined(OS_ANDROID)
#include "brave/content/browser/cosmetic_filters_communication_impl.h"
#endif

namespace content {

RenderFrameHost::RenderFrameHost() {
#if defined(OS_ANDROID)
  cosmetic_filters_communication_impl_ = nullptr;
#endif
}

RenderFrameHost::~RenderFrameHost() {}

}  // namespace content
