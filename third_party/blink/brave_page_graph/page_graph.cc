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
#include "third_party/blink/renderer/bindings/core/v8/script_source_code.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/dom_node_ids.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource.h"
#include "third_party/blink/renderer/platform/weborigin/kurl.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

#include "brave/third_party/blink/brave_page_graph/logging.h"
#include "brave/third_party/blink/brave_page_graph/graphml.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge_attribute_set.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge_attribute_delete.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge_cross_dom.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge_execute.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge_execute_attr.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge_import.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge_node_create.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge_node_delete.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge_node_insert.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge_node_remove.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge_text_change.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/request/edge_request.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/request/edge_request_frame.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/request/edge_request_start.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/request/edge_request_error.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/request/edge_request_complete.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_actor.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_dom_root.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_extension.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_frame.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_html.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_html_element.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_html_text.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_parser.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_resource.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_script.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_script_remote.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_shields.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_storage_cookiejar.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_storage_localstorage.h"
#include "brave/third_party/blink/brave_page_graph/requests/request_tracker.h"
#include "brave/third_party/blink/brave_page_graph/requests/tracked_request.h"
#include "brave/third_party/blink/brave_page_graph/scripts/script_tracker.h"
#include "brave/third_party/blink/brave_page_graph/types.h"
#include "brave/third_party/blink/brave_page_graph/utilities/urls.h"


using ::blink::Document;
using ::blink::DOMNodeId;
using ::blink::KURL;
using ::blink::ScriptSourceCode;
using ::blink::ResourceType;
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

PageGraph::PageGraph(Document& document) :
    parser_node_(new NodeParser(this)),
    shields_node_(new NodeShields(this)),
    cookie_jar_node_(new NodeStorageCookieJar(this)),
    local_storage_node_(new NodeStorageLocalStorage(this)),
    document_(document) {
  Log("init");
  Log(" --- ");
  Log(" - " + URLToString(document_.Url()) + " - ");
  Log(" --- ");
  AddNode(parser_node_);
  AddNode(shields_node_);
  AddNode(cookie_jar_node_);
  AddNode(local_storage_node_);

  const blink::DOMNodeId root_id = blink::DOMNodeIds::IdForNode(&document);
  html_root_node_ = new NodeDOMRoot(this, root_id);
  AddNode(html_root_node_);
  element_nodes_.emplace(root_id, html_root_node_);
  Log("Root document ID: " + to_string(root_id));

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
  LOG_ASSERT(node_id != kRootNodeId);
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

  LOG_ASSERT(element_nodes_.count(parent_node_id) == 1);

  // Create the new DOM root node.
  NodeDOMRoot* const dom_root = new NodeDOMRoot(this, node_id);
  AddNode(dom_root);
  element_nodes_.emplace(node_id, dom_root);
  Log("Child document ID: " + to_string(node_id));

  // Add the node creation edge.
  NodeActor* const acting_node = GetCurrentActingNode();
  const EdgeNodeCreate* const creation_edge = new EdgeNodeCreate(this,
      acting_node, dom_root);
  AddEdge(creation_edge);
  dom_root->AddInEdge(creation_edge);
  acting_node->AddOutEdge(creation_edge);

  NodeHTMLElement* const frame_element_node =
      element_nodes_.at(parent_node_id);
  if (!frame_element_node->out_edges_.empty()) {
    // Add the edge from the (most recently added) frame node of the
    // parent frame element. The |out_edges_| list could be empty if
    // the frame element doesn't have an src attribute at first (in case,
    // blink seems to have an empty document with a URL of "about:blank"
    // created for the frame).
    const Edge* const last_edge = frame_element_node->out_edges_.back();
    Node* const last_node = last_edge->in_node_;
    const EdgeCrossDOM* const structure_edge =
        new EdgeCrossDOM(this, last_node, dom_root);
    AddEdge(structure_edge);
    dom_root->AddInEdge(structure_edge);
    last_node->AddOutEdge(structure_edge);
    // Also mark the frame node from above as representing a local frame.
    reinterpret_cast<NodeFrame*>(last_node)->SetIsLocalFrame();
  }
}

