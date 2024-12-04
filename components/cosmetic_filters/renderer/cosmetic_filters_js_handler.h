/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_COSMETIC_FILTERS_RENDERER_COSMETIC_FILTERS_JS_HANDLER_H_
#define BRAVE_COMPONENTS_COSMETIC_FILTERS_RENDERER_COSMETIC_FILTERS_JS_HANDLER_H_

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/cosmetic_filters/common/cosmetic_filters.mojom.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_frame_observer.h"
#include "mojo/public/cpp/bindings/associated_receiver.h"
#include "mojo/public/cpp/bindings/associated_remote.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "url/gurl.h"
#include "v8/include/v8.h"

namespace cosmetic_filters {

// CosmeticFiltersJSHandler class is responsible for JS execution inside a
// a given render_frame. It also does interactions with CosmeticFiltersResources
// class that lives in the main process.

class CosmeticFiltersJSHandler : public mojom::CosmeticFiltersAgent {
 public:
  CosmeticFiltersJSHandler(content::RenderFrame* render_frame,
                           const int32_t isolated_world_id);
  ~CosmeticFiltersJSHandler() override;

  // Adds the "cf_worker" JavaScript object and its functions to the current
  // render_frame_.
  void AddJavaScriptObjectToFrame(v8::Local<v8::Context> context);
  // Fetches an initial set of resources to inject into the page if cosmetic
  // filtering is enabled, and returns whether or not to proceed with cosmetic
  // filtering.
  bool ProcessURL(const GURL& url, std::optional<base::OnceClosure> callback);
  void ApplyRules(bool de_amp_enabled);

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

  void Bind(
      mojo::PendingAssociatedReceiver<mojom::CosmeticFiltersAgent> receiver);

  // CosmeticFiltersAgent overrides:
  void LaunchContentPicker() override;

  // Injects content_cosmetic bundle (if needed) and calls the entry point.
  void ExecuteObservingBundleEntryPoint();

  void CreateWorkerObject(v8::Isolate* isolate, v8::Local<v8::Context> context);

  // A function to be called from JS
  void HiddenClassIdSelectors(const std::string& input);

  void OnUrlCosmeticResources(base::OnceClosure callback,
                              base::Value result);
  void CSSRulesRoutine(const base::Value::Dict& resources_dict);
  void OnHiddenClassIdSelectors(base::Value::Dict result);
  bool OnIsFirstParty(const std::string& url_string);
  void OnAddSiteCosmeticFilter(const std::string& selector);
  void OnManageCustomFilters();
  void GetCosmeticFilterThemeInfo();
  void OnGetCosmeticFilterThemeInfo(bool is_dark_mode_enabled,
                                    int32_t background_color);
  int OnEventBegin(const std::string& event_name);
  void OnEventEnd(const std::string& event_name, int);

  void InjectStylesheet(const std::string& stylesheet);

  bool generichide_ = false;

  mojo::AssociatedRemote<cosmetic_filters::mojom::CosmeticFiltersHandler>&
  GetElementPickerRemoteHandler();
  void OnRemoteHandlerDisconnect();
  mojo::AssociatedRemote<cosmetic_filters::mojom::CosmeticFiltersHandler>
      element_picker_actions_handler_;
  raw_ptr<content::RenderFrame> render_frame_ = nullptr;
  mojo::Remote<mojom::CosmeticFiltersResources> cosmetic_filters_resources_;
  mojo::AssociatedReceiver<mojom::CosmeticFiltersAgent> receiver_{this};
  int32_t isolated_world_id_;
  bool enabled_1st_party_cf_;
  std::vector<std::string> exceptions_;
  GURL url_;
  std::optional<base::Value::Dict> resources_dict_;

  // True if the content_cosmetic.bundle.js has injected in the current frame.
  bool bundle_injected_ = false;

  std::unique_ptr<class CosmeticFilterPerfTracker> perf_tracker_;

  base::WeakPtrFactory<CosmeticFiltersJSHandler> weak_ptr_factory_{this};
};

// static
v8::Local<v8::Object> GetOrCreateWorkerObject(v8::Isolate* isolate,
                                              v8::Local<v8::Context> context);

}  // namespace cosmetic_filters

#endif  // BRAVE_COMPONENTS_COSMETIC_FILTERS_RENDERER_COSMETIC_FILTERS_JS_HANDLER_H_
