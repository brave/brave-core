/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_COSMETIC_FILTERS_RENDERER_COSMETIC_FILTERS_JS_HANDLER_H_
#define BRAVE_COMPONENTS_COSMETIC_FILTERS_RENDERER_COSMETIC_FILTERS_JS_HANDLER_H_

#include <memory>
#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/cosmetic_filters/common/cosmetic_filters.mojom.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_frame_observer.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "url/gurl.h"
#include "v8/include/v8.h"

namespace cosmetic_filters {

// CosmeticFiltersJSHandler class is responsible for JS execution inside a
// a given render_frame. It also does interactions with CosmeticFiltersResources
// class that lives in the main process.

class CosmeticFiltersJSHandler {
 public:
  CosmeticFiltersJSHandler(content::RenderFrame* render_frame,
                           const int32_t isolated_world_id);
  ~CosmeticFiltersJSHandler();

  // Adds the "cs_worker" JavaScript object and its functions to the current
  // render_frame_.
  void AddJavaScriptObjectToFrame(v8::Local<v8::Context> context);
  // Fetches an initial set of resources to inject into the page if cosmetic
  // filtering is enabled, and returns whether or not to proceed with cosmetic
  // filtering.
  bool ProcessURL(const GURL& url, absl::optional<base::OnceClosure> callback);
  void ApplyRules();

 private:
  void BindFunctionsToObject(v8::Isolate* isolate,
                             v8::Local<v8::Object> javascript_object);

  // Adds a function to the provided object.
  template <typename Sig>
  void BindFunctionToObject(v8::Isolate* isolate,
                            v8::Local<v8::Object> javascript_object,
                            const std::string& name,
                            const base::RepeatingCallback<Sig>& callback);
  bool EnsureConnected();
  void OnRemoteDisconnect();

  // Injects content_cosmetic bundle (if needed) and calls the entry point.
  void ExecuteObservingBundleEntryPoint();

  void CreateWorkerObject(v8::Isolate* isolate, v8::Local<v8::Context> context);

  // A function to be called from JS
  void HiddenClassIdSelectors(const std::string& input);

  void OnUrlCosmeticResources(base::OnceClosure callback,
                              base::Value result);
  void CSSRulesRoutine(base::DictionaryValue* resources_dict);
  void OnHiddenClassIdSelectors(base::Value result);
  bool OnIsFirstParty(const std::string& url_string);

  raw_ptr<content::RenderFrame> render_frame_ = nullptr;
  mojo::Remote<cosmetic_filters::mojom::CosmeticFiltersResources>
      cosmetic_filters_resources_;
  int32_t isolated_world_id_;
  bool enabled_1st_party_cf_;
  std::vector<std::string> exceptions_;
  GURL url_;
  std::unique_ptr<base::DictionaryValue> resources_dict_;

  // True if the content_cosmetic.bundle.js has injected in the current frame.
  bool bundle_injected_ = false;

  base::WeakPtrFactory<CosmeticFiltersJSHandler> weak_ptr_factory_{this};
};

// static
v8::Local<v8::Object> GetOrCreateWorkerObject(v8::Isolate* isolate,
                                              v8::Local<v8::Context> context);

}  // namespace cosmetic_filters

#endif  // BRAVE_COMPONENTS_COSMETIC_FILTERS_RENDERER_COSMETIC_FILTERS_JS_HANDLER_H_
