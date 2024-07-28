// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_PAGE_GRAPH_PROXY_H_
#define BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_PAGE_GRAPH_PROXY_H_

#include <cstdint>
#include <optional>

#include "base/time/time.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/types.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/platform/heap/garbage_collected.h"
#include "third_party/blink/renderer/platform/heap/member.h"
#include "third_party/blink/renderer/platform/heap/weak_cell.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace base {
class UnguessableToken;
}  // namespace base

namespace blink {

class BlobDataHandle;
class ClassicScript;
class ConsoleMessage;
class CoreProbeSink;
class Document;
class DocumentLoader;
class Element;
class EncodedFormData;
class EventTarget;
class ExecutionContext;
class ExceptionState;
class KURL;
class LocalFrame;
class ModuleScriptCreationParams;
class Node;
class QualifiedName;
class ReferrerScriptInfo;
class Resource;
class ResourceError;
class ResourceRequest;
class ResourceResponse;
struct ResourceLoaderOptions;
class RegisteredEventListener;

class PageGraph;

enum class RenderBlockingBehavior : uint8_t;
enum class ResourceType : uint8_t;

class CORE_EXPORT PageGraphProxy : public GarbageCollected<PageGraphProxy>,
                                   public Supplement<LocalFrame> {
 public:
  static const char kSupplementName[];
  static PageGraphProxy* From(LocalFrame&);
  static void ProvideTo(LocalFrame&);

  explicit PageGraphProxy(LocalFrame& local_frame);

  void Trace(Visitor* visitor) const override;

  void NodeCreated(Node* node);
  void RegisterPageGraphNodeFullyCreated(Node* node);
  void DidInsertDOMNode(Node* node);
  void WillRemoveDOMNode(Node* node);
  void DidModifyDOMAttr(Element* element,
                        const QualifiedName& name,
                        const AtomicString& value);
  void DidRemoveDOMAttr(Element* element, const QualifiedName& name);

  void DidCommitLoad(LocalFrame*, DocumentLoader*);
  void WillSendNavigationRequest(uint64_t identifier,
                                 DocumentLoader* loader,
                                 const KURL&,
                                 const AtomicString& http_method,
                                 EncodedFormData*);

  void WillSendRequest(ExecutionContext* execution_context,
                       DocumentLoader* loader,
                       const KURL& fetch_context_url,
                       const ResourceRequest& request,
                       const ResourceResponse& redirect_response,
                       const ResourceLoaderOptions& options,
                       ResourceType resource_type,
                       RenderBlockingBehavior render_blocking_behavior,
                       base::TimeTicks timestamp);
  void DidReceiveResourceResponse(uint64_t identifier,
                                  DocumentLoader* loader,
                                  const ResourceResponse& response,
                                  const Resource* cached_resource);
  void DidReceiveData(uint64_t identifier,
                      DocumentLoader* loader,
                      const char* data,
                      uint64_t data_length);
  void DidReceiveBlob(uint64_t identifier,
                      DocumentLoader* loader,
                      BlobDataHandle*);
  void DidFinishLoading(uint64_t identifier,
                        DocumentLoader* loader,
                        base::TimeTicks finish_time,
                        int64_t encoded_data_length,
                        int64_t decoded_body_length);
  void LifecycleEvent(LocalFrame* local_frame,
                      DocumentLoader* loader,
                      const char* name,
                      double timestamp);

  void DidFailLoading(
      CoreProbeSink* sink,
      uint64_t identifier,
      DocumentLoader* loader,
      const ResourceError&,
      const base::UnguessableToken& devtools_frame_or_worker_token);

  void ApplyCompilationModeOverride(const ClassicScript&,
                                    v8::ScriptCompiler::CachedData**,
                                    v8::ScriptCompiler::CompileOptions*);
  void RegisterPageGraphScriptCompilation(
      ExecutionContext* execution_context,
      const ReferrerScriptInfo& referrer_info,
      const ClassicScript& classic_script,
      v8::Local<v8::Script> script);
  void RegisterPageGraphModuleCompilation(
      ExecutionContext* execution_context,
      const ReferrerScriptInfo& referrer_info,
      const ModuleScriptCreationParams& params,
      v8::Local<v8::Module> script);
  void RegisterPageGraphScriptCompilationFromAttr(
      EventTarget* event_target,
      const String& function_name,
      const String& script_body,
      v8::Local<v8::Function> compiled_function);

  void RegisterPageGraphBindingEvent(ExecutionContext* execution_context,
                                     const char* name,
                                     blink::PageGraphBindingType type,
                                     blink::PageGraphBindingEvent event);
  void RegisterPageGraphWebAPICallWithResult(
      ExecutionContext* execution_context,
      const char* name,
      const blink::PageGraphObject& receiver_data,
      const blink::PageGraphValues& args,
      const ExceptionState* exception_state,
      const std::optional<blink::PageGraphValue>& result);
  // Event listeners tracking:
  void RegisterPageGraphEventListenerAdd(
      EventTarget* event_target,
      const String& event_type,
      RegisteredEventListener* registered_listener);
  void RegisterPageGraphEventListenerRemove(
      EventTarget* event_target,
      const String& event_type,
      RegisteredEventListener* registered_listener);
  void RegisterPageGraphJavaScriptUrl(Document* document, const KURL& url);

  // Console message tracking:
  void ConsoleMessageAdded(ConsoleMessage* console_message);

 private:
  PageGraph* GetPageGraph();
  Member<PageGraph> parent_page_graph_;
};

}  // namespace blink

#endif  // BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_PAGE_GRAPH_PROXY_H_
