/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/policy/configuration_policy_handler_list_factory.h"

#include "brave/browser/policy/brave_simple_policy_map.h"
#include "brave/browser/policy/handlers/brave_adblock_policy_handler.h"
#include "brave/browser/policy/handlers/brave_fingerprinting_v2_policy_handler.h"
#include "brave/browser/policy/handlers/brave_https_upgrade_policy_handler.h"
#include "brave/browser/policy/handlers/brave_referrers_policy_handler.h"
#include "brave/browser/policy/handlers/brave_remember_1p_storage_policy_handler.h"
#include "components/policy/core/browser/configuration_policy_handler.h"

#define BuildHandlerList BuildHandlerList_ChromiumImpl
#include <chrome/browser/policy/configuration_policy_handler_list_factory.cc>
#undef BuildHandlerList

namespace policy {

std::unique_ptr<ConfigurationPolicyHandlerList> BuildHandlerList(
    const Schema& chrome_schema) {
  std::unique_ptr<ConfigurationPolicyHandlerList> handlers =
      BuildHandlerList_ChromiumImpl(chrome_schema);

  for (const auto& entry : kBraveSimplePolicyMap) {
    handlers->AddHandler(std::make_unique<SimplePolicyHandler>(
        entry.policy_name, entry.preference_path, entry.value_type));
  }

  handlers->AddHandler(std::make_unique<BraveAdblockPolicyHandler>());
  handlers->AddHandler(std::make_unique<BraveFingerprintingV2PolicyHandler>());
  handlers->AddHandler(std::make_unique<BraveHttpsUpgradePolicyHandler>());
  handlers->AddHandler(std::make_unique<BraveReferrersPolicyHandler>());
  handlers->AddHandler(std::make_unique<BraveRemember1PStoragePolicyHandler>());

  return handlers;
}

}  // namespace policy
