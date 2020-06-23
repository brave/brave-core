/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_PAGE_GRAPH_H_
#define BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_PAGE_GRAPH_H_

#include <chrono>
#include <map>
#include <memory>
#include <string>

#include "third_party/blink/renderer/core/inspector/protocol/Protocol.h"

#include "brave/third_party/blink/brave_page_graph/types.h"
#include "brave/third_party/blink/brave_page_graph/requests/request_tracker.h"
#include "brave/third_party/blink/brave_page_graph/scripts/script_tracker.h"

namespace blink {

class ExecutionContext;

}

namespace v8 {

class Context;
class Isolate;
template <class T>
class Local;

}

namespace brave_page_graph {

class Edge;
class EdgeEventListenerAction;
class EdgeRequestStart;
class EdgeNodeInsert;
class GraphItem;
class Node;
class NodeActor;
class NodeAdFilter;
class NodeExtensions;
class NodeFingerprintingFilter;
class NodeFrame;
class NodeHTML;
class NodeHTMLElement;
class NodeHTMLText;
class NodeParser;
class NodeResource;
class NodeScript;
class NodeShields;
class NodeShield;
class NodeStorage;
class NodeStorageCookieJar;
class NodeStorageLocalStorage;
class NodeStorageRoot;
class NodeStorageSessionStorage;
class NodeTrackerFilter;
class NodeJSBuiltIn;
class NodeJSWebAPI;
class ResponseMetadata;
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
  static PageGraph* GetFromIsolate(v8::Isolate& isolate);
  static PageGraph* GetFromContext(v8::Local<v8::Context> context);
  static PageGraph* GetFromExecutionContext(blink::ExecutionContext& exec_context);

  PageGraph(blink::ExecutionContext& execution_context,
            const blink::DOMNodeId node_id, const WTF::String& tag_name,
            const blink::KURL& url);
  ~PageGraph();

  void RegisterDocumentRootCreated(const blink::DOMNodeId node_id,
    const blink::DOMNodeId parent_node_id, const WTF::String& tag_name,
    const blink::KURL& url);
  void RegisterRemoteFrameCreated(const blink::DOMNodeId parent_node_id,
    const GURL& url);

  void RegisterHTMLElementNodeCreated(const blink::DOMNodeId node_id,
    const WTF::String& tag_name,
    const ElementType element_type = kElementTypeDefault);
  // Used if it's possible the element has already been added previously
  void TryRegisterHTMLElementNodeCreated(const blink::DOMNodeId node_id,
    const WTF::String& tag_name,
    const ElementType element_type = kElementTypeDefault);
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

