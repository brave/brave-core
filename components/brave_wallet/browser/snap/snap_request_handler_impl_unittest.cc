/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/snap/snap_request_handler_impl.h"

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/files/scoped_temp_dir.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/network_manager.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/browser/snap/snap_permission_controller.h"
#include "brave/components/brave_wallet/browser/snap/snap_test_utils.h"
#include "brave/components/brave_wallet/browser/snap/storage/snap_data_provider.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "components/prefs/testing_pref_service.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

namespace {
using ReqFuture = base::test::TestFuture<std::optional<base::Value>,
                                         std::optional<std::string>>;
constexpr char kSnapId[] = "npm:test-snap";
}  // namespace

class SnapRequestHandlerImplTest : public testing::Test {
 public:
  void SetUp() override {
    // RegisterProfilePrefs already registers the snap registry / connection
    // prefs, so they must not be registered again here.
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
    data_provider_ =
        std::make_unique<SnapDataProvider>(temp_dir_.GetPath(), prefs_);
    permission_controller_ =
        std::make_unique<SnapPermissionController>(prefs_, *data_provider_);
    handler_ = std::make_unique<SnapRequestHandlerImpl>(
        *keyring_service_, *data_provider_, *permission_controller_);
  }

 protected:
  void InstallSnap(const std::string& snap_id,
                   std::vector<std::string> permissions) {
    auto data = mojom::SnapInstallData::New();
    data->snap_id = snap_id;
    data->version = "1.0.0";
    data->manifest = MakeTestSnapManifest(std::move(permissions));
    data->enabled = true;
    base::test::TestFuture<void> future;
    data_provider_->OnSnapInstalled(*data, future.GetCallback());
    ASSERT_TRUE(future.Wait());
  }

  void SetSnapEnabled(const std::string& snap_id, bool enabled) {
    base::test::TestFuture<bool> future;
    data_provider_->SetSnapEnabled(snap_id, enabled, future.GetCallback());
    ASSERT_TRUE(future.Get());
  }

  base::Value StateParams(const std::string& operation,
                          std::optional<std::string> new_state = std::nullopt) {
    base::DictValue dict;
    dict.Set("operation", operation);
    if (new_state) {
      dict.Set("newStateJson", *new_state);
    }
    return base::Value(std::move(dict));
  }

  base::test::TaskEnvironment task_environment_;
  base::ScopedTempDir temp_dir_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  TestingPrefServiceSimple local_state_;
  network::TestURLLoaderFactory url_loader_factory_;
  std::unique_ptr<NetworkManager> network_manager_;
  std::unique_ptr<JsonRpcService> json_rpc_service_;
  std::unique_ptr<KeyringService> keyring_service_;
  std::unique_ptr<SnapDataProvider> data_provider_;
  std::unique_ptr<SnapPermissionController> permission_controller_;
  std::unique_ptr<SnapRequestHandlerImpl> handler_;
};

TEST_F(SnapRequestHandlerImplTest, UnknownSnapRejected) {
  ReqFuture future;
  handler_->HandleSnapRequest(
      "npm:nope", "snap_dialog", base::Value(),
      future.GetCallback<std::optional<base::Value>,
                         const std::optional<std::string>&>());
  EXPECT_FALSE(future.Get<0>());
  EXPECT_EQ(future.Get<1>(), "Unknown snap: npm:nope");
}

TEST_F(SnapRequestHandlerImplTest, DisabledSnapRejected) {
  InstallSnap(kSnapId, {"snap_dialog"});
  SetSnapEnabled(kSnapId, false);

  ReqFuture future;
  handler_->HandleSnapRequest(
      kSnapId, "snap_dialog", base::Value(),
      future.GetCallback<std::optional<base::Value>,
                         const std::optional<std::string>&>());
  EXPECT_FALSE(future.Get<0>());
  EXPECT_EQ(future.Get<1>(), "Snap is disabled");
}

TEST_F(SnapRequestHandlerImplTest, MissingPermissionRejected) {
  InstallSnap(kSnapId, {"snap_dialog"});
  ReqFuture future;
  handler_->HandleSnapRequest(
      kSnapId, "snap_notify", base::Value(),
      future.GetCallback<std::optional<base::Value>,
                         const std::optional<std::string>&>());
  EXPECT_FALSE(future.Get<0>());
  EXPECT_THAT(*future.Get<1>(), testing::HasSubstr("does not have permission"));
}

