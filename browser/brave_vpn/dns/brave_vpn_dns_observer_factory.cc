/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_vpn/dns/brave_vpn_dns_observer_factory.h"

#include <memory>
#include <string>
#include <unordered_map>

#include "brave/browser/brave_vpn/dns/brave_vpn_dns_observer_service.h"
#include "brave/components/brave_vpn/brave_vpn_utils.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/policy/chrome_browser_policy_connector.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/policy/core/common/cloud/cloud_policy_store.h"
#include "components/policy/core/common/cloud/machine_level_user_cloud_policy_manager.h"
#include "components/policy/core/common/cloud/machine_level_user_cloud_policy_store.h"
#include "components/policy/core/common/policy_map.h"

namespace brave_vpn {

namespace {
std::string GetPolicyValue(const std::string& name) {
  auto* service = g_browser_process->policy_service();
  if (!service)
    return std::string();
  auto ns =
      policy::PolicyNamespace(policy::POLICY_DOMAIN_CHROME, std::string());
  const base::Value* mode =
      service->GetPolicies(ns).GetValue(name, base::Value::Type::STRING);
  if (!mode || !mode->is_string())
    return std::string();
  return mode->GetString();
}

}  // namespace
// static
BraveVpnDnsObserverFactory* BraveVpnDnsObserverFactory::GetInstance() {
  return base::Singleton<BraveVpnDnsObserverFactory>::get();
}

BraveVpnDnsObserverFactory::~BraveVpnDnsObserverFactory() = default;

BraveVpnDnsObserverFactory::BraveVpnDnsObserverFactory()
    : BrowserContextKeyedServiceFactory(
          "BraveVpnDNSObserverService",
          BrowserContextDependencyManager::GetInstance()) {}

KeyedService* BraveVpnDnsObserverFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return new BraveVpnDnsObserverService(g_browser_process->local_state(),
                                        base::BindRepeating(&GetPolicyValue));
}

// static
BraveVpnDnsObserverService* BraveVpnDnsObserverFactory::GetServiceForContext(
    content::BrowserContext* context) {
  if (!IsBraveVPNEnabled())
    return nullptr;
  return static_cast<BraveVpnDnsObserverService*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

}  // namespace brave_vpn
