/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_sync/brave_sync_prefs.h"

#include <memory>

#include "base/base64.h"
#include "base/test/gtest_util.h"
#include "base/test/task_environment.h"
#include "build/build_config.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

#if BUILDFLAG(IS_ANDROID)
#include "components/sync/service/sync_prefs.h"
#endif

using testing::Eq;
using testing::IsEmpty;
using testing::Optional;

namespace brave_sync {

class BraveSyncPrefsTest : public testing::Test {
 protected:
  BraveSyncPrefsTest() {
    brave_sync::Prefs::RegisterProfilePrefs(pref_service_.registry());
    brave_sync_prefs_ = std::make_unique<brave_sync::Prefs>(&pref_service_);
#if BUILDFLAG(IS_ANDROID)
    syncer::SyncPrefs::RegisterProfilePrefs(pref_service_.registry());
    sync_prefs_ = std::make_unique<syncer::SyncPrefs>(&pref_service_);
#endif
  }

  brave_sync::Prefs* brave_sync_prefs() { return brave_sync_prefs_.get(); }

  PrefService* pref_service() { return &pref_service_; }

  base::test::SingleThreadTaskEnvironment task_environment_;
  TestingPrefServiceSimple pref_service_;
  std::unique_ptr<brave_sync::Prefs> brave_sync_prefs_;

#if BUILDFLAG(IS_ANDROID)
  std::unique_ptr<syncer::SyncPrefs> sync_prefs_;
#endif
};


using BraveSyncPrefsDeathTest = BraveSyncPrefsTest;

TEST_F(BraveSyncPrefsTest, LeaveChainDetailsMaxLenIOS) {
  brave_sync_prefs()->SetAddLeaveChainDetailBehaviourForTesting(
      brave_sync::Prefs::AddLeaveChainDetailBehaviour::kAdd);

  auto max_len = Prefs::GetLeaveChainDetailsMaxLenForTesting();

  std::string details("a");
  brave_sync_prefs()->AddLeaveChainDetail("", 0, details.c_str());
  details = brave_sync_prefs()->GetLeaveChainDetails();
  EXPECT_LE(details.size(), max_len);
  EXPECT_GE(details.size(), 1u);

  details.assign(max_len + 1, 'a');
  brave_sync_prefs()->AddLeaveChainDetail(__FILE__, __LINE__, details.c_str());
  details = brave_sync_prefs()->GetLeaveChainDetails();
  EXPECT_EQ(details.size(), max_len);
}

#if BUILDFLAG(IS_ANDROID)
// Upstream used to have SyncPrefs::SetPasswordSyncAllowed
// commit: 558ee269a65b72e02fd3e6f27cc0eed6836b9cae
// This ensures we can set passwords as a selected to sync type
TEST_F(BraveSyncPrefsTest, PasswordSyncAllowedExplicitValue) {
  using syncer::UserSelectableType;
  using syncer::UserSelectableTypeSet;

  // Make passwords explicitly enabled (no default value).
  sync_prefs_->SetSelectedTypesForSyncingUser(
      /*keep_everything_synced=*/false,
      /*registered_types=*/UserSelectableTypeSet::All(),
      /*selected_types=*/{UserSelectableType::kPasswords});

  EXPECT_TRUE(sync_prefs_->GetSelectedTypesForSyncingUser().Has(
      UserSelectableType::kPasswords));
}
#endif

}  // namespace brave_sync
