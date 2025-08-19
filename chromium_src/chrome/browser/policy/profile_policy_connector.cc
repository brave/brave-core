/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/policy/profile_policy_connector.h"

#include <algorithm>

#include "brave/components/brave_origin/policy/brave_origin_policy_provider.h"
#include "chrome/browser/browser_process.h"

#include <chrome/browser/policy/profile_policy_connector.cc>  // IWYU pragma: export

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
  auto provider = std::make_unique<brave_origin::BraveOriginPolicyProvider>(
      g_browser_process->local_state());
  // Store raw pointer for internal use.
  brave_origin_policy_provider_ = provider.get();
  policy_providers_.push_back(provider.get());
  wrapped_policy_providers_.push_back(std::move(provider));
  ProfilePolicyConnector::Init(user, schema_registry,
                               configuration_policy_provider, policy_store,
                               browser_policy_connector, force_immediate_load);
  brave_origin_policy_provider_->SetPolicyService(policy_service_.get());
  brave_origin_policy_provider_->Init(schema_registry);
}

void BraveProfilePolicyConnector::Shutdown() {
  brave_origin_policy_provider_->Shutdown();
  ProfilePolicyConnector::Shutdown();
}

}  // namespace policy

#undef BRAVE_PROFILE_POLICY_CONNECTOR_INIT
#undef BRAVE_PROFILE_POLICY_CONNECTOR_SHUTDOWN