TEST_F(SnapRequestHandlerImplTest, DialogReturnsTrue) {
  InstallSnap(kSnapId, {"snap_dialog"});
  ReqFuture future;
  handler_->HandleSnapRequest(
      kSnapId, "snap_dialog", base::Value(),
      future.GetCallback<std::optional<base::Value>,
                         const std::optional<std::string>&>());
  ASSERT_TRUE(future.Get<0>());
  EXPECT_EQ(*future.Get<0>(), base::Value(true));
  EXPECT_FALSE(future.Get<1>());
}

TEST_F(SnapRequestHandlerImplTest, ConfirmAliasesToDialog) {
  // snap_confirm requires the snap_dialog permission and also returns true.
  InstallSnap(kSnapId, {"snap_dialog"});
  ReqFuture future;
  handler_->HandleSnapRequest(
      kSnapId, "snap_confirm", base::Value(),
      future.GetCallback<std::optional<base::Value>,
                         const std::optional<std::string>&>());
  ASSERT_TRUE(future.Get<0>());
  EXPECT_EQ(*future.Get<0>(), base::Value(true));
  EXPECT_FALSE(future.Get<1>());
}

TEST_F(SnapRequestHandlerImplTest, NotifyReturnsNoResultNoError) {
  InstallSnap(kSnapId, {"snap_notify"});
  ReqFuture future;
  handler_->HandleSnapRequest(
      kSnapId, "snap_notify", base::Value(),
      future.GetCallback<std::optional<base::Value>,
                         const std::optional<std::string>&>());
  EXPECT_FALSE(future.Get<0>());
  EXPECT_FALSE(future.Get<1>());
}

TEST_F(SnapRequestHandlerImplTest, UnsupportedMethodRejected) {
  // snap_getBip32Entropy is declared in the manifest but not implemented in the
  // handler, so the permission gate passes and the unsupported-method branch
  // runs.
  InstallSnap(kSnapId, {"snap_getBip32Entropy"});
  ReqFuture future;
  handler_->HandleSnapRequest(
      kSnapId, "snap_getBip32Entropy", base::Value(),
      future.GetCallback<std::optional<base::Value>,
                         const std::optional<std::string>&>());
  EXPECT_FALSE(future.Get<0>());
  EXPECT_EQ(future.Get<1>(), "Unsupported snap method: snap_getBip32Entropy");
}

TEST_F(SnapRequestHandlerImplTest, GetEntropyParamsMustBeDict) {
  InstallSnap(kSnapId, {"snap_getEntropy"});
  ReqFuture future;
  handler_->HandleSnapRequest(
      kSnapId, "snap_getEntropy", base::Value("not-a-dict"),
      future.GetCallback<std::optional<base::Value>,
                         const std::optional<std::string>&>());
  EXPECT_EQ(future.Get<1>(), "snap_getEntropy: params must be a dict");
}

TEST_F(SnapRequestHandlerImplTest, GetEntropyVersionMustBe1) {
  InstallSnap(kSnapId, {"snap_getEntropy"});
  // Missing version.
  {
    ReqFuture future;
    handler_->HandleSnapRequest(
        kSnapId, "snap_getEntropy", base::Value(base::DictValue()),
        future.GetCallback<std::optional<base::Value>,
                           const std::optional<std::string>&>());
    EXPECT_EQ(future.Get<1>(), "snap_getEntropy: version must be 1");
  }
  // Wrong version.
  {
    base::DictValue params;
    params.Set("version", 2);
    ReqFuture future;
    handler_->HandleSnapRequest(
        kSnapId, "snap_getEntropy", base::Value(std::move(params)),
        future.GetCallback<std::optional<base::Value>,
                           const std::optional<std::string>&>());
    EXPECT_EQ(future.Get<1>(), "snap_getEntropy: version must be 1");
  }
}

