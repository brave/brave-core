/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/policy/profile_policy_connector.h"

#include "brave/browser/policy/brave_origin_policy_provider.h"
#include "chrome/browser/browser_process.h"

#define BRAVE_PROFILE_POLICY_CONNECTOR_INIT                      \
  brave_origin_policy_provider_ =                                \
      std::make_unique<brave_origin::BraveOriginPolicyProvider>( \
          g_browser_process->local_state(),                      \
          g_browser_process->policy_service());                  \
  brave_origin_policy_provider_->Init(schema_registry);          \
  policy_providers_.push_back(brave_origin_policy_provider_.get());
#define BRAVE_PROFILE_POLICY_CONNECTOR_SHUTDOWN \
  if (brave_origin_policy_provider_) {          \
    brave_origin_policy_provider_->Shutdown();  \
  }

#include <chrome/browser/policy/profile_policy_connector.cc>  // IWYU pragma: export

#undef BRAVE_PROFILE_POLICY_CONNECTOR_INIT
#undef BRAVE_PROFILE_POLICY_CONNECTOR_SHUTDOWN
