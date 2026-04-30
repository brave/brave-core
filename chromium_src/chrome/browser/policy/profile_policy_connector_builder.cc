/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/files/file_path.h"
#include "brave/components/brave_policy/brave_profile_policy_provider.h"
#include "chrome/browser/policy/profile_policy_connector.h"

#define CreateProfilePolicyConnectorForBrowserContext \
  CreateProfilePolicyConnectorForBrowserContext_ChromiumImpl

#include <chrome/browser/policy/profile_policy_connector_builder.cc>  // IWYU pragma: export

#undef CreateProfilePolicyConnectorForBrowserContext

namespace brave_policy {
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
  // Stash the profile path so that BRAVE_PROFILE_POLICY_CONNECTOR_INIT can
  // call SetProfileID on the BraveProfilePolicyProvider before its Init() runs
  // — which causes the provider's bundle to be populated synchronously,
  // before PolicyServiceImpl's constructor performs its synchronous merge.
  // See chromium_src/chrome/browser/policy/profile_policy_connector.cc for
  // the full rationale.
  brave_policy::BraveProfilePolicyProvider::SetPendingProfilePath(
      context->GetPath());
  auto connector = CreateProfilePolicyConnectorForBrowserContext_ChromiumImpl(
      schema_registry, cloud_policy_manager, policy_provider,
      browser_policy_connector, force_immediate_load, context);
  // Defensive clear: the macro normally consumes the stash, but if upstream
  // changes ever cause the macro not to fire, don't leak the path into the
  // next profile's connector creation.
  brave_policy::BraveProfilePolicyProvider::TakePendingProfilePath();
  // Some upstream browser tests don't do the normal flow so have no provider.
  // Also, the macro-driven SetProfileID above is the load-bearing path; this
  // call is now an idempotent fallback for any flow we might have missed.
  if (connector->GetBraveProfilePolicyProvider()) {
    brave_policy::SetBraveProfilePolicyProviderProfileID(
        connector->GetBraveProfilePolicyProvider().get(), context->GetPath());
  }

  return connector;
}

}  // namespace policy
