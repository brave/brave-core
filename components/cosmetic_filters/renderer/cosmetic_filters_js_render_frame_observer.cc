/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/cosmetic_filters/renderer/cosmetic_filters_js_render_frame_observer.h"

#include <memory>
#include <utility>

#include "base/feature_list.h"
#include "base/functional/bind.h"
#include "brave/components/brave_shields/common/features.h"
#include "brave/components/de_amp/common/features.h"
#include "content/public/renderer/render_frame.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "third_party/blink/public/platform/web_isolated_world_info.h"
#include "third_party/blink/public/platform/web_url.h"
#include "third_party/blink/public/web/web_local_frame.h"

namespace cosmetic_filters {

namespace {

const char kSecurityOrigin[] = "chrome://cosmetic_filters";

void EnsureIsolatedWorldInitialized(int world_id) {
  static absl::optional<int> last_used_world_id;
  if (last_used_world_id) {
    // Early return since the isolated world info. is already initialized.
    DCHECK_EQ(*last_used_world_id, world_id)
        << "EnsureIsolatedWorldInitialized should always be called with the "
           "same |world_id|";
    return;
  }

  last_used_world_id = world_id;

  // Set an empty CSP so that the main world's CSP is not used in the isolated
  // world.
  constexpr char kContentSecurityPolicy[] = "";

  blink::WebIsolatedWorldInfo info;
  info.security_origin =
      blink::WebSecurityOrigin::Create(GURL(kSecurityOrigin));
  info.content_security_policy =
      blink::WebString::FromUTF8(kContentSecurityPolicy);
  blink::SetIsolatedWorldInfo(world_id, info);
}

}  // namespace

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
    absl::optional<blink::WebNavigationType> navigation_type) {
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
    if (native_javascript_handle_->ProcessURL(url_, absl::nullopt)) {
      ready_->Signal();
    }
  } else {
    native_javascript_handle_->ProcessURL(
        url_, absl::make_optional(base::BindOnce(
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

void CosmeticFiltersJsRenderFrameObserver::DidCreateNewDocument() {
  EnsureIsolatedWorldInitialized(isolated_world_id_);
}

void CosmeticFiltersJsRenderFrameObserver::OnDestruct() {
  delete this;
}

}  // namespace cosmetic_filters
