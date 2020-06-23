/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/brave_page_graph/page_graph.h"

#include <climits>
#include <chrono>
#include <iostream>
#include <fstream>
#include <map>
#include <memory>
#include <set>
#include <signal.h>
#include <sstream>
#include <string>

#include <libxml/tree.h>

#include "gin/public/context_holder.h"
#include "gin/public/gin_embedders.h"

#include "third_party/blink/public/platform/web_string.h"

#include "third_party/blink/renderer/bindings/core/v8/script_source_code.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_binding_for_core.h"

#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/dom_node_ids.h"
#include "third_party/blink/renderer/core/dom/node.h"

#include "third_party/blink/renderer/core/execution_context/execution_context.h"

#include "third_party/blink/renderer/core/inspector/protocol/Protocol.h"

#include "third_party/blink/renderer/platform/loader/fetch/resource.h"
#include "third_party/blink/renderer/platform/weborigin/kurl.h"

#include "third_party/blink/renderer/platform/wtf/casting.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

#include "url/gurl.h"

#include "v8/include/v8.h"

#include "brave/components/brave_shields/common/brave_shield_constants.h"

#include "brave/third_party/blink/brave_page_graph/graphml.h"
#include "brave/third_party/blink/brave_page_graph/logging.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge_cross_dom.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge_filter.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge_resource_block.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge_shield.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge_text_change.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/edge/attribute/edge_attribute_set.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/attribute/edge_attribute_delete.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/edge/event_listener/edge_event_listener_add.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/event_listener/edge_event_listener_remove.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/edge/execute/edge_execute.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/execute/edge_execute_attr.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/edge/node/edge_node_create.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/node/edge_node_delete.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/node/edge_node_insert.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/node/edge_node_remove.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/edge/request/edge_request.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/request/edge_request_start.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/request/edge_request_error.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/request/edge_request_complete.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/edge/storage/edge_storage_bucket.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/storage/edge_storage_clear.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/storage/edge_storage_delete.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/storage/edge_storage_read_call.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/storage/edge_storage_read_result.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/storage/edge_storage_set.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/edge/js/edge_js_call.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/js/edge_js_result.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_extensions.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_remote_frame.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_resource.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/node/actor/node_actor.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/actor/node_parser.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/actor/node_script.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/node/filter/node_ad_filter.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/filter/node_fingerprinting_filter.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/filter/node_tracker_filter.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/node/html/node_dom_root.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/html/node_frame_owner.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/html/node_html.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/html/node_html_element.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/html/node_html_text.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/node/shield/node_shields.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/shield/node_shield.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/node/storage/node_storage_root.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/storage/node_storage_cookiejar.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/storage/node_storage_localstorage.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/storage/node_storage_sessionstorage.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/node/js/node_js.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/js/node_js_builtin.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/js/node_js_webapi.h"

#include "brave/third_party/blink/brave_page_graph/requests/request_tracker.h"
#include "brave/third_party/blink/brave_page_graph/requests/tracked_request.h"

#include "brave/third_party/blink/brave_page_graph/scripts/script_tracker.h"

#include "brave/third_party/blink/brave_page_graph/utilities/dispatchers.h"
#include "brave/third_party/blink/brave_page_graph/utilities/response_metadata.h"
#include "brave/third_party/blink/brave_page_graph/utilities/urls.h"

using ::std::endl;
using ::std::make_unique;
using ::std::map;
using ::std::move;
using ::std::shared_ptr;
using ::std::string;
using ::std::stringstream;
using ::std::to_string;
using ::std::unique_ptr;
using ::std::vector;

using ::blink::Document;
using ::blink::DynamicTo;
using ::blink::IsA;
using ::blink::ExecutionContext;
using ::blink::DOMNodeId;
using ::blink::KURL;
using ::blink::ResourceType;
using ::blink::ScriptSourceCode;
using ::blink::To;
using ::blink::ToExecutionContext;
using ::blink::protocol::Array;

using ::v8::Context;
using ::v8::HandleScope;
using ::v8::Isolate;
using ::v8::Local;

