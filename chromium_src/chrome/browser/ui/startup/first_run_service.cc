/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/startup/first_run_service.h"

#include "chrome/browser/profiles/profile_attributes_entry.h"
#include "chrome/browser/profiles/profile_attributes_storage.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/ui/profiles/profile_customization_util.h"

// Because kSigninAllowed is set to false ComputeDevicePolicyEffect returns
// PolicyEffect::kDisabled and TryMarkFirstRunAlreadyFinished calls
// FinishFirstRun with FinishedReason::kSkippedByPolicies. Because we don't
// have primary account, FirstRunService::FinishFirstRun ends up in the code
// path that picks enterprise profile name ("Work"). We don't want this
// change to happen, so we'll just use original profile name instead.
#define FinalizeNewProfileSetup(PROFILE, NAME, IS_DEFAULT_NAME)                \
  ProfileAttributesEntry* entry =                                              \
      g_browser_process->profile_manager()                                     \
          ->GetProfileAttributesStorage()                                      \
          .GetProfileAttributesWithPath(profile_->GetPath());                  \
  CHECK(entry);                                                                \
  std::u16string original_name = entry->GetLocalProfileName();                 \
  CHECK(!original_name.empty());                                               \
  bool is_default =                                                            \
      g_browser_process->profile_manager()                                     \
          ->GetProfileAttributesStorage()                                      \
          .IsDefaultProfileName(                                               \
              original_name, /*include_check_for_legacy_profile_name=*/false); \
  FinalizeNewProfileSetup(PROFILE, original_name, is_default)

#include <chrome/browser/ui/startup/first_run_service.cc>
#undef FinalizeNewProfileSetup
