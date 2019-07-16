/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_PAGE_GRAPH_H_
#define BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_PAGE_GRAPH_H_

#include <map>
#include <memory>
#include <string>
#include "brave/third_party/blink/brave_page_graph/types.h"
#include "brave/third_party/blink/brave_page_graph/requests/request_tracker.h"
#include "brave/third_party/blink/brave_page_graph/scripts/script_tracker.h"

namespace blink {
class Document;
}

namespace brave_page_graph {

class Edge;
class EdgeRequestStart;
class EdgeNodeInsert;
class GraphItem;
class Node;
class NodeActor;
class NodeExtension;
class NodeFrame;
class NodeHTML;
class NodeHTMLElement;
class NodeHTMLText;
class NodeParser;
class NodeResource;
class NodeScript;
class NodeScriptRemote;
class NodeShields;
class NodeStorageCookieJar;
class NodeStorageLocalStorage;
class NodeWebAPI;
class RequestTracker;
class ScriptTracker;
struct TrackedRequestRecord;

class PageGraph {
// Needed so that graph items can assign themself the next graph id.
friend GraphItem;
// Needed so that edges between HTML nodes can find their siblings and parents.
friend EdgeNodeInsert;
// Needed so that HTML element nodes can find the script nodes for their event
// listeners during GraphML generation.
friend NodeHTMLElement;
 public:
  PageGraph(blink::Document& document);
  ~PageGraph();

  void RegisterDocumentRootCreated(const blink::DOMNodeId node_id,
    const blink::DOMNodeId parent_node_id);

  void RegisterHTMLElementNodeCreated(const blink::DOMNodeId node_id,
    const WTF::String& tag_name);
  void RegisterHTMLTextNodeCreated(const blink::DOMNodeId node_id,
    const WTF::String& text);
  void RegisterHTMLElementNodeInserted(const blink::DOMNodeId node_id,
    const blink::DOMNodeId parent_node_id,
    const blink::DOMNodeId before_sibling_id);
  void RegisterHTMLTextNodeInserted(const blink::DOMNodeId node_id,
    const blink::DOMNodeId parent_node_id,
    const blink::DOMNodeId before_sibling_id);
  void RegisterHTMLElementNodeRemoved(const blink::DOMNodeId node_id);
  void RegisterHTMLTextNodeRemoved(const blink::DOMNodeId node_id);

  void RegisterContentFrameSet(const blink::DOMNodeId node_id,
    const WTF::String& url);

  void RegisterInlineStyleSet(const blink::DOMNodeId node_id,
    const WTF::String& attr_name, const WTF::String& attr_value);
  void RegisterInlineStyleDelete(const blink::DOMNodeId node_id,
    const WTF::String& attr_name);
  void RegisterAttributeSet(const blink::DOMNodeId node_id,
    const WTF::String& attr_name, const WTF::String& attr_value);
  void RegisterAttributeDelete(const blink::DOMNodeId node_id,
    const WTF::String& attr_name);

  void RegisterTextNodeChange(const blink::DOMNodeId node_id,
    const WTF::String& new_text);

  void RegisterRequestStartFromElm(const blink::DOMNodeId node_id,
    const InspectorId request_id, const blink::KURL& url,
    const RequestType type);
  void RegisterRequestStartFromCurrentScript(const InspectorId request_id,
    const blink::KURL& url, const RequestType type);
  void RegisterRequestStartFromCSS(const InspectorId request_id,
      const blink::KURL& url, const RequestType type);
  void RegisterRequestComplete(const InspectorId request_id,
    const blink::ResourceType type);
  void RegisterRequestError(const InspectorId request_id);

  // Methods for handling the registration of script units in the document,
  // and v8 script executing.

  // Local scripts are scripts that define their code inline.
  void RegisterElmForLocalScript(const blink::DOMNodeId node_id,
    const blink::ScriptSourceCode& code);
  // Remote scripts are scripts that reference remote code (eg src=...).
  void RegisterElmForRemoteScript(const blink::DOMNodeId node_id,
    const blink::KURL& url);
  void RegisterUrlForScriptSource(const blink::KURL& url,
    const blink::ScriptSourceCode& code);
  void RegisterUrlForExtensionScriptSource(const blink::WebString& url,
    const blink::WebString& code);
  void RegisterScriptCompilation(const blink::ScriptSourceCode& code,
    const ScriptId script_id, const ScriptType type);
  void RegisterScriptCompilationFromAttr(const blink::DOMNodeId node_id,
    const WTF::String& attr_name, const WTF::String& attr_value,
    const ScriptId script_id);

