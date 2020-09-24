/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_IPFS_IPFS_SERVICE_DELEGATE_IMPL_H_
#define BRAVE_BROWSER_IPFS_IPFS_SERVICE_DELEGATE_IMPL_H_

#include "brave/components/ipfs/browser/ipfs_service_delegate.h"

namespace ipfs {

class IpfsServiceDelegateImpl : public IpfsServiceDelegate {
 public:
  explicit IpfsServiceDelegateImpl(content::BrowserContext* context);
  ~IpfsServiceDelegateImpl() override;

  base::FilePath GetUserDataDir() override;
  bool IsTestingProfile() override;
};

}  // namespace ipfs

#endif  // BRAVE_BROWSER_IPFS_IPFS_SERVICE_DELEGATE_IMPL_H_
