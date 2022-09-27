/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/core/loader/image_loader.h"

#include "brave/components/brave_page_graph/common/buildflags.h"
#include "third_party/blink/renderer/core/dom/dom_node_ids.h"
#include "third_party/blink/renderer/core/frame/ad_tracker.h"
#include "third_party/blink/renderer/core/probe/core_probes.h"
#include "third_party/blink/renderer/platform/loader/fetch/fetch_parameters.h"

#if BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH)
#define initiator_info                                          \
  initiator_info.dom_node_id =                                  \
      CoreProbeSink::HasAgentsGlobal(CoreProbeSink::kPageGraph) \
          ? DOMNodeIds::IdForNode(GetElement())                 \
          : kInvalidDOMNodeId;                                  \
  resource_loader_options.initiator_info
#endif  // BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH)

#include "src/third_party/blink/renderer/core/loader/image_loader.cc"

#if BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH)
#undef initiator_info
#endif  // BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH)
