/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/brave_page_graph/page_graph.h"

#include <climits>
#include <iostream>
#include <fstream>
#include <map>
#include <memory>
#include <set>
#include <signal.h>
#include <sstream>
#include <string>

#include "gin/public/context_holder.h"
#include "gin/public/gin_embedders.h"

#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/renderer/bindings/core/v8/script_source_code.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_binding_for_core.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/dom_node_ids.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/inspector/protocol/Protocol.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource.h"
#include "third_party/blink/renderer/platform/weborigin/kurl.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

#include "url/gurl.h"

#include "v8/include/v8.h"

#include "brave/components/brave_shields/common/brave_shield_constants.h"

#include "brave/third_party/blink/brave_page_graph/logging.h"
#include "brave/third_party/blink/brave_page_graph/graphml.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge_attribute_set.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge_attribute_delete.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge_cross_dom.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge_event_listener_add.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge_event_listener_remove.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge_execute.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge_execute_attr.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge_filter.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge_import.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge_node_create.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge_node_delete.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge_node_insert.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge_node_remove.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge_resource_block.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge_shield.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge_storage_bucket.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge_storage_clear.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge_storage_delete.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge_storage_read_call.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge_storage_read_result.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge_storage_set.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge_text_change.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge_webapi_call.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge_webapi_result.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/request/edge_request.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/request/edge_request_frame.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/request/edge_request_start.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/request/edge_request_error.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/request/edge_request_complete.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_actor.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_ad_filter.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_dom_root.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_extensions.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_fingerprinting_filter.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_frame.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_html.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_html_element.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_html_text.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_parser.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_resource.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_script.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_script_remote.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_shields.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_shield.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_storage_root.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_storage_cookiejar.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_storage_localstorage.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_storage_sessionstorage.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_tracker_filter.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_webapi.h"
#include "brave/third_party/blink/brave_page_graph/requests/request_tracker.h"
#include "brave/third_party/blink/brave_page_graph/requests/tracked_request.h"
#include "brave/third_party/blink/brave_page_graph/scripts/script_tracker.h"
#include "brave/third_party/blink/brave_page_graph/types.h"
#include "brave/third_party/blink/brave_page_graph/utilities/dispatchers.h"
#include "brave/third_party/blink/brave_page_graph/utilities/request_metadata.h"
#include "brave/third_party/blink/brave_page_graph/utilities/urls.h"

using ::blink::Document;
using ::blink::ExecutionContext;
using ::blink::DOMNodeId;
using ::blink::KURL;
using ::blink::ResourceType;
using ::blink::ScriptSourceCode;
using ::blink::To;
using ::blink::ToExecutionContext;
using ::blink::protocol::Array;
using ::WTF::String;
using ::std::endl;
using ::std::make_unique;
using ::std::map;
using ::std::move;
using ::std::shared_ptr;
using ::std::string;
using ::std::stringstream;
using ::std::to_string;
using ::std::unique_ptr;
using ::v8::Context;
using ::v8::Local;
using ::v8::Isolate;

