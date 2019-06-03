#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"
#include "brave/third_party/blink/brave_page_graph/dispatcher.h"
#include <iostream>

namespace brave_page_graph {

void DispatchAttributeChanged(PageGraph *page_graph,
    const blink::DOMNodeId node_id,
    const WTF::String &attr_name,
    const WTF::String &old_value, const WTF::String &new_value) {
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