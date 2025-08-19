/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/files/file_path.h"
#include "brave/components/brave_origin/profile_id.h"
#include "chrome/browser/policy/profile_policy_connector.h"

#define ProfilePolicyConnector BraveProfilePolicyConnector
#define CreateProfilePolicyConnectorForBrowserContext \
  CreateBraveProfilePolicyConnectorForBrowserContext
#define CreateAndInitProfilePolicyConnector \
  CreateAndInitBraveProfilePolicyConnector

#include <chrome/browser/policy/profile_policy_connector_builder.cc>  // IWYU pragma: export

#undef ProfilePolicyConnector
#undef CreateProfilePolicyConnectorForBrowserContext
#undef CreateAndInitProfilePolicyConnector

namespace policy {

std::unique_ptr<ProfilePolicyConnector>
CreateProfilePolicyConnectorForBrowserContext(
    SchemaRegistry* schema_registry,
    CloudPolicyManager* cloud_policy_manager,
    ConfigurationPolicyProvider* policy_provider,
    policy::ChromeBrowserPolicyConnector* browser_policy_connector,
    bool force_immediate_load,
    content::BrowserContext* context) {
  Profile* const profile = Profile::FromBrowserContext(context);
  std::unique_ptr<BraveProfilePolicyConnector> connector =
      CreateBraveProfilePolicyConnectorForBrowserContext(
          schema_registry, cloud_policy_manager, policy_provider,
          browser_policy_connector, force_immediate_load, context);
  connector->GetBraveProfilePolicyProvider()->Initialize(
      brave_origin::GetProfileId(profile->GetBaseName().AsUTF8Unsafe()),
      schema_registry);
  return connector;
}

std::unique_ptr<ProfilePolicyConnector> CreateAndInitProfilePolicyConnector(
    SchemaRegistry* schema_registry,
    policy::ChromeBrowserPolicyConnector* browser_policy_connector,
    ConfigurationPolicyProvider* policy_provider,
    const CloudPolicyStore* policy_store,
    bool force_immediate_load,
    const user_manager::User* user) {
  return CreateAndInitBraveProfilePolicyConnector(
      schema_registry, browser_policy_connector, policy_provider, policy_store,
      force_immediate_load, user);
}

}  // namespace policy
