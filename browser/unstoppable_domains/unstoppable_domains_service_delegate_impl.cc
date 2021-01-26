/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/unstoppable_domains/unstoppable_domains_service_delegate_impl.h"

#include "chrome/browser/net/stub_resolver_config_reader.h"
#include "chrome/browser/net/system_network_context_manager.h"

namespace unstoppable_domains {

void UnstoppableDomainsServiceDelegateImpl::UpdateNetworkService() {
  SystemNetworkContextManager::GetStubResolverConfigReader()
      ->UpdateNetworkService(false /* record_metrics */);
}

}  // namespace unstoppable_domains
