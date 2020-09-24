/* Copyright (c) 2020 The Brave Authors. All rights reserved.
+ * This Source Code Form is subject to the terms of the Mozilla Public
+ * License, v. 2.0. If a copy of the MPL was not distributed with this file,
+ * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_IPFS_BROWSER_IPFS_SERVICE_DELEGATE_H_
#define BRAVE_COMPONENTS_IPFS_BROWSER_IPFS_SERVICE_DELEGATE_H_

#include "base/files/file_path.h"

namespace content {
class BrowserContext;
}  // namespace content

namespace ipfs {

class IpfsServiceDelegate {
 public:
  explicit IpfsServiceDelegate(content::BrowserContext* context) {
    context_ = context;
  }
  virtual ~IpfsServiceDelegate() = default;
  virtual base::FilePath GetUserDataDir() = 0;
  virtual bool IsTestingProfile() = 0;

 protected:
  content::BrowserContext* context_;
};

}  // namespace ipfs

#endif  // BRAVE_COMPONENTS_IPFS_BROWSER_IPFS_SERVICE_DELEGATE_H_