// As with snap_getBip44Entropy there is no "locked wallet" test:
// KeyringService::GetEntropyForSnap DCHECKs on encryptor_, so the happy path is
// exercised with an unlocked wallet. The derived entropy must be a 0x-prefixed
// 32-byte (64 hex char) string, deterministic for a given snap id + salt, and
// distinct when a salt is supplied.
TEST_F(SnapRequestHandlerImplTest, GetEntropyUnlockedWalletSucceeds) {
  AccountUtils(keyring_service_.get())
      .CreateWallet(kMnemonicDivideCruise, "brave123");
  InstallSnap(kSnapId, {"snap_getEntropy"});

  auto request = [&](std::optional<std::string> salt) -> std::string {
    base::DictValue params;
    params.Set("version", 1);
    if (salt) {
      params.Set("salt", *salt);
    }
    ReqFuture future;
    handler_->HandleSnapRequest(
        kSnapId, "snap_getEntropy", base::Value(std::move(params)),
        future.GetCallback<std::optional<base::Value>,
                           const std::optional<std::string>&>());
    EXPECT_FALSE(future.Get<1>());
    EXPECT_TRUE(future.Get<0>() && future.Get<0>()->is_string());
    return future.Get<0>()->is_string() ? future.Get<0>()->GetString()
                                        : std::string();
  };

  const std::string no_salt = request(std::nullopt);
  // "0x" + 64 hex chars for a 32-byte private key.
  EXPECT_EQ(no_salt.size(), 66u);
  EXPECT_TRUE(no_salt.starts_with("0x"));

  // Deterministic: same snap id + (no) salt yields the same entropy.
  EXPECT_EQ(no_salt, request(std::nullopt));

  // A salt produces unrelated entropy.
  const std::string salted = request("foo");
  EXPECT_EQ(salted.size(), 66u);
  EXPECT_NE(no_salt, salted);
}

// Byte-exact compatibility with MetaMask's SIP-6 reference implementation.
// Vector from @metamask/snaps-rpc-methods getEntropy.test.ts:
//   mnemonic = "test test test test test test test test test test test ball"
//   origin   = "npm:@metamask/example-snap" (the snap id / derivation input)
//   salt     = "foo"
//   result   =
//   0x6d8e92de419401c7da3cedd5f60ce5635b26059c2a4a8003877fec83653a4921
TEST_F(SnapRequestHandlerImplTest, GetEntropyMatchesMetaMaskReferenceVector) {
  constexpr char kMetaMaskTestMnemonic[] =
      "test test test test test test test test test test test ball";
  constexpr char kMetaMaskSnapId[] = "npm:@metamask/example-snap";
  AccountUtils(keyring_service_.get())
      .CreateWallet(kMetaMaskTestMnemonic, "brave123");
  InstallSnap(kMetaMaskSnapId, {"snap_getEntropy"});

  base::DictValue params;
  params.Set("version", 1);
  params.Set("salt", "foo");
  ReqFuture future;
  handler_->HandleSnapRequest(
      kMetaMaskSnapId, "snap_getEntropy", base::Value(std::move(params)),
      future.GetCallback<std::optional<base::Value>,
                         const std::optional<std::string>&>());
  ASSERT_TRUE(future.Get<0>() && future.Get<0>()->is_string());
  EXPECT_EQ(
      future.Get<0>()->GetString(),
      "0x6d8e92de419401c7da3cedd5f60ce5635b26059c2a4a8003877fec83653a4921");
}

TEST_F(SnapRequestHandlerImplTest, GetBip44ParamsMustBeDict) {
  InstallSnap(kSnapId, {"snap_getBip44Entropy"});
  ReqFuture future;
  handler_->HandleSnapRequest(
      kSnapId, "snap_getBip44Entropy", base::Value("not-a-dict"),
      future.GetCallback<std::optional<base::Value>,
                         const std::optional<std::string>&>());
  EXPECT_EQ(future.Get<1>(), "snap_getBip44Entropy: params must be a dict");
}

TEST_F(SnapRequestHandlerImplTest, GetBip44MissingCoinType) {
  InstallSnap(kSnapId, {"snap_getBip44Entropy"});
  ReqFuture future;
  handler_->HandleSnapRequest(
      kSnapId, "snap_getBip44Entropy", base::Value(base::DictValue()),
      future.GetCallback<std::optional<base::Value>,
                         const std::optional<std::string>&>());
  EXPECT_EQ(future.Get<1>(),
            "snap_getBip44Entropy: missing or invalid coinType");
}

// NOTE: there is no "locked wallet" test for snap_getBip44Entropy.
// KeyringService::GetBip44EntropyForSnap DCHECKs on encryptor_, so calling it
// without a created wallet crashes in DCHECK builds rather than returning the
// "key derivation failed" error. In practice the snap execution environment
// only runs while the wallet is unlocked, so this path is exercised via the
// success test below.
TEST_F(SnapRequestHandlerImplTest, GetBip44UnlockedWalletSucceeds) {
  AccountUtils(keyring_service_.get())
      .CreateWallet(kMnemonicDivideCruise, "brave123");
  InstallSnap(kSnapId, {"snap_getBip44Entropy"});

  base::DictValue params;
  params.Set("coinType", 60);
  ReqFuture future;
  handler_->HandleSnapRequest(
      kSnapId, "snap_getBip44Entropy", base::Value(std::move(params)),
      future.GetCallback<std::optional<base::Value>,
                         const std::optional<std::string>&>());
  EXPECT_TRUE(future.Get<0>());
  EXPECT_FALSE(future.Get<1>());
}

