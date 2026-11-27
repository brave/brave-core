/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/startup/first_run_service.h"

#include <string>

#include "base/check.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_attributes_entry.h"
#include "chrome/browser/profiles/profile_attributes_storage.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/ui/profiles/profile_customization_util.h"

namespace {

// Because kSigninAllowed is set to false ComputeDevicePolicyEffect returns
// PolicyEffect::kDisabled and TryMarkFirstRunAlreadyFinished calls
// FinishFirstRun with FinishedReason::kSkippedByPolicies. Because we don't
// have primary account, FirstRunService::FinishFirstRun ends up in the code
// path that picks enterprise profile name ("Work"). We don't want this
// change to happen, so we'll just use original profile name instead.
void BraveFinalizeNewProfileSetup(Profile* profile) {
  ProfileAttributesStorage& storage =
      g_browser_process->profile_manager()->GetProfileAttributesStorage();
  ProfileAttributesEntry* entry =
      storage.GetProfileAttributesWithPath(profile->GetPath());
  CHECK(entry);
  std::u16string original_name = entry->GetLocalProfileName();
  CHECK(!original_name.empty());
  bool is_default = storage.IsDefaultProfileName(
      original_name, /*include_check_for_legacy_profile_name=*/false);
  FinalizeNewProfileSetup(profile, original_name, is_default);
}

}  // namespace

#include <chrome/browser/ui/startup/first_run_service.cc>
