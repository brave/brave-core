/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UNSTOPPABLE_DOMAINS_UNSTOPPABLE_DOMAINS_SERVICE_DELEGATE_IMPL_H_
#define BRAVE_BROWSER_UNSTOPPABLE_DOMAINS_UNSTOPPABLE_DOMAINS_SERVICE_DELEGATE_IMPL_H_

#include "brave/components/unstoppable_domains/unstoppable_domains_service_delegate.h"

namespace unstoppable_domains {

class UnstoppableDomainsServiceDelegateImpl
    : public UnstoppableDomainsServiceDelegate {
 public:
  UnstoppableDomainsServiceDelegateImpl() = default;
  ~UnstoppableDomainsServiceDelegateImpl() override = default;

  UnstoppableDomainsServiceDelegateImpl(
      const UnstoppableDomainsServiceDelegateImpl&) = delete;
  UnstoppableDomainsServiceDelegateImpl& operator=(
      UnstoppableDomainsServiceDelegateImpl&) = delete;

  void UpdateNetworkService() override;
};

}  // namespace unstoppable_domains

#endif  // BRAVE_BROWSER_UNSTOPPABLE_DOMAINS_UNSTOPPABLE_DOMAINS_SERVICE_DELEGATE_IMPL_H_
