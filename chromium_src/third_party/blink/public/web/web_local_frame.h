/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_PUBLIC_WEB_WEB_LOCAL_FRAME_H_
#define BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_PUBLIC_WEB_WEB_LOCAL_FRAME_H_

#include "brave/components/brave_page_graph/common/buildflags.h"

#if BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH)
#include "brave/third_party/blink/public/web/web_page_graph.h"
#endif  // BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH)

#define BlinkFeatureUsageReport                              \
  NotUsed() = 0;                                             \
  IF_BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH,                      \
               virtual WebPageGraph* GetWebPageGraph() = 0;) \
  virtual void BlinkFeatureUsageReport

#include "src/third_party/blink/public/web/web_local_frame.h"

#undef BlinkFeatureUsageReport

#endif  // BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_PUBLIC_WEB_WEB_LOCAL_FRAME_H_
