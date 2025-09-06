/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/policy/profile_policy_connector.h"

#include <chrome/browser/policy/profile_policy_connector.cc>  // IWYU pragma: export

namespace brave_policy {
std::unique_ptr<policy::ConfigurationPolicyProvider>
CreateBraveProfilePolicyProvider();
}  // namespace brave_policy

namespace policy {

BraveProfilePolicyConnector::BraveProfilePolicyConnector() {}

BraveProfilePolicyConnector::~BraveProfilePolicyConnector() {}

void BraveProfilePolicyConnector::Init(
    const user_manager::User* user,
    SchemaRegistry* schema_registry,
    ConfigurationPolicyProvider* configuration_policy_provider,
    const CloudPolicyStore* policy_store,
    policy::ChromeBrowserPolicyConnector* browser_policy_connector,
    bool force_immediate_load) {
  auto provider = brave_policy::CreateBraveProfilePolicyProvider();
  // Store raw pointer for internal use.
  brave_profile_policy_provider_ = provider.get();
  policy_providers_.push_back(provider.get());
  wrapped_policy_providers_.push_back(std::move(provider));
  ProfilePolicyConnector::Init(user, schema_registry,
                               configuration_policy_provider, policy_store,
                               browser_policy_connector, force_immediate_load);
  // Init will be called on brave_profile_policy_provider_ in the
  // chromium_src profile_policy_connector_builder.cc override for
  // CreateProfilePolicyConnectorForBrowserContext because we have the profile
  // there which we'll get and pass in the profile_id in later work.
}

raw_ptr<policy::ConfigurationPolicyProvider>
BraveProfilePolicyConnector::GetBraveProfilePolicyProvider() {
  return brave_profile_policy_provider_;
}

const std::vector<raw_ptr<ConfigurationPolicyProvider, VectorExperimental>>&
BraveProfilePolicyConnector::GetPolicyProviders() const {
  return policy_providers_;
}

void BraveProfilePolicyConnector::Shutdown() {
  brave_profile_policy_provider_->Shutdown();
  ProfilePolicyConnector::Shutdown();
  // Clear raw pointer after shutdown to prevent dangling pointer
  brave_profile_policy_provider_ = nullptr;
}

}  // namespace policy
