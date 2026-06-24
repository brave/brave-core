/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/snap/snap_controller.h"

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/files/scoped_temp_dir.h"
#include "base/functional/callback.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "base/types/expected.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/snap/snap_permission_controller.h"
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

namespace {
using InvokeFuture =
    base::test::TestFuture<std::optional<base::Value>, std::optional<std::string>>;
}  // namespace

class SnapControllerTest : public testing::Test {
 public:
  void SetUp() override {
    SnapRegistry::RegisterProfilePrefs(prefs_.registry());
    SnapPermissionController::RegisterProfilePrefs(prefs_.registry());
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    data_provider_ =
        std::make_unique<SnapDataProvider>(temp_dir_.GetPath(), prefs_);
    permission_controller_ =
        std::make_unique<SnapPermissionController>(prefs_, *data_provider_);
    controller_ = std::make_unique<SnapController>(
        *data_provider_, *permission_controller_, bridge_);
  }

 protected:
  void InstallSnap(const std::string& snap_id,
                   bool allow_dapps = false,
                   std::vector<std::string> allowed_origins = {}) {
    auto data = mojom::SnapInstallData::New();
    data->snap_id = snap_id;
    data->version = "1.0.0";
    data->manifest =
        MakeTestSnapManifest({"snap_dialog"}, allow_dapps, allowed_origins);
    data->enabled = true;
    data_provider_->OnSnapInstalled(*data);
  }

  url::Origin SecureOrigin() {
    return url::Origin::Create(GURL("https://app.example.com"));
  }

  base::test::TaskEnvironment task_environment_;
  base::ScopedTempDir temp_dir_;
  TestingPrefServiceSimple prefs_;
  FakeSnapBridgeController bridge_;
  std::unique_ptr<SnapDataProvider> data_provider_;
  std::unique_ptr<SnapPermissionController> permission_controller_;
  std::unique_ptr<SnapController> controller_;
};

// --- InvokeSnap -------------------------------------------------------------

TEST_F(SnapControllerTest, InvokeSnapInternalSuccess) {
  InstallSnap("npm:test-snap");

  InvokeFuture future;
  controller_->InvokeSnap("npm:test-snap", "foo", base::Value("params"),
                          std::nullopt, future.GetCallback());

  ASSERT_TRUE(future.Get<0>());
  EXPECT_EQ(*future.Get<0>(), base::Value("ok"));
  EXPECT_FALSE(future.Get<1>());

  ASSERT_EQ(bridge_.load_snap_ids.size(), 1u);
  EXPECT_EQ(bridge_.load_snap_ids[0], "npm:test-snap");
  ASSERT_EQ(bridge_.invoke_calls.size(), 1u);
  EXPECT_EQ(bridge_.invoke_calls[0].method, "foo");
  EXPECT_EQ(bridge_.invoke_calls[0].caller_origin, "brave-wallet");
}

TEST_F(SnapControllerTest, InvokeSnapUnknownSnap) {
  InvokeFuture future;
  controller_->InvokeSnap("npm:test-snap", "foo", base::Value(), std::nullopt,
                          future.GetCallback());
  EXPECT_FALSE(future.Get<0>());
  EXPECT_EQ(future.Get<1>(), "Unknown snap: npm:test-snap");
}

TEST_F(SnapControllerTest, InvokeSnapInsecureOriginRejected) {
  InstallSnap("npm:test-snap");
  InvokeFuture future;
  controller_->InvokeSnap("npm:test-snap", "foo", base::Value(),
                          url::Origin() /* opaque */, future.GetCallback());
  EXPECT_EQ(future.Get<1>(), "requires a secure web origin");
}

TEST_F(SnapControllerTest, InvokeSnapAutoGrantsPermittedOrigin) {
  InstallSnap("npm:test-snap", /*allow_dapps=*/true);
  const url::Origin origin = SecureOrigin();
  ASSERT_FALSE(permission_controller_->IsSnapConnected(origin, "npm:test-snap"));

  InvokeFuture future;
  controller_->InvokeSnap("npm:test-snap", "foo", base::Value(), origin,
                          future.GetCallback());

  ASSERT_TRUE(future.Get<0>());
  EXPECT_EQ(*future.Get<0>(), base::Value("ok"));
  EXPECT_TRUE(permission_controller_->IsSnapConnected(origin, "npm:test-snap"));
}

