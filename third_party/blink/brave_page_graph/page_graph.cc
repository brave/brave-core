/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/brave_page_graph/page_graph.h"
#include <signal.h>
#include <climits>
#include <iostream>
#include <fstream>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/renderer/core/dom/dom_node_ids.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource.h"
#include "third_party/blink/renderer/platform/weborigin/kurl.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"
#include "brave/third_party/blink/brave_page_graph/logging.h"
#include "brave/third_party/blink/brave_page_graph/graphml.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge_attribute_set.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge_attribute_delete.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge_execute.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge_node_create.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge_node_delete.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge_node_insert.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge_node_remove.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge_text_change.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/request/edge_request.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/request/edge_request_start.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/request/edge_request_error.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/request/edge_request_complete.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_actor.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_extension.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_html.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_html_element.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_html_text.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_parser.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_resource.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_script.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_shields.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_storage_cookiejar.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_storage_localstorage.h"
#include "brave/third_party/blink/brave_page_graph/script_tracker.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

using ::blink::DOMNodeId;
using ::blink::KURL;
using ::blink::ScriptSourceCode;
using ::blink::ResourceType;
using ::WTF::String;
using ::std::endl;
using ::std::make_unique;
using ::std::map;
using ::std::move;
using ::std::string;
using ::std::stringstream;
using ::std::to_string;
using ::std::unique_ptr;
using ::std::vector;

