/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/brave_page_graph/scripts/script_in_frame_querier.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/dom_node_ids.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/page/frame_tree.h"
#include "brave/third_party/blink/brave_page_graph/page_graph.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_script.h"
#include "brave/third_party/blink/brave_page_graph/scripts/script_in_frame_query_result.h"

using ::blink::Document;
using ::blink::DOMNodeIds;
using ::blink::LocalFrame;
using ::blink::Frame;
using ::blink::FrameTree;

namespace brave_page_graph {

ScriptInFrameQuerier::ScriptInFrameQuerier(const blink::Document& document,
    const ScriptId script_id) :
      document_(document),
      script_id_(script_id) {}

ScriptInFrameQuerier::~ScriptInFrameQuerier() {}

ScriptInFrameQueryResult ScriptInFrameQuerier::Find() const {
  const LocalFrame* const local_frame = document_.GetFrame();
  if (local_frame == nullptr) {
    return ScriptInFrameQueryResult();
  }

  const FrameTree& frame_tree = local_frame->Tree();
  const Frame& top_frame = frame_tree.Top();
  return FindInFrameSubStree(top_frame);
}

ScriptInFrameQueryResult ScriptInFrameQuerier::FindInFrameSubStree(
    const Frame& frame) const {
  // See if the queried script id is in the currnet frame.
  if (frame.IsLocalFrame() == true) {
    auto* local_frame = blink::DynamicTo<LocalFrame>(frame);
    Document* const frame_document = local_frame->GetDocument();
    const PageGraph* const page_graph = frame_document->GetPageGraph();
    if (page_graph != nullptr) {
      const NodeScript* script_node = page_graph->NodeForScriptInFrame(
        script_id_);
      if (script_node != nullptr) {
        return ScriptInFrameQueryResult(script_node,
          DOMNodeIds::IdForNode(frame_document),
          frame_document->Url().GetString().Utf8().data());
      }
    }
  }

  // Otherwise, recursively look through all the child frames.
  const Frame* next_sib_frame = frame.Tree().NextSibling();
  if (next_sib_frame != nullptr) {
    ScriptInFrameQueryResult result = FindInFrameSubStree(*next_sib_frame);
    if (result.IsMatch()) {
      return result;
    }
  }

  const Frame* first_child_frame = frame.Tree().FirstChild();
  if (first_child_frame != nullptr) {
    ScriptInFrameQueryResult result = FindInFrameSubStree(*first_child_frame);
    if (result.IsMatch()) {
      return result;
    }
  }

  return ScriptInFrameQueryResult();
}

}  // namespace brave_page_graph
