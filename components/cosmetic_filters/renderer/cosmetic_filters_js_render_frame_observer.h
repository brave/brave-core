/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_COSMETIC_FILTERS_RENDERER_COSMETIC_FILTERS_JS_RENDER_FRAME_OBSERVER_H_
#define BRAVE_COMPONENTS_COSMETIC_FILTERS_RENDERER_COSMETIC_FILTERS_JS_RENDER_FRAME_OBSERVER_H_

#include <memory>
#include <optional>

#include "base/memory/weak_ptr.h"
#include "base/one_shot_event.h"
#include "brave/components/cosmetic_filters/renderer/cosmetic_filters_js_handler.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_frame_observer.h"
#include "content/public/renderer/render_frame_observer_tracker.h"
#include "third_party/blink/public/web/web_navigation_type.h"
#include "url/gurl.h"
#include "v8/include/v8.h"

namespace cosmetic_filters {

// CosmeticFiltersJsRenderFrame observer waits for a page to be loaded and then
// adds the Javascript worker object.
class CosmeticFiltersJsRenderFrameObserver
    : public content::RenderFrameObserver,
      public content::RenderFrameObserverTracker<
          CosmeticFiltersJsRenderFrameObserver> {
 public:
  CosmeticFiltersJsRenderFrameObserver(
      content::RenderFrame* render_frame,
      const int32_t isolated_world_id,
      base::RepeatingCallback<bool(void)> get_de_amp_enabled_closure_);
  ~CosmeticFiltersJsRenderFrameObserver() override;

  CosmeticFiltersJsRenderFrameObserver(
      const CosmeticFiltersJsRenderFrameObserver&) = delete;
  CosmeticFiltersJsRenderFrameObserver& operator=(
      const CosmeticFiltersJsRenderFrameObserver&) = delete;

  // RenderFrameObserver implementation.
  void DidStartNavigation(
      const GURL& url,
      std::optional<blink::WebNavigationType> navigation_type) override;
  void ReadyToCommitNavigation(
      blink::WebDocumentLoader* document_loader) override;

  void DidCreateScriptContext(v8::Local<v8::Context> context,
                              int32_t world_id) override;

  void RunScriptsAtDocumentStart();

 private:
  void OnProcessURL();
  void ApplyRules();

  // RenderFrameObserver implementation.
  void OnDestruct() override;

  // The isolated world that the cosmetic filters object should be written to.
  int32_t isolated_world_id_;

  // Handle to "handler" JavaScript object functionality.
  std::unique_ptr<CosmeticFiltersJSHandler> native_javascript_handle_;

  GURL url_;
  base::RepeatingCallback<bool(void)> get_de_amp_enabled_closure_;

  std::unique_ptr<base::OneShotEvent> ready_;

  base::WeakPtrFactory<CosmeticFiltersJsRenderFrameObserver> weak_factory_{
      this};
};

}  // namespace cosmetic_filters

#endif  // BRAVE_COMPONENTS_COSMETIC_FILTERS_RENDERER_COSMETIC_FILTERS_JS_RENDER_FRAME_OBSERVER_H_
