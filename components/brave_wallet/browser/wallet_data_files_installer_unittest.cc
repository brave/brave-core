/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/wallet_data_files_installer.h"

#include <optional>
#include <utility>

#include "base/files/file_path.h"
#include "base/files/scoped_temp_dir.h"
#include "base/run_loop.h"
#include "base/test/bind.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "base/time/time.h"
#include "brave/components/brave_component_updater/browser/mock_on_demand_updater.h"
#include "brave/components/brave_wallet/browser/blockchain_registry.h"
#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service_delegate.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "brave/components/brave_wallet/browser/tx_service.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/features.h"
#include "components/component_updater/mock_component_updater_service.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "services/data_decoder/public/cpp/test_support/in_process_data_decoder.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

#define FPL(x) FILE_PATH_LITERAL(x)

namespace brave_wallet {

namespace {

constexpr char kComponentId[] = "bbckkcdiepaecefgfnibemejliemjnio";

class MockWalletDataFilesInstallerDelegateImpl
    : public WalletDataFilesInstallerDelegate {
 public:
  MockWalletDataFilesInstallerDelegateImpl() = default;

  explicit MockWalletDataFilesInstallerDelegateImpl(
      component_updater::ComponentUpdateService* cus)
      : cus_(cus) {}

  ~MockWalletDataFilesInstallerDelegateImpl() override = default;

  component_updater::ComponentUpdateService* GetComponentUpdater() override {
    return cus_.get();
  }

 private:
  raw_ptr<component_updater::ComponentUpdateService> cus_;
};

class MockBraveWalletServiceDelegateImpl : public BraveWalletServiceDelegate {
 public:
  MockBraveWalletServiceDelegateImpl() = default;
  ~MockBraveWalletServiceDelegateImpl() override = default;

