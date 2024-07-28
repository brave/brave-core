// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/third_party/blink/renderer/core/brave_page_graph/page_graph_proxy.h"

#include "base/logging.h"
#include "base/time/time.h"
#include "brave/components/brave_page_graph/common/features.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/blink_probe_types.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/page_graph.h"
#include "third_party/blink/renderer/bindings/core/v8/referrer_script_info.h"
#include "third_party/blink/renderer/core/core_probe_sink.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/element.h"
#include "third_party/blink/renderer/core/dom/events/event_target.h"
#include "third_party/blink/renderer/core/dom/events/registered_event_listener.h"
#include "third_party/blink/renderer/core/dom/node.h"
#include "third_party/blink/renderer/core/dom/qualified_name.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/frame/frame.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/html/html_frame_owner_element.h"
#include "third_party/blink/renderer/core/inspector/console_message.h"
#include "third_party/blink/renderer/core/loader/document_loader.h"
#include "third_party/blink/renderer/core/loader/modulescript/module_script_creation_params.h"
#include "third_party/blink/renderer/core/page/page.h"
#include "third_party/blink/renderer/core/script/classic_script.h"
#include "third_party/blink/renderer/platform/bindings/exception_state.h"
#include "third_party/blink/renderer/platform/loader/fetch/render_blocking_behavior.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_error.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_loader_options.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_request.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_response.h"
#include "third_party/blink/renderer/platform/weborigin/kurl.h"
#include "third_party/blink/renderer/platform/wtf/text/atomic_string.h"
#include "v8/include/v8.h"

namespace {

bool IsPartOfPlaceholderDocument(blink::Node* node) {
  blink::Document* document_node = nullptr;
  if (node->IsDocumentNode()) {
    document_node = DynamicTo<blink::Document>(node);
  } else {
    document_node = &node->GetDocument();
  }

  if (document_node->IsInitialEmptyDocument()) {
    return true;
  }
  return false;
}

}  // namespace

