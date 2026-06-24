/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/snaps_service.h"

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/files/scoped_temp_dir.h"
#include "base/functional/callback_helpers.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/network_manager.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/browser/snap/snap_test_utils.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "components/prefs/testing_pref_service.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "net/http/http_status_code.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace brave_wallet {

namespace {

class FakeSnapsServiceObserver : public mojom::SnapsServiceObserver {
 public:
  mojo::PendingRemote<mojom::SnapsServiceObserver> BindNewPipeAndPassRemote() {
    return receiver_.BindNewPipeAndPassRemote();
  }
  void OnPendingSnapInstallChanged() override { ++change_count; }
  void OnPendingSnapConnectionChanged() override { ++connection_change_count; }
  int change_count = 0;
  int connection_change_count = 0;

 private:
  mojo::Receiver<mojom::SnapsServiceObserver> receiver_{this};
};

}  // namespace

class SnapsServiceTest : public testing::Test {
 public:
  void SetUp() override {
    RegisterProfilePrefs(prefs_.registry());
    RegisterProfilePrefsForMigration(prefs_.registry());
    RegisterLocalStatePrefs(local_state_.registry());
    RegisterLocalStatePrefsForMigration(local_state_.registry());

    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    network_manager_ = std::make_unique<NetworkManager>(&prefs_);
    json_rpc_service_ = std::make_unique<JsonRpcService>(
        url_loader_factory_.GetSafeWeakWrapper(), network_manager_.get(),
        &prefs_, nullptr);
    keyring_service_ = std::make_unique<KeyringService>(json_rpc_service_.get(),
                                                        &prefs_, &local_state_);
    // browser_context = nullptr selects the WalletPageSnapBridgeController path,
    // so no WebContents is needed.
    service_ = std::make_unique<SnapsService>(
        *keyring_service_, prefs_, temp_dir_.GetPath(),
        url_loader_factory_.GetSafeWeakWrapper(),
        /*browser_context=*/nullptr,
        /*open_wallet_page=*/base::DoNothing(),
        /*open_snap_host_tab=*/base::DoNothing());
  }

 protected:
  // Serves npm metadata + tarball responses for a valid snap.
  void RegisterValidSnap(const std::string& snap_id,
                         const std::string& version = "1.0.0",
                         bool allow_dapps = false) {
    const std::string bundle = "export const onRpcRequest = () => 1;";
    TestSnapManifestOptions options;
    options.shasum = ComputeSnapBundleShasum(bundle);
    if (allow_dapps) {
      options.include_rpc_endowment = true;
      options.allow_dapps = true;
    }
    const std::string package = snap_id.substr(4);  // strip "npm:"
    url_loader_factory_.AddResponse(
        "https://registry.npmjs.org/" + package + "/" + version,
        MakeNpmRegistryMetadataJson("https://registry.npmjs.org/" + package +
                                    "/-/pkg.tgz"));
    url_loader_factory_.AddResponse(
        "https://registry.npmjs.org/" + package + "/-/pkg.tgz",
        BuildSnapTarball(MakeSnapManifestJson(options), bundle,
                         options.bundle_file_path));
  }

  mojom::PendingSnapInstallPtr GetPending() {
    base::test::TestFuture<mojom::PendingSnapInstallPtr> future;
    service_->GetPendingSnapInstall(future.GetCallback());
    return future.Take();
  }

  mojom::SnapInstallState State() { return GetPending()->state; }

  void RequestInstall(const std::string& snap_id,
                      const std::string& version,
                      bool* out_success = nullptr,
                      std::optional<std::string>* out_error = nullptr) {
    bool success = false;
    std::optional<std::string> error;
    service_->RequestInstallSnap(
        snap_id, version,
        base::BindLambdaForTesting(
            [&](bool s, const std::optional<std::string>& e) {
              success = s;
              error = e;
            }));
    if (out_success) {
      *out_success = success;
    }
    if (out_error) {
      *out_error = error;
    }
  }

  void NotifyProcessed(bool approved) {
    base::test::TestFuture<void> future;
    service_->NotifySnapInstallRequestProcessed(approved, future.GetCallback());
    ASSERT_TRUE(future.Wait());
  }