  using GetImportInfoCallback =
      base::OnceCallback<void(bool, ImportInfo, ImportError)>;
  void GetImportInfoFromExternalWallet(
      mojom::ExternalWalletType type,
      const std::string& password,
      GetImportInfoCallback callback) override {
    std::move(callback).Run(true, ImportInfo({kMnemonicDivideCruise, false, 1}),
                            ImportError::kNone);
  }
};

}  // namespace

class WalletDataFilesInstallerUnitTest : public testing::Test {
 public:
  WalletDataFilesInstallerUnitTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME),
        shared_url_loader_factory_(
            base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                &url_loader_factory_)) {}

  ~WalletDataFilesInstallerUnitTest() override = default;

  void SetUp() override {
    base::test::ScopedFeatureList feature_list;
    feature_list.InitAndEnableFeature(
        brave_wallet::features::kNativeBraveWalletFeature);

    RegisterProfilePrefs(prefs_.registry());
    RegisterLocalStatePrefs(local_state_.registry());
    RegisterProfilePrefsForMigration(prefs_.registry());
    RegisterLocalStatePrefsForMigration(local_state_.registry());

    keyring_service_ =
        std::make_unique<KeyringService>(nullptr, &prefs_, &local_state_);
    json_rpc_service_ = std::make_unique<brave_wallet::JsonRpcService>(
        shared_url_loader_factory_, &prefs_);
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    tx_service_ = std::make_unique<TxService>(
        json_rpc_service_.get(),
        nullptr,  // BitcoinWalletService
        nullptr,  // ZCashWalletService
        keyring_service_.get(), &prefs_, temp_dir_.GetPath(),
        base::SequencedTaskRunner::GetCurrentDefault());
    brave_wallet_service_ = std::make_unique<BraveWalletService>(
        shared_url_loader_factory_,
        std::make_unique<MockBraveWalletServiceDelegateImpl>(),
        keyring_service_.get(), json_rpc_service_.get(), tx_service_.get(),
        nullptr, nullptr, &prefs_, &local_state_, false);

    cus_ = std::make_unique<component_updater::MockComponentUpdateService>();
    installer().SetDelegate(
        std::make_unique<MockWalletDataFilesInstallerDelegateImpl>(cus_.get()));

    url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [&](const network::ResourceRequest& request) {
          url_loader_factory_.ClearResponses();
          url_loader_factory_.AddResponse(request.url.spec(), "",
                                          net::HTTP_REQUEST_TIMEOUT);
        }));

    ASSERT_TRUE(install_dir_.CreateUniqueTempDir());
  }

  void TearDown() override {
    installer().ResetForTesting();
    registry()->ResetForTesting();
  }

  void WriteCoingeckoIdsMapToFile() {
    const std::string coingecko_ids_map_json = R"({
      "0xa": {
        "0x7f5c764cbc14f9669b88837ca1490cca17c31607": "usd-coin"
      }
    })";
    ASSERT_TRUE(base::WriteFile(install_dir().Append(FPL("coingecko-ids.json")),
                                coingecko_ids_map_json));
  }

  void CreateWallet() {
    base::RunLoop run_loop;
    keyring_service_->CreateWallet(
        kMnemonicDivideCruise, kTestWalletPassword,
        base::BindLambdaForTesting([&run_loop](const std::string& mnemonic) {
          ASSERT_FALSE(mnemonic.empty());
          run_loop.Quit();
        }));
    run_loop.Run();
  }

  void RestoreWallet() {
    base::RunLoop run_loop;
    keyring_service_->RestoreWallet(
        kMnemonicDivideCruise, kTestWalletPassword, false,
        base::BindLambdaForTesting([&](bool success) {
          ASSERT_TRUE(success);
          run_loop.Quit();
        }));
    run_loop.Run();
  }

  void ImportFromExternalWallet() {
    base::RunLoop run_loop;
    brave_wallet_service_->ImportFromExternalWallet(
        mojom::ExternalWalletType::MetaMask, kTestWalletPassword,
        kTestWalletPassword,
        base::BindLambdaForTesting(
            [&run_loop](bool success, const std::optional<std::string>& err) {
              ASSERT_TRUE(success);
              ASSERT_FALSE(err);
              run_loop.Quit();
            }));
    run_loop.Run();
  }

  PrefService* local_state() { return &local_state_; }
  base::FilePath install_dir() { return install_dir_.GetPath(); }
  component_updater::MockComponentUpdateService* updater() {
    return cus_.get();
  }

  WalletDataFilesInstaller& installer() {
    return WalletDataFilesInstaller::GetInstance();
  }
  BlockchainRegistry* registry() { return BlockchainRegistry::GetInstance(); }

  void SetOnDemandUpdateCallbackWithComponentReady(const base::FilePath& path) {
    EXPECT_CALL(on_demand_updater_,
                OnDemandUpdate(kComponentId, testing::_, testing::_))
        .WillOnce(
            [path, this](const std::string& id,
                         component_updater::OnDemandUpdater::Priority priority,
                         component_updater::Callback callback) {
              // Unblock CreateWallet once the component is registered.
              installer().OnComponentReady(path);
            });
  }

  void SetOnDemandUpdateCallbackWithComponentUpdateError() {
    EXPECT_CALL(on_demand_updater_,
                OnDemandUpdate(kComponentId, testing::_, testing::_))
        .WillOnce([this](const std::string& id,
                         component_updater::OnDemandUpdater::Priority priority,
                         component_updater::Callback callback) {
          installer().OnEvent(update_client::UpdateClient::Observer::Events::
                                  COMPONENT_UPDATE_ERROR,
                              kComponentId);
        });
  }

 protected:
  void RunUntilIdle() {
    task_environment_.RunUntilIdle();
    base::RunLoop().RunUntilIdle();
  }

 private:
  base::test::TaskEnvironment task_environment_;
  brave_component_updater::MockOnDemandUpdater on_demand_updater_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  sync_preferences::TestingPrefServiceSyncable local_state_;
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  data_decoder::test::InProcessDataDecoder in_process_data_decoder_;
  std::unique_ptr<KeyringService> keyring_service_;
  std::unique_ptr<JsonRpcService> json_rpc_service_;
  std::unique_ptr<TxService> tx_service_;
  std::unique_ptr<BraveWalletService> brave_wallet_service_;
  std::unique_ptr<component_updater::MockComponentUpdateService> cus_;
  base::ScopedTempDir install_dir_;
  base::ScopedTempDir temp_dir_;
};

TEST_F(WalletDataFilesInstallerUnitTest,
       MaybeRegisterWalletDataFilesComponent_NoRegisterWithoutWallets) {
  EXPECT_CALL(*updater(), RegisterComponent(testing::_)).Times(0);
  installer().MaybeRegisterWalletDataFilesComponent(updater(), local_state());
  RunUntilIdle();
}

TEST_F(WalletDataFilesInstallerUnitTest,
       MaybeRegisterWalletDataFilesComponent_RegisterWithWallets) {
  EXPECT_CALL(*updater(), RegisterComponent(testing::_))
      .Times(1)
      .WillOnce(testing::Return(true));
  // Mimic created wallets.
  local_state()->SetTime(kBraveWalletLastUnlockTime, base::Time::Now());
  installer().MaybeRegisterWalletDataFilesComponent(updater(), local_state());
  RunUntilIdle();
}

