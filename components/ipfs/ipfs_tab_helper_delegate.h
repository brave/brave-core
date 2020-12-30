/* Copyright (c) 2020 The Brave Authors. All rights reserved.
+ * This Source Code Form is subject to the terms of the Mozilla Public
+ * License, v. 2.0. If a copy of the MPL was not distributed with this file,
+ * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_IPFS_IPFS_TAB_HELPER_DELEGATE_H_
#define BRAVE_COMPONENTS_IPFS_IPFS_TAB_HELPER_DELEGATE_H_

namespace content {
class WebContents;
}

namespace ipfs {

class IpfsTabHelperDelegate {
 public:
  IpfsTabHelperDelegate() = default;
  virtual ~IpfsTabHelperDelegate() = default;
  virtual void CreateInfoBarDelegateForWebContents(content::WebContents*) = 0;
};

}  // namespace ipfs

#endif  // BRAVE_COMPONENTS_IPFS_IPFS_TAB_HELPER_DELEGATE_H_