TEST_F(SnapControllerTest, InvokeSnapOriginNotConnected) {
  // allow_dapps=false → no auto-grant, so the origin is never connected.
  InstallSnap("npm:test-snap", /*allow_dapps=*/false);
  InvokeFuture future;
  controller_->InvokeSnap("npm:test-snap", "foo", base::Value(), SecureOrigin(),
                          future.GetCallback());
  EXPECT_EQ(future.Get<1>(), "Snap is not connected to this origin");
}

TEST_F(SnapControllerTest, InvokeSnapConnectedButManifestDisallows) {
  InstallSnap("npm:test-snap", /*allow_dapps=*/false);
  const url::Origin origin = SecureOrigin();
  permission_controller_->GrantSnapConnection(origin, "npm:test-snap");

  InvokeFuture future;
  controller_->InvokeSnap("npm:test-snap", "foo", base::Value(), origin,
                          future.GetCallback());
  EXPECT_EQ(future.Get<1>(), "Origin not permitted by snap manifest");
}

TEST_F(SnapControllerTest, InvokeSnapNoBridge) {
  InstallSnap("npm:test-snap");
  bridge_.is_bound = false;
  InvokeFuture future;
  controller_->InvokeSnap("npm:test-snap", "foo", base::Value(), std::nullopt,
                          future.GetCallback());
  EXPECT_EQ(future.Get<1>(), "no_bridge");
}

TEST_F(SnapControllerTest, InvokeSnapLoadFailure) {
  InstallSnap("npm:test-snap");
  bridge_.load_success = false;
  bridge_.load_error = "boom";
  InvokeFuture future;
  controller_->InvokeSnap("npm:test-snap", "foo", base::Value(), std::nullopt,
                          future.GetCallback());
  EXPECT_EQ(future.Get<1>(), "boom");
}

TEST_F(SnapControllerTest, InvokeSnapDisconnectFailsPending) {
  InstallSnap("npm:test-snap");
  bridge_.auto_run_load = false;  // Keep the call in-flight.

  InvokeFuture future;
  controller_->InvokeSnap("npm:test-snap", "foo", base::Value(), std::nullopt,
                          future.GetCallback());
  EXPECT_FALSE(future.IsReady());

  bridge_.FireDisconnect();

  ASSERT_TRUE(future.IsReady());
  EXPECT_EQ(future.Get<1>(), "SnapBridge disconnected");
}

TEST_F(SnapControllerTest, InvokeSnapConnectionApprovalApproveSucceeds) {
  InstallSnap("npm:test-snap", /*allow_dapps=*/true);
  const url::Origin origin = SecureOrigin();
  ASSERT_FALSE(permission_controller_->IsSnapConnected(origin, "npm:test-snap"));

  std::optional<std::string> requested_snap;
  controller_->SetRequestConnectionDelegate(base::BindLambdaForTesting(
      [&](url::Origin /*o*/, std::string id,
          SnapController::RequestConnectionResultCallback cb) {
        requested_snap = id;
        std::move(cb).Run(/*approved=*/true);
      }));

  InvokeFuture future;
  controller_->InvokeSnap("npm:test-snap", "foo", base::Value(), origin,
                          future.GetCallback());

  EXPECT_EQ(requested_snap, "npm:test-snap");
  ASSERT_TRUE(future.Get<0>());
  EXPECT_EQ(*future.Get<0>(), base::Value("ok"));
  EXPECT_TRUE(permission_controller_->IsSnapConnected(origin, "npm:test-snap"));
}

