/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ipfs/ipfs_service_impl_delegate.h"

#include <memory>


namespace ipfs {

IpfsServiceImplDelegate::IpfsServiceImplDelegate(
    PrefService* local_state)
    : local_state_(local_state)
{
}

IpfsServiceImplDelegate::~IpfsServiceImplDelegate() = default;

void IpfsServiceImplDelegate::OnImportToIpfsFinished(
    IpfsService* ipfs_service) {
}

}  // namespace ipfs