void PageGraph::RegisterHTMLElementNodeCreated(const DOMNodeId node_id,
    const String& tag_name) {
  string local_tag_name(tag_name.Utf8().data());
  Log("RegisterHTMLElementNodeCreated) node id: " + to_string(node_id)
    + " (" + local_tag_name + ")");

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
  Log("RegisterHTMLTextNodeCreated) node id: " + to_string(node_id)
    + ", text: '" + local_text + "'");
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

  LOG_ASSERT(parent_node_id != kRootNodeId);
  const DOMNodeId inserted_parent_node_id = parent_node_id;

  Log("RegisterHTMLElementNodeInserted) node id: " + to_string(node_id)
    + ", parent id: " + to_string(inserted_parent_node_id)
    + ", prev sibling id: " + to_string(before_sibling_id));

  LOG_ASSERT(element_nodes_.count(node_id) == 1);
  LOG_ASSERT(element_nodes_.count(parent_node_id) == 1);
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

  LOG_ASSERT(parent_node_id != kRootNodeId);
  const DOMNodeId inserted_parent_node_id = parent_node_id;

  Log("RegisterHTMLTextNodeInserted) node id: " + to_string(node_id)
    + ", parent id: " + to_string(inserted_parent_node_id)
    + ", prev sibling id: " + to_string(before_sibling_id));

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
  Log("RegisterHTMLElementNodeRemoved) node id: " + to_string(node_id));

  if (element_nodes_.count(node_id) == 0) {
    string local_node_name(blink::DOMNodeIds::NodeForId(node_id)->nodeName().Utf8().data());
    Log("looks like we're missing: " + local_node_name);
    string local_url(blink::DOMNodeIds::NodeForId(node_id)->GetDocument().Url().GetString().Utf8().data());
    Log("We're in frame: " + local_url);
    return;
  }
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
  Log("RegisterHTMLTextNodeRemoved) node id: " + to_string(node_id));

  LOG_ASSERT(text_nodes_.count(node_id) == 1);
  NodeHTMLText* const removed_node = text_nodes_.at(node_id);

  NodeActor* const acting_node = GetCurrentActingNode();

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

void PageGraph::RegisterInlineStyleSet(const DOMNodeId node_id,
    const String& attr_name, const String& attr_value) {
  string local_attr_name(attr_name.Utf8().data());
  string local_attr_value(attr_value.Utf8().data());

  Log("RegisterInlineStyleSet) node id: " + to_string(node_id)
    + ", attr: " + local_attr_name
    + ", value: " + local_attr_value);

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

  Log("RegisterInlineStyleDelete) node id: " + to_string(node_id)
    + ", attr: " + local_attr_name);

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

  Log("RegisterAttributeSet) node id: " + to_string(node_id)
    + ", attr: " + local_attr_name
    + ", value: " + local_attr_value);

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

  Log("RegisterAttributeDelete) node id: " + to_string(node_id)
    + ", attr: " + local_attr_name);

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
  Log("RegisterNewTextNodeText) node id: " + to_string(node_id));
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
  NodeResource* requested_node;
  if (resource_nodes_.count(local_url) == 0) {
    requested_node = new NodeResource(this, local_url);
    AddNode(requested_node);
    resource_nodes_.emplace(local_url, requested_node);
  } else {
    requested_node = resource_nodes_.at(local_url);
  }

  const shared_ptr<const TrackedRequestRecord> request_record =
    request_tracker_ .RegisterRequestStart(
        request_id, requesting_node, requested_node, type);

  PossiblyWriteRequestsIntoGraph(request_record);
}

void PageGraph::RegisterRequestStartFromCurrentScript(
    const InspectorId request_id, const KURL& url, const RequestType type) {
  NodeActor* const acting_node = GetCurrentActingNode();
  const KURL normalized_url = NormalizeUrl(url);
  const string local_url(normalized_url.GetString().Utf8().data());

  Log("RegisterRequestStartFromCurrentScript) script id: "
    + to_string((static_cast<NodeScript*>(acting_node))->GetScriptId())
    + ", request id: " + to_string(request_id) +
    + ", url: " + local_url
    + ", type: " + to_string(type));
  if (!acting_node->IsScript()) {
    Log("Skipping, i hope this is pre-fetch...");
    return;
  }
  LOG_ASSERT(acting_node->IsScript());

  NodeResource* requested_node;
  if (resource_nodes_.count(local_url) == 0) {
    requested_node = new NodeResource(this, local_url);
    AddNode(requested_node);
    resource_nodes_.emplace(local_url, requested_node);
  } else {
    requested_node = resource_nodes_.at(local_url);
  }

  const shared_ptr<const TrackedRequestRecord> request_record =
    request_tracker_ .RegisterRequestStart(
        request_id, acting_node, requested_node, type);

  PossiblyWriteRequestsIntoGraph(request_record);
}

void PageGraph::RegisterRequestComplete(const InspectorId request_id,
    const ResourceType type) {
  Log("RegisterRequestComplete) request id: " + to_string(request_id)
    + ", resource type: " + ResourceTypeToString(type));

  const shared_ptr<const TrackedRequestRecord> request_record =
    request_tracker_ .RegisterRequestComplete(request_id, type);

  PossiblyWriteRequestsIntoGraph(request_record);
}

void PageGraph::RegisterRequestError(const InspectorId request_id) {
  Log("RegisterRequestError) request id: " + to_string(request_id));

  const shared_ptr<const TrackedRequestRecord> request_record =
    request_tracker_ .RegisterRequestError(request_id);

  PossiblyWriteRequestsIntoGraph(request_record);
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

void PageGraph::RegisterUrlForScriptSource(const KURL& url,
    const ScriptSourceCode& code) {
  const KURL normalized_url = NormalizeUrl(url);
  Log("RegisterUrlForScriptSource) url: " + URLToString(normalized_url));
  script_tracker_.AddCodeFetchedFromUrl(code, normalized_url);
}

void PageGraph::RegisterUrlForExtensionScriptSource(const blink::WebString& url,
    const blink::WebString& code) {
  const WTF::String url_string(url.Latin1().c_str(), url.length());
  const WTF::String code_string(code.Latin1().c_str(), code.length());
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

void PageGraph::RegisterScriptCompilationFromAttr(
    const blink::DOMNodeId node_id, const WTF::String& attr_name,
    const WTF::String& attr_value, const ScriptId script_id) {
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

void PageGraph::RegisterScriptExecStart(const ScriptId script_id) {
  static ScriptId prev_script_id = 0;
  // Just keep the logs a little quieter...
  if (script_id != prev_script_id) {
    Log("RegisterScriptExecStart) script id: " + to_string(script_id));
    prev_script_id = script_id;
  }

  LOG_ASSERT(script_nodes_.count(script_id) == 1);
  if (script_nodes_.count(script_id) == 1) {
    PushActiveScript(script_id);
    return;
  }
}

void PageGraph::RegisterScriptExecStop(const ScriptId script_id) {
  Log("RegisterScriptExecStop) script id: " + to_string(script_id));
  LOG_ASSERT(script_nodes_.count(script_id) == 1);
  ScriptId popped_script_id = PopActiveScript();
  if (popped_script_id != script_id) {
    Log("* Expected " + to_string(script_id) + " but got " + to_string(popped_script_id));
  }
  LOG_ASSERT(popped_script_id == script_id);
}

GraphMLXML PageGraph::ToGraphML() const {
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
  static ScriptId last_reported_script_id = 0;
  const ScriptId current_script_id = PeekActiveScript();
  const bool should_log = last_reported_script_id != current_script_id;
  last_reported_script_id = current_script_id;

  if (should_log) {
    Log("GetCurrentActingNode) script id: " + to_string(current_script_id));
  }

  if (current_script_id == 0) {
    if (should_log) {
      Log("GetCurrentActingNode) NodeParser");
    }
    return parser_node_;
  }

  LOG_ASSERT(script_nodes_.count(current_script_id) +
    remote_script_nodes_.count(current_script_id) == 1);

  if (script_nodes_.count(current_script_id) == 1) {
    return script_nodes_.at(current_script_id);
  }

  return remote_script_nodes_.at(current_script_id);
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
        this, resource, requester, request_id, resource_type);
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
      this, resource, requester, request_id);
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

NodeExtension* PageGraph::GetExtensionNode() {
  if (extension_node_ == nullptr) {
    extension_node_ = new NodeExtension(this);
    AddNode(extension_node_);
  }
  return extension_node_;
}

void PageGraph::PushActiveScript(const ScriptId script_id) {
  active_script_stack_.push_back(script_id);
}

ScriptId PageGraph::PopActiveScript() {
  LOG_ASSERT(active_script_stack_.size() > 0);
  ScriptId top_script_id = active_script_stack_.back();
  active_script_stack_.pop_back();
  return top_script_id;
}

ScriptId PageGraph::PeekActiveScript() const {
  if (active_script_stack_.empty() == true) {
    return 0;
  }
  return active_script_stack_.back();
}

void PageGraph::Log(const string& str) const {
  PG_LOG(str);
}

}  // namespace brave_page_graph
