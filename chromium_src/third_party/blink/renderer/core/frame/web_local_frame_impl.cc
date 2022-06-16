/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "src/third_party/blink/renderer/core/frame/web_local_frame_impl.cc"

#if BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH)
#include "brave/third_party/blink/renderer/core/brave_page_graph/page_graph.h"
#endif  // BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH)

namespace blink {

#if BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH)
WebPageGraph* WebLocalFrameImpl::GetWebPageGraph() {
  DCHECK(GetFrame());
  return blink::PageGraph::From(*GetFrame());
}
#endif  // BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH)

}  // namespace blink
