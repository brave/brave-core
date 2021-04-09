/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SEARCH_RENDERER_BRAVE_SEARCH_JS_HANDLER_H_
#define BRAVE_COMPONENTS_BRAVE_SEARCH_RENDERER_BRAVE_SEARCH_JS_HANDLER_H_

#include <memory>
#include <string>
#include <vector>

#include "brave/components/brave_search/common/brave_search.mojom.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "url/gurl.h"
#include "v8/include/v8.h"

namespace brave_search {

class BraveSearchJSHandler {
 public:
  BraveSearchJSHandler();
  BraveSearchJSHandler(const BraveSearchJSHandler&) = delete;
  BraveSearchJSHandler& operator=(const BraveSearchJSHandler&) = delete;
  ~BraveSearchJSHandler();

  void AddJavaScriptObject(v8::Local<v8::Context> context);
  v8::Local<v8::Context> Context();
  v8::Isolate* GetIsolate();

 private:
  // Adds a function to the provided object.
  template <typename Sig>
  void BindFunctionToObject(v8::Local<v8::Object> javascript_object,
                            const std::string& name,
                            const base::RepeatingCallback<Sig>& callback);
  void BindFunctionsToObject(v8::Local<v8::Context> context);
  bool EnsureConnected();

  // A function to be called from JS
  v8::Local<v8::Promise> FetchBackupResults(const std::string& query_string,
                                            const std::string& lang,
                                            const std::string& country,
                                            const std::string& geo);
  void OnFetchBackupResults(
      std::unique_ptr<v8::Global<v8::Promise::Resolver>> promise_resolver,
      const std::string& response);

  mojo::Remote<brave_search::mojom::BraveSearchFallback> brave_search_fallback_;
  std::unique_ptr<v8::Global<v8::Context>> context_;
  v8::Isolate* isolate_;
};

}  // namespace brave_search

#endif  // BRAVE_COMPONENTS_BRAVE_SEARCH_RENDERER_BRAVE_SEARCH_JS_HANDLER_H_
