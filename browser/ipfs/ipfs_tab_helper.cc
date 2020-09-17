/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ipfs/ipfs_tab_helper.h"

#include <vector>

#include "brave/browser/brave_browser_process_impl.h"
#include "brave/browser/infobars/ipfs_infobar_delegate.h"
#include "brave/browser/ipfs/ipfs_service.h"
#include "brave/common/pref_names.h"
#include "brave/components/ipfs/common/ipfs_constants.h"
#include "brave/components/ipfs/common/ipfs_utils.h"
#include "chrome/browser/infobars/infobar_service.h"
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
    content::WebContents* web_contents) {
  if (!ipfs::IpfsService::IsIpfsEnabled(web_contents->GetBrowserContext()))
    return;

  CreateForWebContents(web_contents);
}

void IPFSTabHelper::UpdateActiveState(content::NavigationHandle* handle) {
  DCHECK(handle);
  DCHECK(handle->IsInMainFrame());
  active_ = true;
  auto resolve_method = static_cast<ipfs::IPFSResolveMethodTypes>(
      pref_service_->GetInteger(kIPFSResolveMethod));
  if (resolve_method == ipfs::IPFSResolveMethodTypes::IPFS_ASK &&
      IpfsUtils::IsIPFSURL(handle->GetURL())) {
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
