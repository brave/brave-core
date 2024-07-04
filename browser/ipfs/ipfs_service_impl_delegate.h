/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_IPFS_IPFS_SERVICE_IMPL_DELEGATE_H_
#define BRAVE_BROWSER_IPFS_IPFS_SERVICE_IMPL_DELEGATE_H_

#include "base/memory/raw_ptr.h"
#include "brave/components/ipfs/ipfs_service_delegate.h"
#include "build/build_config.h"

class PrefService;
#if !BUILDFLAG(IS_ANDROID)
class BraveGlobalInfobarService;
#endif  // !BUILDFLAG(IS_ANDROID)

namespace ipfs {

class IpfsServiceImplDelegate : public IpfsServiceDelegate {
 public:
  explicit IpfsServiceImplDelegate(
      PrefService* local_state
#if !BUILDFLAG(IS_ANDROID)
      ,
      BraveGlobalInfobarService* global_infobar_service
#endif  // !BUILDFLAG(IS_ANDROID)
  );
  ~IpfsServiceImplDelegate() override;
  void OnImportToIpfsFinished(IpfsService* ipfs_service) override;

 private:
  raw_ptr<PrefService> local_state_ = nullptr;
#if !BUILDFLAG(IS_ANDROID)
  raw_ptr<BraveGlobalInfobarService> global_infobar_service_;
#endif  // !BUILDFLAG(IS_ANDROID)
};

}  // namespace ipfs

#endif  // BRAVE_BROWSER_IPFS_IPFS_SERVICE_IMPL_DELEGATE_H_
