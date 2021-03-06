/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_DECENTRALIZED_DNS_DECENTRALIZED_DNS_SERVICE_DELEGATE_H_
#define BRAVE_COMPONENTS_DECENTRALIZED_DNS_DECENTRALIZED_DNS_SERVICE_DELEGATE_H_

namespace decentralized_dns {

class DecentralizedDnsServiceDelegate {
 public:
  DecentralizedDnsServiceDelegate() = default;
  virtual ~DecentralizedDnsServiceDelegate() = default;
  virtual void UpdateNetworkService() = 0;
};

}  // namespace decentralized_dns

#endif  // BRAVE_COMPONENTS_DECENTRALIZED_DNS_DECENTRALIZED_DNS_SERVICE_DELEGATE_H_
