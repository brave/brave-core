// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/brave_shields/ad_block_popup_navigation_throttle.h"

#include <memory>

#include "base/feature_list.h"
#include "base/functional/bind.h"
#include "base/task/single_thread_task_runner.h"
#include "brave/browser/brave_shields/ad_block_popup_util.h"
#include "brave/components/brave_shields/core/common/features.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "url/gurl.h"

namespace brave_shields {

// static
void AdBlockPopupNavigationThrottle::MaybeCreateAndAdd(
    content::NavigationThrottleRegistry& registry,
    AdBlockService* ad_block_service,
    HostContentSettingsMap* content_settings) {
  if (!ad_block_service) {
    return;
  }

  if (!base::FeatureList::IsEnabled(features::kBraveAdblockPopupBlocking)) {
    return;
  }

  content::NavigationHandle& handle = registry.GetNavigationHandle();

  if (!handle.IsInPrimaryMainFrame() || !handle.IsRendererInitiated()) {
    return;
  }

  // Also true with noopener; browser-opened windows have no original opener.
  content::WebContents* web_contents = handle.GetWebContents();

  if (!web_contents->HasLiveOriginalOpenerChain()) {
    return;
  }

  // Covers window.open() followed by a location change.
  content::NavigationEntry* entry =
      web_contents->GetController().GetLastCommittedEntry();

  if (!entry || !entry->IsInitialEntry()) {
    return;
  }

  registry.AddThrottle(std::make_unique<AdBlockPopupNavigationThrottle>(
      registry, ad_block_service, content_settings));
}

AdBlockPopupNavigationThrottle::AdBlockPopupNavigationThrottle(
    content::NavigationThrottleRegistry& registry,
    AdBlockService* ad_block_service,
    HostContentSettingsMap* content_settings)
    : content::NavigationThrottle(registry),
      ad_block_service_(ad_block_service),
      content_settings_(content_settings) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
}

AdBlockPopupNavigationThrottle::~AdBlockPopupNavigationThrottle() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
}

content::NavigationThrottle::ThrottleCheckResult
AdBlockPopupNavigationThrottle::WillStartRequest() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  content::NavigationHandle* handle = navigation_handle();
  const GURL& url = handle->GetURL();

  if (!opener_read_) {
    ReadOpener();
  }

  if (!opener_ || !ShouldCheckPopupUrl(content_settings_, url)) {
    return content::NavigationThrottle::PROCEED;
  }

  ShouldBlockPopup(
      ad_block_service_, *opener_, url,
      base::BindOnce(&AdBlockPopupNavigationThrottle::OnShouldBlockPopup,
                     weak_ptr_factory_.GetWeakPtr()));
  return content::NavigationThrottle::DEFER;
}

content::NavigationThrottle::ThrottleCheckResult
AdBlockPopupNavigationThrottle::WillRedirectRequest() {
  return WillStartRequest();
}

void AdBlockPopupNavigationThrottle::ReadOpener() {
  opener_read_ = true;
  content::NavigationHandle* handle = navigation_handle();
  content::WebContents* opener_contents =
      handle->GetWebContents()->GetFirstWebContentsInLiveOriginalOpenerChain();

  if (!opener_contents) {
    return;
  }

  content::RenderFrameHost* opener = opener_contents->GetPrimaryMainFrame();
  opener_ = GetPopupOpener(
      content_settings_, opener,
      handle->GetInitiatorOrigin().value_or(opener->GetLastCommittedOrigin()),
      handle->GetURL());
}

void AdBlockPopupNavigationThrottle::OnShouldBlockPopup(bool should_block) {
  if (!should_block) {
    Resume();
    return;
  }

  content::WebContents* web_contents = navigation_handle()->GetWebContents();
  // The throttle can't close its own window synchronously.
  base::SingleThreadTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(
                     [](base::WeakPtr<content::WebContents> web_contents) {
                       if (web_contents) {
                         web_contents->Close();
                       }
                     },
                     web_contents->GetWeakPtr()));
  CancelDeferredNavigation(content::NavigationThrottle::CANCEL_AND_IGNORE);
}

const char* AdBlockPopupNavigationThrottle::GetNameForLogging() {
  return "AdBlockPopupNavigationThrottle";
}

}  // namespace brave_shields