TEST_F(SnapControllerTest, InvokeSnapConnectionApprovalRejectFails) {
  InstallSnap("npm:test-snap", /*allow_dapps=*/true);
  const url::Origin origin = SecureOrigin();

  controller_->SetRequestConnectionDelegate(base::BindLambdaForTesting(
      [&](url::Origin /*o*/, std::string /*id*/,
          SnapController::RequestConnectionResultCallback cb) {
        std::move(cb).Run(/*approved=*/false);
      }));

  InvokeFuture future;
  controller_->InvokeSnap("npm:test-snap", "foo", base::Value(), origin,
                          future.GetCallback());

  EXPECT_EQ(future.Get<1>(), "user_rejected");
  EXPECT_FALSE(permission_controller_->IsSnapConnected(origin, "npm:test-snap"));
}

TEST_F(SnapControllerTest, InvokeSnapAlreadyConnectedSkipsApproval) {
  InstallSnap("npm:test-snap", /*allow_dapps=*/true);
  const url::Origin origin = SecureOrigin();
  permission_controller_->GrantSnapConnection(origin, "npm:test-snap");

  bool delegate_called = false;
  controller_->SetRequestConnectionDelegate(base::BindLambdaForTesting(
      [&](url::Origin /*o*/, std::string /*id*/,
          SnapController::RequestConnectionResultCallback cb) {
        delegate_called = true;
        std::move(cb).Run(/*approved=*/true);
      }));

  InvokeFuture future;
  controller_->InvokeSnap("npm:test-snap", "foo", base::Value(), origin,
                          future.GetCallback());

  EXPECT_FALSE(delegate_called);
  ASSERT_TRUE(future.Get<0>());
  EXPECT_EQ(*future.Get<0>(), base::Value("ok"));
}

// --- RequestSnaps -----------------------------------------------------------

TEST_F(SnapControllerTest, RequestSnapsInsecureOriginRejected) {
  base::test::TestFuture<std::optional<base::DictValue>,
                         std::optional<std::string>>
      future;
  controller_->RequestSnaps(url::Origin(), base::DictValue(),
                            future.GetCallback());
  EXPECT_EQ(future.Get<1>(),
            "wallet_requestSnaps requires a secure web origin");
}

TEST_F(SnapControllerTest, RequestSnapsAlreadyInstalledGrants) {
  InstallSnap("npm:test-snap", /*allow_dapps=*/true);
  const url::Origin origin = SecureOrigin();

  base::DictValue snaps;
  snaps.Set("npm:test-snap", base::DictValue());

  base::test::TestFuture<std::optional<base::DictValue>,
                         std::optional<std::string>>
      future;
  controller_->RequestSnaps(origin, snaps, future.GetCallback());

  ASSERT_TRUE(future.Get<0>());
  EXPECT_TRUE(future.Get<0>()->contains("npm:test-snap"));
  EXPECT_TRUE(permission_controller_->IsSnapConnected(origin, "npm:test-snap"));
}

TEST_F(SnapControllerTest, RequestSnapsInstallsMissing) {
  const url::Origin origin = SecureOrigin();
  std::vector<std::pair<std::string, std::string>> installs;
  controller_->SetInstallSnapDelegate(base::BindLambdaForTesting(
      [&](std::string id, std::string version,
          SnapController::InstallSnapResultCallback cb) {
        installs.emplace_back(id, version);
        std::move(cb).Run(base::ok());
      }));

  base::DictValue opts;
  opts.Set("version", "2.0.0");
  base::DictValue snaps;
  snaps.Set("npm:new-snap", std::move(opts));

  base::test::TestFuture<std::optional<base::DictValue>,
                         std::optional<std::string>>
      future;
  controller_->RequestSnaps(origin, snaps, future.GetCallback());

  ASSERT_EQ(installs.size(), 1u);
  EXPECT_EQ(installs[0].first, "npm:new-snap");
  EXPECT_EQ(installs[0].second, "2.0.0");
  ASSERT_TRUE(future.Get<0>());
  EXPECT_TRUE(future.Get<0>()->contains("npm:new-snap"));
  EXPECT_TRUE(permission_controller_->IsSnapConnected(origin, "npm:new-snap"));
}

