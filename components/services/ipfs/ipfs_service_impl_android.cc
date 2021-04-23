/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/services/ipfs/ipfs_service_impl.h"

#include "base/bind.h"
#include "base/threading/thread.h"

namespace ipfs {

IpfsServiceImpl::IpfsServiceImpl(
    mojo::PendingReceiver<mojom::IpfsService> receiver)
    : receiver_(this, std::move(receiver)) {
  receiver_.set_disconnect_handler(
      base::BindOnce(&IpfsServiceImpl::Cleanup, base::Unretained(this)));
}

IpfsServiceImpl::~IpfsServiceImpl() {}

void IpfsServiceImpl::Cleanup() {}

void IpfsServiceImpl::Launch(mojom::IpfsConfigPtr config,
                             LaunchCallback callback) {
  std::move(callback).Run(false, -1);
}

void IpfsServiceImpl::Shutdown() {}

void IpfsServiceImpl::SetCrashHandler(SetCrashHandlerCallback callback) {}

}  // namespace ipfs
