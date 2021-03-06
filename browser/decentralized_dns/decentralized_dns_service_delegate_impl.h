/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_DECENTRALIZED_DNS_DECENTRALIZED_DNS_SERVICE_DELEGATE_IMPL_H_
#define BRAVE_BROWSER_DECENTRALIZED_DNS_DECENTRALIZED_DNS_SERVICE_DELEGATE_IMPL_H_

#include "brave/components/decentralized_dns/decentralized_dns_service_delegate.h"

namespace decentralized_dns {

class DecentralizedDnsServiceDelegateImpl
    : public DecentralizedDnsServiceDelegate {
 public:
  DecentralizedDnsServiceDelegateImpl() = default;
  ~DecentralizedDnsServiceDelegateImpl() override = default;

  DecentralizedDnsServiceDelegateImpl(
      const DecentralizedDnsServiceDelegateImpl&) = delete;
  DecentralizedDnsServiceDelegateImpl& operator=(
      DecentralizedDnsServiceDelegateImpl&) = delete;

  void UpdateNetworkService() override;
};

}  // namespace decentralized_dns

#endif  // BRAVE_BROWSER_DECENTRALIZED_DNS_DECENTRALIZED_DNS_SERVICE_DELEGATE_IMPL_H_
