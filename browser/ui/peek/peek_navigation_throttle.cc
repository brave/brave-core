/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/peek/peek_navigation_throttle.h"

#include <memory>

#include "base/feature_list.h"
#include "base/functional/bind.h"
#include "base/location.h"
#include "base/memory/weak_ptr.h"
#include "base/task/sequenced_task_runner.h"
#include "brave/browser/ui/peek/peek_features.h"
#include "brave/browser/ui/peek/peek_utils.h"
#include "brave/browser/ui/peek/public/peek_controller.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/navigation_throttle_registry.h"
#include "content/public/browser/page_navigator.h"
#include "content/public/browser/site_instance.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/referrer.h"
#include "net/http/http_request_headers.h"
#include "ui/base/page_transition_types.h"
#include "ui/base/window_open_disposition.h"

namespace {

void OpenInPeek(base::WeakPtr<tabs::TabInterface> tab,
                content::OpenURLParams params) {
  if (!tab) {
    return;
  }
  if (auto* controller =
          PeekController::From(tab->GetBrowserWindowInterface())) {
    controller->Open(params);
  }
}

}  // namespace

// static
void PeekNavigationThrottle::MaybeCreateAndAdd(
    content::NavigationThrottleRegistry& registry) {
  if (base::FeatureList::IsEnabled(peek::features::kBravePeek)) {
    registry.AddThrottle(std::make_unique<PeekNavigationThrottle>(registry));
  }
}

PeekNavigationThrottle::PeekNavigationThrottle(
    content::NavigationThrottleRegistry& registry)
    : content::NavigationThrottle(registry) {}

PeekNavigationThrottle::~PeekNavigationThrottle() = default;

content::NavigationThrottle::ThrottleCheckResult
PeekNavigationThrottle::WillStartRequest() {
  content::NavigationHandle* handle = navigation_handle();
  if (!handle->IsInPrimaryMainFrame() || !handle->HasUserGesture() ||
      !handle->IsRendererInitiated() || handle->IsSameDocument() ||
      handle->IsDownload()) {
    return PROCEED;
  }

  // Without an initiator origin the navigation re-issued in Peek would look
  // browser-initiated, sending SameSite=Strict cookies and
  // `Sec-Fetch-Site: none`.
  if (!handle->GetInitiatorOrigin()) {
    return PROCEED;
  }

  // Only plain link clicks; GET carries all of the request in the URL.
  if (handle->GetRequestMethod() != net::HttpRequestHeaders::kGetMethod ||
      !ui::PageTransitionCoreTypeIs(handle->GetPageTransition(),
                                    ui::PAGE_TRANSITION_LINK)) {
    return PROCEED;
  }

  content::WebContents* web_contents = handle->GetWebContents();
  tabs::TabInterface* tab =
      tabs::TabInterface::MaybeGetFromContents(web_contents);
  if (!tab || !PeekController::From(tab->GetBrowserWindowInterface())) {
    return PROCEED;
  }

  if (!peek::ShouldPeekFromPinnedTab(tab->IsPinned(),
                                     web_contents->GetLastCommittedURL(),
                                     handle->GetURL())) {
    return PROCEED;
  }

  content::OpenURLParams params(
      handle->GetURL(), content::Referrer(handle->GetReferrer()),
      WindowOpenDisposition::NEW_FOREGROUND_TAB, ui::PAGE_TRANSITION_LINK,
      /*is_renderer_initiated=*/true);
  params.initiator_origin = handle->GetInitiatorOrigin();
  params.source_site_instance = handle->GetSourceSiteInstance();
  params.user_gesture = true;

  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE,
      base::BindOnce(&OpenInPeek, tab->GetWeakPtr(), std::move(params)));
  return CANCEL;
}

const char* PeekNavigationThrottle::GetNameForLogging() {
  return "PeekNavigationThrottle";
}
