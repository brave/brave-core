// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/debounce/content/browser/debounce_navigation_throttle.h"

#include <memory>
#include <string>
#include <vector>

#include "base/logging.h"
#include "brave/components/brave_shields/content/browser/restart_navigation.h"
#include "brave/components/debounce/core/browser/debounce_service.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/navigation_throttle.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "ui/base/page_transition_types.h"
#include "url/gurl.h"
#include "url/origin.h"

using content::NavigationHandle;
using content::NavigationThrottle;
using content::WebContents;

namespace debounce {

namespace {

class DebounceTabHelper
    : public content::WebContentsObserver,
      public content::WebContentsUserData<DebounceTabHelper> {
 public:
  explicit DebounceTabHelper(content::WebContents* web_contents)
      : content::WebContentsObserver(web_contents),
        content::WebContentsUserData<DebounceTabHelper>(*web_contents) {}

  ~DebounceTabHelper() override = default;

  DebounceTabHelper(const DebounceTabHelper&) = delete;
  DebounceTabHelper& operator=(const DebounceTabHelper&) = delete;

  void AddToRedirectChain(const GURL& url) {
    redirects_.push_back(std::string(url.host()));
  }
  void ClearRedirectChain() { redirects_.clear(); }
  bool IsInRedirectChain(const GURL& url) {
    return std::ranges::contains(redirects_, url.host());
  }

 private:
  friend class content::WebContentsUserData<DebounceTabHelper>;

  std::vector<std::string> redirects_;
  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

WEB_CONTENTS_USER_DATA_KEY_IMPL(DebounceTabHelper);

void ClearRedirectChain(NavigationHandle* navigation_handle) {
  if (!navigation_handle->IsInMainFrame() ||
      !navigation_handle->GetNavigationEntry() ||
      navigation_handle->GetNavigationEntry()->GetTransitionType() &
          ui::PAGE_TRANSITION_IS_REDIRECT_MASK) {
    return;
  }

  WebContents* web_contents = navigation_handle->GetWebContents();
  if (web_contents) {
    DebounceTabHelper::CreateForWebContents(web_contents);
    auto* debounce_helper = DebounceTabHelper::FromWebContents(web_contents);
    debounce_helper->ClearRedirectChain();
  }
}

}  // namespace

// static
void DebounceNavigationThrottle::MaybeCreateAndAdd(
    content::NavigationThrottleRegistry& registry,
    DebounceService* debounce_service) {
  // If debouncing is disabled in brave://flags, debounce service will
  // never be created (will be null) so we won't create the throttle
  // either. Caller must nullcheck this.
  if (!debounce_service) {
    return;
  }

  if (!debounce_service->IsEnabled()) {
    return;
  }

  registry.AddThrottle(std::make_unique<DebounceNavigationThrottle>(
      registry, *debounce_service));
}

DebounceNavigationThrottle::DebounceNavigationThrottle(
    content::NavigationThrottleRegistry& registry,
    DebounceService& debounce_service)
    : NavigationThrottle(registry), debounce_service_(debounce_service) {}

DebounceNavigationThrottle::~DebounceNavigationThrottle() = default;

NavigationThrottle::ThrottleCheckResult
DebounceNavigationThrottle::WillStartRequest() {
  ClearRedirectChain(navigation_handle());
  return MaybeRedirect();
}

NavigationThrottle::ThrottleCheckResult
DebounceNavigationThrottle::WillRedirectRequest() {
  return MaybeRedirect();
}

NavigationThrottle::ThrottleCheckResult
DebounceNavigationThrottle::MaybeRedirect() {
  WebContents* web_contents = navigation_handle()->GetWebContents();
  if (!web_contents || !navigation_handle()->IsInMainFrame()) {
    return NavigationThrottle::PROCEED;
  }

  GURL debounced_url;
  GURL original_url = navigation_handle()->GetURL();

  if (!debounce_service_->Debounce(original_url, &debounced_url)) {
    return NavigationThrottle::PROCEED;
  }

  auto* debounce_helper = DebounceTabHelper::FromWebContents(web_contents);
  if (debounce_helper && !debounce_helper->IsInRedirectChain(debounced_url)) {
    debounce_helper->AddToRedirectChain(debounced_url);
  } else {
    return NavigationThrottle::PROCEED;
  }

  VLOG(1) << "Debouncing rule applied: " << original_url << " -> "
          << debounced_url;

  if (brave_shields::RestartNavigationWithURL(
          *navigation_handle(), debounced_url,
          ui::PAGE_TRANSITION_CLIENT_REDIRECT)) {
    return NavigationThrottle::CANCEL;
  }
  return NavigationThrottle::PROCEED;
}

const char* DebounceNavigationThrottle::GetNameForLogging() {
  return "DebounceNavigationThrottle";
}

}  // namespace debounce