using ::WTF::String;

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
  HandleScope handle_scope(&isolate);

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

static void OnBuiltInFuncCall(v8::Isolate& isolate, const char* built_in_name,
    const vector<const string>& args) {
  PageGraph* const page_graph = PageGraph::GetFromIsolate(isolate);
  if (page_graph) {
    page_graph->RegisterJSBuiltInCall(
        JSBuiltInFromString(built_in_name), args);
  }
}

static void OnBuiltInFuncResponse(v8::Isolate& isolate,
    const char* built_in_name, const string& value) {
  PageGraph* const page_graph = PageGraph::GetFromIsolate(isolate);
  if (page_graph) {
    page_graph->RegisterJSBuiltInResponse(
        JSBuiltInFromString(built_in_name), value);
  }
}

PageGraph::PageGraph(blink::ExecutionContext& execution_context,
    const DOMNodeId node_id, const WTF::String& tag_name,
    const blink::KURL& url) :
      parser_node_(new NodeParser(this)),
      extensions_node_(new NodeExtensions(this)),
      shields_node_(new NodeShields(this)),
      ad_shield_node_(new NodeShield(this, brave_shields::kAds)),
      tracker_shield_node_(new NodeShield(this, brave_shields::kTrackers)),
      js_shield_node_(new NodeShield(this, brave_shields::kJavaScript)),
      fingerprinting_shield_node_(
          new NodeShield(this, brave_shields::kFingerprinting)),
      storage_node_(new NodeStorageRoot(this)),
      cookie_jar_node_(new NodeStorageCookieJar(this)),
      local_storage_node_(new NodeStorageLocalStorage(this)),
      session_storage_node_(new NodeStorageSessionStorage(this)),
      execution_context_(execution_context),
      start_(std::chrono::high_resolution_clock::now()) {
  const string local_tag_name(tag_name.Utf8().data());

  const KURL normalized_url = NormalizeUrl(url);
  const string local_url(normalized_url.GetString().Utf8().data());

  Log("init");
  Log(" --- ");
  Log(" - " + local_url + " - ");
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

  html_root_node_ = new NodeDOMRoot(this, node_id, local_tag_name, local_url);
  AddNode(html_root_node_);
  element_nodes_.emplace(node_id, html_root_node_);
  Log("Root document ID: " + to_string(node_id));

  Isolate* const isolate = execution_context_.GetIsolate();
  if (isolate) {
    isolate->SetEvalScriptCompiledFunc(&OnEvalScriptCompiled);
    isolate->SetBuiltInFuncCallFunc(&OnBuiltInFuncCall);
    isolate->SetBuiltInFuncResponseFunc(&OnBuiltInFuncResponse);
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

NodeScript* PageGraph::GetScriptNode(const ScriptId script_id) const {
  Log("GetScriptNode) script id: " + to_string(script_id));
  LOG_ASSERT(script_nodes_.count(script_id) == 1);
  return script_nodes_.at(script_id);
}

void PageGraph::RegisterDocumentRootCreated(const blink::DOMNodeId node_id,
    const blink::DOMNodeId parent_node_id, const String& tag_name,
    const KURL& url) {
  if (element_nodes_.count(node_id) != 0) {
    return;  // Already registered.
  }

  const string local_tag_name(tag_name.Utf8().data());

  const KURL normalized_url = NormalizeUrl(url);
  const string local_url(normalized_url.GetString().Utf8().data());

  Log("RegisterDocumentRootCreated) node id: " + to_string(node_id)
    + ", parent node id: " + to_string(parent_node_id)
    + ", tag name: " + local_tag_name
    + ", url: " + local_url);
  NodeActor* const acting_node = GetCurrentActingNode();

  LOG_ASSERT(element_nodes_.count(parent_node_id) == 1);

  // Create the new DOM root node.
  NodeDOMRoot* const dom_root = new NodeDOMRoot(this, node_id, local_tag_name,
                                                local_url);
  AddNode(dom_root);
  element_nodes_.emplace(node_id, dom_root);
  Log("Child document ID: " + to_string(node_id));

  // Add the node creation edge.
  AddEdge(new EdgeNodeCreate(this, acting_node, dom_root));

  // Add the cross-DOM edge.
  NodeHTMLElement* const parent_node = element_nodes_.at(parent_node_id);
  if (NodeDOMRoot* const dom_root_parent_node =
          DynamicTo<NodeDOMRoot>(parent_node)) {
    AddEdge(new EdgeCrossDOM(this, dom_root_parent_node, dom_root));
  } else if (NodeFrameOwner* const frame_owner_parent_node =
                 DynamicTo<NodeFrameOwner>(parent_node)) {
    AddEdge(new EdgeCrossDOM(this, frame_owner_parent_node, dom_root));
  } else {
    LOG_ASSERT(false); // Unsupported parent node type.
  }
}

void PageGraph::RegisterRemoteFrameCreated(
    const blink::DOMNodeId parent_node_id, const GURL& url) {
  const KURL normalized_url = NormalizeUrl(KURL(url));
  const string local_url(normalized_url.GetString().Utf8().data());

  Log("RegisterRemoteFrameCreated) parent node id: " + to_string(parent_node_id)
    + ", url: " + local_url);

  LOG_ASSERT(element_nodes_.count(parent_node_id) == 1);

  // Create the new remote frame node.
  NodeRemoteFrame* const remote_frame = new NodeRemoteFrame(this, local_url);
  AddNode(remote_frame);

  // Add the cross-DOM edge.
  NodeFrameOwner* const parent_node =
      To<NodeFrameOwner>(element_nodes_.at(parent_node_id));
  AddEdge(new EdgeCrossDOM(this, parent_node, remote_frame));
}


void PageGraph::RegisterHTMLElementNodeCreated(const DOMNodeId node_id,
    const String& tag_name, const ElementType element_type) {
  string local_tag_name(tag_name.Utf8().data());

  Log("RegisterHTMLElementNodeCreated) node id: " + to_string(node_id)
    + " (" + local_tag_name + ")");
  NodeActor* const acting_node = GetCurrentActingNode();

  LOG_ASSERT(element_nodes_.count(node_id) == 0);

  NodeHTMLElement* new_node = nullptr;
  switch (element_type) {
    case kElementTypeDefault: {
      new_node = new NodeHTMLElement(this, node_id, local_tag_name);
      break;
    }
    case kElementTypeFrameOwner: {
      new_node = new NodeFrameOwner(this, node_id, local_tag_name);
      Log("(type = kElementTypeFrameOwner");
      break;
    }
  }
  LOG_ASSERT(new_node);

  AddNode(new_node);
  element_nodes_.emplace(node_id, new_node);

  AddEdge(new EdgeNodeCreate(this, acting_node, new_node));
}

void PageGraph::TryRegisterHTMLElementNodeCreated(const DOMNodeId node_id,
    const String& tag_name, const ElementType element_type) {
  if(element_nodes_.count(node_id) == 0) {
    RegisterHTMLElementNodeCreated(node_id, tag_name, element_type);
  }
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

  AddEdge(new EdgeNodeCreate(this, acting_node, new_node));
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
  LOG_ASSERT(before_sibling_id == 0
                 || element_nodes_.count(before_sibling_id) +
                        text_nodes_.count(before_sibling_id) == 1);
  NodeHTMLElement* const inserted_node = element_nodes_.at(node_id);

  AddEdge(new EdgeNodeInsert(this, acting_node, inserted_node,
                             inserted_parent_node_id, before_sibling_id));
}

void PageGraph::RegisterHTMLTextNodeInserted(const DOMNodeId node_id,
    const DOMNodeId parent_node_id, const DOMNodeId before_sibling_id) {
  const DOMNodeId inserted_parent_node_id = parent_node_id;

  Log("RegisterHTMLTextNodeInserted) node id: " + to_string(node_id)
    + ", parent id: " + to_string(inserted_parent_node_id)
    + ", prev sibling id: " + to_string(before_sibling_id));
  NodeActor* const acting_node = GetCurrentActingNode();

  LOG_ASSERT(text_nodes_.count(node_id) == 1);
  LOG_ASSERT(element_nodes_.count(parent_node_id) == 1);
  LOG_ASSERT(before_sibling_id == 0
                 || element_nodes_.count(before_sibling_id) +
                        text_nodes_.count(before_sibling_id) == 1);
  NodeHTMLText* const inserted_node = text_nodes_.at(node_id);

  AddEdge(new EdgeNodeInsert(this, acting_node, inserted_node,
          inserted_parent_node_id, before_sibling_id));
}

void PageGraph::RegisterHTMLElementNodeRemoved(const DOMNodeId node_id) {
  Log("RegisterHTMLElementNodeRemoved) node id: " + to_string(node_id));
  NodeActor* const acting_node = GetCurrentActingNode();

  LOG_ASSERT(element_nodes_.count(node_id) == 1);
  NodeHTMLElement* const removed_node = element_nodes_.at(node_id);

  AddEdge(new EdgeNodeRemove(this, static_cast<NodeScript*>(acting_node),
                             removed_node));
}

void PageGraph::RegisterHTMLTextNodeRemoved(const DOMNodeId node_id) {
  Log("RegisterHTMLTextNodeRemoved) node id: " + to_string(node_id));
  NodeActor* const acting_node = GetCurrentActingNode();

  LOG_ASSERT(text_nodes_.count(node_id) == 1);
  NodeHTMLText* const removed_node = text_nodes_.at(node_id);

  AddEdge(new EdgeNodeRemove(this, static_cast<NodeScript*>(acting_node),
                             removed_node));
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

  AddEdge(new EdgeEventListenerAdd(this, acting_node, element_node,
                                   local_event_type, listener_id,
                                   listener_script_id));
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

  AddEdge(new EdgeEventListenerRemove(this, acting_node, element_node,
                                      local_event_type, listener_id,
                                      listener_script_id));
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

  AddEdge(new EdgeAttributeSet(this, acting_node, target_node, local_attr_name,
                               local_attr_value, true));
}

void PageGraph::RegisterInlineStyleDelete(const DOMNodeId node_id,
    const String& attr_name) {
  string local_attr_name(attr_name.Utf8().data());

  Log("RegisterInlineStyleDelete) node id: " + to_string(node_id)
    + ", attr: " + local_attr_name);
  NodeActor* const acting_node = GetCurrentActingNode();

  LOG_ASSERT(element_nodes_.count(node_id) == 1);
  NodeHTMLElement* const target_node = element_nodes_.at(node_id);

  AddEdge(new EdgeAttributeDelete(this, acting_node, target_node,
                                  local_attr_name, true));
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

  AddEdge(new EdgeAttributeSet(this, acting_node, target_node, local_attr_name,
                               local_attr_value));
}

void PageGraph::RegisterAttributeDelete(const DOMNodeId node_id,
    const String& attr_name) {
  string local_attr_name(attr_name.Utf8().data());

  Log("RegisterAttributeDelete) node id: " + to_string(node_id)
    + ", attr: " + local_attr_name);
  NodeActor* const acting_node = GetCurrentActingNode();

  LOG_ASSERT(element_nodes_.count(node_id) == 1);
  NodeHTMLElement* const target_node = element_nodes_.at(node_id);

  AddEdge(new EdgeAttributeDelete(this, acting_node, target_node,
                                  local_attr_name));
}

void PageGraph::RegisterTextNodeChange(const blink::DOMNodeId node_id,
    const String& new_text) {
  Log("RegisterNewTextNodeText) node id: " + to_string(node_id));
  NodeScript* const acting_node = static_cast<NodeScript*>(
    GetCurrentActingNode());

  LOG_ASSERT(text_nodes_.count(node_id) == 1);
  NodeHTMLText* const text_node = text_nodes_.at(node_id);

  string local_new_text(new_text.Utf8().data());

  AddEdge(new EdgeTextChange(this, acting_node, text_node, local_new_text));
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

  if (!IsA<NodeScript>(acting_node)) {
    Log("Skipping, I hope this is pre-fetch...");
    return;
  }

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

  if (IsA<NodeParser>(acting_node)) {
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
    const ResourceType type, const ResponseMetadata& metadata,
    const string& resource_hash) {
  Log("RegisterRequestComplete) request id: " + to_string(request_id)
    + ", resource type: " + ResourceTypeToString(type)
    + ", hash: " + resource_hash);

  const shared_ptr<const TrackedRequestRecord> request_record =
    request_tracker_ .RegisterRequestComplete(request_id, type);

  TrackedRequest* request = request_record->request.get();
  if (request != nullptr) {
    request->SetResponseMetadata(metadata);
    request->SetResponseBodyHash(resource_hash);
  }

  PossiblyWriteRequestsIntoGraph(request_record);
}

void PageGraph::RegisterRequestError(const InspectorId request_id,
    const ResponseMetadata& metadata) {
  Log("RegisterRequestError) request id: " + to_string(request_id));

  const shared_ptr<const TrackedRequestRecord> request_record =
    request_tracker_ .RegisterRequestError(request_id);

  TrackedRequest* request = request_record->request.get();
  if (request != nullptr) {
    request->SetResponseMetadata(metadata);
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

  AddEdge(new EdgeResourceBlock(this, filter_node, resource_node));
}

void PageGraph::RegisterResourceBlockTracker(const GURL& url,
    const std::string& host) {
  const KURL normalized_url = NormalizeUrl(KURL(url));
  const string local_url(normalized_url.GetString().Utf8().data());

  Log("RegisterResourceBlockTracker) url: " + local_url
    + ", host: " + host);

  NodeResource* const resource_node = GetResourceNodeForUrl(local_url);
  NodeTrackerFilter* const filter_node = GetTrackerFilterNodeForHost(host);

  AddEdge(new EdgeResourceBlock(this, filter_node, resource_node));
}

void PageGraph::RegisterResourceBlockJavaScript(const GURL& url) {
  const KURL normalized_url = NormalizeUrl(KURL(url));
  const string local_url(normalized_url.GetString().Utf8().data());

  Log("RegisterResourceBlockJavaScript) url: " + local_url);

  NodeResource* const resource_node = GetResourceNodeForUrl(local_url);

  AddEdge(new EdgeResourceBlock(this, js_shield_node_, resource_node));
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

  AddEdge(new EdgeResourceBlock(this, filter_node, resource_node));
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
  script_tracker_.AddScriptSourceForElm(code, html_root_node_->GetNodeId());
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
    script_tracker_.AddScriptId(script_id,
      code.Source().ToString().Impl()->GetHash());
    script_tracker_.SetScriptIdForCode(script_id, code);

    NodeScript* const code_node = new NodeScript(this, script_id, type);
    AddNode(code_node);
    script_nodes_.emplace(script_id, code_node);

    // If this is a root-level module script, it can still be associated with
    // an HTML script element
    DOMNodeIdList node_ids = script_tracker_.GetElmsForScriptId(script_id);
    for (const DOMNodeId node_id : node_ids) {
      NodeHTMLElement* const script_elm_node = GetHTMLElementNode(node_id);
      AddEdge(new EdgeExecute(this, script_elm_node, code_node));
    }

    // Other module scripts are pulled by URL from a parent module script
    ScriptIdList parent_script_ids = script_tracker_.GetModuleScriptParentsForScriptId(script_id);
    for (const ScriptId parent_script_id : parent_script_ids) {
      NodeScript* const parent_node = GetScriptNode(parent_script_id);
      AddEdge(new EdgeExecute(this, parent_node, code_node));
    }

    // Dynamically imported module scripts are pulled by URL in a different way
    //ScriptIdList dynamic_parent_script_ids = script_tracker_.Get

    // The URL for a script only gets set by AddEdge if it comes from a script
    // element with the src attribute set. We need to add it manually for
    // scripts pulled in by another module script.
    if (node_ids.size() == 0) {
      const blink::KURL source_url = script_tracker_.GetModuleScriptSourceUrl(script_id);
      code_node->SetURL(source_url.GetString().Utf8());
    }

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
      AddEdge(new EdgeExecute(this, script_elm_node, code_node));
    }
  } else {
    AddEdge(new EdgeExecute(this, extensions_node_, code_node));
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
  AddEdge(new EdgeExecuteAttr(this, html_node, code_node, local_attr_name));
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

void PageGraph::RegisterModuleScriptForDescendant(const blink::KURL& parent_location,
    const blink::KURL& descendant_location) {
  const KURL parent_location_norm = NormalizeUrl(parent_location);
  const KURL descendant_location_norm = NormalizeUrl(descendant_location);
  Log("RegisterModuleScriptForDescendant) parent location: " + URLToString(parent_location) + ", descendant location: " + URLToString(descendant_location));
  script_tracker_.AddDescendantUrlForParent(descendant_location_norm, parent_location_norm);
}

void PageGraph::RegisterModuleScriptForDescendant(const ScriptId parent_id,
    const blink::KURL& descendant_location) {
  const KURL descendant_location_norm = NormalizeUrl(descendant_location);
  Log("RegisterModuleScriptForDescendant) parent id: " + to_string(parent_id) + ", descendant location: " + URLToString(descendant_location));
  script_tracker_.AddDescendantUrlForParent(descendant_location_norm, parent_id);
}

// Functions for handling storage read, write, and deletion
void PageGraph::RegisterStorageRead(const String& key, const String& value,
    const StorageLocation location) {
  string local_key(key.Utf8().data());
  string local_value(value.Utf8().data());

  Log("RegisterStorageRead) key: " + local_key + ", value: " + local_value
    + ", location: " + StorageLocationToString(location));
  NodeActor* const acting_node = GetCurrentActingNode();

  LOG_ASSERT(IsA<NodeScript>(acting_node));

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

  AddEdge(new EdgeStorageReadCall(this, static_cast<NodeScript*>(acting_node),
                                  storage_node, local_key));
  AddEdge(new EdgeStorageReadResult(this, storage_node,
                                    static_cast<NodeScript*>(acting_node),
                                    local_key, local_value));
}

void PageGraph::RegisterStorageWrite(const String& key, const String& value,
    const StorageLocation location) {
  string local_key(key.Utf8().data());
  string local_value(value.Utf8().data());

  Log("RegisterStorageWrite) key: " + local_key + ", value: " + local_value
    + ", location: " + StorageLocationToString(location));
  NodeActor* const acting_node = GetCurrentActingNode();

  LOG_ASSERT(IsA<NodeScript>(acting_node));

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

  AddEdge(new EdgeStorageSet(this, static_cast<NodeScript*>(acting_node),
                             storage_node, local_key, local_value));
}

void PageGraph::RegisterStorageDelete(const String& key,
    const StorageLocation location) {
  string local_key(key.Utf8().data());

  Log("RegisterStorageDelete) key: " + local_key + ", location: "
    + StorageLocationToString(location));
  NodeActor* const acting_node = GetCurrentActingNode();

  LOG_ASSERT(IsA<NodeScript>(acting_node));

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

  AddEdge(new EdgeStorageDelete(this, static_cast<NodeScript*>(acting_node),
                                storage_node, local_key));
}

void PageGraph::RegisterStorageClear(const StorageLocation location) {
  Log("RegisterStorageClear) location: " + StorageLocationToString(location));
  NodeActor* const acting_node = GetCurrentActingNode();

  LOG_ASSERT(IsA<NodeScript>(acting_node));

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

  AddEdge(new EdgeStorageClear(this, static_cast<NodeScript*>(acting_node),
                               storage_node));
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
    if (IsA<NodeActor>(pred)) {
      for (const Edge* edge : pred->out_edges_) {
        if (edge->in_node_ == node) {
          std::string reportItem(
              edge->GetItemDesc() + "\r\n\r\nby: " + pred->GetItemDesc()
          );
          report.push_back(WTF::String::FromUTF8(reportItem.data()));
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
            edge->GetItemDesc()
            + "\r\n\r\nby: " + edge->out_node_->GetItemDesc()
        );
        report.push_back(WTF::String::FromUTF8(reportItem.data()));
      }
    }
  }
}

void PageGraph::RegisterWebAPICall(const WebAPI web_api,
    const vector<const String>& arguments) {
  RegisterWebAPICall(WebAPIToString(web_api), arguments);
}

void PageGraph::RegisterWebAPICall(const MethodName& method,
    const vector<const String>& arguments) {
  vector<const string> local_args;
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
  LOG_ASSERT(IsA<NodeScript>(acting_node));

  NodeJSWebAPI* webapi_node;
  if (webapi_nodes_.count(method) == 0) {
    webapi_node = new NodeJSWebAPI(this, method);
    AddNode(webapi_node);
    webapi_nodes_.emplace(method, webapi_node);
  } else {
    webapi_node = webapi_nodes_.at(method);
  }

  AddEdge(new EdgeJSCall(this, static_cast<NodeScript*>(acting_node),
    webapi_node, local_args));
}

void PageGraph::RegisterWebAPIResult(const WebAPI web_api,
    const String& result) {
  RegisterWebAPIResult(WebAPIToString(web_api), result);
}

void PageGraph::RegisterWebAPIResult(const MethodName& method,
    const String& result) {
  const string local_result = result.Utf8().data();
  Log("RegisterWebAPIResult) method: " + method + ", result: " + local_result);

  NodeActor* const caller_node = GetCurrentActingNode();
  LOG_ASSERT(IsA<NodeScript>(caller_node));

  LOG_ASSERT(webapi_nodes_.count(method) != 0);
  NodeJSWebAPI* webapi_node = webapi_nodes_.at(method);

  AddEdge(new EdgeJSResult(this, webapi_node,
    static_cast<NodeScript*>(caller_node), local_result));
}

void PageGraph::RegisterJSBuiltInCall(const JSBuiltIn built_in,
    const vector<const string>& arguments) {
  vector<const string> local_args;
  stringstream buffer;
  const size_t args_length = arguments.size();
  for (size_t i = 0; i < args_length; ++i) {
    local_args.push_back(arguments[i]);
    if (i != args_length - 1) {
      buffer << local_args.at(i) << ", ";
    } else {
      buffer << local_args.at(i);
    }
  }
  Log("RegisterJSBuiltInCall) built in: " + JSBuiltInToSting(built_in)
    + ", arguments: " + buffer.str());

  NodeActor* const acting_node = GetCurrentActingNode();
  LOG_ASSERT(IsA<NodeScript>(acting_node));

  NodeJSBuiltIn* js_built_in_node;
  if (builtin_js_nodes_.count(built_in) == 0) {
    js_built_in_node = new NodeJSBuiltIn(this, built_in);
    AddNode(js_built_in_node);
    builtin_js_nodes_.emplace(built_in, js_built_in_node);
  } else {
    js_built_in_node = builtin_js_nodes_.at(built_in);
  }

  AddEdge(new EdgeJSCall(this, static_cast<NodeScript*>(acting_node),
    js_built_in_node, local_args));
}

void PageGraph::RegisterJSBuiltInResponse(const JSBuiltIn built_in,
    const string& value) {
  const string local_result(value);
  Log("RegisterJSBuiltInResponse) built in: " + JSBuiltInToSting(built_in)
    + ", result: " + local_result);

  NodeActor* const caller_node = GetCurrentActingNode();
  LOG_ASSERT(IsA<NodeScript>(caller_node));

  LOG_ASSERT(builtin_js_nodes_.count(built_in) != 0);
  NodeJSBuiltIn* js_built_in_node = builtin_js_nodes_.at(built_in);

  AddEdge(new EdgeJSResult(this, js_built_in_node,
    static_cast<NodeScript*>(caller_node), local_result));
}

string PageGraph::ToGraphML() const {
  GraphItem::StartGraphMLExport(id_counter_);

  xmlDocPtr graphml_doc = xmlNewDoc(BAD_CAST "1.0");
  xmlNodePtr graphml_root_node = xmlNewNode(NULL, BAD_CAST "graphml");
  xmlDocSetRootElement(graphml_doc, graphml_root_node);

  xmlNewNs(graphml_root_node,
      BAD_CAST "http://graphml.graphdrawing.org/xmlns", NULL);
  xmlNsPtr xsi_ns = xmlNewNs(graphml_root_node,
      BAD_CAST "http://www.w3.org/2001/XMLSchema-instance", BAD_CAST "xsi");
  xmlNewNsProp(graphml_root_node, xsi_ns, BAD_CAST "schemaLocation",
      BAD_CAST "http://graphml.graphdrawing.org/xmlns http://graphml.graphdrawing.org/xmlns/1.0/graphml.xsd");

  for (const GraphMLAttr* const graphml_attr : GetGraphMLAttrs()) {
    graphml_attr->AddDefinitionNode(graphml_root_node);
  }

  xmlNodePtr graph_node = xmlNewChild(graphml_root_node, NULL,
      BAD_CAST "graph", NULL);
  xmlSetProp(graph_node, BAD_CAST "id", BAD_CAST "G");
  xmlSetProp(graph_node, BAD_CAST "edgedefault", BAD_CAST "directed");

  for (const unique_ptr<Node>& elm : Nodes()) {
    elm->AddGraphMLTag(graphml_doc, graph_node);
  }
  for (const unique_ptr<const Edge>& elm : Edges()) {
    elm->AddGraphMLTag(graphml_doc, graph_node);
  }

  xmlChar *xml_string;
  int size;
  xmlDocDumpMemoryEnc(graphml_doc, &xml_string, &size, "UTF-8");
  const string graphml_string(reinterpret_cast<const char*>(xml_string));

  xmlFree(xml_string);
  xmlFree(graphml_doc);

  return graphml_string;
}

const std::chrono::time_point<std::chrono::high_resolution_clock>&
    PageGraph::GetTimestamp() const {
  return start_;
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

  LOG_ASSERT(script_nodes_.count(script_id) == 1);
  return script_nodes_.at(script_id);
}

ScriptId PageGraph::GetExecutingScriptId() const {
  return script_tracker_.ResolveScriptId(
      execution_context_.GetIsolate()->GetExecutingScriptId());
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

    AddEdge(new EdgeFilter(this, ad_shield_node_, filter_node));

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

    AddEdge(new EdgeFilter(this, tracker_shield_node_, filter_node));

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

    AddEdge(new EdgeFilter(this, fingerprinting_shield_node_, filter_node));

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
  if (!record->is_first_reply || !request->IsComplete()) {
    Log("Not (yet) writing request id: " + to_string(request->GetRequestId()));
    return;
  }

  NodeResource* const resource = request->GetResource();
  const bool was_error = request->GetIsError();
  const RequestType request_type = request->GetRequestType();
  const InspectorId request_id = request->GetRequestId();

  if (was_error) {
    // Handling the case when the requests returned with errors.
    for (Node* const requester : request->GetRequesters()) {
      AddEdge(new EdgeRequestStart(this, requester, resource, request_id,
                                   request_type));
      AddEdge(new EdgeRequestError(this, resource, requester, request_id,
                                   request->GetResponseMetadata()));
    }
  } else {
    const ResourceType resource_type = request->GetResourceType();
    for (Node* const requester : request->GetRequesters()) {
      AddEdge(new EdgeRequestStart(this, requester, resource, request_id,
                                   request_type));
      AddEdge(new EdgeRequestComplete(this, resource, requester, request_id,
                                      resource_type,
                                      request->GetResponseMetadata(),
                                      request->GetResponseBodyHash()));
    }
    return;
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

  edge->GetInNode()->AddInEdge(edge);
  edge->GetOutNode()->AddOutEdge(edge);
}

void PageGraph::AddShieldNode(NodeShield* const shield_node) {
  AddNode(shield_node);
  AddEdge(new EdgeShield(this, shields_node_, shield_node));
}

void PageGraph::AddStorageNode(NodeStorage* const storage_node) {
  AddNode(storage_node);
  AddEdge(new EdgeStorageBucket(this, storage_node_, storage_node));
}

void PageGraph::Log(const string& str) const {
  PG_LOG(str);
}

}  // namespace brave_page_graph
