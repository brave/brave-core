/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_IPFS_IPFS_SERVICE_DELEGATE_H_
#define BRAVE_COMPONENTS_IPFS_IPFS_SERVICE_DELEGATE_H_

namespace ipfs {

class IpfsService;

class IpfsServiceDelegate {
 public:
  virtual ~IpfsServiceDelegate() = default;
  virtual void OnImportToIpfsFinished(IpfsService* ipfs_service) = 0;
};

}  // namespace ipfs

#endif  // BRAVE_COMPONENTS_IPFS_IPFS_SERVICE_DELEGATE_H_