namespace blink {

// static
const char PageGraphProxy::kSupplementName[] = "PageGraphProxy";

// static
PageGraphProxy* PageGraphProxy::From(LocalFrame& frame) {
  CHECK(!frame.GetPage()->IsOrdinary());
  return Supplement<LocalFrame>::From<PageGraphProxy>(frame);
}

// static
void PageGraphProxy::ProvideTo(LocalFrame& frame) {
  // Cache feature enabled status to not slow down LocalFrame creation.
  static const bool is_enabled =
      base::FeatureList::IsEnabled(brave_page_graph::features::kPageGraph);
  if (!is_enabled) {
    return;
  }
  DCHECK(!PageGraphProxy::From(frame));
  DCHECK(frame.IsLocalRoot());
  Supplement<LocalFrame>::ProvideTo(
      frame, MakeGarbageCollected<PageGraphProxy>(frame));
}

PageGraphProxy::PageGraphProxy(LocalFrame& local_frame)
    : Supplement<LocalFrame>(local_frame) {
  VLOG(2) << "PageGraphProxy::PageGraphProxy)";
  DCHECK(local_frame.IsLocalRoot());
  local_frame.GetProbeSink()->AddPageGraphProxy(this);
}

void PageGraphProxy::Trace(blink::Visitor* visitor) const {
  Supplement<LocalFrame>::Trace(visitor);
  visitor->Trace(parent_page_graph_);
}

PageGraph* PageGraphProxy::GetPageGraph() {
  VLOG(2) << "PageGraphProxy::GetPageGraph) ";
  if (parent_page_graph_) {
    return parent_page_graph_;
  }
  PageGraph* pagegraph;

  LocalFrame* local_frame = GetSupplementable();
  CHECK(local_frame->IsLocalRoot());
  Page* page = local_frame->GetPage();

  auto num_local_ordinary_pages = page->OrdinaryPages().size();
  if (num_local_ordinary_pages > 0) {
    blink::Page* ordinary_page = page->OrdinaryPages().TakeAny();
    Frame* ordinary_main_frame = ordinary_page->MainFrame();
    CHECK(ordinary_main_frame->IsLocalFrame());
    LocalFrame* frame = To<LocalFrame>(ordinary_main_frame);
    pagegraph = Supplement<LocalFrame>::From<PageGraph>(frame);
    if (pagegraph) {
      parent_page_graph_ = pagegraph;
      return pagegraph;
    }
  }

  // Otherwise, we go buck wild and just try to find an open ordinary page
  // with the same isolate that this page has.
  unsigned num_pagegraphs = PageGraph::NumAttachedPageGraphs();
  CHECK(num_pagegraphs == 1);
  for (auto&& a_pagegraph : PageGraph::AllPageGraphs()) {
    if (!a_pagegraph->IsDocumentDetached()) {
      parent_page_graph_ = a_pagegraph;
      return a_pagegraph;
    }
  }

  CHECK(false) << "Couldn't find any parent graph";
  return nullptr;
}

void PageGraphProxy::NodeCreated(blink::Node* node) {
  if (!IsPartOfPlaceholderDocument(node)) {
    VLOG(2) << "PageGraphProxy::NodeCreated)";
    GetPageGraph()->NodeCreated(node);
  }
}

void PageGraphProxy::RegisterPageGraphNodeFullyCreated(Node* node) {
  if (!IsPartOfPlaceholderDocument(node)) {
    VLOG(2) << "PageGraphProxy::RegisterPageGraphNodeFullyCreated)";
    GetPageGraph()->RegisterPageGraphNodeFullyCreated(node);
  }
}

void PageGraphProxy::DidInsertDOMNode(Node* node) {
  if (!IsPartOfPlaceholderDocument(node)) {
    VLOG(2) << "PageGraphProxy::DidInsertDOMNode)";
    GetPageGraph()->DidInsertDOMNode(node);
  }
}

void PageGraphProxy::WillRemoveDOMNode(Node* node) {
  VLOG(2) << "PageGraphProxy::WillRemoveDOMNode)";
  GetPageGraph()->WillRemoveDOMNode(node);
}

void PageGraphProxy::DidModifyDOMAttr(Element* element,
                                      const QualifiedName& name,
                                      const AtomicString& value) {
  if (!IsPartOfPlaceholderDocument(element)) {
    VLOG(2) << "PageGraphProxy::DidModifyDOMAttr)";
    GetPageGraph()->DidModifyDOMAttr(element, name, value);
  }
}

void PageGraphProxy::DidRemoveDOMAttr(Element* element,
                                      const QualifiedName& name) {
  VLOG(2) << "PageGraphProxy::DidRemoveDOMAttr)";
  GetPageGraph()->DidRemoveDOMAttr(element, name);
}

void PageGraphProxy::DidCommitLoad(LocalFrame* frame, DocumentLoader* loader) {
  VLOG(2) << "PageGraphProxy::DidCommitLoad)";
  GetPageGraph()->DidCommitLoad(frame, loader);
}

void PageGraphProxy::WillSendNavigationRequest(uint64_t identifier,
                                               DocumentLoader* loader,
                                               const KURL& url,
                                               const AtomicString& http_method,
                                               EncodedFormData* form_data) {
  GetPageGraph()->WillSendNavigationRequest(identifier, loader, url,
                                            http_method, form_data);
}

void PageGraphProxy::WillSendRequest(
    ExecutionContext* execution_context,
    DocumentLoader* loader,
    const KURL& fetch_context_url,
    const ResourceRequest& request,
    const ResourceResponse& redirect_response,
    const ResourceLoaderOptions& options,
    ResourceType resource_type,
    RenderBlockingBehavior render_blocking_behavior,
    base::TimeTicks timestamp) {
  GetPageGraph()->WillSendRequest(
      execution_context, loader, fetch_context_url, request, redirect_response,
      options, resource_type, render_blocking_behavior, timestamp);
}

void PageGraphProxy::DidReceiveResourceResponse(
    uint64_t identifier,
    DocumentLoader* loader,
    const ResourceResponse& response,
    const Resource* cached_resource) {
  VLOG(2) << "PageGraphProxy::DidReceiveResourceResponse)";
  GetPageGraph()->DidReceiveResourceResponse(identifier, loader, response,
                                             cached_resource);
}

void PageGraphProxy::DidReceiveData(uint64_t identifier,
                                    DocumentLoader* loader,
                                    const char* data,
                                    uint64_t data_length) {
  GetPageGraph()->DidReceiveData(identifier, loader, data, data_length);
}

void PageGraphProxy::DidReceiveBlob(uint64_t identifier,
                                    DocumentLoader* loader,
                                    BlobDataHandle* handle) {
  GetPageGraph()->DidReceiveBlob(identifier, loader, handle);
}

void PageGraphProxy::DidFinishLoading(uint64_t identifier,
                                      DocumentLoader* loader,
                                      base::TimeTicks finish_time,
                                      int64_t encoded_data_length,
                                      int64_t decoded_body_length) {
  GetPageGraph()->DidFinishLoading(identifier, loader, finish_time,
                                   encoded_data_length, decoded_body_length);
}

void PageGraphProxy::DidFailLoading(
    CoreProbeSink* sink,
    uint64_t identifier,
    DocumentLoader* loader,
    const ResourceError& error,
    const base::UnguessableToken& devtools_frame_or_worker_token) {
  GetPageGraph()->DidFailLoading(sink, identifier, loader, error,
                                 devtools_frame_or_worker_token);
}

void PageGraphProxy::ApplyCompilationModeOverride(
    const ClassicScript& script,
    v8::ScriptCompiler::CachedData** data,
    v8::ScriptCompiler::CompileOptions* options) {
  GetPageGraph()->ApplyCompilationModeOverride(script, data, options);
}

void PageGraphProxy::RegisterPageGraphScriptCompilation(
    ExecutionContext* execution_context,
    const ReferrerScriptInfo& referrer_info,
    const ClassicScript& classic_script,
    v8::Local<v8::Script> script) {
  GetPageGraph()->RegisterPageGraphScriptCompilation(
      execution_context, referrer_info, classic_script, script);
}

void PageGraphProxy::RegisterPageGraphModuleCompilation(
    ExecutionContext* execution_context,
    const ReferrerScriptInfo& referrer_info,
    const ModuleScriptCreationParams& params,
    v8::Local<v8::Module> script) {
  GetPageGraph()->RegisterPageGraphModuleCompilation(
      execution_context, referrer_info, params, script);
}

void PageGraphProxy::RegisterPageGraphScriptCompilationFromAttr(
    EventTarget* event_target,
    const String& function_name,
    const String& script_body,
    v8::Local<v8::Function> compiled_function) {
  GetPageGraph()->RegisterPageGraphScriptCompilationFromAttr(
      event_target, function_name, script_body, compiled_function);
}

void PageGraphProxy::RegisterPageGraphBindingEvent(
    ExecutionContext* execution_context,
    const char* name,
    blink::PageGraphBindingType type,
    blink::PageGraphBindingEvent event) {
  GetPageGraph()->RegisterPageGraphBindingEvent(execution_context, name, type,
                                                event);
}
void PageGraphProxy::RegisterPageGraphWebAPICallWithResult(
    ExecutionContext* execution_context,
    const char* name,
    const blink::PageGraphObject& receiver_data,
    const blink::PageGraphValues& args,
    const ExceptionState* exception_state,
    const std::optional<blink::PageGraphValue>& result) {
  GetPageGraph()->RegisterPageGraphWebAPICallWithResult(
      execution_context, name, receiver_data, args, exception_state, result);
}
// Event listeners tracking:
void PageGraphProxy::RegisterPageGraphEventListenerAdd(
    EventTarget* event_target,
    const String& event_type,
    RegisteredEventListener* registered_listener) {
  GetPageGraph()->RegisterPageGraphEventListenerAdd(event_target, event_type,
                                                    registered_listener);
}
void PageGraphProxy::RegisterPageGraphEventListenerRemove(
    EventTarget* event_target,
    const String& event_type,
    RegisteredEventListener* registered_listener) {
  GetPageGraph()->RegisterPageGraphEventListenerRemove(event_target, event_type,
                                                       registered_listener);
}

void PageGraphProxy::RegisterPageGraphJavaScriptUrl(Document* document,
                                                    const KURL& url) {
  GetPageGraph()->RegisterPageGraphJavaScriptUrl(document, url);
}

// Console message tracking:
void PageGraphProxy::ConsoleMessageAdded(ConsoleMessage* console_message) {
  GetPageGraph()->ConsoleMessageAdded(console_message);
}

}  // namespace blink
