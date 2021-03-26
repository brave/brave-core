/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/renderer/brave_content_renderer_client.h"

#include "base/feature_list.h"
#include "brave/components/brave_search/renderer/brave_search_render_frame_observer.h"
#include "brave/components/brave_shields/common/features.h"
#include "brave/components/cosmetic_filters/renderer/cosmetic_filters_js_render_frame_observer.h"
#include "chrome/common/chrome_isolated_world_ids.h"
#include "third_party/blink/public/platform/web_runtime_features.h"

BraveContentRendererClient::BraveContentRendererClient()
    : ChromeContentRendererClient() {}

void BraveContentRendererClient::
SetRuntimeFeaturesDefaultsBeforeBlinkInitialization() {
  ChromeContentRendererClient::
      SetRuntimeFeaturesDefaultsBeforeBlinkInitialization();

  blink::WebRuntimeFeatures::EnableSharedArrayBuffer(false);
  blink::WebRuntimeFeatures::EnableWebNfc(false);

  // These features don't have dedicated WebRuntimeFeatures wrappers.
  blink::WebRuntimeFeatures::EnableFeatureFromString("DigitalGoods", false);
  blink::WebRuntimeFeatures::EnableFeatureFromString("FileSystemAccess", false);
  blink::WebRuntimeFeatures::EnableFeatureFromString(
      "FileSystemAccessAPIExperimental", false);
  blink::WebRuntimeFeatures::EnableFeatureFromString("Serial", false);
}

BraveContentRendererClient::~BraveContentRendererClient() = default;

void BraveContentRendererClient::RenderFrameCreated(
    content::RenderFrame* render_frame) {
  ChromeContentRendererClient::RenderFrameCreated(render_frame);

#if !defined(OS_ANDROID) && !defined(CHROME_OS)
  if (base::FeatureList::IsEnabled(
          brave_shields::features::kBraveAdblockCosmeticFilteringNative))
#endif
    new cosmetic_filters::CosmeticFiltersJsRenderFrameObserver(
        render_frame, ISOLATED_WORLD_ID_BRAVE_INTERNAL);
  new brave_search::BraveSearchRenderFrameObserver(
      render_frame, content::ISOLATED_WORLD_ID_GLOBAL);
}
