/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_sync/features.h"
#include "build/build_config.h"

#include <components/password_manager/core/browser/sync/password_data_type_controller_unittest.cc>

#if BUILDFLAG(IS_ANDROID)

namespace password_manager {
namespace {

syncer::ConfigureContext MakeFullSyncContext() {
  syncer::ConfigureContext context;
  context.authenticated_gaia_id = GaiaId("gaia");
  context.cache_guid = "cache_guid";
  context.sync_mode = syncer::SyncMode::kFull;
  context.reason = syncer::ConfigureReason::kReconfiguration;
  context.configuration_start_time = base::Time::Now();
  return context;
}

// With kBraveAndroidSyncPasswordsInProfileStore on, a syncing user must keep
// the incoming kFull mode, so that passwords are stored in the profile store
// like on desktop. The delegate that gets started is the observable side of
// that.
TEST_F(PasswordDataTypeControllerTest, BraveKeepsFullSyncModeWhenFlagEnabled) {
  base::test::ScopedFeatureList features;
  features.InitAndEnableFeature(
      brave_sync::features::kBraveAndroidSyncPasswordsInProfileStore);

  EXPECT_CALL(*full_sync_delegate(), OnSyncStarting);
  EXPECT_CALL(*transport_only_delegate(), OnSyncStarting).Times(0);

  controller()->LoadModels(MakeFullSyncContext(), base::DoNothing());
}

// Without the flag the upstream override still applies. Upstream's
// OverrideFullSyncMode asserts the same thing while relying on the flag being
// disabled by default; this pins it explicitly so a default flip cannot make
// that test silently cover the Brave path instead.
TEST_F(PasswordDataTypeControllerTest,
       BraveForcesTransportOnlyModeWithoutFlag) {
  base::test::ScopedFeatureList features;
  features.InitAndDisableFeature(
      brave_sync::features::kBraveAndroidSyncPasswordsInProfileStore);

  EXPECT_CALL(*full_sync_delegate(), OnSyncStarting).Times(0);
  EXPECT_CALL(*transport_only_delegate(), OnSyncStarting);

  controller()->LoadModels(MakeFullSyncContext(), base::DoNothing());
}

}  // namespace
}  // namespace password_manager

#endif  // BUILDFLAG(IS_ANDROID)
