/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ipfs/ipfs_tab_helper.h"

#include "brave/browser/brave_browser_process_impl.h"
#include "brave/browser/infobars/ipfs_infobar_delegate.h"
#include "chrome/browser/infobars/infobar_service.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/navigation_handle.h"
#include "extensions/common/url_pattern.h"

namespace {
bool IsIPFSURL(const GURL& gurl) {
  // Temporary to manually test the infobar
  static std::vector<URLPattern> updater_patterns({
      URLPattern(URLPattern::SCHEME_ALL, "https://brianbondy.com/*"),
      URLPattern(URLPattern::SCHEME_ALL, "https://brave.com/*")
  });
  return std::any_of(
      updater_patterns.begin(), updater_patterns.end(),
      [&gurl](URLPattern pattern) { return pattern.MatchesURL(gurl); });
}
}

namespace ipfs {

IPFSTabHelper::~IPFSTabHelper() = default;

IPFSTabHelper::IPFSTabHelper(content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents) {}

void IPFSTabHelper::UpdateActiveState(content::NavigationHandle* handle) {
  DCHECK(handle);
  DCHECK(handle->IsInMainFrame());
  active_ = true;
  if (IsIPFSURL(handle->GetURL())) {
    InfoBarService* infobar_service =
        InfoBarService::FromWebContents(web_contents());
    if (infobar_service) {
      IPFSInfoBarDelegate::Create(infobar_service);
    }
  }
}

void IPFSTabHelper::DidStartNavigation(
    content::NavigationHandle* navigation_handle) {
  if (navigation_handle->IsInMainFrame()) {
    UpdateActiveState(navigation_handle);
  }
}

void IPFSTabHelper::DidRedirectNavigation(
    content::NavigationHandle* navigation_handle) {
  if (navigation_handle->IsInMainFrame()) {
    UpdateActiveState(navigation_handle);
  }
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(IPFSTabHelper)

}  // namespace ipfs
