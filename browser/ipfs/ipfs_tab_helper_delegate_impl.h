/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_IPFS_IPFS_TAB_HELPER_DELEGATE_IMPL_H_
#define BRAVE_BROWSER_IPFS_IPFS_TAB_HELPER_DELEGATE_IMPL_H_

#include "brave/components/ipfs/browser/ipfs_tab_helper_delegate.h"

namespace ipfs {

class IpfsTabHelperDelegateImpl : public IpfsTabHelperDelegate {
 public:
  IpfsTabHelperDelegateImpl() = default;
  ~IpfsTabHelperDelegateImpl() override = default;

  void CreateInfoBarDelegateForWebContents(content::WebContents*) override;
};

}  // namespace ipfs

#endif  // BRAVE_BROWSER_IPFS_IPFS_TAB_HELPER_DELEGATE_IMPL_H_
