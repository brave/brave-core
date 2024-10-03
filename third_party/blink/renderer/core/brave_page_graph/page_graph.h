/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_PAGE_GRAPH_H_
#define BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_PAGE_GRAPH_H_

#include <map>
#include <memory>
#include <optional>
#include <string>

#include "base/containers/span_or_size.h"
#include "base/time/time.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/blink_probe_types.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/page_graph_context.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/requests/request_tracker.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/scripts/script_tracker.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/types.h"
#include "third_party/abseil-cpp/absl/types/variant.h"
#include "third_party/blink/public/platform/web_url.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/dom/dom_node_ids.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/inspector/console_message.h"
#include "third_party/blink/renderer/core/inspector/protocol/dom.h"
#include "third_party/blink/renderer/core/inspector/protocol/protocol.h"
#include "third_party/blink/renderer/platform/allow_discouraged_type.h"
#include "third_party/blink/renderer/platform/heap/garbage_collected.h"
#include "third_party/blink/renderer/platform/heap/member.h"
#include "third_party/blink/renderer/platform/supplementable.h"
#include "third_party/blink/renderer/platform/weborigin/kurl_hash.h"
#include "third_party/blink/renderer/platform/wtf/hash_map.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace base {
class UnguessableToken;
}  // namespace base

namespace brave_page_graph {

class GraphEdge;
class GraphNode;
class NodeActor;
class NodeAdFilter;
class NodeBinding;
class NodeExtensions;
class NodeFingerprintingFilter;
class NodeHTML;
class NodeHTMLElement;
class NodeHTMLText;
class NodeParser;
class NodeScriptRemote;
class NodeResource;
class NodeShields;
class NodeShield;
class NodeStorageCookieJar;
class NodeStorageLocalStorage;
class NodeStorageRoot;
class NodeStorageSessionStorage;
class NodeTrackerFilter;
class NodeUnknown;
class NodeJSBuiltin;
class NodeJSWebAPI;
class RequestTracker;
class ScriptTracker;
struct TrackedRequestRecord;

}  // namespace brave_page_graph

