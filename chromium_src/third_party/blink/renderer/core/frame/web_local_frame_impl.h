/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_FRAME_WEB_LOCAL_FRAME_IMPL_H_
#define BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_FRAME_WEB_LOCAL_FRAME_IMPL_H_

#include "third_party/blink/public/web/web_local_frame.h"

#define BlinkFeatureUsageReport                           \
  NotUsed() override {}                                   \
  IF_BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH,                   \
               WebPageGraph* GetWebPageGraph() override;) \
  void BlinkFeatureUsageReport

#include "src/third_party/blink/renderer/core/frame/web_local_frame_impl.h"

#undef BlinkFeatureUsageReport

#endif  // BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_FRAME_WEB_LOCAL_FRAME_IMPL_H_
