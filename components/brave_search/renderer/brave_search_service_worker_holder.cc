/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_search/renderer/brave_search_service_worker_holder.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/auto_reset.h"
#include "base/ranges/algorithm.h"
#include "base/threading/thread_checker.h"
#include "brave/components/brave_search/common/brave_search_utils.h"
#include "brave/components/brave_search/renderer/brave_search_fallback_js_handler.h"
#include "content/public/renderer/worker_thread.h"
#include "url/gurl.h"

namespace brave_search {

namespace {

class JsHandlersForCurrentThread;

// A thread local pointer for the js handlers available.
constinit thread_local JsHandlersForCurrentThread* current_js_handlers =
    nullptr;

// A scoping class to create a local thread storage for the JS handlers, storing
// BraveSearchFallbackJSHandler instance in local thread storage, and providing
// ways to add/remove them. The instance self deletes on
// content::WorkerThread::Observer::WillStopCurrentWorkerThread.
class [[maybe_unused, nodiscard]] JsHandlersForCurrentThread
    : public content::WorkerThread::Observer {
 public:
  JsHandlersForCurrentThread(const JsHandlersForCurrentThread&) = delete;
  JsHandlersForCurrentThread& operator=(const JsHandlersForCurrentThread&) =
      delete;

  ~JsHandlersForCurrentThread() override;

  // Gets the current js handlers, or creates an instance of it for the current
  // thread.
  static JsHandlersForCurrentThread* Get();

  // Adds a JS handler to the scope.
  void AddJsHandler(std::unique_ptr<BraveSearchFallbackJSHandler> js_handler);

  // Searches and deletes
  void RemoveContext(const v8::Local<v8::Context>& v8_context);

 private:
  JsHandlersForCurrentThread();

  // WorkerThread::Observer:
  void WillStopCurrentWorkerThread() override;

  // A vector for the handlers being held.
  std::vector<std::unique_ptr<BraveSearchFallbackJSHandler>> js_handlers_;

  // A resetter responsible to make sure the local thread storage is set back to
  // nullptr once the scope is gone.
  const base::AutoReset<JsHandlersForCurrentThread*> resetter_;

  // This object should always be constructed and destructed on the same thread.
  THREAD_CHECKER(thread_checker_);
};

JsHandlersForCurrentThread::JsHandlersForCurrentThread()
    : resetter_(&current_js_handlers, this) {
  content::WorkerThread::AddObserver(this);
}

JsHandlersForCurrentThread::~JsHandlersForCurrentThread() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  content::WorkerThread::RemoveObserver(this);
  for (auto& js_handler : js_handlers_) {
    js_handler->Invalidate();
  }
}

// static
JsHandlersForCurrentThread* JsHandlersForCurrentThread::Get() {
  if (current_js_handlers) {
    return current_js_handlers;
  }

  return new JsHandlersForCurrentThread();
}

void JsHandlersForCurrentThread::AddJsHandler(
    std::unique_ptr<BraveSearchFallbackJSHandler> js_handler) {
  js_handlers_.push_back(std::move(js_handler));
}

void JsHandlersForCurrentThread::RemoveContext(
    const v8::Local<v8::Context>& v8_context) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  js_handlers_.erase(
      base::ranges::remove_if(
          js_handlers_,
          [&v8_context](
              const std::unique_ptr<BraveSearchFallbackJSHandler>& js_handler) {
            v8::HandleScope handle_scope(js_handler->GetIsolate());
            v8::Context::Scope context_scope(js_handler->Context());
            return js_handler->Context() == v8_context;
          }),
      js_handlers_.end());
}

void JsHandlersForCurrentThread::WillStopCurrentWorkerThread() {
  DCHECK(current_js_handlers);
  delete this;
}

}  // namespace

BraveSearchServiceWorkerHolder::BraveSearchServiceWorkerHolder()
    : broker_(nullptr) {}

BraveSearchServiceWorkerHolder::~BraveSearchServiceWorkerHolder() = default;

void BraveSearchServiceWorkerHolder::SetBrowserInterfaceBrokerProxy(
    blink::ThreadSafeBrowserInterfaceBrokerProxy* broker) {
  broker_ = broker;
}

void BraveSearchServiceWorkerHolder::WillEvaluateServiceWorkerOnWorkerThread(
    blink::WebServiceWorkerContextProxy* context_proxy,
    v8::Local<v8::Context> v8_context,
    int64_t service_worker_version_id,
    const GURL& service_worker_scope,
    const GURL& script_url) {
  DCHECK(broker_);
  if (!service_worker_scope.is_valid() ||
      !service_worker_scope.SchemeIsHTTPOrHTTPS() ||
      !IsAllowedHost(service_worker_scope))
    return;

  std::unique_ptr<BraveSearchFallbackJSHandler> js_handler(
      new BraveSearchFallbackJSHandler(v8_context, broker_));
  js_handler->AddJavaScriptObject();

  JsHandlersForCurrentThread::Get()->AddJsHandler(std::move(js_handler));
}

void BraveSearchServiceWorkerHolder::
    WillDestroyServiceWorkerContextOnWorkerThread(
        v8::Local<v8::Context> v8_context,
        int64_t service_worker_version_id,
        const GURL& service_worker_scope,
        const GURL& script_url) {
  if (!service_worker_scope.is_valid() ||
      !service_worker_scope.SchemeIsHTTPOrHTTPS() ||
      !IsAllowedHost(service_worker_scope))
    return;

  if (!current_js_handlers) {
    return;
  }

  current_js_handlers->RemoveContext(v8_context);
}

}  // namespace brave_search
