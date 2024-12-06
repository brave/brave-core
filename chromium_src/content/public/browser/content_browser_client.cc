/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "src/content/public/browser/content_browser_client.cc"

#include "base/debug/dump_without_crashing.h"

namespace content {

std::string ContentBrowserClient::GetEffectiveUserAgent(
    BrowserContext* browser_context,
    const GURL& url) {
  return std::string();
}

bool ContentBrowserClient::AllowWorkerFingerprinting(
    const GURL& url,
    BrowserContext* browser_context) {
  return true;
}

std::optional<base::UnguessableToken>
ContentBrowserClient::GetEphemeralStorageToken(
    RenderFrameHost* render_frame_host,
    const url::Origin& origin) {
  return std::nullopt;
}

brave_shields::mojom::ShieldsSettingsPtr
ContentBrowserClient::WorkerGetBraveShieldSettings(
    const GURL& url,
    BrowserContext* browser_context) {
  // BraveContentBrowserClient should implement this. It's possible this is
  // reached somehow, add dumps to see if it's true.
  base::debug::DumpWithoutCrashing();
  return brave_shields::mojom::ShieldsSettingsPtr();
}

std::optional<GURL> ContentBrowserClient::SanitizeURL(content::RenderFrameHost*,
                                                      const GURL& url) {
  return std::nullopt;
}

}  // namespace content
