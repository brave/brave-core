/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "ios/chrome/browser/policy/model/configuration_policy_handler_list_factory.h"

#include <memory>

#include "brave/ios/browser/policy/brave_simple_policy_map_ios.h"
#include "components/policy/core/browser/configuration_policy_handler.h"

#define BuildPolicyHandlerList BuildPolicyHandlerList_ChromiumImpl
#include <ios/chrome/browser/policy/model/configuration_policy_handler_list_factory.mm>
#undef BuildPolicyHandlerList

std::unique_ptr<policy::ConfigurationPolicyHandlerList> BuildPolicyHandlerList(
    bool are_future_policies_allowed_by_default,
    const policy::Schema& chrome_schema) {
  std::unique_ptr<policy::ConfigurationPolicyHandlerList> handlers =
      BuildPolicyHandlerList_ChromiumImpl(
          are_future_policies_allowed_by_default, chrome_schema);

  for (const auto& entry : policy::kBraveSimplePolicyMap) {
    handlers->AddHandler(std::make_unique<SimplePolicyHandler>(
        entry.policy_name, entry.preference_path, entry.value_type));
  }

  return handlers;
}
