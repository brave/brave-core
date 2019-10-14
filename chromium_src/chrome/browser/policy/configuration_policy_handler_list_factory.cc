/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/stl_util.h"
#include "brave/browser/tor/buildflags.h"
#include "brave/common/pref_names.h"
#include "chrome/browser/policy/configuration_policy_handler_list_factory.h"
#include "components/policy/core/browser/configuration_policy_handler.h"
#include "components/policy/policy_constants.h"

namespace {

const policy::PolicyToPreferenceMapEntry kBraveSimplePolicyMap[] = {
#if BUILDFLAG(ENABLE_TOR)
  { policy::key::kTorDisabled,
    kTorDisabled,
    base::Value::Type::BOOLEAN },
#endif
};

}  // namespace

#define BuildHandlerList BuildHandlerList_ChromiumImpl
#include "../../../../../chrome/browser/policy/configuration_policy_handler_list_factory.cc"  // NOLINT
#undef BuildHandlerList

namespace policy {

std::unique_ptr<ConfigurationPolicyHandlerList> BuildHandlerList(
    const Schema& chrome_schema) {
  std::unique_ptr<ConfigurationPolicyHandlerList> handlers =
      BuildHandlerList_ChromiumImpl(chrome_schema);

  for (size_t i = 0; i < base::size(kBraveSimplePolicyMap); ++i) {
    handlers->AddHandler(std::make_unique<SimplePolicyHandler>(
        kBraveSimplePolicyMap[i].policy_name,
        kBraveSimplePolicyMap[i].preference_path,
        kBraveSimplePolicyMap[i].value_type));
  }
  return handlers;
}

}  // namespace policy