TEST_F(SnapRequestHandlerImplTest, ManageStateGetReturnsNullWhenEmpty) {
  InstallSnap(kSnapId, {"snap_manageState"});
  ReqFuture future;
  handler_->HandleSnapRequest(
      kSnapId, "snap_manageState", StateParams("get"),
      future.GetCallback<std::optional<base::Value>,
                         const std::optional<std::string>&>());
  ASSERT_TRUE(future.Get<0>());
  EXPECT_EQ(*future.Get<0>(), base::Value("null"));
  EXPECT_FALSE(future.Get<1>());
}

TEST_F(SnapRequestHandlerImplTest, ManageStateUpdateThenGet) {
  InstallSnap(kSnapId, {"snap_manageState"});
  {
    ReqFuture future;
    handler_->HandleSnapRequest(
        kSnapId, "snap_manageState", StateParams("update", R"({"a":1})"),
        future.GetCallback<std::optional<base::Value>,
                           const std::optional<std::string>&>());
    ASSERT_TRUE(future.Get<0>());
    EXPECT_EQ(*future.Get<0>(), base::Value("null"));
    EXPECT_FALSE(future.Get<1>());
  }
  {
    ReqFuture future;
    handler_->HandleSnapRequest(
        kSnapId, "snap_manageState", StateParams("get"),
        future.GetCallback<std::optional<base::Value>,
                           const std::optional<std::string>&>());
    ASSERT_TRUE(future.Get<0>());
    EXPECT_EQ(*future.Get<0>(), base::Value(R"({"a":1})"));
  }
}

TEST_F(SnapRequestHandlerImplTest, ManageStateClear) {
  InstallSnap(kSnapId, {"snap_manageState"});
  {
    ReqFuture future;
    handler_->HandleSnapRequest(
        kSnapId, "snap_manageState", StateParams("update", R"({"a":1})"),
        future.GetCallback<std::optional<base::Value>,
                           const std::optional<std::string>&>());
    ASSERT_TRUE(future.Get<0>());
  }
  {
    ReqFuture future;
    handler_->HandleSnapRequest(
        kSnapId, "snap_manageState", StateParams("clear"),
        future.GetCallback<std::optional<base::Value>,
                           const std::optional<std::string>&>());
    ASSERT_TRUE(future.Get<0>());
    EXPECT_EQ(*future.Get<0>(), base::Value("null"));
  }
  {
    ReqFuture future;
    handler_->HandleSnapRequest(
        kSnapId, "snap_manageState", StateParams("get"),
        future.GetCallback<std::optional<base::Value>,
                           const std::optional<std::string>&>());
    ASSERT_TRUE(future.Get<0>());
    EXPECT_EQ(*future.Get<0>(), base::Value("null"));
  }
}

TEST_F(SnapRequestHandlerImplTest, ManageStateMissingOperation) {
  InstallSnap(kSnapId, {"snap_manageState"});
  ReqFuture future;
  handler_->HandleSnapRequest(
      kSnapId, "snap_manageState", base::Value(base::DictValue()),
      future.GetCallback<std::optional<base::Value>,
                         const std::optional<std::string>&>());
  EXPECT_EQ(future.Get<1>(), "snap_manageState: missing operation");
}

TEST_F(SnapRequestHandlerImplTest, ManageStateMissingNewStateJson) {
  InstallSnap(kSnapId, {"snap_manageState"});
  ReqFuture future;
  handler_->HandleSnapRequest(
      kSnapId, "snap_manageState", StateParams("update"),
      future.GetCallback<std::optional<base::Value>,
                         const std::optional<std::string>&>());
  EXPECT_EQ(future.Get<1>(), "snap_manageState: missing newStateJson");
}

TEST_F(SnapRequestHandlerImplTest, ManageStateUnknownOperation) {
  InstallSnap(kSnapId, {"snap_manageState"});
  ReqFuture future;
  handler_->HandleSnapRequest(
      kSnapId, "snap_manageState", StateParams("frobnicate"),
      future.GetCallback<std::optional<base::Value>,
                         const std::optional<std::string>&>());
  EXPECT_EQ(future.Get<1>(), "snap_manageState: unknown operation: frobnicate");
}

}  // namespace brave_wallet
