#ifndef BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_DISPATCHER_H_
#define BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_DISPATCHER_H_

#include "brave/third_party/blink/brave_page_graph/page_graph.h"

namespace brave_page_graph {

void DispatchAttributeChanged(PageGraph *page_graph,
    const blink::DOMNodeId node_id,
    const WTF::String &attr_name,
    const WTF::String &old_value, const WTF::String &new_value);

}  // namespace brave_page_graph

#endif  // BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_DISPATCHER_H_