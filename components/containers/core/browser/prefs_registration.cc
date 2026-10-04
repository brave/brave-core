// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/containers/core/browser/prefs_registration.h"

#include "base/metrics/field_trial_params.h"
#include "base/version_info/channel.h"
#include "brave/components/containers/core/browser/default_containers_list.h"
#include "brave/components/containers/core/browser/pref_names.h"
#include "brave/components/containers/core/browser/prefs.h"
#include "brave/components/containers/core/common/features.h"
#include "components/pref_registry/pref_registry_syncable.h"

namespace containers {

namespace {

bool GetContainersEnabledPrefDefault(version_info::Channel channel) {
  const bool enabled_by_default = channel != version_info::Channel::STABLE;
  return base::GetFieldTrialParamByFeatureAsBool(
      features::kContainers, "enabled_by_default", enabled_by_default);
}

}  // namespace

void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry,
                          version_info::Channel channel) {
  registry->RegisterBooleanPref(prefs::kContainersEnabled,
                                GetContainersEnabledPrefDefault(channel));
  registry->RegisterListPref(
      prefs::kContainersList,
      ConvertContainersToListValue(CreateDefaultContainersList()),
      user_prefs::PrefRegistrySyncable::SYNCABLE_PREF);
  registry->RegisterDictionaryPref(prefs::kLocallyUsedContainers);
}

}  // namespace containers
