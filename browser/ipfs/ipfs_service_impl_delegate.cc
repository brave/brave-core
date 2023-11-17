/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ipfs/ipfs_service_impl_delegate.h"

#include <memory>

#include "brave/browser/infobars/brave_ipfs_always_start_infobar_delegate.h"
#if !BUILDFLAG(IS_ANDROID)
#include "brave/browser/ui/views/infobars/brave_global_infobar_manager.h"
#endif  // !BUILDFLAG(IS_ANDROID)

namespace ipfs {

IpfsServiceImplDelegate::IpfsServiceImplDelegate(PrefService* local_state)
    : local_state_(local_state)
#if !BUILDFLAG(IS_ANDROID)
      ,
      allways_start_global_infobar_(std::make_unique<BraveGlobalInfoBarManager>(
          std::make_unique<BraveIPFSAlwaysStartInfoBarDelegateFactory>(
              local_state)))
#endif  // !BUILDFLAG(IS_ANDROID)
{
}

IpfsServiceImplDelegate::~IpfsServiceImplDelegate() = default;

void IpfsServiceImplDelegate::OnImportToIpfsFinished(
    IpfsService* ipfs_service) {
#if !BUILDFLAG(IS_ANDROID)
  allways_start_global_infobar_->Show();
#endif  // !BUILDFLAG(IS_ANDROID)
}

}  // namespace ipfs
