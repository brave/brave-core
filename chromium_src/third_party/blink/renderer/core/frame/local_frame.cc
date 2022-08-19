/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/core/frame/local_frame.h"

#include "brave/components/brave_page_graph/common/buildflags.h"

#if BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH)
#include "brave/third_party/blink/renderer/core/brave_page_graph/page_graph.h"
#endif  // BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH)

#define BRAVE_LOCAL_FRAME_CONSTRUCTOR                                \
  IF_BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH, {                            \
    if (IsLocalRoot()) {                                             \
      /* InstallSupplements call is too late, do it here instead. */ \
      PageGraph::ProvideTo(*this);                                   \
    }                                                                \
  })

#include "src/third_party/blink/renderer/core/frame/local_frame.cc"

#undef BRAVE_LOCAL_FRAME_CONSTRUCTOR
