/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/cosmetic_filters/renderer/cosmetic_filters_js_render_frame_observer.h"

#include <memory>
#include <optional>
#include <utility>

#include "base/feature_list.h"
#include "base/functional/bind.h"
#include "brave/components/brave_shields/core/common/features.h"
#include "brave/components/de_amp/common/features.h"
#include "content/public/renderer/render_frame.h"
#include "third_party/blink/public/web/web_local_frame.h"

namespace cosmetic_filters {

CosmeticFiltersJsRenderFrameObserver::CosmeticFiltersJsRenderFrameObserver(
    content::RenderFrame* render_frame,
    const int32_t isolated_world_id,
    base::RepeatingCallback<bool(void)> get_de_amp_enabled_closure)
    : RenderFrameObserver(render_frame),
      RenderFrameObserverTracker<CosmeticFiltersJsRenderFrameObserver>(
          render_frame),
      isolated_world_id_(isolated_world_id),
      native_javascript_handle_(
          new CosmeticFiltersJSHandler(render_frame, isolated_world_id)),
      get_de_amp_enabled_closure_(std::move(get_de_amp_enabled_closure)),
      ready_(new base::OneShotEvent()) {}

CosmeticFiltersJsRenderFrameObserver::~CosmeticFiltersJsRenderFrameObserver() =
    default;

void CosmeticFiltersJsRenderFrameObserver::DidStartNavigation(
    const GURL& url,
    std::optional<blink::WebNavigationType> navigation_type) {
  url_ = url;
}

void CosmeticFiltersJsRenderFrameObserver::ReadyToCommitNavigation(
    blink::WebDocumentLoader* document_loader) {
  ready_ = std::make_unique<base::OneShotEvent>();
  // invalidate weak pointers on navigation so we don't get callbacks from the
  // previous url load
  weak_factory_.InvalidateWeakPtrs();

  // There could be empty, invalid and "about:blank" URLs,
  // they should fallback to the main frame rules
  if (url_.is_empty() || !url_.is_valid() || url_.spec() == "about:blank")
    url_ = url::Origin(render_frame()->GetWebFrame()->GetSecurityOrigin())
               .GetURL();

  if (!url_.SchemeIsHTTPOrHTTPS())
    return;

  if (base::FeatureList::IsEnabled(
          ::brave_shields::features::kCosmeticFilteringSyncLoad)) {
    if (native_javascript_handle_->ProcessURL(url_, std::nullopt)) {
      ready_->Signal();
    }
  } else {
    native_javascript_handle_->ProcessURL(
        url_, std::make_optional(base::BindOnce(
                  &CosmeticFiltersJsRenderFrameObserver::OnProcessURL,
                  weak_factory_.GetWeakPtr())));
  }
}

void CosmeticFiltersJsRenderFrameObserver::RunScriptsAtDocumentStart() {
  if (ready_->is_signaled()) {
    ApplyRules();
  } else {
    ready_->Post(
        FROM_HERE,
        base::BindOnce(&CosmeticFiltersJsRenderFrameObserver::ApplyRules,
                       weak_factory_.GetWeakPtr()));
  }
}

void CosmeticFiltersJsRenderFrameObserver::ApplyRules() {
  bool de_amp_enabled = get_de_amp_enabled_closure_.Run();
  native_javascript_handle_->ApplyRules(de_amp_enabled);
}

void CosmeticFiltersJsRenderFrameObserver::OnProcessURL() {
  ready_->Signal();
}

void CosmeticFiltersJsRenderFrameObserver::DidCreateScriptContext(
    v8::Local<v8::Context> context,
    int32_t world_id) {
  if (world_id != isolated_world_id_)
    return;

  native_javascript_handle_->AddJavaScriptObjectToFrame(context);
}

void CosmeticFiltersJsRenderFrameObserver::OnDestruct() {
  delete this;
}

}  // namespace cosmetic_filters
