/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_IPFS_IPFS_DNS_RESOLVER_H_
#define BRAVE_COMPONENTS_IPFS_IPFS_DNS_RESOLVER_H_

#include <string>

#include "base/callback_list.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace ipfs {

class IpfsDnsResolver {
 public:
  IpfsDnsResolver();
  virtual ~IpfsDnsResolver();
  IpfsDnsResolver(const IpfsDnsResolver&) = delete;
  IpfsDnsResolver& operator=(const IpfsDnsResolver&) = delete;

  virtual absl::optional<std::string> GetFirstDnsOverHttpsServer() = 0;

  using IpfsDnsResolverObserverList =
      base::RepeatingCallbackList<void(absl::optional<std::string>)>;
  using IpfsDnsResolverObserver = IpfsDnsResolverObserverList::CallbackType;

  base::CallbackListSubscription AddObserver(IpfsDnsResolverObserver observer);

 protected:
  void Notify(absl::optional<std::string> value);

 private:
  IpfsDnsResolverObserverList observers_;
};

}  // namespace ipfs

#endif  // BRAVE_COMPONENTS_IPFS_IPFS_DNS_RESOLVER_H_
