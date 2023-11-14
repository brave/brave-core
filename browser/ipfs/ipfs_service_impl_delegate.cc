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
    : local_state_(local_state) {}

IpfsServiceImplDelegate::~IpfsServiceImplDelegate() = default;

void IpfsServiceImplDelegate::ShowAlwaysStartInfoBar(
    IpfsService* ipfs_service) {
#if !BUILDFLAG(IS_ANDROID)
  BraveGlobalInfoBarManager::Show(
      std::make_unique<BraveIPFSAlwaysStartInfoBarDelegateFactory>(
          ipfs_service, local_state_));
#endif  // !BUILDFLAG(IS_ANDROID)
}

}  // namespace ipfs
