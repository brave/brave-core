/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_POLICY_PROFILE_POLICY_CONNECTOR_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_POLICY_PROFILE_POLICY_CONNECTOR_H_

#include "base/timer/timer.h"
#include "brave/components/brave_origin/policy/brave_origin_policy_provider.h"

// Add virtual to Init and Shutdown
#define Init(user, schema_registry, configuration_policy_provider,         \
             policy_store, browser_policy_connector, force_immediate_load) \
  virtual Init(user, schema_registry, configuration_policy_provider,       \
               policy_store, browser_policy_connector, force_immediate_load)
#define Shutdown()    \
  virtual Shutdown(); \
  friend class BraveProfilePolicyConnector
#define final

#include <chrome/browser/policy/profile_policy_connector.h>  // IWYU pragma: export

#undef final
#undef Init
#undef Shutdown

namespace policy {

class BraveProfilePolicyConnector : public ProfilePolicyConnector {
 public:
  BraveProfilePolicyConnector();
  ~BraveProfilePolicyConnector() override;

  void Init(const user_manager::User* user,
            SchemaRegistry* schema_registry,
            ConfigurationPolicyProvider* configuration_policy_provider,
            const CloudPolicyStore* policy_store,
            policy::ChromeBrowserPolicyConnector* browser_policy_connector,
            bool force_immediate_load) override;
  void Shutdown() override;

 private:
  // std::unique_ptr<brave_origin::BraveOriginPolicyProvider>
  //   brave_origin_policy_provider_;
  raw_ptr<brave_origin::BraveOriginPolicyProvider>
      brave_origin_policy_provider_;
};

}  // namespace policy

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_POLICY_PROFILE_POLICY_CONNECTOR_H_
