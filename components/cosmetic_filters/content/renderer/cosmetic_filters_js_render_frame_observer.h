/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_COSMETIC_FILTERS_CONTENT_RENDERER_COSMETIC_FILTERS_JS_RENDER_FRAME_OBSERVER_H_
#define BRAVE_COMPONENTS_COSMETIC_FILTERS_CONTENT_RENDERER_COSMETIC_FILTERS_JS_RENDER_FRAME_OBSERVER_H_

#include <memory>

#include "brave/components/cosmetic_filters/content/renderer/cosmetic_filters_js_handler.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_frame_observer.h"
#include "url/gurl.h"
#include "v8/include/v8.h"

namespace cosmetic_filters {

// CosmeticFiltersJsRenderFrame observer waits for a page to be loaded and then
// adds the Javascript worker object.
class CosmeticFiltersJsRenderFrameObserver
    : public content::RenderFrameObserver {
 public:
  CosmeticFiltersJsRenderFrameObserver(content::RenderFrame* render_frame,
                                       const int32_t isolated_world_id);
  ~CosmeticFiltersJsRenderFrameObserver() override;

  // RenderFrameObserver implementation.
  void DidStartNavigation(
      const GURL& url,
      base::Optional<blink::WebNavigationType> navigation_type) override;
  void DidCreateScriptContext(v8::Local<v8::Context> context,
                              int32_t world_id) override;
  void DidCreateNewDocument() override;

 private:
  // RenderFrameObserver implementation.
  void OnDestruct() override;

  // The isolated world that the distiller object should be written to.
  int32_t worker_isolated_world_id_;

  // Handle to "handler" JavaScript object functionality.
  std::unique_ptr<CosmeticFiltersJSHandler> native_javascript_handle_;

  GURL url_;
};

}  // namespace cosmetic_filters

#endif  // BRAVE_COMPONENTS_COSMETIC_FILTERS_CONTENT_RENDERER_COSMETIC_FILTERS_JS_RENDER_FRAME_OBSERVER_H_
