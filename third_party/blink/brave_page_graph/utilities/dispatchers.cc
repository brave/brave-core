/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/brave_page_graph/utilities/dispatchers.h"
#include "third_party/blink/renderer/core/dom/dom_node_ids.h"
#include "third_party/blink/renderer/platform/wtf/text/atomic_string.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"
#include "brave/third_party/blink/brave_page_graph/page_graph.h"

namespace brave_page_graph {

void DispatchAttributeChanged(PageGraph* const page_graph,
    const blink::DOMNodeId node_id, const WTF::String& attr_name,
    const WTF::String& old_value, const WTF::String& new_value) {
  if (new_value == WTF::g_null_atom) {
    // Attribute delete.
    page_graph->RegisterAttributeDelete(node_id, attr_name);
  } else {
    // Attribute set - can further differentiate attribute creation
    // based on |old_value|.
    page_graph->RegisterAttributeSet(node_id, attr_name, new_value);
  }
}

}  // namespace brave_page_graph
