/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_search/renderer/brave_search_sw_holder.h"

#include <string>
#include <utility>

#include "base/no_destructor.h"
#include "brave/components/brave_search/renderer/brave_search_js_handler.h"
#include "url/gurl.h"

namespace {

static base::NoDestructor<std::vector<std::string>> g_vetted_hosts(
    {"search.brave.com", "search-dev.brave.com"});

bool IsAllowedHost(const GURL& url) {
  std::string host = url.host();
  for (size_t i = 0; i < g_vetted_hosts->size(); i++) {
    if ((*g_vetted_hosts)[i] == host)
      return true;
  }

  return false;
}

}  // namespace

namespace brave_search {

using JSHandlersVector = std::vector<std::unique_ptr<BraveSearchJSHandler>>;

JSHandlersVector::iterator FindContext(JSHandlersVector* contexts,
                                       v8::Local<v8::Context> v8_context) {
  auto context_matches =
      [&v8_context](const std::unique_ptr<BraveSearchJSHandler>& context) {
        v8::HandleScope handle_scope(context->GetIsolate());
        v8::Context::Scope context_scope(context->Context());

        return context->Context() == v8_context;
      };

  return std::find_if(contexts->begin(), contexts->end(), context_matches);
}

// static
BraveSearchSWHolder* BraveSearchSWHolder::GetInstance() {
  static base::NoDestructor<BraveSearchSWHolder> instance;
  return instance.get();
}

BraveSearchSWHolder::BraveSearchSWHolder() {}

BraveSearchSWHolder::~BraveSearchSWHolder() = default;

void BraveSearchSWHolder::WillEvaluateServiceWorkerOnWorkerThread(
    blink::WebServiceWorkerContextProxy* context_proxy,
    v8::Local<v8::Context> v8_context,
    int64_t service_worker_version_id,
    const GURL& service_worker_scope,
    const GURL& script_url) {
  if (!service_worker_scope.is_valid() ||
      !service_worker_scope.SchemeIsHTTPOrHTTPS() ||
      !IsAllowedHost(service_worker_scope))
    return;

  std::unique_ptr<BraveSearchJSHandler> js_handler(new BraveSearchJSHandler());
  js_handler->AddJavaScriptObject(v8_context);

  JSHandlersVector* js_handlers = js_handlers_tls_.Get();
  if (!js_handlers) {
    js_handlers = new JSHandlersVector();
    js_handlers_tls_.Set(js_handlers);
  }
  js_handlers->push_back(std::move(js_handler));
}

void BraveSearchSWHolder::WillDestroyServiceWorkerContextOnWorkerThread(
    v8::Local<v8::Context> v8_context,
    int64_t service_worker_version_id,
    const GURL& service_worker_scope,
    const GURL& script_url) {
  if (!service_worker_scope.is_valid() ||
      !service_worker_scope.SchemeIsHTTPOrHTTPS() ||
      !IsAllowedHost(service_worker_scope))
    return;

  JSHandlersVector* js_handlers = js_handlers_tls_.Get();
  if (!js_handlers)
    return;

  auto context_it = FindContext(js_handlers, v8_context);
  js_handlers->erase(context_it);
}

}  // namespace brave_search
