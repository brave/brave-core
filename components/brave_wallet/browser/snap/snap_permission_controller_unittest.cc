/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/snap/snap_permission_controller.h"

#include <memory>
#include <string>
#include <vector>

#include "base/files/scoped_temp_dir.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_wallet/browser/snap/snap_test_utils.h"
#include "brave/components/brave_wallet/browser/snap/storage/snap_data_provider.h"
#include "brave/components/brave_wallet/browser/snap/storage/snap_registry.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace brave_wallet {

class SnapPermissionControllerTest : public testing::Test {
 public:
  void SetUp() override {
    SnapRegistry::RegisterProfilePrefs(prefs_.registry());
    SnapPermissionController::RegisterProfilePrefs(prefs_.registry());
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    data_provider_ =
        std::make_unique<SnapDataProvider>(temp_dir_.GetPath(), prefs_);
    controller_ =
        std::make_unique<SnapPermissionController>(prefs_, *data_provider_);
  }

 protected:
  void InstallSnap(const std::string& snap_id,
                   std::vector<std::string> permissions = {"snap_dialog"},
                   bool allow_dapps = false,
                   std::vector<std::string> allowed_origins = {}) {
    auto data = mojom::SnapInstallData::New();
    data->snap_id = snap_id;
    data->version = "1.0.0";
    data->manifest = MakeTestSnapManifest(std::move(permissions), allow_dapps,
                                          std::move(allowed_origins));
    data->enabled = true;
    data_provider_->OnSnapInstalled(*data);
  }

  url::Origin Origin(const std::string& url) {
    return url::Origin::Create(GURL(url));
  }

  base::test::TaskEnvironment task_environment_;
  base::ScopedTempDir temp_dir_;
  TestingPrefServiceSimple prefs_;
  std::unique_ptr<SnapDataProvider> data_provider_;
  std::unique_ptr<SnapPermissionController> controller_;
};

// --- Connection grants ------------------------------------------------------

TEST_F(SnapPermissionControllerTest, GrantAndIsConnected) {
  const url::Origin origin = Origin("https://app.example.com");
  EXPECT_FALSE(controller_->IsSnapConnected(origin, "npm:test-snap"));
  controller_->GrantSnapConnection(origin, "npm:test-snap");
  EXPECT_TRUE(controller_->IsSnapConnected(origin, "npm:test-snap"));
}

TEST_F(SnapPermissionControllerTest, GrantIsIdempotent) {
  const url::Origin origin = Origin("https://app.example.com");
  controller_->GrantSnapConnection(origin, "npm:test-snap");
  controller_->GrantSnapConnection(origin, "npm:test-snap");
  EXPECT_THAT(controller_->GetConnectedSnaps(origin),
              testing::ElementsAre("npm:test-snap"));
}

TEST_F(SnapPermissionControllerTest, GetConnectedSnapsReturnsAll) {
  const url::Origin origin = Origin("https://app.example.com");
  controller_->GrantSnapConnection(origin, "npm:a");
  controller_->GrantSnapConnection(origin, "npm:b");
  EXPECT_THAT(controller_->GetConnectedSnaps(origin),
              testing::UnorderedElementsAre("npm:a", "npm:b"));
}

TEST_F(SnapPermissionControllerTest, RevokeRemovesSnap) {
  const url::Origin origin = Origin("https://app.example.com");
  controller_->GrantSnapConnection(origin, "npm:a");
  controller_->GrantSnapConnection(origin, "npm:b");

  controller_->RevokeSnapConnection(origin, "npm:a");
  EXPECT_FALSE(controller_->IsSnapConnected(origin, "npm:a"));
  EXPECT_THAT(controller_->GetConnectedSnaps(origin),
              testing::ElementsAre("npm:b"));

  // Revoking the last snap leaves the origin with no grants.
  controller_->RevokeSnapConnection(origin, "npm:b");
  EXPECT_TRUE(controller_->GetConnectedSnaps(origin).empty());
}

TEST_F(SnapPermissionControllerTest, RevokeNonexistentIsNoOp) {
  const url::Origin origin = Origin("https://app.example.com");
  controller_->RevokeSnapConnection(origin, "npm:never-connected");
  EXPECT_TRUE(controller_->GetConnectedSnaps(origin).empty());
}