TEST_F(SnapControllerTest, RequestSnapsAggregatesMultiple) {
  controller_->SetInstallSnapDelegate(base::BindLambdaForTesting(
      [&](std::string id, std::string version,
          SnapController::InstallSnapResultCallback cb) {
        std::move(cb).Run(base::ok());
      }));

  base::DictValue snaps;
  snaps.Set("npm:a", base::DictValue());
  snaps.Set("npm:b", base::DictValue());

  base::test::TestFuture<std::optional<base::DictValue>,
                         std::optional<std::string>>
      future;
  controller_->RequestSnaps(SecureOrigin(), snaps, future.GetCallback());

  ASSERT_TRUE(future.Get<0>());
  EXPECT_TRUE(future.Get<0>()->contains("npm:a"));
  EXPECT_TRUE(future.Get<0>()->contains("npm:b"));
}

TEST_F(SnapControllerTest, RequestSnapsInstallFailureExcludesSnap) {
  const url::Origin origin = SecureOrigin();
  controller_->SetInstallSnapDelegate(base::BindLambdaForTesting(
      [&](std::string id, std::string version,
          SnapController::InstallSnapResultCallback cb) {
        std::move(cb).Run(base::unexpected("install failed"));
      }));

  base::DictValue snaps;
  snaps.Set("npm:new-snap", base::DictValue());

  base::test::TestFuture<std::optional<base::DictValue>,
                         std::optional<std::string>>
      future;
  controller_->RequestSnaps(origin, snaps, future.GetCallback());

  ASSERT_TRUE(future.Get<0>());
  EXPECT_FALSE(future.Get<0>()->contains("npm:new-snap"));
  EXPECT_FALSE(permission_controller_->IsSnapConnected(origin, "npm:new-snap"));
}

// --- Homepage / user input --------------------------------------------------

TEST_F(SnapControllerTest, GetSnapHomePageSuccess) {
  base::test::TestFuture<std::optional<std::string>, std::optional<std::string>,
                         std::optional<std::string>>
      future;
  controller_->GetSnapHomePage(
      "npm:test-snap",
      future.GetCallback<const std::optional<std::string>&,
                         const std::optional<std::string>&,
                         const std::optional<std::string>&>());
  EXPECT_EQ(future.Get<0>(), "{}");
  EXPECT_EQ(future.Get<1>(), "iface-1");
  EXPECT_FALSE(future.Get<2>());
}

TEST_F(SnapControllerTest, GetSnapHomePageNoBridge) {
  bridge_.is_bound = false;
  base::test::TestFuture<std::optional<std::string>, std::optional<std::string>,
                         std::optional<std::string>>
      future;
  controller_->GetSnapHomePage(
      "npm:test-snap",
      future.GetCallback<const std::optional<std::string>&,
                         const std::optional<std::string>&,
                         const std::optional<std::string>&>());
  EXPECT_EQ(future.Get<2>(), "no_bridge");
}

TEST_F(SnapControllerTest, GetSnapHomePageLoadFailure) {
  bridge_.load_success = false;
  bridge_.load_error = "nope";
  base::test::TestFuture<std::optional<std::string>, std::optional<std::string>,
                         std::optional<std::string>>
      future;
  controller_->GetSnapHomePage(
      "npm:test-snap",
      future.GetCallback<const std::optional<std::string>&,
                         const std::optional<std::string>&,
                         const std::optional<std::string>&>());
  EXPECT_EQ(future.Get<2>(), "nope");
}

TEST_F(SnapControllerTest, SendSnapUserInputSuccess) {
  base::test::TestFuture<std::optional<std::string>, std::optional<std::string>>
      future;
  controller_->SendSnapUserInput(
      "npm:test-snap", "iface-1", "{}",
      future.GetCallback<const std::optional<std::string>&,
                         const std::optional<std::string>&>());
  EXPECT_EQ(future.Get<0>(), "{}");
  EXPECT_FALSE(future.Get<1>());
}

TEST_F(SnapControllerTest, SendSnapUserInputNoBridge) {
  bridge_.is_bound = false;
  base::test::TestFuture<std::optional<std::string>, std::optional<std::string>>
      future;
  controller_->SendSnapUserInput(
      "npm:test-snap", "iface-1", "{}",
      future.GetCallback<const std::optional<std::string>&,
                         const std::optional<std::string>&>());
  EXPECT_EQ(future.Get<1>(), "no_bridge");
}

}  // namespace brave_wallet