namespace blink {

class ExecutionContext;
class LocalFrame;
class Node;
class CharacterData;
class Element;
class Document;
class DocumentLoader;
class ExceptionState;
class ConsoleMessage;
class KURL;
class ResourceError;
struct ResourceLoaderOptions;
class ResourceResponse;
class CoreProbeSink;
class ResourceRequest;
class Resource;
class BlobDataHandle;
class ClassicScript;
class EventTarget;
class EncodedFormData;
class ModuleScriptCreationParams;
class ReferrerScriptInfo;

enum class RenderBlockingBehavior : uint8_t;
enum class ResourceType : uint8_t;

// ID generation rules:
//   * blink::DOMNodeId is a global counter.
//   * Request Id (uint64_t identifier) is a global counter.
//   * Script Id (int script_id) is a v8::Isolate-bound counter.
//
// Concepts relation:
//   * LocalFrame : LocalDOMWindow (ExecutionContext) : Document = 1 : 1 : 1 at
//     any point in time, but the mapping may change over time.
//   * LocalFrame : DocumentLoader = 1 : 1
//
// See for details:
// https://docs.google.com/document/d/1aitSOucL0VHZa9Z2vbRJSyAIsAz24kX8LFByQ5xQnUg/edit
// https://docs.google.com/presentation/d/1pHjF3TNCX--j0ss3SK09pXlVOFK0Cdq6HkMcOzcov1o/edit#slide=id.g4983c55b2d55fcc7_42

class CORE_EXPORT PageGraph : public GarbageCollected<PageGraph>,
                              public Supplement<LocalFrame>,
                              public brave_page_graph::PageGraphContext {
 public:
  static const char kSupplementName[];
  static PageGraph* From(LocalFrame&);
  static void ProvideTo(LocalFrame&);

  explicit PageGraph(LocalFrame& local_frame);
  ~PageGraph() override;

  void Trace(Visitor* visitor) const override;

  // *** CoreProbe handlers ***
  // Node tracking:
  void NodeCreated(blink::Node* node);
  void RegisterPageGraphNodeFullyCreated(blink::Node* node);
  void DidInsertDOMNode(blink::Node* node);
  void WillRemoveDOMNode(blink::Node* node);
  // Node attributes tracking:
  void DidModifyDOMAttr(blink::Element* element,
                        const blink::QualifiedName& name,
                        const AtomicString& value);
  void DidRemoveDOMAttr(blink::Element* element,
                        const blink::QualifiedName& name);
  // Document/navigation tracking:
  void DidCommitLoad(blink::LocalFrame*, blink::DocumentLoader*);
  void WillSendNavigationRequest(uint64_t identifier,
                                 blink::DocumentLoader* loader,
                                 const blink::KURL&,
                                 const AtomicString& http_method,
                                 blink::EncodedFormData*);
  // Requests tracking:
  void WillSendRequest(blink::ExecutionContext* execution_context,
                       blink::DocumentLoader* loader,
                       const blink::KURL& fetch_context_url,
                       const blink::ResourceRequest& request,
                       const blink::ResourceResponse& redirect_response,
                       const blink::ResourceLoaderOptions& options,
                       blink::ResourceType resource_type,
                       blink::RenderBlockingBehavior render_blocking_behavior,
                       base::TimeTicks timestamp);
  void DidReceiveResourceResponse(uint64_t identifier,
                                  blink::DocumentLoader* loader,
                                  const blink::ResourceResponse& response,
                                  const blink::Resource* cached_resource);
  void DidReceiveData(uint64_t identifier,
                      blink::DocumentLoader* loader,
                      base::SpanOrSize<const char> data);
  void DidReceiveBlob(uint64_t identifier,
                      blink::DocumentLoader* loader,
                      blink::BlobDataHandle*);
  void DidFinishLoading(uint64_t identifier,
                        blink::DocumentLoader* loader,
                        base::TimeTicks finish_time,
                        int64_t encoded_data_length,
                        int64_t decoded_body_length);
  void DidFailLoading(
      blink::CoreProbeSink* sink,
      uint64_t identifier,
      blink::DocumentLoader* loader,
      const blink::ResourceError&,
      const base::UnguessableToken& devtools_frame_or_worker_token);
  // Script/module compilation tracking:
  void ApplyCompilationModeOverride(const blink::ClassicScript&,
                                    v8::ScriptCompiler::CachedData**,
                                    v8::ScriptCompiler::CompileOptions*);
  void RegisterPageGraphScriptCompilation(
      blink::ExecutionContext* execution_context,
      const blink::ReferrerScriptInfo& referrer_info,
      const blink::ClassicScript& classic_script,
      v8::Local<v8::Script> script);
  void RegisterPageGraphModuleCompilation(
      blink::ExecutionContext* execution_context,
      const blink::ReferrerScriptInfo& referrer_info,
      const blink::ModuleScriptCreationParams& params,
      v8::Local<v8::Module> script);
  void RegisterPageGraphScriptCompilationFromAttr(
      blink::EventTarget* event_target,
      const String& function_name,
      const String& script_body,
      v8::Local<v8::Function> compiled_function);
  // WebAPI calls tracking:
  void RegisterPageGraphBindingEvent(blink::ExecutionContext* execution_context,
                                     const char* name,
                                     blink::PageGraphBindingType type,
                                     blink::PageGraphBindingEvent event);
  void RegisterPageGraphWebAPICallWithResult(
      blink::ExecutionContext* execution_context,
      const char* name,
      const blink::PageGraphObject& receiver_data,
      const blink::PageGraphValues& args,
      const blink::ExceptionState* exception_state,
      const std::optional<blink::PageGraphValue>& result);
  // Event listeners tracking:
  void RegisterPageGraphEventListenerAdd(
      blink::EventTarget* event_target,
      const String& event_type,
      blink::RegisteredEventListener* registered_listener);
  void RegisterPageGraphEventListenerRemove(
      blink::EventTarget* event_target,
      const String& event_type,
      blink::RegisteredEventListener* registered_listener);
  void RegisterPageGraphJavaScriptUrl(blink::Document* document,
                                      const KURL& url);

  // Console message tracking:
  void ConsoleMessageAdded(blink::ConsoleMessage* console_message);
  // *** CoreProbe handlers end ***

  // *** v8 handlers ***
  void RegisterV8ScriptCompilationFromEval(v8::Isolate* isolate,
                                           const int script_id,
                                           v8::Local<v8::String> source);
  void RegisterV8JSBuiltinCall(
      blink::ExecutionContext* receiver_context,
      const char* builtin_name,
      const blink::PageGraphValues& args,
      const std::optional<blink::PageGraphValue>& result);
  // *** v8 handlers end ***

  // PageGraphContext:
  base::TimeTicks GetGraphStartTime() const override;
  brave_page_graph::GraphItemId GetNextGraphItemId() override;
  void AddGraphItem(
      std::unique_ptr<brave_page_graph::GraphItem> graph_item) override;

  void GenerateReportForNode(const blink::DOMNodeId node_id,
                             blink::protocol::Array<String>& report);
  String ToGraphML() const;

 private:
#define PAGE_GRAPH_USING_DECL(type) using type = brave_page_graph::type
  PAGE_GRAPH_USING_DECL(Binding);
  PAGE_GRAPH_USING_DECL(BindingEvent);
  PAGE_GRAPH_USING_DECL(BindingType);
  PAGE_GRAPH_USING_DECL(EdgeList);
  PAGE_GRAPH_USING_DECL(EventListenerId);
  PAGE_GRAPH_USING_DECL(FingerprintingRule);
  PAGE_GRAPH_USING_DECL(GraphEdge);
  PAGE_GRAPH_USING_DECL(GraphItemId);
  PAGE_GRAPH_USING_DECL(GraphItemUniquePtrList);
  PAGE_GRAPH_USING_DECL(GraphNode);
  PAGE_GRAPH_USING_DECL(InspectorId);
  PAGE_GRAPH_USING_DECL(MethodName);
  PAGE_GRAPH_USING_DECL(NodeActor);
  PAGE_GRAPH_USING_DECL(NodeAdFilter);
  PAGE_GRAPH_USING_DECL(NodeBinding);
  PAGE_GRAPH_USING_DECL(NodeExtensions);
  PAGE_GRAPH_USING_DECL(NodeFingerprintingFilter);
  PAGE_GRAPH_USING_DECL(NodeHTML);
  PAGE_GRAPH_USING_DECL(NodeHTMLElement);
  PAGE_GRAPH_USING_DECL(NodeHTMLText);
  PAGE_GRAPH_USING_DECL(NodeJSBuiltin);
  PAGE_GRAPH_USING_DECL(NodeJSWebAPI);
  PAGE_GRAPH_USING_DECL(NodeList);
  PAGE_GRAPH_USING_DECL(NodeParser);
  PAGE_GRAPH_USING_DECL(NodeResource);
  PAGE_GRAPH_USING_DECL(NodeScriptRemote);
  PAGE_GRAPH_USING_DECL(NodeShield);
  PAGE_GRAPH_USING_DECL(NodeShields);
  PAGE_GRAPH_USING_DECL(NodeStorageCookieJar);
  PAGE_GRAPH_USING_DECL(NodeStorageLocalStorage);
  PAGE_GRAPH_USING_DECL(NodeStorageRoot);
  PAGE_GRAPH_USING_DECL(NodeStorageSessionStorage);
  PAGE_GRAPH_USING_DECL(NodeTrackerFilter);
  PAGE_GRAPH_USING_DECL(NodeUnknown);
  PAGE_GRAPH_USING_DECL(RequestTracker);
  PAGE_GRAPH_USING_DECL(RequestURL);
  PAGE_GRAPH_USING_DECL(ScriptData);
  PAGE_GRAPH_USING_DECL(ScriptId);
  PAGE_GRAPH_USING_DECL(ScriptPosition);
  PAGE_GRAPH_USING_DECL(ScriptTracker);
  PAGE_GRAPH_USING_DECL(StorageLocation);
  PAGE_GRAPH_USING_DECL(TrackedRequestRecord);
#undef PAGE_GRAPH_USING_DECL

  struct ExecutionContextNodes {
    NodeParser* parser_node;
    NodeExtensions* extensions_node;
  };

  struct ProcessedJavascriptURL {
    String script_code;
    ScriptId parent_script_id = 0;
  };

  NodeHTML* GetHTMLNode(const blink::DOMNodeId node_id) const;
  NodeHTMLElement* GetHTMLElementNode(
      absl::variant<blink::DOMNodeId, blink::Node*> node_var);
  NodeHTMLText* GetHTMLTextNode(const blink::DOMNodeId node_id) const;
  bool RegisterCurrentlyConstructedNode(blink::Node* node);

  void RegisterDocumentNodeCreated(blink::Document* node);
  void RegisterHTMLTextNodeCreated(blink::CharacterData* node);
  void RegisterHTMLElementNodeCreated(blink::Node* node);
  void RegisterHTMLTextNodeInserted(blink::CharacterData* node,
                                    blink::Node* parent_node,
                                    const blink::DOMNodeId before_sibling_id);
  void RegisterHTMLElementNodeInserted(
      blink::Node* node,
      blink::Node* parent_node,
      const blink::DOMNodeId before_sibling_id);
  void RegisterHTMLTextNodeRemoved(blink::CharacterData* node);
  void RegisterHTMLElementNodeRemoved(blink::Node* node);

  void RegisterEventListenerAdd(blink::Node* node,
                                const String& event_type,
                                const EventListenerId listener_id,
                                ScriptId listener_script_id);
  void RegisterEventListenerRemove(blink::Node* node,
                                   const String& event_type,
                                   const EventListenerId listener_id,
                                   ScriptId listener_script_id);

  void RegisterInlineStyleSet(blink::Element* element,
                              const String& attr_name,
                              const String& attr_value);
  void RegisterInlineStyleDelete(blink::Element* element,
                                 const String& attr_name);
  void RegisterAttributeSet(blink::Element* element,
                            const String& attr_name,
                            const String& attr_value);
  void RegisterAttributeDelete(blink::Element* element,
                               const String& attr_name);

  void RegisterTextNodeChange(blink::CharacterData* node,
                              const String& new_text);

  void DoRegisterRequestStart(const InspectorId request_id,
                              GraphNode* requesting_node,
                              const brave_page_graph::FrameId& frame_id,
                              const KURL& local_url,
                              const String& resource_type);
  void RegisterRequestStartFromElm(const blink::DOMNodeId node_id,
                                   const InspectorId request_id,
                                   const brave_page_graph::FrameId& frame_id,
                                   const blink::KURL& url,
                                   const String& resource_type);
  void RegisterRequestStartFromCurrentScript(
      blink::ExecutionContext* execution_context,
      const InspectorId request_id,
      const blink::KURL& url,
      const String& resource_type);
  void RegisterRequestStartFromScript(
      blink::ExecutionContext* execution_context,
      const ScriptId script_id,
      const InspectorId request_id,
      const blink::KURL& url,
      const String& resource_type);
  void RegisterRequestStartFromCSSOrLink(blink::DocumentLoader* loader,
                                         const InspectorId request_id,
                                         const blink::KURL& url,
                                         const String& resource_type);
  void RegisterRequestStartForDocument(blink::DocumentLoader* loader,
                                       const InspectorId request_id,
                                       const blink::KURL& url);
  void RegisterRequestRedirect(const ResourceRequest& request,
                               const ResourceResponse& redirect_response,
                               const brave_page_graph::FrameId& frame_id);
  void RegisterRequestComplete(const InspectorId request_id,
                               int64_t encoded_data_length,
                               const brave_page_graph::FrameId& frame_id);
  void RegisterRequestCompleteForDocument(
      const InspectorId request_id,
      const int64_t size,
      const brave_page_graph::FrameId& frame_id);
  void RegisterRequestError(const InspectorId request_id,
                            const brave_page_graph::FrameId& frame_id);

  void RegisterResourceBlockAd(const blink::WebURL& url, const String& rule);
  void RegisterResourceBlockTracker(const blink::WebURL& url,
                                    const String& host);
  void RegisterResourceBlockJavaScript(const blink::WebURL& url);
  void RegisterResourceBlockFingerprinting(const blink::WebURL& url,
                                           const FingerprintingRule& rule);

  void RegisterScriptCompilation(blink::ExecutionContext* execution_context,
                                 const ScriptId script_id,
                                 const ScriptData& script_data);
  void RegisterScriptCompilationFromAttr(
      blink::ExecutionContext* execution_context,
      const ScriptId script_id,
      const ScriptData& script_data);

  void RegisterStorageRead(blink::ExecutionContext* execution_context,
                           const String& key,
                           const blink::PageGraphValue& value,
                           const StorageLocation location);
  void RegisterStorageWrite(blink::ExecutionContext* execution_context,
                            const String& key,
                            const blink::PageGraphValue& value,
                            const StorageLocation location);
  void RegisterStorageDelete(blink::ExecutionContext* execution_context,
                             const String& key,
                             const StorageLocation location);
  void RegisterStorageClear(blink::ExecutionContext* execution_context,
                            const StorageLocation location);

  void RegisterWebAPICall(blink::ExecutionContext* execution_context,
                          const MethodName& method,
                          const blink::PageGraphValues& arguments);
  void RegisterWebAPIResult(blink::ExecutionContext* execution_context,
                            const MethodName& method,
                            const blink::PageGraphValue& result);

  void RegisterJSBuiltInCall(blink::ExecutionContext* receiver_context,
                             const char* builtin_name,
                             const blink::PageGraphValues& args);
  void RegisterJSBuiltInResponse(blink::ExecutionContext* receiver_context,
                                 const char* builtin_name,
                                 const blink::PageGraphValue& result);

  void RegisterBindingEvent(blink::ExecutionContext* execution_context,
                            const Binding binding,
                            const BindingType binding_type,
                            const BindingEvent binding_event);

  NodeActor* GetCurrentActingNode(
      blink::ExecutionContext* execution_context,
      ScriptPosition* out_script_position = nullptr);
  ScriptId GetExecutingScriptId(
      blink::ExecutionContext* execution_context,
      ScriptPosition* out_script_position = nullptr) const;

  NodeUnknown* GetUnknownActorNode();

  NodeResource* GetResourceNodeForUrl(const KURL& url);
  NodeAdFilter* GetAdFilterNodeForRule(const String& rule);
  NodeTrackerFilter* GetTrackerFilterNodeForHost(const String& host);
  NodeFingerprintingFilter* GetFingerprintingFilterNodeForRule(
      const FingerprintingRule& rule);
  NodeBinding* GetBindingNode(const Binding binding,
                              const BindingType binding_type);
  NodeJSWebAPI* GetJSWebAPINode(const MethodName& method);
  NodeJSBuiltin* GetJSBuiltinNode(const MethodName& method);

  // Return true if this PageGraph instance is instrumenting the top level
  // frame tree.
  bool IsRootFrame() const;

  // The blink assigned frame id for the local root's frame.
  const brave_page_graph::FrameId frame_id_;
  // Script tracker helper.
  ScriptTracker script_tracker_;
  // Data structure for keeping track of all the in-air requests that
  // have been made, but have not completed.
  RequestTracker request_tracker_;
  // Page Graph start time stamp.
  base::TimeTicks start_;
  // Monotonically increasing counter, used so that we can replay the
  // the graph's construction if needed.
  GraphItemId id_counter_ = 0;

  // These vectors own all of the items that are shared and indexed across
  // the rest of the graph.  All the other pointers (the weak pointers)
  // do not own their data.
  GraphItemUniquePtrList graph_items_;
  EdgeList edges_;
  NodeList nodes_;

  // Non-owning references to singleton items in the graph. (the owning
  // references will be in the above vectors).
  blink::HeapHashMap<blink::Member<ExecutionContext>, ExecutionContextNodes>
      execution_context_nodes_;

  // Tracks nodes currently being constructed. This is used to properly handle
  // events that may occur during node construction such as attribute change,
  // child node insert, etc.
  // bool=true is set when a node has been speculatively registered.
  base::flat_map<blink::UntracedMember<blink::Node>, bool>
      currently_constructed_nodes_;

  // Holds JavascriptURL parent script id to use during the script compilation
  // event.
  blink::HeapHashMap<blink::Member<ExecutionContext>,
                     Vector<ProcessedJavascriptURL>>
      processed_js_urls_;

  // Index structure for looking up HTML nodes.
  // This map does not own the references.
  HashMap<blink::DOMNodeId, NodeHTMLElement*> element_nodes_;
  HashMap<blink::DOMNodeId, NodeHTMLText*> text_nodes_;

  // Makes sure we don't have more than one node in the graph representing
  // a single URL (not required for correctness, but keeps things tidier
  // and makes some kinds of queries nicer).
  HashMap<RequestURL, NodeResource*> resource_nodes_;

  // Similarly, make sure we only have one node for each script executed
  // somewhere outside the document.
  HashMap<ScriptId, NodeScriptRemote*> remote_scripts_;

  // Index structure for looking up binding nodes.
  // This map does not own the references.
  HashMap<Binding, NodeBinding*> binding_nodes_;
  // Index structure for storing and looking up webapi nodes.
  // This map does not own the references.
  HashMap<MethodName, NodeJSWebAPI*> js_webapi_nodes_;
  // Index structure for storing and looking up nodes representing built
  // in JS funcs and methods. This map does not own the references.
  HashMap<MethodName, NodeJSBuiltin*> js_builtin_nodes_;

  using FingerprintingFilterNodes ALLOW_DISCOURAGED_TYPE(
      "TODO(https://github.com/brave/brave-browser/issues/28238)") =
      std::map<FingerprintingRule, NodeFingerprintingFilter*>;

  // Index structure for looking up filter nodes.
  // These maps do not own the references.
  HashMap<String, NodeAdFilter*> ad_filter_nodes_;
  HashMap<String, NodeTrackerFilter*> tracker_filter_nodes_;
  FingerprintingFilterNodes fingerprinting_filter_nodes_;

  NodeShields* shields_node_;
  NodeShield* ad_shield_node_;
  NodeShield* tracker_shield_node_;
  NodeShield* js_shield_node_;
  NodeShield* fingerprinting_shield_node_;

  String source_url_;

  NodeStorageRoot* storage_node_;
  NodeStorageCookieJar* cookie_jar_node_;
  NodeStorageLocalStorage* local_storage_node_;
  NodeStorageSessionStorage* session_storage_node_;

  NodeUnknown* unknown_actor_node_;
};

}  // namespace blink

#endif  // BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_PAGE_GRAPH_H_