namespace brave_page_graph {

namespace {
  PageGraph* yuck = nullptr;
}

const string URLToString(const KURL& url) {
  return string(url.GetString().Utf8().data());
}

void write_to_disk(int signal) {
  std::cout << "GOT THAT SIG" << endl;
  std::ofstream outfile("/tmp/pagegraph.log");
  string output = yuck->ToGraphML();
  std::cout << output;
  outfile.write(output.c_str(), output.size());
  outfile.close();
}

PageGraph::PageGraph() :
    parser_node_(new NodeParser(this)),
    shields_node_(new NodeShields(this)),
    cookie_jar_node_(new NodeStorageCookieJar(this)),
    local_storage_node_(new NodeStorageLocalStorage(this)),
    html_root_node_(new NodeHTMLElement(this, kRootNodeId, "(root)")) {
  PG_LOG("init");
  AddNode(parser_node_);
  AddNode(shields_node_);
  AddNode(cookie_jar_node_);
  AddNode(local_storage_node_);
  AddNode(html_root_node_);
  element_nodes_.emplace(kRootNodeId, html_root_node_);
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
  PG_LOG("GetHTMLElementNode: " + to_string(node_id));
  if (node_id == kRootNodeId) {
    return html_root_node_;
  }
  LOG_ASSERT(element_nodes_.count(node_id) == 1);
  return element_nodes_.at(node_id);
}

NodeHTMLText* PageGraph::GetHTMLTextNode(const DOMNodeId node_id) const {
  LOG_ASSERT(text_nodes_.count(node_id) == 1);
  return text_nodes_.at(node_id);
}

void PageGraph::RegisterHTMLElementNodeCreated(const DOMNodeId node_id,
    const String& tag_name) {
  string local_tag_name(tag_name.Utf8().data());
  PG_LOG("RegisterHTMLElementNodeCreated: " + to_string(node_id) + " (" + local_tag_name + ")");
  LOG_ASSERT(element_nodes_.count(node_id) == 0);
  NodeHTMLElement* const new_node = new NodeHTMLElement(this,
    node_id, local_tag_name);

  AddNode(new_node);
  element_nodes_.emplace(node_id, new_node);

  NodeActor* const acting_node = GetCurrentActingNode();

  const EdgeNodeCreate* const edge = new EdgeNodeCreate(this,
    acting_node, new_node);
  AddEdge(edge);

  new_node->AddInEdge(edge);
  acting_node->AddOutEdge(edge);
}

void PageGraph::RegisterHTMLTextNodeCreated(const DOMNodeId node_id,
    const String& text) {
  string local_text(text.Utf8().data());
  PG_LOG("RegisterHTMLTextNodeCreated: " + to_string(node_id) +
    ", '" + local_text + "'");
  LOG_ASSERT(text_nodes_.count(node_id) == 0);
  NodeHTMLText* const new_node = new NodeHTMLText(this, node_id, local_text);
  AddNode(new_node);
  text_nodes_.emplace(node_id, new_node);

  NodeActor* const acting_node = GetCurrentActingNode();

  const EdgeNodeCreate* const edge = new EdgeNodeCreate(this,
    acting_node, new_node);
  AddEdge(edge);

  new_node->AddInEdge(edge);
  acting_node->AddOutEdge(edge);
}

void PageGraph::RegisterHTMLElementNodeInserted(const DOMNodeId node_id,
    const DOMNodeId parent_node_id, const DOMNodeId before_sibling_id) {

  const DOMNodeId inserted_parent_node_id = (parent_node_id)
    ? parent_node_id
    : kRootNodeId;

  PG_LOG("RegisterHTMLElementNodeInserted: " + to_string(node_id) + ", "
    + to_string(inserted_parent_node_id) + ", " + to_string(before_sibling_id));

  LOG_ASSERT(element_nodes_.count(node_id) == 1);
  NodeHTMLElement* const inserted_node = element_nodes_.at(node_id);

  NodeActor* const acting_node = GetCurrentActingNode();

  const EdgeNodeInsert* const edge = new EdgeNodeInsert(this,
    acting_node, inserted_node, inserted_parent_node_id, before_sibling_id);
  AddEdge(edge);

  inserted_node->AddInEdge(edge);
  acting_node->AddOutEdge(edge);
}

void PageGraph::RegisterHTMLTextNodeInserted(const DOMNodeId node_id,
    const DOMNodeId parent_node_id, const DOMNodeId before_sibling_id) {

  const DOMNodeId inserted_parent_node_id = (parent_node_id)
    ? parent_node_id
    : kRootNodeId;

  PG_LOG("RegisterHTMLTextNodeInserted: (" + to_string(node_id) + "), " +
    to_string(inserted_parent_node_id) + ", " + to_string(before_sibling_id));
  LOG_ASSERT(text_nodes_.count(node_id) == 1);
  NodeHTMLText* const inserted_node = text_nodes_.at(node_id);

  NodeActor* const acting_node = GetCurrentActingNode();

  const EdgeNodeInsert* const edge = new EdgeNodeInsert(this,
    acting_node, inserted_node, inserted_parent_node_id, before_sibling_id);
  AddEdge(edge);

  inserted_node->AddInEdge(edge);
  acting_node->AddOutEdge(edge);
}

void PageGraph::RegisterHTMLElementNodeRemoved(const DOMNodeId node_id) {
  PG_LOG("RegisterHTMLElementNodeRemoved: " + to_string(node_id));
  LOG_ASSERT(element_nodes_.count(node_id) == 1);
  NodeHTMLElement* const removed_node = element_nodes_.at(node_id);

  NodeActor* const acting_node = GetCurrentActingNode();

  const EdgeNodeRemove* const edge = new EdgeNodeRemove(this,
    static_cast<NodeScript*>(acting_node), removed_node);
  AddEdge(edge);

  acting_node->AddOutEdge(edge);
  removed_node->AddInEdge(edge);
}

void PageGraph::RegisterHTMLTextNodeRemoved(const DOMNodeId node_id) {
  PG_LOG("RegisterHTMLTextNodeRemoved: " + to_string(node_id));
  LOG_ASSERT(text_nodes_.count(node_id) == 1);
  NodeHTMLText* const removed_node = text_nodes_.at(node_id);

  NodeActor* const acting_node = GetCurrentActingNode();

  const EdgeNodeRemove* const edge = new EdgeNodeRemove(this,
    static_cast<NodeScript*>(acting_node), removed_node);
  AddEdge(edge);

  acting_node->AddOutEdge(edge);
  removed_node->AddInEdge(edge);
}

void PageGraph::RegisterInlineStyleSet(const DOMNodeId node_id,
    const String& attr_name, const String& attr_value) {
  string local_attr_name(attr_name.Utf8().data());
  string local_attr_value(attr_value.Utf8().data());

  PG_LOG("RegisterInlineStyleSet: " + to_string(node_id) + ", "
    + local_attr_name + "=" + local_attr_value);
  LOG_ASSERT(element_nodes_.count(node_id) == 1);

  NodeHTMLElement* const target_node = element_nodes_.at(node_id);
  NodeActor* const acting_node = GetCurrentActingNode();

  const EdgeAttributeSet* const edge = new EdgeAttributeSet(this,
    acting_node, target_node, local_attr_name, local_attr_value, true);
  AddEdge(edge);

  acting_node->AddOutEdge(edge);
  target_node->AddInEdge(edge);
}

void PageGraph::RegisterInlineStyleDelete(const DOMNodeId node_id,
    const String& attr_name) {
  string local_attr_name(attr_name.Utf8().data());
  PG_LOG("RegisterInlineStyleDelete: " + to_string(node_id) + ", "
    + local_attr_name + "=");
  LOG_ASSERT(element_nodes_.count(node_id) == 1);

  NodeHTMLElement* const target_node = element_nodes_.at(node_id);
  NodeActor* const acting_node = GetCurrentActingNode();

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

  PG_LOG("RegisterAttributeSet: " + to_string(node_id) + ", "
    + local_attr_name + "=" + local_attr_value);
  LOG_ASSERT(element_nodes_.count(node_id) == 1);

  NodeHTMLElement* const target_node = element_nodes_.at(node_id);
  NodeActor* const acting_node = GetCurrentActingNode();

  const EdgeAttributeSet* const edge = new EdgeAttributeSet(this,
    acting_node, target_node, local_attr_name, local_attr_value);
  AddEdge(edge);

  acting_node->AddOutEdge(edge);
  target_node->AddInEdge(edge);
}

void PageGraph::RegisterAttributeDelete(const DOMNodeId node_id,
    const String& attr_name) {
  string local_attr_name(attr_name.Utf8().data());
  PG_LOG("RegisterAttributeDelete: " + to_string(node_id) + ", "
    + local_attr_name + "=");
  LOG_ASSERT(element_nodes_.count(node_id) == 1);

  NodeHTMLElement* const target_node = element_nodes_.at(node_id);

  NodeActor* const acting_node = GetCurrentActingNode();
  const EdgeAttributeDelete* const edge = new EdgeAttributeDelete(this,
    acting_node, target_node, local_attr_name);
  AddEdge(edge);

  acting_node->AddOutEdge(edge);
  target_node->AddInEdge(edge);
}

void PageGraph::RegisterTextNodeChange(const blink::DOMNodeId node_id,
    const WTF::String& new_text) {
  PG_LOG("RegisterNewTextNodeText: " + to_string(node_id));
  LOG_ASSERT(text_nodes_.count(node_id) == 1);

  NodeHTMLText* const text_node = text_nodes_.at(node_id);
  NodeScript* const acting_node = static_cast<NodeScript*>(
    GetCurrentActingNode());

  string local_new_text(new_text.Utf8().data());
  const EdgeTextChange* const edge = new EdgeTextChange(this,
    acting_node, text_node, local_new_text);
  AddEdge(edge);

  acting_node->AddOutEdge(edge);
  text_node->AddInEdge(edge);
}

void PageGraph::RegisterRequestStartFromElm(const DOMNodeId node_id,
    const InspectorId request_id, const KURL& url,
    const RequestType type) {
  const string local_url(url.GetString().Utf8().data());
  // For now, explode if we're getting duplicate requests for the same
  // URL in the same document.  This might need to be changed.
  PG_LOG("RegisterRequestStartFromElm: " + to_string(node_id)
    + ", request id: " + to_string(request_id) +
    + ", url:" + local_url
    + ", type: " + to_string(type));

  // We should know about the node thats issuing the request.
  LOG_ASSERT(element_nodes_.count(node_id) == 1);
  // We should also not have seen a request with this id before.
  LOG_ASSERT(current_requests_.count(request_id) == 0);

  NodeHTMLElement* const requesting_node = element_nodes_.at(node_id);
  NodeResource* requested_node;
  if (resource_nodes_.count(local_url) == 0) {
    PG_LOG("RegisterRequestStartFromElm: First time seeing request for " + local_url);
    requested_node = new NodeResource(this, local_url);
    AddNode(requested_node);
    resource_nodes_.emplace(local_url, requested_node);
  } else {
    requested_node = resource_nodes_.at(local_url);
  }

  const EdgeRequestStart* const edge = new EdgeRequestStart(this,
    requesting_node, requested_node, request_id, type);
  current_requests_.emplace(request_id, edge);
  AddEdge(edge);

  requesting_node->AddOutEdge(edge);
  requested_node->AddInEdge(edge);
}

void PageGraph::RegisterRequestStartFromCurrentScript(
    const InspectorId request_id, const KURL& url, const RequestType type) {
  NodeActor* const acting_node = GetCurrentActingNode();
  LOG_ASSERT(acting_node->IsScript());
  const string local_url(url.GetString().Utf8().data());

  PG_LOG("RegisterRequestStartFromCurrentScript: script id:"
    + to_string((static_cast<NodeScript*>(acting_node))->GetScriptId())
    + ", request id: " + to_string(request_id) +
    + ", url:" + local_url
    + ", type: " + to_string(type));

  NodeResource* requested_node;
  if (resource_nodes_.count(local_url) == 0) {
    PG_LOG("RegisterRequestStartFromCurrentScript: First time seeing request for " + local_url);
    requested_node = new NodeResource(this, local_url);
    AddNode(requested_node);
    resource_nodes_.emplace(local_url, requested_node);
  } else {
    requested_node = resource_nodes_.at(local_url);
  }

  const EdgeRequestStart* const edge = new EdgeRequestStart(this,
    acting_node, requested_node, request_id, type);
  current_requests_.emplace(request_id, edge);
  AddEdge(edge);

  acting_node->AddOutEdge(edge);
  requested_node->AddInEdge(edge);
}

void PageGraph::RegisterRequestComplete(const InspectorId request_id,
    const ResourceType type) {
  PG_LOG("RegisterRequestComplete: " + to_string(request_id) +
    ", successful: " + resource_type_to_string(type));
  // There should be an outstanding request that is being closed here,
  // otherwise, there is a request we didn't correctly register being sent.
  LOG_ASSERT(current_requests_.count(request_id) == 1);

  const EdgeRequestStart* const request_start_edge = current_requests_.at(request_id);
  NodeResource* const resource_node = request_start_edge->GetResourceNode();
  Node* const requesting_node = request_start_edge->GetRequestingNode();

  const EdgeRequestComplete* const request_edge = new EdgeRequestComplete(
    this, resource_node, requesting_node, request_id, type);
  current_requests_.erase(request_id);
  AddEdge(request_edge);

  resource_node->AddOutEdge(request_edge);
  requesting_node->AddInEdge(request_edge);
}

void PageGraph::RegisterRequestError(const InspectorId request_id) {
  PG_LOG("RegisterRequestError: " + to_string(request_id));
  // There should be an outstanding request that is being closed here,
  // otherwise, there is a request we didn't correctly register being sent.
  LOG_ASSERT(current_requests_.count(request_id) == 1);

  const EdgeRequestStart* const request_start_edge = current_requests_.at(request_id);
  NodeResource* const resource_node = request_start_edge->GetResourceNode();
  Node* const requesting_node = request_start_edge->GetRequestingNode();

  const EdgeRequestError* const request_edge = new EdgeRequestError(
    this, resource_node, requesting_node, request_id);
  current_requests_.erase(request_id);
  AddEdge(request_edge);

  resource_node->AddOutEdge(request_edge);
  requesting_node->AddInEdge(request_edge);
}

void PageGraph::RegisterElmForLocalScript(const DOMNodeId node_id,
    const ScriptSourceCode& code) {
  PG_LOG("RegisterElmForLocalScript: node_id: " + to_string(node_id));
  script_tracker_.AddScriptSourceForElm(code, node_id);
}

void PageGraph::RegisterElmForRemoteScript(const DOMNodeId node_id,
    const KURL& url) {
  PG_LOG("RegisterElmForRemoteScript: node_id: " + to_string(node_id) +
    " url: " + URLToString(url));
  script_tracker_.AddScriptUrlForElm(url, node_id);
}

void PageGraph::RegisterUrlForScriptSource(const KURL& url,
    const ScriptSourceCode& code) {
  PG_LOG("RegisterUrlForScriptSource: url: " + URLToString(url));
  script_tracker_.AddCodeFetchedFromUrl(code, url);
}

void PageGraph::RegisterUrlForExtensionScriptSource(const blink::WebString& url,
    const blink::WebString& code) {
  const WTF::String url_string(url.Latin1().c_str(), url.length());
  const WTF::String code_string(code.Latin1().c_str(), code.length());
  PG_LOG("RegisterUrlForExtensionScriptSource: url: " +
    string(url_string.Utf8().data()));
  script_tracker_.AddExtensionCodeFetchedFromUrl(code_string.Impl()->GetHash(),
    url_string.Impl()->GetHash());
}

void PageGraph::RegisterScriptCompilation(
    const ScriptSourceCode& code, const ScriptId script_id,
    const ScriptType type) {
  PG_LOG("RegisterScriptCompilation: " + to_string(script_id));
  script_tracker_.AddTopLevelScriptId(script_id);
  script_tracker_.SetScriptIdForCode(script_id, code);

  // Note that at the end of this method, the script node exists in the
  // graph, but isn't connected to anything.  That association
  NodeScript* const code_node = new NodeScript(this, script_id, type);
  AddNode(code_node);
  script_nodes_.emplace(script_id, code_node);

  const ScriptTrackerScriptSource script_source = script_tracker_.GetSourceOfScript(script_id);
  if (script_source == kScriptTrackerScriptSourcePage) {
    vector<DOMNodeId> node_ids = script_tracker_.GetElmsForScriptId(script_id);
    LOG_ASSERT(node_ids.size() > 0);

    for (const DOMNodeId node_id : node_ids) {
      NodeHTMLElement* const script_elm_node = GetHTMLElementNode(node_id);
      EdgeExecute* const execute_edge = new EdgeExecute(this, script_elm_node,
        code_node);
      AddEdge(execute_edge);
      script_elm_node->AddOutEdge(execute_edge);
      code_node->AddInEdge(execute_edge);
    }
  } else {
    NodeExtension* const extension_node = GetExtensionNode();
    EdgeExecute* const execute_edge = new EdgeExecute(this, extension_node,
      code_node);
    AddEdge(execute_edge);
    extension_node->AddOutEdge(execute_edge);
    code_node->AddInEdge(execute_edge);
  }
}

void PageGraph::RegisterTopLevelScriptId(const ScriptId script_id) {
  PG_LOG("RegisterTopLevelScriptId: " + to_string(script_id));
  script_tracker_.AddTopLevelScriptId(script_id);
}

void PageGraph::RegisterChildScriptIdForParentScriptId(
    const ScriptId child_script_id, const ScriptId parent_script_id) {
  PG_LOG("RegisterChildScriptIdForParentScriptId: parent: " +
    to_string(parent_script_id) + " -> child: " +
    to_string(child_script_id));
  script_tracker_.AddChildScriptIdForParentScriptId(child_script_id,
    parent_script_id);
}

void PageGraph::RegisterScriptExecStart(const ScriptId script_id) {
  PG_LOG("RegisterScriptExecStart: " + to_string(script_id));
  const ScriptId top_script_id = script_tracker_.TopLevelScriptIdForScriptId(
    script_id);
  LOG_ASSERT(script_nodes_.count(top_script_id) == 1);
  PushActiveScript(top_script_id);
}

void PageGraph::RegisterScriptExecStop(const ScriptId script_id) {
  PG_LOG("RegisterScriptExecStop: " + to_string(script_id));
  const ScriptId top_script_id = script_tracker_.TopLevelScriptIdForScriptId(
    script_id);
  LOG_ASSERT(script_nodes_.count(top_script_id) == 1);
  ScriptId popped_script_id = PopActiveScript();
  LOG_ASSERT(popped_script_id == top_script_id);
}

GraphMLXML PageGraph::ToGraphML() const {
  stringstream builder;
  builder << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << endl;
  builder << "<graphml xmlns=\"http://graphml.graphdrawing.org/xmlns\"" << endl;
  builder << "\t\txmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" << endl;
  builder << "\t\txsi:schemaLocation=\"http://graphml.graphdrawing.org/xmlns" << endl;
  builder << "\t\t\thttp://graphml.graphdrawing.org/xmlns/1.0/graphml.xsd\">" << endl;

  for (const GraphMLAttr* const graphml_attr : get_graphml_attrs()) {
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
  const ScriptId current_script_id = PeekActiveScript();
  PG_LOG("GetCurrentActingNode: script_id: " + to_string(current_script_id));
  if (current_script_id == 0) {
    PG_LOG("GetCurrentActingNode: NodeParser");
    return parser_node_;
  }
  LOG_ASSERT(script_nodes_.count(current_script_id) == 1);
  return script_nodes_.at(current_script_id);
}

const vector<unique_ptr<Node> >& PageGraph::Nodes() const {
  return nodes_;
}

const vector<unique_ptr<const Edge> >& PageGraph::Edges() const {
  return edges_;
}

const vector<const GraphItem*>& PageGraph::GraphItems() const {
  return graph_items_;
}

ChildFrameId PageGraph::GetNewChildFrameId() {
  return ++current_max_child_frame_id_;
}

void PageGraph::AddNode(Node* const node) {
  nodes_.push_back(unique_ptr<Node>(node));
  graph_items_.push_back(node);
}

void PageGraph::AddEdge(const Edge* const edge) {
  edges_.push_back(unique_ptr<const Edge>(edge));
  graph_items_.push_back(edge);
}

vector<DOMNodeId> PageGraph::NodeIdsForScriptId(const ScriptId script_id) const {
  PG_LOG("NodeIdsForScriptId: script_id: " + to_string(script_id));
  return script_tracker_.GetElmsForScriptId(script_id);
}

vector<ScriptId> PageGraph::ScriptIdsForNodeId(const DOMNodeId node_id) const {
  PG_LOG("ScriptIdsForNodeId: node_id: " + to_string(node_id));
  return script_tracker_.GetScriptIdsForElm(node_id);
}

NodeExtension* PageGraph::GetExtensionNode() {
  if (extension_node_ == nullptr) {
    extension_node_ = new NodeExtension(this);
    AddNode(extension_node_);
  }
  return extension_node_;
}

void PageGraph::PushActiveScript(const ScriptId script_id) {
  PG_LOG("PushActiveScript: script_id: " + to_string(script_id));
  active_script_stack_.push_back(script_id);
}

ScriptId PageGraph::PopActiveScript() {
  PG_LOG("PopActiveScript");
  ScriptId top_script_id = 0;
  if (active_script_stack_.empty() == false) {
    top_script_id = active_script_stack_.back();
    active_script_stack_.pop_back();
  }
  return top_script_id;
}

ScriptId PageGraph::PeekActiveScript() const {
  if (active_script_stack_.empty() == true) {
    return 0;
  }
  return active_script_stack_.back();
}



}  // namespace brave_page_graph