  void InstallFully(const std::string& snap_id,
                    const std::string& version = "1.0.0",
                    bool allow_dapps = false) {
    RegisterValidSnap(snap_id, version, allow_dapps);
    RequestInstall(snap_id, version);
    task_environment_.RunUntilIdle();
    ASSERT_EQ(State(), mojom::SnapInstallState::kPendingApproval);
    NotifyProcessed(true);
    task_environment_.RunUntilIdle();
    ASSERT_TRUE(service_->IsSnapAvailable(snap_id));
  }

  std::optional<std::string> GetBundle(const std::string& snap_id) {
    base::test::TestFuture<std::optional<std::string>,
                           std::optional<std::string>>
        future;
    service_->GetSnapBundle(
        snap_id, future.GetCallback<const std::optional<std::string>&,
                                    const std::optional<std::string>&>());
    return future.Get<0>();
  }

  base::test::TaskEnvironment task_environment_{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME};
  base::ScopedTempDir temp_dir_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  TestingPrefServiceSimple local_state_;
  network::TestURLLoaderFactory url_loader_factory_;
  std::unique_ptr<NetworkManager> network_manager_;
  std::unique_ptr<JsonRpcService> json_rpc_service_;
  std::unique_ptr<KeyringService> keyring_service_;
  std::unique_ptr<SnapsService> service_;
};

TEST_F(SnapsServiceTest, RequestInstallApproveFlow) {
  RegisterValidSnap("npm:test-snap", "1.0.0");

  bool success = false;
  std::optional<std::string> error;
  RequestInstall("npm:test-snap", "1.0.0", &success, &error);
  EXPECT_TRUE(success);
  EXPECT_FALSE(error);

  task_environment_.RunUntilIdle();
  ASSERT_EQ(State(), mojom::SnapInstallState::kPendingApproval);
  auto pending = GetPending();
  ASSERT_TRUE(pending->install_data);
  EXPECT_EQ(pending->install_data->snap_id, "npm:test-snap");

  NotifyProcessed(/*approved=*/true);
  task_environment_.RunUntilIdle();
  EXPECT_TRUE(service_->IsSnapAvailable("npm:test-snap"));
  EXPECT_EQ(State(), mojom::SnapInstallState::kSuccess);

  // kSuccess auto-clears to kIdle after a short delay.
  task_environment_.FastForwardBy(base::Seconds(2));
  EXPECT_EQ(State(), mojom::SnapInstallState::kIdle);
}

TEST_F(SnapsServiceTest, RequestInstallRejectFlow) {
  RegisterValidSnap("npm:test-snap", "1.0.0");
  RequestInstall("npm:test-snap", "1.0.0");
  task_environment_.RunUntilIdle();
  ASSERT_EQ(State(), mojom::SnapInstallState::kPendingApproval);

  NotifyProcessed(/*approved=*/false);
  EXPECT_EQ(State(), mojom::SnapInstallState::kIdle);
  EXPECT_FALSE(service_->IsSnapAvailable("npm:test-snap"));
}

TEST_F(SnapsServiceTest, RequestInstallAlreadyPending) {
  RegisterValidSnap("npm:test-snap", "1.0.0");
  bool first_success = false;
  RequestInstall("npm:test-snap", "1.0.0", &first_success);
  EXPECT_TRUE(first_success);

  // A second request while one is in flight is rejected.
  bool second_success = true;
  std::optional<std::string> second_error;
  RequestInstall("npm:other-snap", "1.0.0", &second_success, &second_error);
  EXPECT_FALSE(second_success);
  EXPECT_EQ(second_error, "already_pending");
}

TEST_F(SnapsServiceTest, RequestInstallFailureGoesToFailed) {
  url_loader_factory_.AddResponse("https://registry.npmjs.org/test-snap/1.0.0",
                                  "", net::HTTP_NOT_FOUND);
  RequestInstall("npm:test-snap", "1.0.0");
  task_environment_.RunUntilIdle();

  ASSERT_EQ(State(), mojom::SnapInstallState::kFailed);
  EXPECT_TRUE(GetPending()->error);

  // Dismissing a failed install returns to idle.
  NotifyProcessed(/*approved=*/false);
  EXPECT_EQ(State(), mojom::SnapInstallState::kIdle);
}