TEST_F(WalletDataFilesInstallerUnitTest, OnDemandInstallAndParsing_EmptyPath) {
  EXPECT_CALL(*updater(), RegisterComponent(testing::_))
      .Times(1)
      .WillOnce(testing::Return(true));
  SetOnDemandUpdateCallbackWithComponentReady(base::FilePath());
  RunUntilIdle();
  CreateWallet();

  RunUntilIdle();
  EXPECT_TRUE(registry()->IsEmptyForTesting());
}

TEST_F(WalletDataFilesInstallerUnitTest,
       OnDemandInstallAndParsing_FileNotFound) {
  EXPECT_CALL(*updater(), RegisterComponent(testing::_))
      .Times(1)
      .WillOnce(testing::Return(true));
  SetOnDemandUpdateCallbackWithComponentReady(install_dir());
  CreateWallet();

  RunUntilIdle();
  EXPECT_TRUE(registry()->IsEmptyForTesting());
}

// This test case covers: 1) Normal JSON file parsing, 2) Parse JSON file
// failed, 3) Cannot find file. CreateWallet should be done still with.
// CreateWallet call should be resolved, and blockchain registry will have
// data where parsing is successful.
TEST_F(WalletDataFilesInstallerUnitTest,
       OnDemandInstallAndParsing_ParseJsonFiles) {
  EXPECT_CALL(*updater(), RegisterComponent(testing::_))
      .Times(1)
      .WillOnce(testing::Return(true));
  SetOnDemandUpdateCallbackWithComponentReady(install_dir());

  WriteCoingeckoIdsMapToFile();
  ASSERT_TRUE(
      base::WriteFile(install_dir().Append(FPL("contract-map.json")), "bad"));

  const std::string ofac_list_json = R"({
    "addresses": [
      "0xb9ef770b6a5e12e45983c5d80545258aa38f3b78"
    ]
  })";
  ASSERT_TRUE(base::WriteFile(
      install_dir().Append(
          FPL("ofac-sanctioned-digital-currency-addresses.json")),
      ofac_list_json));

  CreateWallet();

  RunUntilIdle();
  EXPECT_FALSE(registry()->IsEmptyForTesting());
  EXPECT_TRUE(registry()->GetPrepopulatedNetworks().empty());
  EXPECT_EQ(registry()->GetCoingeckoId(
                "0xa", "0x7f5c764cbc14f9669b88837ca1490cca17c31607"),
            "usd-coin");
  EXPECT_TRUE(
      registry()->IsOfacAddress("0xb9ef770b6a5e12e45983c5d80545258aa38f3b78"));
}

TEST_F(WalletDataFilesInstallerUnitTest,
       OnDemandInstallAndParsing_InstallFail) {
  EXPECT_CALL(*updater(), RegisterComponent(testing::_))
      .Times(1)
      .WillOnce(testing::Return(true));
  SetOnDemandUpdateCallbackWithComponentUpdateError();
  CreateWallet();

  RunUntilIdle();
  EXPECT_TRUE(registry()->IsEmptyForTesting());
}

TEST_F(WalletDataFilesInstallerUnitTest,
       OnDemandInstallAndParsing_RestoreWallet) {
  EXPECT_CALL(*updater(), RegisterComponent(testing::_))
      .Times(1)
      .WillOnce(testing::Return(true));
  SetOnDemandUpdateCallbackWithComponentReady(install_dir());
  WriteCoingeckoIdsMapToFile();

  RestoreWallet();
  RunUntilIdle();
  EXPECT_FALSE(registry()->IsEmptyForTesting());
  EXPECT_EQ(registry()->GetCoingeckoId(
                "0xa", "0x7f5c764cbc14f9669b88837ca1490cca17c31607"),
            "usd-coin");
}

TEST_F(WalletDataFilesInstallerUnitTest,
       OnDemandInstallAndParsing_ImportFromExternalWallet) {
  EXPECT_CALL(*updater(), RegisterComponent(testing::_))
      .Times(1)
      .WillOnce(testing::Return(true));
  SetOnDemandUpdateCallbackWithComponentReady(install_dir());
  WriteCoingeckoIdsMapToFile();

  ImportFromExternalWallet();
  RunUntilIdle();
  EXPECT_FALSE(registry()->IsEmptyForTesting());
  EXPECT_EQ(registry()->GetCoingeckoId(
                "0xa", "0x7f5c764cbc14f9669b88837ca1490cca17c31607"),
            "usd-coin");
}

}  // namespace brave_wallet
