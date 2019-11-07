/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/stl_util.h"
#include "brave/browser/tor/buildflags.h"
#include "brave/common/pref_names.h"
#include "brave/common/tor/pref_names.h"
#include "chrome/browser/policy/configuration_policy_handler_list_factory.h"
#include "components/policy/core/browser/configuration_policy_handler.h"
#include "components/policy/policy_constants.h"

namespace {

// Wrap whole array definition in TOR to avoid unused varilable build error.
// It can happen if platform doesn't support tor.
#if BUILDFLAG(ENABLE_TOR)
const policy::PolicyToPreferenceMapEntry kBraveSimplePolicyMap[] = {
  { policy::key::kTorDisabled,
    tor::prefs::kTorDisabled,
    base::Value::Type::BOOLEAN },
};
#endif

}  // namespace

#define BuildHandlerList BuildHandlerList_ChromiumImpl
#include "../../../../../chrome/browser/policy/configuration_policy_handler_list_factory.cc"  // NOLINT
#undef BuildHandlerList

namespace policy {

std::unique_ptr<ConfigurationPolicyHandlerList> BuildHandlerList(
    const Schema& chrome_schema) {
  std::unique_ptr<ConfigurationPolicyHandlerList> handlers =
      BuildHandlerList_ChromiumImpl(chrome_schema);

  // TODO(simonhong): Remove this guard when array size is not empty w/o tor.
  // base::size failed to instantiate with zero-size array.
#if BUILDFLAG(ENABLE_TOR)
  for (size_t i = 0; i < base::size(kBraveSimplePolicyMap); ++i) {
    handlers->AddHandler(std::make_unique<SimplePolicyHandler>(
        kBraveSimplePolicyMap[i].policy_name,
        kBraveSimplePolicyMap[i].preference_path,
        kBraveSimplePolicyMap[i].value_type));
  }
#endif
  return handlers;
}

}  // namespace policy
