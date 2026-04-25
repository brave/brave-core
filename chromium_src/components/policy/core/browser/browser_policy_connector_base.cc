/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <components/policy/core/browser/browser_policy_connector_base.cc>

namespace policy {

std::vector<std::unique_ptr<ConfigurationPolicyProvider>>
BrowserPolicyConnectorBase::CreatePolicyProviders_ChromiumImpl() {
  return {};
}

}  // namespace policy
