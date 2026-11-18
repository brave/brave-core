// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/brave_shields/ad_block_popup_throttle_delegate.h"

#include <memory>

#include "brave/browser/brave_shields/brave_shields_web_contents_observer.h"
#include "brave/components/brave_shields/core/common/brave_shield_constants.h"
#include "chrome/browser/ui/blocked_content/blocked_window_params.h"
#include "chrome/browser/ui/blocked_content/chrome_popup_navigation_delegate.h"
#include "components/blocked_content/popup_blocker.h"
#include "components/blocked_content/popup_blocker_tab_helper.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/referrer.h"
#include "third_party/blink/public/mojom/window_features/window_features.mojom.h"

namespace brave_shields {

AdBlockPopupThrottleDelegate::AdBlockPopupThrottleDelegate() = default;

AdBlockPopupThrottleDelegate::~AdBlockPopupThrottleDelegate() = default;

void AdBlockPopupThrottleDelegate::OnPopupBlocked(
    const AdBlockPopupOpener& opener,
    content::NavigationHandle& handle) {
  if (!opener.frame_token) {
    return;
  }

  BraveShieldsWebContentsObserver::DispatchBlockedEvent(
      handle.GetURL(), *opener.frame_token, kAds);

  content::RenderFrameHost* opener_frame =
      content::RenderFrameHost::FromFrameToken(*opener.frame_token);

  if (!opener_frame || !opener_frame->IsActive()) {
    return;
  }

  content::WebContents* web_contents =
      content::WebContents::FromRenderFrameHost(opener_frame);
  auto* popup_blocker =
      blocked_content::PopupBlockerTabHelper::FromWebContents(web_contents);

  if (!popup_blocker) {
    return;
  }

  // The window's name and features aren't available here, so a reopened popup
  // gets the defaults.
  content::WebContents* popup = handle.GetWebContents();
  const BlockedWindowParams params(
      handle.GetRedirectChain().front(), *opener.frame_token, opener.origin,
      opener_frame->GetSiteInstance(), content::Referrer(handle.GetReferrer()),
      /*frame_name=*/"", popup->GetOriginalWindowOpenDisposition(),
      blink::mojom::WindowFeatures(), handle.HasUserGesture(),
      !popup->HasOpener());
  // The block type is only used for metrics.
  popup_blocker->AddBlockedPopup(
      std::make_unique<ChromePopupNavigationDelegate>(
          params.CreateNavigateParams(opener_frame->GetProcess(),
                                      web_contents)),
      params.features(), blocked_content::PopupBlockType::kAbusive);
}

}  // namespace brave_shields