  void RegisterScriptExecStart(const ScriptId script_id);
  // The Script ID is only used here as a sanity check to make sure we're
  // correctly tracking script execution.
  void RegisterScriptExecStop(const ScriptId script_id);

  void GenerateReportForNode(const blink::DOMNodeId node_id);

  GraphMLXML ToGraphML() const;

  void PushActiveScript(const ScriptId script_id);
  ScriptId PopActiveScript();
  ScriptId PeekActiveScript() const;

  void Log(const std::string& str) const;

 protected:
  void AddNode(Node* const node);
  void AddEdge(const Edge* const edge);

  const NodeUniquePtrList& Nodes() const;
  const EdgeUniquePtrList& Edges() const;
  const GraphItemList& GraphItems() const;

  NodeHTML* GetHTMLNode(const blink::DOMNodeId node_id) const;
  NodeHTMLElement* GetHTMLElementNode(const blink::DOMNodeId node_id) const;
  NodeHTMLText* GetHTMLTextNode(const blink::DOMNodeId node_id) const;
  NodeExtension* GetExtensionNode();
  NodeActor* GetCurrentActingNode() const;

  void DoRegisterRequestStart(const InspectorId request_id,
      Node* const requesting_node, const std::string& local_url,
      const RequestType type);
  void PossiblyWriteRequestsIntoGraph(
    const std::shared_ptr<const TrackedRequestRecord> record);

  // Monotonically increasing counter, used so that we can replay the
  // the graph's construction if needed.
  PageGraphId id_counter_ = 0;

  // These vectors own all of the items that are shared and indexed across
  // the rest of the graph.  All the other pointers (the weak pointers)
  // do not own their data.
  NodeUniquePtrList nodes_;
  EdgeUniquePtrList edges_;

  // Vectors for tracking other ways of referencing graph elements, non-owning.
  GraphItemList graph_items_;

  // Non-owning references to singleton items in the graph. (the owning
  // references will be in the above vectors).
  NodeParser* const parser_node_;
  NodeShields* const shields_node_;
  NodeStorageCookieJar* const cookie_jar_node_;
  NodeStorageLocalStorage* const local_storage_node_;
  NodeExtension* extension_node_ = nullptr;

  // Non-owning reference to the HTML root of the document (i.e. <html>).
  NodeHTMLElement* html_root_node_;

  // Index structure for storing and looking up webapi nodes.
  // This map does not own the references.
  std::map<MethodName, NodeWebAPI* const> webapi_nodes_;

  // Index structure for looking up HTML nodes.
  // This map does not own the references.
  std::map<blink::DOMNodeId, NodeHTMLElement* const> element_nodes_;
  std::map<blink::DOMNodeId, NodeHTMLText* const> text_nodes_;

  // Index structure for looking up script nodes.
  // This map does not own the references.
  std::map<ScriptId, NodeScript* const> script_nodes_;
  std::map<ScriptId, NodeScriptRemote* const> remote_script_nodes_;

  // Keeps track of which scripts are running, and conceptually mirrors the
  // JS stack.
  ScriptIdList active_script_stack_;

  // Data structure used for mapping HTML script elements (and other
  // sources of script in a document) to v8 script units.
  ScriptTracker script_tracker_;

  // Makes sure we don't have more than one node in the graph representing
  // a single URL (not required for correctness, but keeps things tidier
  // and makes some kinds of queries nicer).
  std::map<RequestUrl, NodeResource* const> resource_nodes_;

  // Data structure for keeping track of all the in-air requests that
  // have been made, but have not completed.
  RequestTracker request_tracker_;

  // Reference to the document that owns page-graph, which if it exists,
  // will always be the document of the top level frame.
  blink::Document& document_;
};

}  // namespace brave_page_graph

#endif  // BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_PAGE_GRAPH_H_
