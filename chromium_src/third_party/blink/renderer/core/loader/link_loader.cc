/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/core/loader/link_loader.h"

#include "brave/components/brave_page_graph/common/buildflags.h"
#include "third_party/blink/renderer/core/dom/dom_node_ids.h"
#include "third_party/blink/renderer/core/html/html_link_element.h"
#include "third_party/blink/renderer/core/probe/core_probes.h"

#if BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH)
#define initiator_info                                          \
  initiator_info.dom_node_id =                                  \
      CoreProbeSink::HasAgentsGlobal(CoreProbeSink::kPageGraph) \
          ? DOMNodeIds::IdForNode(client_->GetOwner())          \
          : kInvalidDOMNodeId;                                  \
  options.initiator_info
#endif  // BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH)

#include "src/third_party/blink/renderer/core/loader/link_loader.cc"

#if BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH)
#undef initiator_info
#endif  // BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH)
