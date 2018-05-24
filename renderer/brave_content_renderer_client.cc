/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/renderer/brave_content_renderer_client.h"
#include "third_party/blink/public/platform/web_runtime_features.h"

BraveContentRendererClient::BraveContentRendererClient()
    : ChromeContentRendererClient() {}

void BraveContentRendererClient::SetRuntimeFeaturesDefaultsBeforeBlinkInitialization() {
  blink::WebRuntimeFeatures::EnableWebUsb(false);
  blink::WebRuntimeFeatures::EnableSharedArrayBuffer(false);
}
BraveContentRendererClient::~BraveContentRendererClient() = default;
