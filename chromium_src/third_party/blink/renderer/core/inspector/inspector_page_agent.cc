/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "src/third_party/blink/renderer/core/inspector/inspector_page_agent.cc"

#include "brave/components/brave_page_graph/common/buildflags.h"

#if BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH)
#include "brave/third_party/blink/renderer/core/brave_page_graph/page_graph.h"
#endif  // BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH)

namespace blink {

Response InspectorPageAgent::generatePageGraph(String* data) {
#if BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH)
  LocalFrame* main_frame = inspected_frames_->Root();
  if (!main_frame) {
    return Response::ServerError("No main frame found");
  }

  PageGraph* page_graph = blink::PageGraph::From(*main_frame);
  if (!page_graph) {
    return Response::ServerError("No Page Graph for main frame");
  }

  *data = page_graph->ToGraphML();
  return Response::Success();
#else
  return Response::ServerError("Page Graph buildflag is disabled");
#endif  // BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH)
}

Response InspectorPageAgent::generatePageGraphNodeReport(
    int node_id,
    std::unique_ptr<protocol::Array<String>>* report) {
#if BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH)
  LocalFrame* main_frame = inspected_frames_->Root();
  if (!main_frame) {
    return Response::ServerError("No main frame found");
  }

  PageGraph* page_graph = blink::PageGraph::From(*main_frame);
  if (!page_graph) {
    return Response::ServerError("No Page Graph for main frame");
  }

  *report = std::make_unique<protocol::Array<String>>();
  page_graph->GenerateReportForNode(node_id, **report);
  return Response::Success();
#else
  return Response::ServerError("Page Graph buildflag is disabled");
#endif  // BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH)
}

}  // namespace blink
