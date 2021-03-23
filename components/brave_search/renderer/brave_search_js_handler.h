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
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_frame_observer.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "url/gurl.h"
#include "v8/include/v8.h"

namespace brave_search {

class BraveSearchJSHandler {
 public:
  explicit BraveSearchJSHandler(content::RenderFrame* render_frame);
  BraveSearchJSHandler(const BraveSearchJSHandler&) = delete;
  BraveSearchJSHandler& operator=(const BraveSearchJSHandler&) = delete;
  ~BraveSearchJSHandler();

  void AddJavaScriptObjectToFrame(v8::Local<v8::Context> context);

 private:
  void BindFunctionsToObject(v8::Isolate* isolate,
                             v8::Local<v8::Context> context,
                             v8::Local<v8::Object> javascript_object);

  // Adds a function to the provided object.
  template <typename Sig>
  void BindFunctionToObject(v8::Isolate* isolate,
                            v8::Local<v8::Object> javascript_object,
                            const std::string& name,
                            const base::RepeatingCallback<Sig>& callback);
  void CreateAFallbackObject(v8::Isolate* isolate,
                             v8::Local<v8::Context> context);
  bool EnsureConnected();

  // A function to be called from JS
  v8::Local<v8::Promise> FetchBackupResults(v8::Isolate* isolate,
                                            const std::string& query_string,
                                            const std::string& lang,
                                            const std::string& country,
                                            const std::string& geo);
  void OnFetchBackupResults(
      std::unique_ptr<v8::Global<v8::Promise::Resolver>> promise_resolver,
      v8::Isolate* isolate,
      std::unique_ptr<v8::Global<v8::Context>> context_old,
      const std::string& response);

  content::RenderFrame* render_frame_;
  mojo::Remote<brave_search::mojom::BraveSearchFallback> brave_search_fallback_;
};

}  // namespace brave_search

#endif  // BRAVE_COMPONENTS_BRAVE_SEARCH_RENDERER_BRAVE_SEARCH_JS_HANDLER_H_
