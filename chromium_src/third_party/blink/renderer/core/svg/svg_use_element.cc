/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/core/svg/svg_use_element.h"

#include "brave/components/brave_page_graph/common/buildflags.h"
#include "third_party/blink/renderer/core/probe/core_probes.h"
#include "third_party/blink/renderer/platform/graphics/dom_node_id.h"
#include "third_party/blink/renderer/platform/loader/fetch/fetch_parameters.h"

#if BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH)
#define initiator_info                                          \
  initiator_info.dom_node_id =                                  \
      CoreProbeSink::HasAgentsGlobal(CoreProbeSink::kPageGraph) \
          ? GetDomNodeId()                                      \
          : kInvalidDOMNodeId;                                  \
  options.initiator_info
#endif  // BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH)

#include "src/third_party/blink/renderer/core/svg/svg_use_element.cc"

#if BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH)
#undef initiator_info
#endif  // BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH)
