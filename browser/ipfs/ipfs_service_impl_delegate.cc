/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ipfs/ipfs_service_impl_delegate.h"

#include <memory>

#include "brave/browser/infobars/brave_ipfs_always_start_infobar_delegate.h"
#if !BUILDFLAG(IS_ANDROID)
#include "brave/browser/infobars/brave_global_infobar_service.h"
#endif  // !BUILDFLAG(IS_ANDROID)

namespace ipfs {

IpfsServiceImplDelegate::IpfsServiceImplDelegate(
    PrefService* local_state
#if !BUILDFLAG(IS_ANDROID)
    ,
    BraveGlobalInfobarService* global_infobar_service
#endif  // !BUILDFLAG(IS_ANDROID)
    )
    : local_state_(local_state)
#if !BUILDFLAG(IS_ANDROID)
      ,
      global_infobar_service_(global_infobar_service)
#endif  // !BUILDFLAG(IS_ANDROID)
{
}

IpfsServiceImplDelegate::~IpfsServiceImplDelegate() = default;

void IpfsServiceImplDelegate::OnImportToIpfsFinished(
    IpfsService* ipfs_service) {
#if !BUILDFLAG(IS_ANDROID)
  if (global_infobar_service_) {
    global_infobar_service_->ShowAlwaysStartInfobar();
  }
#endif  // !BUILDFLAG(IS_ANDROID)
}

}  // namespace ipfs
