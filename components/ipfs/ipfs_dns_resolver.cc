/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ipfs/ipfs_dns_resolver.h"

namespace ipfs {

IpfsDnsResolver::IpfsDnsResolver() = default;

IpfsDnsResolver::~IpfsDnsResolver() = default;

void IpfsDnsResolver::Notify(absl::optional<std::string> value) {
  observers_.Notify(value);
}

base::CallbackListSubscription IpfsDnsResolver::AddObserver(
    IpfsDnsResolverObserver observer) {
  return observers_.Add(observer);
}

}  // namespace ipfs