  void RegisterEventListenerAdd(const blink::DOMNodeId,
    const WTF::String& event_type, const EventListenerId listener_id,
    ScriptId listener_script_id);
  void RegisterEventListenerRemove(const blink::DOMNodeId,
    const WTF::String& event_type, const EventListenerId listener_id,
    ScriptId listener_script_id);

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
    const blink::ResourceType type, const ResponseMetadata& metadata,
    const std::string& resource_hash);
  void RegisterRequestError(const InspectorId request_id,
    const ResponseMetadata& metadata);

  void RegisterResourceBlockAd(const GURL& url, const std::string& rule);
  void RegisterResourceBlockTracker(const GURL& url, const std::string& host);
  void RegisterResourceBlockJavaScript(const GURL& url);
  void RegisterResourceBlockFingerprinting(const GURL& url,
    const FingerprintingRule& rule);

  void RegisterStorageRead(const WTF::String& key, const WTF::String& value,
    const StorageLocation location);
  void RegisterStorageWrite(const WTF::String& key, const WTF::String& value,
    const StorageLocation location);
  void RegisterStorageDelete(const WTF::String& key,
    const StorageLocation location);
  void RegisterStorageClear(const StorageLocation location);

  void RegisterWebAPICall(const WebAPI web_api,
    const std::vector<const WTF::String>& arguments);
  void RegisterWebAPICall(const MethodName& method,
    const std::vector<const WTF::String>& arguments);
  void RegisterWebAPIResult(const WebAPI web_api,
    const WTF::String& result);
  void RegisterWebAPIResult(const MethodName& method,
    const WTF::String& result);
  void RegisterJSBuiltInCall(const JSBuiltIn built_in,
    const std::vector<const std::string>& args);
  void RegisterJSBuiltInResponse(const JSBuiltIn built_in,
    const std::string& result);

  // Methods for handling the registration of script units in the document,
  // and v8 script executing.

  // Local scripts are scripts that define their code inline.
  void RegisterElmForLocalScript(const blink::DOMNodeId node_id,
    const blink::ScriptSourceCode& code);
  // Remote scripts are scripts that reference remote code (eg src=...).
  void RegisterElmForRemoteScript(const blink::DOMNodeId node_id,
    const blink::KURL& url);
  // JavaScript URLs ("javascript:" schemes).
  void RegisterJavaScriptURL(const blink::ScriptSourceCode& code);
  void RegisterUrlForScriptSource(const blink::KURL& url,
    const blink::ScriptSourceCode& code);
  void RegisterUrlForExtensionScriptSource(const blink::WebString& url,
    const blink::WebString& code);
  void RegisterScriptCompilation(const blink::ScriptSourceCode& code,
    const ScriptId script_id, const ScriptType type);
  void RegisterScriptCompilationFromAttr(const blink::DOMNodeId node_id,
    const WTF::String& attr_name, const WTF::String& attr_value,
    const ScriptId script_id);
  void RegisterScriptCompilationFromEval(const ScriptId parent_script_id,
      const ScriptId script_id);

  void RegisterModuleScriptForDescendant(const blink::KURL& parent_location,
    const blink::KURL& descendant_location);
  void RegisterModuleScriptForDescendant(const ScriptId parent_id,
    const blink::KURL& descendant_location);

  void GenerateReportForNode(const blink::DOMNodeId node_id,
                             blink::protocol::Array<WTF::String>& report);

  std::string ToGraphML() const;

  const std::chrono::time_point<std::chrono::high_resolution_clock>&
    GetTimestamp() const;

  void Log(const std::string& str) const;

 protected:
  void AddNode(Node* const node);
  void AddEdge(const Edge* const edge);

  void AddShieldNode(NodeShield* const shield_node);
  void AddStorageNode(NodeStorage* const storage_node);

  const NodeUniquePtrList& Nodes() const;
  const EdgeUniquePtrList& Edges() const;
  const GraphItemList& GraphItems() const;

  NodeHTML* GetHTMLNode(const blink::DOMNodeId node_id) const;
  NodeHTMLElement* GetHTMLElementNode(const blink::DOMNodeId node_id) const;
  NodeHTMLText* GetHTMLTextNode(const blink::DOMNodeId node_id) const;
  NodeScript* GetScriptNode(const ScriptId script_id) const;

  NodeActor* GetCurrentActingNode() const;
  NodeActor* GetNodeActorForScriptId(const ScriptId script_id) const;
  ScriptId GetExecutingScriptId() const;

  NodeResource* GetResourceNodeForUrl(const std::string& url);

  NodeAdFilter* GetAdFilterNodeForRule(const std::string& rule);
  NodeTrackerFilter* GetTrackerFilterNodeForHost(const std::string& host);
  NodeFingerprintingFilter* GetFingerprintingFilterNodeForRule(
    const FingerprintingRule& rule);

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
  NodeExtensions* const extensions_node_;

  NodeShields* const shields_node_;
  NodeShield* const ad_shield_node_;
  NodeShield* const tracker_shield_node_;
  NodeShield* const js_shield_node_;
  NodeShield* const fingerprinting_shield_node_;

  NodeStorageRoot* const storage_node_;
  NodeStorageCookieJar* const cookie_jar_node_;
  NodeStorageLocalStorage* const local_storage_node_;
  NodeStorageSessionStorage* const session_storage_node_;

  // Non-owning reference to the HTML root of the document (i.e. <html>).
  NodeHTMLElement* html_root_node_;

  // Index structure for storing and looking up webapi nodes.
  // This map does not own the references.
  std::map<MethodName, NodeJSWebAPI* const> webapi_nodes_;

  // Index structure for storing and looking up nodes representing built
  // in JS funcs and methods. This map does not own the references.
  std::map<JSBuiltIn, NodeJSBuiltIn* const> builtin_js_nodes_;

  // Index structure for looking up HTML nodes.
  // This map does not own the references.
  std::map<blink::DOMNodeId, NodeHTMLElement* const> element_nodes_;
  std::map<blink::DOMNodeId, NodeHTMLText* const> text_nodes_;

  // Index structure for looking up script nodes.
  // This map does not own the references.
  std::map<ScriptId, NodeScript* const> script_nodes_;

  // Index structure for looking up filter nodes.
  // This map does not own the references.
  std::map<std::string, NodeAdFilter* const> ad_filter_nodes_;
  std::map<std::string, NodeTrackerFilter* const> tracker_filter_nodes_;
  std::map<FingerprintingRule, NodeFingerprintingFilter* const>
    fingerprinting_filter_nodes_;

  // Data structure used for mapping HTML script elements (and other
  // sources of script in a document) to v8 script units.
  ScriptTracker script_tracker_;

  // Makes sure we don't have more than one node in the graph representing
  // a single URL (not required for correctness, but keeps things tidier
  // and makes some kinds of queries nicer).
  std::map<RequestURL, NodeResource* const> resource_nodes_;

  // Data structure for keeping track of all the in-air requests that
  // have been made, but have not completed.
  RequestTracker request_tracker_;

  // Reference to the execution context associated with the page graph, which if
  // it exists, will always be for the top level frame.
  blink::ExecutionContext& execution_context_;

  std::chrono::time_point<std::chrono::high_resolution_clock> start_;
};

}  // namespace brave_page_graph

#endif  // BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_PAGE_GRAPH_H_
