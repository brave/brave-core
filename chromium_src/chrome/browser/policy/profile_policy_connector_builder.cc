/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/files/file_path.h"
#include "chrome/browser/policy/profile_policy_connector.h"

#define CreateProfilePolicyConnectorForBrowserContext \
  CreateProfilePolicyConnectorForBrowserContext_ChromiumImpl

#include <chrome/browser/policy/profile_policy_connector_builder.cc>  // IWYU pragma: export

#undef CreateProfilePolicyConnectorForBrowserContext

namespace brave_policy {
class BraveProfilePolicyProvider;
void SetBraveProfilePolicyProviderProfileID(
    policy::ConfigurationPolicyProvider* provider,
    const base::FilePath& profile_path);
}  // namespace brave_policy

namespace policy {

std::unique_ptr<ProfilePolicyConnector>
CreateProfilePolicyConnectorForBrowserContext(
    SchemaRegistry* schema_registry,
    CloudPolicyManager* cloud_policy_manager,
    ConfigurationPolicyProvider* policy_provider,
    policy::ChromeBrowserPolicyConnector* browser_policy_connector,
    bool force_immediate_load,
    content::BrowserContext* context) {
  auto connector = CreateProfilePolicyConnectorForBrowserContext_ChromiumImpl(
      schema_registry, cloud_policy_manager, policy_provider,
      browser_policy_connector, force_immediate_load, context);
  // Some upstream browser tests don't do the normal flow so have no provider
  if (connector->GetBraveProfilePolicyProvider()) {
    brave_policy::SetBraveProfilePolicyProviderProfileID(
        connector->GetBraveProfilePolicyProvider().get(), context->GetPath());
  }

  return connector;
}

}  // namespace policy
