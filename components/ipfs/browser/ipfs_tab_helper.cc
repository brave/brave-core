/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ipfs/browser/ipfs_tab_helper.h"

#include <vector>

#include "brave/components/ipfs/browser/ipfs_service.h"
#include "brave/components/ipfs/browser/ipfs_tab_helper_delegate.h"
#include "brave/components/ipfs/common/ipfs_constants.h"
#include "brave/components/ipfs/common/ipfs_utils.h"
#include "brave/components/ipfs/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_handle.h"

namespace ipfs {

IPFSTabHelper::~IPFSTabHelper() = default;

IPFSTabHelper::IPFSTabHelper(content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents) {
  pref_service_ = user_prefs::UserPrefs::Get(web_contents->GetBrowserContext());
}

// static
void IPFSTabHelper::MaybeCreateForWebContents(
    content::WebContents* web_contents, bool regular_profile,
    IpfsTabHelperDelegate* delegate) {
  auto* browser_context = web_contents->GetBrowserContext();
  if (!ipfs::IpfsService::IsIpfsEnabled(browser_context, regular_profile)) {
    return;
  }

  CreateForWebContents(web_contents);
  auto* ipfs_tab_helper = FromWebContents(web_contents);
  ipfs_tab_helper->set_delegate(delegate);
}

void IPFSTabHelper::UpdateActiveState(content::NavigationHandle* handle) {
  DCHECK(handle);
  DCHECK(handle->IsInMainFrame());
  active_ = true;
  auto resolve_method = static_cast<ipfs::IPFSResolveMethodTypes>(
      pref_service_->GetInteger(kIPFSResolveMethod));
  if (resolve_method == ipfs::IPFSResolveMethodTypes::IPFS_ASK &&
      IpfsUtils::IsIPFSURL(handle->GetURL())) {
    if (delegate_) {
      delegate_->CreateInfoBarDelegateForWebContents(web_contents());
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

void IPFSTabHelper::set_delegate(IpfsTabHelperDelegate* delegate) {
  delegate_.reset(delegate);
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(IPFSTabHelper)

}  // namespace ipfs
