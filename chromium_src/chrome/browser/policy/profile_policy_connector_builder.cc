/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/files/file_path.h"
#include "chrome/browser/policy/profile_policy_connector.h"

// This renames ProfilePolicyConnector to BraveProfilePolicyConnector
// so that the Brave subclass gets created instead of the Chromium one.
#define ProfilePolicyConnector BraveProfilePolicyConnector

// This renames away the original CreateProfilePolicyConnectorForBrowserContext
// because the ProfilePolicyConnector rename above changes the return type of
// CreateProfilePolicyConnectorForBrowserContext.
// We also need it because we need to call Init on the Brave profile policy
// provider.
// We provide a replacement with the original return type below.
#define CreateProfilePolicyConnectorForBrowserContext \
  CreateProfilePolicyConnectorForBrowserContext_ChromiumImpl

// This needed to be renamed away only because the #define above for
// ProfilePolicyConnector changes the return type of the original function.
// We define a new one with the original return type below.
#define CreateAndInitProfilePolicyConnector \
  CreateAndInitProfilePolicyConnector_ChromiumImpl

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
  std::unique_ptr<BraveProfilePolicyConnector> connector =
      CreateProfilePolicyConnectorForBrowserContext_ChromiumImpl(
          schema_registry, cloud_policy_manager, policy_provider,
          browser_policy_connector, force_immediate_load, context);
  connector->GetBraveProfilePolicyProvider()->Init(schema_registry);
  return connector;
}

std::unique_ptr<ProfilePolicyConnector> CreateAndInitProfilePolicyConnector(
    SchemaRegistry* schema_registry,
    policy::ChromeBrowserPolicyConnector* browser_policy_connector,
    ConfigurationPolicyProvider* policy_provider,
    const CloudPolicyStore* policy_store,
    bool force_immediate_load,
    const user_manager::User* user) {
  // Note: The CreateAndInitProfilePolicyConnector_ChromiumImpl creates a
  // BraveProfilePolicyConnector but it is wrapped here to return the original
  // needed type.
  return CreateAndInitProfilePolicyConnector_ChromiumImpl(
      schema_registry, browser_policy_connector, policy_provider, policy_store,
      force_immediate_load, user);
}

}  // namespace policy
