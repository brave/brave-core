/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_DISPATCHERS_H_
#define BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_DISPATCHERS_H_

#include "brave/third_party/blink/brave_page_graph/types.h"

namespace brave_page_graph {

class PageGraph;

void DispatchAttributeChanged(PageGraph* const page_graph,
    const blink::DOMNodeId node_id, const WTF::String& attr_name,
    const WTF::String& old_value, const WTF::String& new_value);

}  // namespace brave_page_graph

#endif  // BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_DISPATCHERS_H_
