// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/brave_shields/ad_block_popup_util.h"

#include <utility>

#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "brave/browser/brave_shields/brave_shields_web_contents_observer.h"
#include "brave/components/brave_shields/core/browser/ad_block_engine_wrapper.h"
#include "brave/components/brave_shields/core/browser/ad_block_request_type.h"
#include "brave/components/brave_shields/core/browser/ad_block_service.h"
#include "brave/components/brave_shields/core/browser/brave_shields_utils.h"
#include "brave/components/brave_shields/core/common/brave_shield_constants.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "url/gurl.h"
#include "url/scheme_host_port.h"

namespace brave_shields {

namespace {

// Sandboxed frames and blank windows resolve to the page they came from.
url::Origin NonOpaqueOrigin(const url::Origin& origin) {
  if (!origin.opaque()) {
    return origin;
  }

  const url::SchemeHostPort& precursor =
      origin.GetTupleOrPrecursorTupleIfOpaque();
  return precursor.IsValid() ? url::Origin::Create(precursor.GetURL()) : origin;
}

bool ShouldBlockPopupOnTaskRunner(const GURL& url,
                                  const url::Origin& source,
                                  bool aggressive,
                                  AdBlockEngineWrapper* engine_wrapper) {
  auto result = engine_wrapper->ShouldStartRequest(
      url, AdBlockRequestType::kPopup, source, "GET", aggressive, false, false,
      false);
  return result.important || (result.matched && !result.has_exception);
}

void OnShouldBlockPopup(const GURL& url,
                        content::GlobalRenderFrameHostToken frame_token,
                        base::OnceCallback<void(bool)> callback,
                        bool should_block) {
  if (should_block) {
    BraveShieldsWebContentsObserver::DispatchBlockedEvent(url, frame_token,
                                                          kAds);
  }

  std::move(callback).Run(should_block);
}

}  // namespace

std::optional<PopupOpener> GetPopupOpener(
    HostContentSettingsMap* content_settings,
    content::RenderFrameHost* opener_main_frame,
    const url::Origin& source,
    const GURL& url) {
  auto* observer = BraveShieldsWebContentsObserver::FromWebContents(
      content::WebContents::FromRenderFrameHost(opener_main_frame));

  if (observer && observer->IsPressedLink(url)) {
    return std::nullopt;
  }

  const GURL tab_url =
      NonOpaqueOrigin(opener_main_frame->GetLastCommittedOrigin()).GetURL();

  if (!IsBraveShieldsEnabled(content_settings, tab_url) ||
      GetAdControlType(content_settings, tab_url) != ControlType::BLOCK) {
    return std::nullopt;
  }

  return PopupOpener{
      .source = NonOpaqueOrigin(source),
      .aggressive = GetCosmeticFilteringControlType(
                        content_settings, tab_url) == ControlType::BLOCK,
      .frame_token = opener_main_frame->GetGlobalFrameToken(),
  };
}

bool ShouldCheckPopupUrl(HostContentSettingsMap* content_settings,
                         const GURL& url) {
  // Shields down on the popup's site skips the check.
  return url.SchemeIsHTTPOrHTTPS() &&
         IsBraveShieldsEnabled(content_settings, url);
}

void ShouldBlockPopup(AdBlockService* ad_block_service,
                      const PopupOpener& opener,
                      const GURL& url,
                      base::OnceCallback<void(bool)> callback) {
  ad_block_service->AsyncCallAndReplyWithResult<bool>(
      base::BindOnce(&ShouldBlockPopupOnTaskRunner, url, opener.source,
                     opener.aggressive),
      base::BindOnce(&OnShouldBlockPopup, url, opener.frame_token,
                     std::move(callback)));
}

}  // namespace brave_shields
