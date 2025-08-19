/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_POLICY_PROFILE_POLICY_CONNECTOR_BUILDER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_POLICY_PROFILE_POLICY_CONNECTOR_BUILDER_H_

#include "chrome/browser/policy/profile_policy_connector.h"
// #include "chrome/browser/policy/profile_policy_connector_builder.h"

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
  return CreateBraveProfilePolicyConnectorForBrowserContext(
      schema_registry, cloud_policy_manager, policy_provider,
      browser_policy_connector, force_immediate_load, context);
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

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_POLICY_PROFILE_POLICY_CONNECTOR_BUILDER_H_