TEST_F(SnapsServiceTest, ObserverNotifiedOnStateChange) {
  FakeSnapsServiceObserver observer;
  service_->AddObserver(observer.BindNewPipeAndPassRemote());

  RegisterValidSnap("npm:test-snap", "1.0.0");
  RequestInstall("npm:test-snap", "1.0.0");
  task_environment_.RunUntilIdle();

  // At least kInstalling + kPendingApproval transitions were observed.
  EXPECT_GE(observer.change_count, 2);
}

TEST_F(SnapsServiceTest, AccessorsAfterInstall) {
  InstallFully("npm:test-snap");

  EXPECT_TRUE(service_->IsSnapAvailable("npm:test-snap"));
  EXPECT_EQ(service_->GetAllSnaps().size(), 1u);

  base::test::TestFuture<std::vector<mojom::SnapInstallDataPtr>> installed;
  service_->GetInstalledSnaps(installed.GetCallback());
  EXPECT_EQ(installed.Get().size(), 1u);

  base::test::TestFuture<mojom::SnapManifestPtr> manifest;
  service_->GetSnapManifest("npm:test-snap", manifest.GetCallback());
  EXPECT_TRUE(manifest.Get());

  auto bundle = GetBundle("npm:test-snap");
  ASSERT_TRUE(bundle);
  EXPECT_EQ(*bundle, "export const onRpcRequest = () => 1;");
}

TEST_F(SnapsServiceTest, GrantAndQueryConnection) {
  InstallFully("npm:test-snap");
  const url::Origin origin =
      url::Origin::Create(GURL("https://app.example.com"));
  EXPECT_FALSE(service_->IsSnapConnected(origin, "npm:test-snap"));
  service_->GrantSnapConnection(origin, "npm:test-snap");
  EXPECT_TRUE(service_->IsSnapConnected(origin, "npm:test-snap"));
}

TEST_F(SnapsServiceTest, GetSnapsForOrigin) {
  InstallFully("npm:test-snap", "1.0.0", /*allow_dapps=*/true);
  const url::Origin origin =
      url::Origin::Create(GURL("https://app.example.com"));

  base::DictValue snaps = service_->GetSnapsForOrigin(origin);
  EXPECT_TRUE(snaps.contains("npm:test-snap"));

  // Opaque / non-web origins see nothing.
  EXPECT_TRUE(service_->GetSnapsForOrigin(url::Origin()).empty());
}

TEST_F(SnapsServiceTest, UninstallSnapRemovesAndPurges) {
  InstallFully("npm:test-snap");
  const url::Origin origin =
      url::Origin::Create(GURL("https://app.example.com"));
  service_->GrantSnapConnection(origin, "npm:test-snap");
  ASSERT_TRUE(service_->IsSnapConnected(origin, "npm:test-snap"));

  base::test::TestFuture<void> uninstall;
  service_->UninstallSnap("npm:test-snap", uninstall.GetCallback());
  ASSERT_TRUE(uninstall.Wait());

  EXPECT_FALSE(service_->IsSnapAvailable("npm:test-snap"));
  EXPECT_FALSE(service_->IsSnapConnected(origin, "npm:test-snap"));
  task_environment_.RunUntilIdle();  // Flush async bundle deletion.
}

TEST_F(SnapsServiceTest, GetSnapManifestUnknownReturnsNull) {
  base::test::TestFuture<mojom::SnapManifestPtr> manifest;
  service_->GetSnapManifest("npm:unknown", manifest.GetCallback());
  EXPECT_FALSE(manifest.Get());
}

TEST_F(SnapsServiceTest, GetSnapBundleUnknownReturnsError) {
  base::test::TestFuture<std::optional<std::string>, std::optional<std::string>>
      future;
  service_->GetSnapBundle(
      "npm:unknown", future.GetCallback<const std::optional<std::string>&,
                                        const std::optional<std::string>&>());
  EXPECT_FALSE(future.Get<0>());
  EXPECT_EQ(future.Get<1>(), "Bundle not found");
}

TEST_F(SnapsServiceTest, InvokeSnapInvalidParamsJson) {
  base::test::TestFuture<std::optional<std::string>, std::optional<std::string>>
      future;
  service_->InvokeSnap(
      "npm:test-snap", "foo", "this is not json",
      future.GetCallback<const std::optional<std::string>&,
                         const std::optional<std::string>&>());
  EXPECT_FALSE(future.Get<0>());
  EXPECT_EQ(future.Get<1>(), "Invalid params JSON");
}

}  // namespace brave_wallet
