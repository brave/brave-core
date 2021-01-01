/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ipfs/ipfs_tab_helper.h"

#include <vector>

#include "brave/browser/infobars/ipfs_infobar_delegate.h"
#include "brave/browser/ipfs/ipfs_service_factory.h"
#include "brave/components/ipfs/ipfs_constants.h"
#include "brave/components/ipfs/ipfs_service.h"
#include "brave/components/ipfs/ipfs_utils.h"
#include "brave/components/ipfs/pref_names.h"
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
bool IPFSTabHelper::MaybeCreateForWebContents(
    content::WebContents* web_contents) {
  if (!ipfs::IpfsServiceFactory::GetForContext(
          web_contents->GetBrowserContext())) {
    return false;
  }

  CreateForWebContents(web_contents);
  return true;
}

void IPFSTabHelper::DidFinishNavigation(content::NavigationHandle* handle) {
  DCHECK(handle);
  if (!handle->IsInMainFrame()) {
    return;
  }

  auto resolve_method = static_cast<ipfs::IPFSResolveMethodTypes>(
      pref_service_->GetInteger(kIPFSResolveMethod));
  if (resolve_method == ipfs::IPFSResolveMethodTypes::IPFS_ASK &&
      handle->GetResponseHeaders() &&
      handle->GetResponseHeaders()->HasHeader("x-ipfs-path") &&
      IsDefaultGatewayURL(handle->GetURL(),
                          web_contents()->GetBrowserContext())) {
    InfoBarService* infobar_service =
        InfoBarService::FromWebContents(web_contents());
    if (infobar_service) {
      auto* browser_context = web_contents()->GetBrowserContext();
      IPFSInfoBarDelegate::Create(infobar_service, browser_context);
    }
  }
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(IPFSTabHelper)

}  // namespace ipfs
