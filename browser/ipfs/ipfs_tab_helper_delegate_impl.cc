/* Copyright (c) 2020 The Brave Authors. All rights reserved.
+ * This Source Code Form is subject to the terms of the Mozilla Public
+ * License, v. 2.0. If a copy of the MPL was not distributed with this file,
+ * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ipfs/ipfs_tab_helper_delegate_impl.h"

#include "brave/browser/infobars/ipfs_infobar_delegate.h"
#include "chrome/browser/infobars/infobar_service.h"

namespace ipfs {

void IpfsTabHelperDelegateImpl::CreateInfoBarDelegateForWebContents(
    content::WebContents* web_contents) {
  InfoBarService* infobar_service =
      InfoBarService::FromWebContents(web_contents);
  if (infobar_service) {
    IPFSInfoBarDelegate::Create(infobar_service);
  }
}

}  // namespace ipfs
