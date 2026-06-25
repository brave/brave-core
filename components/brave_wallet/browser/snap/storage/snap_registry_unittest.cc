/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/snap/storage/snap_registry.h"

#include <memory>
#include <set>
#include <string>

#include "brave/components/brave_wallet/browser/snap/snap_test_utils.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

class SnapRegistryTest : public testing::Test {
 public:
  void SetUp() override {
    SnapRegistry::RegisterProfilePrefs(prefs_.registry());
    registry_ = std::make_unique<SnapRegistry>(prefs_);
  }

 protected:
  TestingPrefServiceSimple prefs_;
  std::unique_ptr<SnapRegistry> registry_;
};

TEST_F(SnapRegistryTest, RegisterAndQuery) {
  EXPECT_FALSE(registry_->IsKnownSnap("npm:@test/snap"));

  registry_->RegisterInstalledSnap(
      *MakeTestSnapInstallData("npm:@test/snap", "1.0.0"));

  EXPECT_TRUE(registry_->IsKnownSnap("npm:@test/snap"));
  auto snap = registry_->GetSnap("npm:@test/snap");
  ASSERT_TRUE(snap);
  EXPECT_EQ(snap->snap_id, "npm:@test/snap");
  EXPECT_EQ(snap->version, "1.0.0");
  ASSERT_TRUE(snap->manifest);
  EXPECT_EQ(registry_->GetAllSnaps().size(), 1u);
}

TEST_F(SnapRegistryTest, UnknownSnap) {
  EXPECT_FALSE(registry_->IsKnownSnap("npm:@nope/snap"));
  EXPECT_FALSE(registry_->GetSnap("npm:@nope/snap"));
}

TEST_F(SnapRegistryTest, GetSnapReturnsIndependentClone) {
  registry_->RegisterInstalledSnap(
      *MakeTestSnapInstallData("npm:@test/snap", "1.0.0"));

  auto first = registry_->GetSnap("npm:@test/snap");
  ASSERT_TRUE(first);
  first->version = "mutated";

  auto second = registry_->GetSnap("npm:@test/snap");
  ASSERT_TRUE(second);
  EXPECT_EQ(second->version, "1.0.0");
}

TEST_F(SnapRegistryTest, UnregisterSnap) {
  registry_->RegisterInstalledSnap(
      *MakeTestSnapInstallData("npm:@test/snap", "1.0.0"));
  ASSERT_TRUE(registry_->IsKnownSnap("npm:@test/snap"));

  registry_->UnregisterSnap("npm:@test/snap");
  EXPECT_FALSE(registry_->IsKnownSnap("npm:@test/snap"));
  EXPECT_TRUE(registry_->GetAllSnaps().empty());
}

TEST_F(SnapRegistryTest, GetAllSnapsReturnsEveryEntry) {
  registry_->RegisterInstalledSnap(*MakeTestSnapInstallData("npm:a", "1.0.0"));
  registry_->RegisterInstalledSnap(*MakeTestSnapInstallData("npm:b", "1.0.0"));

  std::set<std::string> ids;
  for (const auto& snap : registry_->GetAllSnaps()) {
    ids.insert(snap->snap_id);
  }
  EXPECT_THAT(ids, testing::UnorderedElementsAre("npm:a", "npm:b"));
}

TEST_F(SnapRegistryTest, RegisterOverwritesExisting) {
  registry_->RegisterInstalledSnap(
      *MakeTestSnapInstallData("npm:@test/snap", "1.0.0"));
  registry_->RegisterInstalledSnap(
      *MakeTestSnapInstallData("npm:@test/snap", "2.0.0"));

  auto snap = registry_->GetSnap("npm:@test/snap");
  ASSERT_TRUE(snap);
  EXPECT_EQ(snap->version, "2.0.0");
  EXPECT_EQ(registry_->GetAllSnaps().size(), 1u);
}

TEST_F(SnapRegistryTest, SetSnapEnabledPersists) {
  registry_->RegisterInstalledSnap(
      *MakeTestSnapInstallData("npm:@test/snap", "1.0.0"));
  ASSERT_TRUE(registry_->GetSnap("npm:@test/snap")->enabled);

  registry_->SetSnapEnabled("npm:@test/snap", false);
  EXPECT_FALSE(registry_->GetSnap("npm:@test/snap")->enabled);

  SnapRegistry reloaded(prefs_);
  EXPECT_FALSE(reloaded.GetSnap("npm:@test/snap")->enabled);

  registry_->SetSnapEnabled("npm:unknown", true);
}

TEST_F(SnapRegistryTest, PersistsAcrossReconstruction) {
  auto data = MakeTestSnapInstallData("npm:@test/snap", "2.0.0");
  data->bundle_size_bytes = 4096;
  data->enabled = false;
  data->manifest->proposed_name = "My Snap";
  data->manifest->description = "A description";
  data->manifest->permissions = {"snap_dialog", "endowment:rpc"};
  data->manifest->allow_dapps = true;
  data->manifest->allow_snaps = true;
  data->manifest->allowed_rpc_origins = {"https://app.example.com"};
  data->manifest->name_lookup_chains = {"eip155:1"};
  data->manifest->name_lookup_tlds = {"crypto", "eth"};
  data->manifest->name_lookup_schemes = {"farcaster"};
  registry_->RegisterInstalledSnap(*data);

  // A registry rebuilt from the same prefs reloads the persisted snap.
  SnapRegistry reloaded(prefs_);
  auto snap = reloaded.GetSnap("npm:@test/snap");
  ASSERT_TRUE(snap);
  EXPECT_EQ(snap->version, "2.0.0");
  EXPECT_EQ(snap->bundle_size_bytes, 4096u);
  EXPECT_FALSE(snap->enabled);
  ASSERT_TRUE(snap->manifest);
  EXPECT_EQ(snap->manifest->proposed_name, "My Snap");
  EXPECT_EQ(snap->manifest->description, "A description");
  EXPECT_THAT(snap->manifest->permissions,
              testing::ElementsAre("snap_dialog", "endowment:rpc"));
  EXPECT_TRUE(snap->manifest->allow_dapps);
  EXPECT_TRUE(snap->manifest->allow_snaps);
  EXPECT_THAT(snap->manifest->allowed_rpc_origins,
              testing::ElementsAre("https://app.example.com"));
  EXPECT_THAT(snap->manifest->name_lookup_chains,
              testing::ElementsAre("eip155:1"));
  EXPECT_THAT(snap->manifest->name_lookup_tlds,
              testing::ElementsAre("crypto", "eth"));
  EXPECT_THAT(snap->manifest->name_lookup_schemes,
              testing::ElementsAre("farcaster"));
}

}  // namespace brave_wallet
