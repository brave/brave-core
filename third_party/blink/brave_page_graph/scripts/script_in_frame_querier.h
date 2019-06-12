/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_SCRIPTS_SCRIPT_IN_FRAME_QUERIER_RESULT_H_
#define BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_SCRIPTS_SCRIPT_IN_FRAME_QUERIER_RESULT_H_

#include "brave/third_party/blink/brave_page_graph/types.h"

namespace blink {
class Document;
class Frame;
}

namespace brave_page_graph {

class ScriptInFrameQueryResult;

class ScriptInFrameQuerier {
 public:
  ScriptInFrameQuerier(const blink::Document& document,
    const ScriptId script_id);
  ~ScriptInFrameQuerier();

  ScriptInFrameQueryResult Find() const;

 protected:
  ScriptInFrameQueryResult FindInFrameSubStree(const blink::Frame& frame) const;

  const blink::Document& document_;
  const ScriptId script_id_;
};

}  // namespace brave_page_graph

#endif  // BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_SCRIPTS_SCRIPT_IN_FRAME_QUERIER_RESULT_H_