namespace brave_page_graph {

namespace {
  PageGraph* yuck = nullptr;
}

void write_to_disk(int signal) {
  std::ofstream outfile("/tmp/pagegraph.log");
  string output = yuck->ToGraphML();
  std::cout << output;
  outfile.write(output.c_str(), output.size());
  outfile.close();
}

static constexpr int kV8ContextPerContextDataIndex = static_cast<int>(
    gin::kPerContextDataStartIndex + gin::kEmbedderBlink);

/* static */
PageGraph* PageGraph::GetFromIsolate(Isolate& isolate) {
  Local<Context> context = isolate.GetCurrentContext();
  if (context.IsEmpty() == true) {
    return nullptr;
  }

  return GetFromContext(context);
}

/* static */
PageGraph* PageGraph::GetFromContext(Local<Context> context) {
  if (kV8ContextPerContextDataIndex >=
      context->GetNumberOfEmbedderDataFields()) {
    return nullptr;  // This is not a blink::ExecutionContext.
  }

  ExecutionContext* exec_context = ToExecutionContext(context);
  if (exec_context == nullptr) {
    return nullptr;
  }

  return GetFromExecutionContext(*exec_context);
}

/* static */
PageGraph* PageGraph::GetFromExecutionContext(ExecutionContext& exec_context) {
  if (!exec_context.IsDocument()) {
    return nullptr;
  }

  Document& document = To<Document>(exec_context);
  return document.GetPageGraph();
}

static void OnEvalScriptCompiled(v8::Isolate& isolate,
    const ScriptId parent_script_id, const ScriptId script_id) {
  PageGraph* const page_graph = PageGraph::GetFromIsolate(isolate);
  if (page_graph) {
    page_graph->RegisterScriptCompilationFromEval(parent_script_id, script_id);
  }
}

PageGraph::PageGraph(Document& document) :
    parser_node_(new NodeParser(this)),
    extensions_node_(new NodeExtensions(this)),
    shields_node_(new NodeShields(this)),
    ad_shield_node_(new NodeShield(this, brave_shields::kAds)),
    tracker_shield_node_(new NodeShield(this, brave_shields::kTrackers)),
    js_shield_node_(new NodeShield(this, brave_shields::kJavaScript)),
    fingerprinting_shield_node_(new NodeShield(this,
                                               brave_shields::kFingerprinting)),
    storage_node_(new NodeStorageRoot(this)),
    cookie_jar_node_(new NodeStorageCookieJar(this)),
    local_storage_node_(new NodeStorageLocalStorage(this)),
    session_storage_node_(new NodeStorageSessionStorage(this)),
    document_(document) {
  Log("init");
  Log(" --- ");
  Log(" - " + URLToString(document_.Url()) + " - ");
  Log(" --- ");

  AddNode(parser_node_);
  AddNode(extensions_node_);

  AddNode(shields_node_);
  AddShieldNode(ad_shield_node_);
  AddShieldNode(tracker_shield_node_);
  AddShieldNode(js_shield_node_);
  AddShieldNode(fingerprinting_shield_node_);

  AddNode(storage_node_);
  AddStorageNode(cookie_jar_node_);
  AddStorageNode(local_storage_node_);
  AddStorageNode(session_storage_node_);

  const blink::DOMNodeId root_id = blink::DOMNodeIds::IdForNode(&document);
  html_root_node_ = new NodeDOMRoot(this, root_id);
  AddNode(html_root_node_);
  element_nodes_.emplace(root_id, html_root_node_);
  Log("Root document ID: " + to_string(root_id));

  Isolate* const isolate = document.GetIsolate();
  if (isolate) {
    isolate->SetEvalScriptCompiledFunc(&OnEvalScriptCompiled);
  }

  yuck = this;
  signal(30, &write_to_disk);
}

PageGraph::~PageGraph() {}

NodeHTML* PageGraph::GetHTMLNode(const DOMNodeId node_id) const {
  LOG_ASSERT(element_nodes_.count(node_id) + text_nodes_.count(node_id) == 1);
  if (element_nodes_.count(node_id) == 1) {
    return element_nodes_.at(node_id);
  }
  return text_nodes_.at(node_id);
}

NodeHTMLElement* PageGraph::GetHTMLElementNode(const DOMNodeId node_id) const {
  Log("GetHTMLElementNode) node id: " + to_string(node_id));
  LOG_ASSERT(element_nodes_.count(node_id) == 1);
  return element_nodes_.at(node_id);
}

NodeHTMLText* PageGraph::GetHTMLTextNode(const DOMNodeId node_id) const {
  LOG_ASSERT(text_nodes_.count(node_id) == 1);
  return text_nodes_.at(node_id);
}

void PageGraph::RegisterDocumentRootCreated(const blink::DOMNodeId node_id,
    const blink::DOMNodeId parent_node_id) {
  if (element_nodes_.count(node_id) != 0)
    return;  // Already registered.

  Log("RegisterDocumentRootCreated) node id: " + to_string(node_id)
    + " parent node id: " + to_string(parent_node_id));
  NodeActor* const acting_node = GetCurrentActingNode();

  LOG_ASSERT(element_nodes_.count(parent_node_id) == 1);

  // Create the new DOM root node.
  NodeDOMRoot* const dom_root = new NodeDOMRoot(this, node_id);
  AddNode(dom_root);
  element_nodes_.emplace(node_id, dom_root);
  Log("Child document ID: " + to_string(node_id));

  // Add the node creation edge.
  const EdgeNodeCreate* const creation_edge = new EdgeNodeCreate(this,
      acting_node, dom_root);
  AddEdge(creation_edge);
  dom_root->AddInEdge(creation_edge);
  acting_node->AddOutEdge(creation_edge);

  NodeHTMLElement* const parent_node =
      element_nodes_.at(parent_node_id);

  Node* dom_link_node = nullptr;
  // TODO: Replace NodeFrame with NodeHTMLFrameElement and move URL to
  // NodeDOMRoot. Include some way of detecting NodeHTMLFrameElements and
  // replace this with that.
  if (parent_node->TagName() == "iframe"
      || parent_node->TagName() == "object"
      || parent_node->TagName() == "embed"
      || parent_node->TagName() == "frame"
      || parent_node->TagName() == "portal") {
    if (!parent_node->out_edges_.empty()) {
      // Add the edge from the (most recently added) frame node of the parent
      // frame element. The |out_edges_| list could be empty if the frame
      // element doesn't have an src attribute at first (in case, blink seems to
      // have an empty document with a URL of "about:blank" created for the
      // frame).
      const Edge* const last_edge = parent_node->out_edges_.back();
      dom_link_node = last_edge->in_node_;

      // Also mark the frame node from above as representing a local frame.
      reinterpret_cast<NodeFrame*>(dom_link_node)->SetIsLocalFrame();
    }
  } else {
    dom_link_node = parent_node;
  }

  if (dom_link_node) {
    const EdgeCrossDOM* const structure_edge =
        new EdgeCrossDOM(this, dom_link_node, dom_root);
    AddEdge(structure_edge);
    dom_root->AddInEdge(structure_edge);
    dom_link_node->AddOutEdge(structure_edge);
  }
}

void PageGraph::RegisterHTMLElementNodeCreated(const DOMNodeId node_id,
    const String& tag_name) {
  string local_tag_name(tag_name.Utf8().data());

  Log("RegisterHTMLElementNodeCreated) node id: " + to_string(node_id)
    + " (" + local_tag_name + ")");
  NodeActor* const acting_node = GetCurrentActingNode();

  LOG_ASSERT(element_nodes_.count(node_id) == 0);
  NodeHTMLElement* const new_node = new NodeHTMLElement(this,
    node_id, local_tag_name);

  AddNode(new_node);
  element_nodes_.emplace(node_id, new_node);

  const EdgeNodeCreate* const edge = new EdgeNodeCreate(this,
    acting_node, new_node);
  AddEdge(edge);

  new_node->AddInEdge(edge);
  acting_node->AddOutEdge(edge);
}

void PageGraph::RegisterHTMLTextNodeCreated(const DOMNodeId node_id,
    const String& text) {
  string local_text(text.Utf8().data());

  Log("RegisterHTMLTextNodeCreated) node id: " + to_string(node_id)
    + ", text: '" + local_text + "'");
  NodeActor* const acting_node = GetCurrentActingNode();

  LOG_ASSERT(text_nodes_.count(node_id) == 0);
  NodeHTMLText* const new_node = new NodeHTMLText(this, node_id, local_text);
  AddNode(new_node);
  text_nodes_.emplace(node_id, new_node);

  const EdgeNodeCreate* const edge = new EdgeNodeCreate(this,
    acting_node, new_node);
  AddEdge(edge);

  new_node->AddInEdge(edge);
  acting_node->AddOutEdge(edge);
}

void PageGraph::RegisterHTMLElementNodeInserted(const DOMNodeId node_id,
    const DOMNodeId parent_node_id, const DOMNodeId before_sibling_id) {
  const DOMNodeId inserted_parent_node_id = parent_node_id;

  Log("RegisterHTMLElementNodeInserted) node id: " + to_string(node_id)
    + ", parent id: " + to_string(inserted_parent_node_id)
    + ", prev sibling id: " + to_string(before_sibling_id));
  NodeActor* const acting_node = GetCurrentActingNode();

  LOG_ASSERT(element_nodes_.count(node_id) == 1);
  LOG_ASSERT(element_nodes_.count(parent_node_id) == 1);
  NodeHTMLElement* const inserted_node = element_nodes_.at(node_id);

  const EdgeNodeInsert* const edge = new EdgeNodeInsert(this,
    acting_node, inserted_node, inserted_parent_node_id, before_sibling_id);
  AddEdge(edge);

  inserted_node->AddInEdge(edge);
  acting_node->AddOutEdge(edge);
}

void PageGraph::RegisterHTMLTextNodeInserted(const DOMNodeId node_id,
    const DOMNodeId parent_node_id, const DOMNodeId before_sibling_id) {
  const DOMNodeId inserted_parent_node_id = parent_node_id;

  Log("RegisterHTMLTextNodeInserted) node id: " + to_string(node_id)
    + ", parent id: " + to_string(inserted_parent_node_id)
    + ", prev sibling id: " + to_string(before_sibling_id));
  NodeActor* const acting_node = GetCurrentActingNode();

  LOG_ASSERT(text_nodes_.count(node_id) == 1);
  NodeHTMLText* const inserted_node = text_nodes_.at(node_id);

  const EdgeNodeInsert* const edge = new EdgeNodeInsert(this,
    acting_node, inserted_node, inserted_parent_node_id, before_sibling_id);
  AddEdge(edge);

  inserted_node->AddInEdge(edge);
  acting_node->AddOutEdge(edge);
}

void PageGraph::RegisterHTMLElementNodeRemoved(const DOMNodeId node_id) {
  Log("RegisterHTMLElementNodeRemoved) node id: " + to_string(node_id));
  NodeActor* const acting_node = GetCurrentActingNode();

  LOG_ASSERT(element_nodes_.count(node_id) == 1);
  NodeHTMLElement* const removed_node = element_nodes_.at(node_id);

  const EdgeNodeRemove* const edge = new EdgeNodeRemove(this,
    static_cast<NodeScript*>(acting_node), removed_node);
  AddEdge(edge);

  acting_node->AddOutEdge(edge);
  removed_node->AddInEdge(edge);
}

void PageGraph::RegisterHTMLTextNodeRemoved(const DOMNodeId node_id) {
  Log("RegisterHTMLTextNodeRemoved) node id: " + to_string(node_id));
  NodeActor* const acting_node = GetCurrentActingNode();

  LOG_ASSERT(text_nodes_.count(node_id) == 1);
  NodeHTMLText* const removed_node = text_nodes_.at(node_id);

  const EdgeNodeRemove* const edge = new EdgeNodeRemove(this,
    static_cast<NodeScript*>(acting_node), removed_node);
  AddEdge(edge);

  acting_node->AddOutEdge(edge);
  removed_node->AddInEdge(edge);
}

void PageGraph::RegisterContentFrameSet(const blink::DOMNodeId node_id,
    const WTF::String& url) {
  string local_url(url.Utf8().data());

  LOG_ASSERT(element_nodes_.count(node_id) == 1);

  NodeHTMLElement* const frame_element_node = element_nodes_.at(node_id);

  // Create the new frame node.
  NodeFrame* const frame_node = new NodeFrame(this, local_url);
  AddNode(frame_node);
  Log("NodeFrame url: " + local_url +
      ", parent element node id:" + to_string(node_id));

  // Create the new EdgeRequestFrame linking the frame element node
  // and the frame node.
  EdgeRequestFrame* const request_edge = new EdgeRequestFrame(this,
      frame_element_node, frame_node);
  AddEdge(request_edge);
  frame_node->AddInEdge(request_edge);
  frame_element_node->AddOutEdge(request_edge);
}

void PageGraph::RegisterEventListenerAdd(const blink::DOMNodeId node_id,
    const WTF::String& event_type, const EventListenerId listener_id,
    ScriptId listener_script_id) {
  string local_event_type(event_type.Utf8().data());
  listener_script_id = script_tracker_.ResolveScriptId(listener_script_id);

  Log("RegisterEventListenerAdd) node id: " + to_string(node_id)
    + ", event_type: " + local_event_type
    + ", listener_id: " + to_string(listener_id)
    + ", listener_script_id: " + to_string(listener_script_id));
  NodeActor* const acting_node = GetCurrentActingNode();

  LOG_ASSERT(element_nodes_.count(node_id) == 1);
  NodeHTMLElement* const element_node = element_nodes_.at(node_id);

  const EdgeEventListenerAdd* const edge = new EdgeEventListenerAdd(this,
      acting_node, element_node, local_event_type, listener_id,
      listener_script_id);
  AddEdge(edge);

  element_node->AddInEdge(edge);
  acting_node->AddOutEdge(edge);
}

void PageGraph::RegisterEventListenerRemove(const blink::DOMNodeId node_id,
    const WTF::String& event_type, const EventListenerId listener_id,
    ScriptId listener_script_id) {
  string local_event_type(event_type.Utf8().data());
  listener_script_id = script_tracker_.ResolveScriptId(listener_script_id);

  Log("RegisterEventListenerRemove) node id: " + to_string(node_id)
    + ", event_type: " + local_event_type
    + ", listener_id: " + to_string(listener_id)
    + ", listener_script_id: " + to_string(listener_script_id));
  NodeActor* const acting_node = GetCurrentActingNode();

  LOG_ASSERT(element_nodes_.count(node_id) == 1);
  NodeHTMLElement* const element_node = element_nodes_.at(node_id);

  const EdgeEventListenerRemove* const edge = new EdgeEventListenerRemove(this,
      acting_node, element_node, local_event_type, listener_id,
      listener_script_id);
  AddEdge(edge);

  element_node->AddInEdge(edge);
  acting_node->AddOutEdge(edge);
}

void PageGraph::RegisterInlineStyleSet(const DOMNodeId node_id,
    const String& attr_name, const String& attr_value) {
  string local_attr_name(attr_name.Utf8().data());
  string local_attr_value(attr_value.Utf8().data());

  Log("RegisterInlineStyleSet) node id: " + to_string(node_id)
    + ", attr: " + local_attr_name
    + ", value: " + local_attr_value);
  NodeActor* const acting_node = GetCurrentActingNode();

  LOG_ASSERT(element_nodes_.count(node_id) == 1);
  NodeHTMLElement* const target_node = element_nodes_.at(node_id);

  const EdgeAttributeSet* const edge = new EdgeAttributeSet(this,
    acting_node, target_node, local_attr_name, local_attr_value, true);
  AddEdge(edge);

  acting_node->AddOutEdge(edge);
  target_node->AddInEdge(edge);
}

void PageGraph::RegisterInlineStyleDelete(const DOMNodeId node_id,
    const String& attr_name) {
  string local_attr_name(attr_name.Utf8().data());

  Log("RegisterInlineStyleDelete) node id: " + to_string(node_id)
    + ", attr: " + local_attr_name);
  NodeActor* const acting_node = GetCurrentActingNode();

  LOG_ASSERT(element_nodes_.count(node_id) == 1);
  NodeHTMLElement* const target_node = element_nodes_.at(node_id);

  const EdgeAttributeDelete* const edge = new EdgeAttributeDelete(this,
    acting_node, target_node, local_attr_name, true);
  AddEdge(edge);

  acting_node->AddOutEdge(edge);
  target_node->AddInEdge(edge);
}

void PageGraph::RegisterAttributeSet(const DOMNodeId node_id,
    const String& attr_name, const String& attr_value) {
  string local_attr_name(attr_name.Utf8().data());
  string local_attr_value(attr_value.Utf8().data());

  Log("RegisterAttributeSet) node id: " + to_string(node_id)
    + ", attr: " + local_attr_name
    + ", value: " + local_attr_value);
  NodeActor* const acting_node = GetCurrentActingNode();

  LOG_ASSERT(element_nodes_.count(node_id) == 1);
  NodeHTMLElement* const target_node = element_nodes_.at(node_id);

  const EdgeAttributeSet* const edge = new EdgeAttributeSet(this,
    acting_node, target_node, local_attr_name, local_attr_value);
  AddEdge(edge);

  acting_node->AddOutEdge(edge);
  target_node->AddInEdge(edge);
}

void PageGraph::RegisterAttributeDelete(const DOMNodeId node_id,
    const String& attr_name) {
  string local_attr_name(attr_name.Utf8().data());

  Log("RegisterAttributeDelete) node id: " + to_string(node_id)
    + ", attr: " + local_attr_name);
  NodeActor* const acting_node = GetCurrentActingNode();

  LOG_ASSERT(element_nodes_.count(node_id) == 1);
  NodeHTMLElement* const target_node = element_nodes_.at(node_id);

  const EdgeAttributeDelete* const edge = new EdgeAttributeDelete(this,
    acting_node, target_node, local_attr_name);
  AddEdge(edge);

  acting_node->AddOutEdge(edge);
  target_node->AddInEdge(edge);
}

void PageGraph::RegisterTextNodeChange(const blink::DOMNodeId node_id,
    const String& new_text) {
  Log("RegisterNewTextNodeText) node id: " + to_string(node_id));
  NodeScript* const acting_node = static_cast<NodeScript*>(
    GetCurrentActingNode());

  LOG_ASSERT(text_nodes_.count(node_id) == 1);
  NodeHTMLText* const text_node = text_nodes_.at(node_id);

  string local_new_text(new_text.Utf8().data());
  const EdgeTextChange* const edge = new EdgeTextChange(this,
    acting_node, text_node, local_new_text);
  AddEdge(edge);

  acting_node->AddOutEdge(edge);
  text_node->AddInEdge(edge);
}

void PageGraph::DoRegisterRequestStart(const InspectorId request_id,
    Node* const requesting_node, const std::string& local_url,
    const RequestType type) {
  NodeResource* const requested_node = GetResourceNodeForUrl(local_url);

  const shared_ptr<const TrackedRequestRecord> request_record =
    request_tracker_ .RegisterRequestStart(
        request_id, requesting_node, requested_node, type);

  PossiblyWriteRequestsIntoGraph(request_record);
}

void PageGraph::RegisterRequestStartFromElm(const DOMNodeId node_id,
    const InspectorId request_id, const KURL& url,
    const RequestType type) {
  const KURL normalized_url = NormalizeUrl(url);
  const string local_url(normalized_url.GetString().Utf8().data());

  // For now, explode if we're getting duplicate requests for the same
  // URL in the same document.  This might need to be changed.
  Log("RegisterRequestStartFromElm) node id: " + to_string(node_id)
    + ", request id: " + to_string(request_id) +
    + ", url: " + local_url
    + ", type: " + to_string(type));

  // We should know about the node thats issuing the request.
  LOG_ASSERT(element_nodes_.count(node_id) == 1);

  NodeHTMLElement* const requesting_node = element_nodes_.at(node_id);
  DoRegisterRequestStart(request_id, requesting_node, local_url, type);
}

void PageGraph::RegisterRequestStartFromCurrentScript(
    const InspectorId request_id, const KURL& url, const RequestType type) {
  const KURL normalized_url = NormalizeUrl(url);
  const string local_url(normalized_url.GetString().Utf8().data());

  Log("RegisterRequestStartFromCurrentScript) request id: " + to_string(request_id)
    + ", url: " + local_url
    + ", type: " + to_string(type));
  NodeActor* const acting_node = GetCurrentActingNode();

  if (!acting_node->IsScript()) {
    Log("Skipping, i hope this is pre-fetch...");
    return;
  }
  LOG_ASSERT(acting_node->IsScript());

  DoRegisterRequestStart(request_id, acting_node, local_url, type);
}

// This is basically the same as |RegisterRequestStartFromCurrentScript|,
// except we don't require the acting node to be a script (CSS fetches
// can be initiated by the parser).
void PageGraph::RegisterRequestStartFromCSS(const InspectorId request_id,
    const blink::KURL& url, const RequestType type) {
  NodeActor* const acting_node = GetCurrentActingNode();
  const KURL normalized_url = NormalizeUrl(url);
  const string local_url(normalized_url.GetString().Utf8().data());

  if (acting_node->IsParser()) {
    Log("RegisterRequestStartFromCSS) request id: " + to_string(request_id)
        + ", url: " + local_url
        + ", type: " + to_string(type));
  } else {
    Log("RegisterRequestStartFromCSS) script id: "
        + to_string((static_cast<NodeScript*>(acting_node))->GetScriptId())
        + ", request id: " + to_string(request_id) +
        + ", url: " + local_url
        + ", type: " + to_string(type));
  }

  DoRegisterRequestStart(request_id, acting_node, local_url, type);
}

void PageGraph::RegisterRequestComplete(const InspectorId request_id,
    const ResourceType type, const RequestMetadata& metadata) {
  Log("RegisterRequestComplete) request id: " + to_string(request_id)
    + ", resource type: " + ResourceTypeToString(type));

  const shared_ptr<const TrackedRequestRecord> request_record =
    request_tracker_ .RegisterRequestComplete(request_id, type);

  TrackedRequest* request = request_record->request.get();
  if (request != nullptr) {
    request->SetResponseHeaderString(
        string(metadata.response_header_.Utf8().data()));
    request->SetResponseBodyLength(metadata.response_body_length_);
  }

  PossiblyWriteRequestsIntoGraph(request_record);
}

void PageGraph::RegisterRequestError(const InspectorId request_id,
    const RequestMetadata& metadata) {
  Log("RegisterRequestError) request id: " + to_string(request_id));

  const shared_ptr<const TrackedRequestRecord> request_record =
    request_tracker_ .RegisterRequestError(request_id);

  TrackedRequest* request = request_record->request.get();
  if (request != nullptr) {
    request->SetResponseHeaderString(
        string(metadata.response_header_.Utf8().data()));
    request->SetResponseBodyLength(metadata.response_body_length_);
  }

  PossiblyWriteRequestsIntoGraph(request_record);
}

void PageGraph::RegisterResourceBlockAd(const GURL& url,
    const std::string& rule) {
  const KURL normalized_url = NormalizeUrl(KURL(url));
  const string local_url(normalized_url.GetString().Utf8().data());

  Log("RegisterResourceBlockAd) url: " + local_url
    + ", rule: " + rule);

  NodeResource* const resource_node = GetResourceNodeForUrl(local_url);
  NodeAdFilter* const filter_node = GetAdFilterNodeForRule(rule);

  const EdgeResourceBlock* const edge = new EdgeResourceBlock(this,
      filter_node, resource_node);
  AddEdge(edge);

  resource_node->AddInEdge(edge);
  filter_node->AddOutEdge(edge);
}

void PageGraph::RegisterResourceBlockTracker(const GURL& url,
    const std::string& host) {
  const KURL normalized_url = NormalizeUrl(KURL(url));
  const string local_url(normalized_url.GetString().Utf8().data());

  Log("RegisterResourceBlockTracker) url: " + local_url
    + ", host: " + host);

  NodeResource* const resource_node = GetResourceNodeForUrl(local_url);
  NodeTrackerFilter* const filter_node = GetTrackerFilterNodeForHost(host);

  const EdgeResourceBlock* const edge = new EdgeResourceBlock(this,
      filter_node, resource_node);
  AddEdge(edge);

  resource_node->AddInEdge(edge);
  filter_node->AddOutEdge(edge);
}

void PageGraph::RegisterResourceBlockJavaScript(const GURL& url) {
  const KURL normalized_url = NormalizeUrl(KURL(url));
  const string local_url(normalized_url.GetString().Utf8().data());

  Log("RegisterResourceBlockJavaScript) url: " + local_url);

  NodeResource* const resource_node = GetResourceNodeForUrl(local_url);

  const EdgeResourceBlock* const edge = new EdgeResourceBlock(this,
      js_shield_node_, resource_node);
  AddEdge(edge);

  resource_node->AddInEdge(edge);
  js_shield_node_->AddOutEdge(edge);
}

void PageGraph::RegisterResourceBlockFingerprinting(const GURL& url,
    const FingerprintingRule& rule) {
  const KURL normalized_url = NormalizeUrl(KURL(url));
  const string local_url(normalized_url.GetString().Utf8().data());

  Log("RegisterResourceBlockFingerprinting) url: " + local_url
    + ", rule: " + rule.ToString());

  NodeResource* const resource_node = GetResourceNodeForUrl(local_url);
  NodeFingerprintingFilter* const filter_node =
      GetFingerprintingFilterNodeForRule(rule);

  const EdgeResourceBlock* const edge = new EdgeResourceBlock(this,
      filter_node, resource_node);
  AddEdge(edge);

  resource_node->AddInEdge(edge);
  filter_node->AddOutEdge(edge);
}

void PageGraph::RegisterElmForLocalScript(const DOMNodeId node_id,
    const ScriptSourceCode& code) {
  Log("RegisterElmForLocalScript) node_id: " + to_string(node_id));
  Log("Script: " + string(code.Source().ToString().Utf8().data()));
  script_tracker_.AddScriptSourceForElm(code, node_id);
}

void PageGraph::RegisterElmForRemoteScript(const DOMNodeId node_id,
    const KURL& url) {
  const KURL normalized_url = NormalizeUrl(url);
  Log("RegisterElmForRemoteScript) node_id: " + to_string(node_id)
    + ", url: " + URLToString(normalized_url));
  script_tracker_.AddScriptUrlForElm(normalized_url, node_id);
}

void PageGraph::RegisterJavaScriptURL(const blink::ScriptSourceCode& code) {
  Log("RegisterJavaScriptURL) script: " +
      string(code.Source().ToString().Utf8().data()));
  // Use the document node as the "owning element" of JavaScript URLs for now.
  script_tracker_.AddScriptSourceForElm(code,
      blink::DOMNodeIds::IdForNode(&document_));
}

void PageGraph::RegisterUrlForScriptSource(const KURL& url,
    const ScriptSourceCode& code) {
  const KURL normalized_url = NormalizeUrl(url);
  Log("RegisterUrlForScriptSource) url: " + URLToString(normalized_url));
  script_tracker_.AddCodeFetchedFromUrl(code, normalized_url);
}

void PageGraph::RegisterUrlForExtensionScriptSource(const blink::WebString& url,
    const blink::WebString& code) {
  const String url_string(url.Latin1().c_str(), url.length());
  const String code_string(code.Latin1().c_str(), code.length());
  Log("RegisterUrlForExtensionScriptSource: url: "
    + string(url_string.Utf8().data()));
  script_tracker_.AddExtensionCodeFetchedFromUrl(code_string.Impl()->GetHash(),
    url_string.Impl()->GetHash());
}

void PageGraph::RegisterScriptCompilation(
    const ScriptSourceCode& code, const ScriptId script_id,
    const ScriptType type) {
  Log("RegisterScriptCompilation) script id: " + to_string(script_id));
  Log("source: " + string(code.Source().ToString().Utf8().data()));

  if (type == ScriptType::kScriptTypeModule) {
    NodeScript* const code_node = new NodeScript(this, script_id, type);
    AddNode(code_node);
    script_nodes_.emplace(script_id, code_node);
    return;
  }

  script_tracker_.AddScriptId(script_id,
    code.Source().ToString().Impl()->GetHash());
  script_tracker_.SetScriptIdForCode(script_id, code);

  // Note that at the end of this method, the script node exists in the
  // graph, but isn't connected to anything.  That association
  NodeScript* const code_node = new NodeScript(this, script_id, type);
  AddNode(code_node);
  script_nodes_.emplace(script_id, code_node);

  const ScriptTrackerScriptSource script_source =
    script_tracker_.GetSourceOfScript(script_id);
  if (script_source == kScriptTrackerScriptSourcePage) {
    DOMNodeIdList node_ids = script_tracker_.GetElmsForScriptId(script_id);
    LOG_ASSERT(node_ids.size() > 0);

    for (const DOMNodeId node_id : node_ids) {
      NodeHTMLElement* const script_elm_node = GetHTMLElementNode(node_id);
      EdgeExecute* const execute_edge = new EdgeExecute(this, script_elm_node,
        code_node);
      AddEdge(execute_edge);
      script_elm_node->AddOutEdge(execute_edge);
      code_node->AddInEdge(execute_edge);

      if (script_elm_node->HasAttribute("src")) {
        code_node->SetUrl(script_elm_node->GetAttribute("src"));
      }
    }
  } else {
    EdgeExecute* const execute_edge = new EdgeExecute(this, extensions_node_,
      code_node);
    AddEdge(execute_edge);
    extensions_node_->AddOutEdge(execute_edge);
    code_node->AddInEdge(execute_edge);
  }
}

void PageGraph::RegisterScriptCompilationFromAttr(
    const blink::DOMNodeId node_id, const String& attr_name,
    const String& attr_value, const ScriptId script_id) {
  string local_attr_name(attr_name.Utf8().data());
  string local_attr_value(attr_value.Utf8().data());
  Log("RegisterScriptCompilationFromAttr) script id: "
    + to_string(script_id)
    + ", node id: " + to_string(node_id)
    + ", attr name: " );
  script_tracker_.AddScriptId(script_id, attr_value.Impl()->GetHash());

  NodeScript* const code_node = new NodeScript(this, script_id,
      kScriptTypeClassic);
  AddNode(code_node);
  script_nodes_.emplace(script_id, code_node);

  NodeHTMLElement* const html_node = GetHTMLElementNode(node_id);
  EdgeExecute* const execute_edge = new EdgeExecuteAttr(this, html_node,
      code_node, local_attr_name);
  AddEdge(execute_edge);
  html_node->AddOutEdge(execute_edge);
  code_node->AddInEdge(execute_edge);
}

void PageGraph::RegisterScriptCompilationFromEval(ScriptId parent_script_id,
    const ScriptId script_id) {
  parent_script_id = script_tracker_.ResolveScriptId(parent_script_id);

  if (parent_script_id == 0) {
    return;
  }

  Log("RegisterScriptCompilationFromEval) script id: " + to_string(script_id)
      + ", parent script id: " + to_string(parent_script_id));

  script_tracker_.AddScriptIdAlias(script_id, parent_script_id);
}

// Functions for handling storage read, write, and deletion
void PageGraph::RegisterStorageRead(const String& key, const String& value,
    const StorageLocation location) {
  string local_key(key.Utf8().data());
  string local_value(value.Utf8().data());

  Log("RegisterStorageRead) key: " + local_key + ", value: " + local_value
    + ", location: " + StorageLocationToString(location));
  NodeActor* const acting_node = GetCurrentActingNode();

  LOG_ASSERT(acting_node->IsScript());

  NodeStorage* storage_node = nullptr;
  switch (location) {
    case kStorageLocationCookie:
      storage_node = cookie_jar_node_;
      break;
    case kStorageLocationLocalStorage:
      storage_node = local_storage_node_;
      break;
    case kStorageLocationSessionStorage:
      storage_node = session_storage_node_;
      break;
  }

  const EdgeStorageReadCall* const edge_call = new EdgeStorageReadCall(this,
    static_cast<NodeScript*>(acting_node), storage_node, local_key);
  AddEdge(edge_call);
  acting_node->AddOutEdge(edge_call);
  storage_node->AddInEdge(edge_call);

  const EdgeStorageReadResult* const edge_result = new EdgeStorageReadResult(
    this, storage_node, static_cast<NodeScript*>(acting_node), local_key,
    local_value);
  AddEdge(edge_result);
  storage_node->AddOutEdge(edge_result);
  acting_node->AddInEdge(edge_result);
}

void PageGraph::RegisterStorageWrite(const String& key, const String& value,
    const StorageLocation location) {
  string local_key(key.Utf8().data());
  string local_value(value.Utf8().data());

  Log("RegisterStorageWrite) key: " + local_key + ", value: " + local_value
    + ", location: " + StorageLocationToString(location));
  NodeActor* const acting_node = GetCurrentActingNode();

  LOG_ASSERT(acting_node->IsScript());

  NodeStorage* storage_node = nullptr;
  switch (location) {
    case kStorageLocationCookie:
      storage_node = cookie_jar_node_;
      break;
    case kStorageLocationLocalStorage:
      storage_node = local_storage_node_;
      break;
    case kStorageLocationSessionStorage:
      storage_node = session_storage_node_;
      break;
  }

  const EdgeStorageSet* const edge_storage = new EdgeStorageSet(this,
    static_cast<NodeScript*>(acting_node), storage_node, local_key,
    local_value);
  AddEdge(edge_storage);
  acting_node->AddOutEdge(edge_storage);
  storage_node->AddInEdge(edge_storage);
}

void PageGraph::RegisterStorageDelete(const String& key,
    const StorageLocation location) {
  string local_key(key.Utf8().data());

  Log("RegisterStorageDelete) key: " + local_key + ", location: "
    + StorageLocationToString(location));
  NodeActor* const acting_node = GetCurrentActingNode();

  LOG_ASSERT(acting_node->IsScript());

  NodeStorage* storage_node = nullptr;
  switch (location) {
    case kStorageLocationLocalStorage:
      storage_node = local_storage_node_;
      break;
    case kStorageLocationSessionStorage:
      storage_node = session_storage_node_;
      break;
    case kStorageLocationCookie:
      LOG_ASSERT(location != kStorageLocationCookie);
  }

  const EdgeStorageDelete* const edge_storage = new EdgeStorageDelete(this,
    static_cast<NodeScript*>(acting_node), storage_node, local_key);
  AddEdge(edge_storage);
  acting_node->AddOutEdge(edge_storage);
  storage_node->AddInEdge(edge_storage);
}

void PageGraph::RegisterStorageClear(const StorageLocation location) {
  Log("RegisterStorageClear) location: " + StorageLocationToString(location));
  NodeActor* const acting_node = GetCurrentActingNode();

  LOG_ASSERT(acting_node->IsScript());

  NodeStorage* storage_node = nullptr;
  switch (location) {
    case kStorageLocationLocalStorage:
      storage_node = local_storage_node_;
      break;
    case kStorageLocationSessionStorage:
      storage_node = session_storage_node_;
      break;
    case kStorageLocationCookie:
      LOG_ASSERT(location != kStorageLocationCookie);
  }

  const EdgeStorageClear* const edge_storage = new EdgeStorageClear(this,
    static_cast<NodeScript*>(acting_node), storage_node);
  AddEdge(edge_storage);
  acting_node->AddOutEdge(edge_storage);
  storage_node->AddInEdge(edge_storage);
}

void PageGraph::GenerateReportForNode(const blink::DOMNodeId node_id,
                                      Array<WTF::String>& report) {
  const Node* node;
  if (element_nodes_.count(node_id)) {
    node = element_nodes_.at(node_id);
  } else if (text_nodes_.count(node_id)) {
    node = text_nodes_.at(node_id);
  } else {
    return;
  }

  std::set<const Node*> predecessors;
  for (const unique_ptr<const Edge>& elm : Edges()) {
    if (elm->in_node_ == node)
      predecessors.insert(elm->out_node_);
  }

  std::set<const Node*> successors;
  for (const unique_ptr<const Edge>& elm : Edges()) {
    if (elm->out_node_ == node)
      successors.insert(elm->in_node_);
  }

  for (std::set<const Node*>::iterator it = predecessors.begin();
      it != predecessors.end(); it++) {
    const Node* pred = *it;
    if (pred->IsNodeActor()) {
      for (const Edge* edge : pred->out_edges_) {
        if (edge->in_node_ == node) {
          std::string reportItem(
              edge->GetDescBody() + "\r\n\r\nby: " + pred->GetDescBody()
          );
          report.addItem(WTF::String::FromUTF8(reportItem.data()));
        }
      }
    }
  }

  for (std::set<const Node*>::iterator it = successors.begin();
      it != successors.end(); it++) {
    const Node* succ = *it;
    ItemName item_name = succ->GetItemName();
    if (item_name.find("resource #") == 0) {
      for (const Edge* edge : succ->in_edges_) {
        std::string reportItem(
            edge->GetDescBody() + "\r\n\r\nby: " + edge->out_node_->GetDescBody()
        );
        report.addItem(WTF::String::FromUTF8(reportItem.data()));
      }
    }
  }
}

void PageGraph::RegisterWebAPICall(const MethodName& method,
    const std::vector<const String>& arguments) {
  std::vector<const string> local_args;
  stringstream buffer;
  size_t args_length = arguments.size();
  for (size_t i = 0; i < args_length; ++i) {
    local_args.push_back(arguments[i].Utf8().data());
    if (i != args_length - 1) {
      buffer << local_args.at(i) << ", ";
    } else {
      buffer << local_args.at(i);
    }
  }
  Log("RegisterWebAPICall) method: " + method + ", arguments: "
    + buffer.str());

  NodeActor* const acting_node = GetCurrentActingNode();
  LOG_ASSERT(acting_node->IsScript());

  NodeWebAPI* webapi_node;
  if (webapi_nodes_.count(method) == 0) {
    webapi_node = new NodeWebAPI(this, method);
    AddNode(webapi_node);
    webapi_nodes_.emplace(method, webapi_node);
  } else {
    webapi_node = webapi_nodes_.at(method);
  }

  const EdgeWebAPICall* const edge_call = new EdgeWebAPICall(this,
    static_cast<NodeScript*>(acting_node), webapi_node, method, local_args);
  AddEdge(edge_call);
  acting_node->AddOutEdge(edge_call);
  webapi_node->AddInEdge(edge_call);
}

void PageGraph::RegisterWebAPIResult(const MethodName& method,
    const String& result) {
  const string local_result = result.Utf8().data();
  Log("RegisterWebAPIResult) method: " + method + ", result: " + local_result);

  NodeActor* const caller_node = GetCurrentActingNode();
  LOG_ASSERT(caller_node->IsScript());

  LOG_ASSERT(webapi_nodes_.count(method) != 0);
  NodeWebAPI* webapi_node = webapi_nodes_.at(method);

  const EdgeWebAPIResult* const edge_result = new EdgeWebAPIResult(this,
    webapi_node, static_cast<NodeScript*>(caller_node), method, local_result);

  AddEdge(edge_result);
  webapi_node->AddOutEdge(edge_result);
  caller_node->AddInEdge(edge_result);
}

GraphMLXML PageGraph::ToGraphML() const {
  GraphItem::StartGraphMLExport(id_counter_);

  stringstream builder;
  builder << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << endl
    << "<graphml xmlns=\"http://graphml.graphdrawing.org/xmlns\"" << endl
    << "\t\txmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" << endl
    << "\t\txsi:schemaLocation=\"http://graphml.graphdrawing.org/xmlns" << endl
    << "\t\t\thttp://graphml.graphdrawing.org/xmlns/1.0/graphml.xsd\">" << endl;

  for (const GraphMLAttr* const graphml_attr : GetGraphMLAttrs()) {
    builder << "\t" << graphml_attr->ToDefinition() << "\n";
  }

  builder << "\t<graph id=\"G\" edgedefault=\"directed\">" << endl;

  for (const unique_ptr<Node>& elm : Nodes()) {
    builder << elm->GetGraphMLTag() << endl;
  }
  for (const unique_ptr<const Edge>& elm : Edges()) {
    builder << elm->GetGraphMLTag() << endl;
  }

  builder << "\t</graph>" << endl;
  builder << "</graphml>" << endl;
  return builder.str();
}

NodeActor* PageGraph::GetCurrentActingNode() const {
  const ScriptId current_script_id = GetExecutingScriptId();

  static ScriptId last_reported_script_id = 0;
  const bool should_log = last_reported_script_id != current_script_id;
  last_reported_script_id = current_script_id;
  if (should_log) {
    Log("GetCurrentActingNode) script id: " + to_string(current_script_id));
  }

  return GetNodeActorForScriptId(current_script_id);
}

NodeActor* PageGraph::GetNodeActorForScriptId(const ScriptId script_id) const {
  if (script_id == 0) {
    return parser_node_;
  }

  LOG_ASSERT(script_nodes_.count(script_id) +
    remote_script_nodes_.count(script_id) == 1);

  if (script_nodes_.count(script_id) == 1) {
    return script_nodes_.at(script_id);
  }

  return remote_script_nodes_.at(script_id);
}

ScriptId PageGraph::GetExecutingScriptId() const {
  return script_tracker_.ResolveScriptId(
      document_.GetIsolate()->GetExecutingScriptId());
}

NodeResource* PageGraph::GetResourceNodeForUrl(const std::string& url) {
  if (resource_nodes_.count(url) == 0) {
    NodeResource* const resource_node = new NodeResource(this, url);
    AddNode(resource_node);
    resource_nodes_.emplace(url, resource_node);
    return resource_node;
  }

  return resource_nodes_.at(url);
}

NodeAdFilter* PageGraph::GetAdFilterNodeForRule(const std::string& rule) {
  if (ad_filter_nodes_.count(rule) == 0) {
    NodeAdFilter* const filter_node = new NodeAdFilter(this, rule);
    AddNode(filter_node);
    ad_filter_nodes_.emplace(rule, filter_node);

    const EdgeFilter* const filter_edge = new EdgeFilter(this, ad_shield_node_,
        filter_node);
    AddEdge(filter_edge);

    filter_node->AddInEdge(filter_edge);
    ad_shield_node_->AddOutEdge(filter_edge);

    return filter_node;
  }

  return ad_filter_nodes_.at(rule);
}

NodeTrackerFilter* PageGraph::GetTrackerFilterNodeForHost(
    const std::string& host) {
  if (tracker_filter_nodes_.count(host) == 0) {
    NodeTrackerFilter* const filter_node = new NodeTrackerFilter(this, host);
    AddNode(filter_node);
    tracker_filter_nodes_.emplace(host, filter_node);

    const EdgeFilter* const filter_edge = new EdgeFilter(this,
        tracker_shield_node_, filter_node);
    AddEdge(filter_edge);

    filter_node->AddInEdge(filter_edge);
    tracker_shield_node_->AddOutEdge(filter_edge);

    return filter_node;
  }

  return tracker_filter_nodes_.at(host);
}

NodeFingerprintingFilter* PageGraph::GetFingerprintingFilterNodeForRule(
    const FingerprintingRule& rule) {
  if (fingerprinting_filter_nodes_.count(rule) == 0) {
    NodeFingerprintingFilter* const filter_node =
        new NodeFingerprintingFilter(this, rule);
    AddNode(filter_node);
    fingerprinting_filter_nodes_.emplace(rule, filter_node);

    const EdgeFilter* const filter_edge = new EdgeFilter(this,
        fingerprinting_shield_node_, filter_node);
    AddEdge(filter_edge);

    filter_node->AddInEdge(filter_edge);
    fingerprinting_shield_node_->AddOutEdge(filter_edge);

    return filter_node;
  }

  return fingerprinting_filter_nodes_.at(rule);
}

void PageGraph::PossiblyWriteRequestsIntoGraph(
    const shared_ptr<const TrackedRequestRecord> record) {
  const TrackedRequest* const request = record->request.get();

  // Don't record anything into the graph if we've already recorded
  // this batch of requests (first condition) or if this batch of requests
  // hasn't finished yet (e.g. we don't have both a request and a response)
  // (second condition).
  if (record->is_first_reply == false ||
      request->IsComplete() == false) {
    return;
  }

  NodeResource* const resource = request->GetResource();
  const bool was_error = request->GetIsError();
  const RequestType request_type = request->GetRequestType();
  const InspectorId request_id = request->GetRequestId();

  if (was_error == false) {
    const ResourceType resource_type = request->GetResourceType();
    for (Node* const requester : request->GetRequesters()) {
      const EdgeRequestStart* const start_edge = new EdgeRequestStart(this,
        requester, resource, request_id, request_type);
      AddEdge(start_edge);
      requester->AddOutEdge(start_edge);
      resource->AddInEdge(start_edge);

      const EdgeRequestComplete* const complete_edge = new EdgeRequestComplete(
        this, resource, requester, request_id, resource_type,
        request->ResponseHeaderString(), request->ResponseBodyLength());
      AddEdge(complete_edge);
      resource->AddOutEdge(complete_edge);
      requester->AddInEdge(complete_edge);
    }
    return;
  }

  // Handling the case when the requests returned with errors.
  for (Node* const requester : request->GetRequesters()) {
    const EdgeRequestStart* const start_edge = new EdgeRequestStart(this,
      requester, resource, request_id, request_type);
    AddEdge(start_edge);
    requester->AddOutEdge(start_edge);
    resource->AddInEdge(start_edge);

    const EdgeRequestError* const error_edge = new EdgeRequestError(
      this, resource, requester, request_id,
      request->ResponseHeaderString(), request->ResponseBodyLength());
    AddEdge(error_edge);
    resource->AddOutEdge(error_edge);
    requester->AddInEdge(error_edge);
  }
}

const NodeUniquePtrList& PageGraph::Nodes() const {
  return nodes_;
}

const EdgeUniquePtrList& PageGraph::Edges() const {
  return edges_;
}

const GraphItemList& PageGraph::GraphItems() const {
  return graph_items_;
}

void PageGraph::AddNode(Node* const node) {
  nodes_.push_back(unique_ptr<Node>(node));
  graph_items_.push_back(node);
}

void PageGraph::AddEdge(const Edge* const edge) {
  edges_.push_back(unique_ptr<const Edge>(edge));
  graph_items_.push_back(edge);
}

void PageGraph::AddShieldNode(NodeShield* const shield_node) {
  AddNode(shield_node);

  const EdgeShield* const shield_edge =
      new EdgeShield(this, shields_node_, shield_node);
  AddEdge(shield_edge);

  shield_node->AddInEdge(shield_edge);
  shields_node_->AddOutEdge(shield_edge);
}

void PageGraph::AddStorageNode(NodeStorage* const storage_node) {
  AddNode(storage_node);

  const EdgeStorageBucket* const storage_edge =
      new EdgeStorageBucket(this, storage_node_, storage_node);
  AddEdge(storage_edge);

  storage_node->AddInEdge(storage_edge);
  storage_node_->AddOutEdge(storage_edge);
}

void PageGraph::Log(const string& str) const {
  PG_LOG(str);
}

}  // namespace brave_page_graph