TEST_F(SnapPermissionControllerTest, PurgeConnectionGrantsForSnap) {
  const url::Origin origin_a = Origin("https://a.example.com");
  const url::Origin origin_b = Origin("https://b.example.com");
  controller_->GrantSnapConnection(origin_a, "npm:target");
  controller_->GrantSnapConnection(origin_b, "npm:target");
  controller_->GrantSnapConnection(origin_a, "npm:other");

  controller_->PurgeConnectionGrantsForSnap("npm:target");

  EXPECT_FALSE(controller_->IsSnapConnected(origin_a, "npm:target"));
  EXPECT_FALSE(controller_->IsSnapConnected(origin_b, "npm:target"));
  // Other grants survive.
  EXPECT_TRUE(controller_->IsSnapConnected(origin_a, "npm:other"));
}

// --- IsOriginAllowedByManifest ----------------------------------------------

TEST_F(SnapPermissionControllerTest, OriginAllowedUnknownSnap) {
  EXPECT_FALSE(controller_->IsOriginAllowedByManifest(
      Origin("https://app.example.com"), "npm:unknown"));
}

TEST_F(SnapPermissionControllerTest, OriginAllowedRequiresAllowDapps) {
  InstallSnap("npm:test-snap", {"endowment:rpc"}, /*allow_dapps=*/false);
  EXPECT_FALSE(controller_->IsOriginAllowedByManifest(
      Origin("https://app.example.com"), "npm:test-snap"));
}

TEST_F(SnapPermissionControllerTest, OriginAllowedEmptyListAllowsAny) {
  InstallSnap("npm:test-snap", {"endowment:rpc"}, /*allow_dapps=*/true,
              /*allowed_origins=*/{});
  EXPECT_TRUE(controller_->IsOriginAllowedByManifest(
      Origin("https://anything.example.com"), "npm:test-snap"));
}

TEST_F(SnapPermissionControllerTest, OriginAllowedRespectsAllowlist) {
  InstallSnap("npm:test-snap", {"endowment:rpc"}, /*allow_dapps=*/true,
              /*allowed_origins=*/{"https://app.example.com"});
  EXPECT_TRUE(controller_->IsOriginAllowedByManifest(
      Origin("https://app.example.com"), "npm:test-snap"));
  EXPECT_FALSE(controller_->IsOriginAllowedByManifest(
      Origin("https://evil.example.com"), "npm:test-snap"));
}

// --- CheckSnapMethodPermission ----------------------------------------------

TEST_F(SnapPermissionControllerTest, CheckMethodUnknownSnap) {
  auto error = controller_->CheckSnapMethodPermission("npm:unknown", "snap_dialog");
  ASSERT_TRUE(error);
  EXPECT_EQ(*error, "Unknown snap: npm:unknown");
}

TEST_F(SnapPermissionControllerTest, CheckMethodAllowedWhenDeclared) {
  InstallSnap("npm:test-snap", {"snap_dialog"});
  EXPECT_FALSE(
      controller_->CheckSnapMethodPermission("npm:test-snap", "snap_dialog"));
}

TEST_F(SnapPermissionControllerTest, CheckMethodRejectedWhenMissing) {
  InstallSnap("npm:test-snap", {"snap_dialog"});
  auto error =
      controller_->CheckSnapMethodPermission("npm:test-snap", "snap_manageState");
  ASSERT_TRUE(error);
  EXPECT_THAT(*error, testing::HasSubstr("does not have permission"));
}

TEST_F(SnapPermissionControllerTest, CheckMethodConfirmAliasesToDialog) {
  InstallSnap("npm:test-snap", {"snap_dialog"});
  // snap_confirm maps to the snap_dialog permission entry.
  EXPECT_FALSE(
      controller_->CheckSnapMethodPermission("npm:test-snap", "snap_confirm"));
}

TEST_F(SnapPermissionControllerTest, CheckMethodConfirmRejectedWithoutDialog) {
  InstallSnap("npm:test-snap", {"snap_notify"});
  auto error =
      controller_->CheckSnapMethodPermission("npm:test-snap", "snap_confirm");
  ASSERT_TRUE(error);
  EXPECT_THAT(*error, testing::HasSubstr("does not have permission"));
}

}  // namespace brave_wallet
