/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/fil_tx_manager.h"

#include "base/json/json_reader.h"
#include "base/test/bind.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"
#include "brave/components/brave_wallet/browser/fil_tx_manager.h"
#include "brave/components/brave_wallet/browser/fil_tx_meta.h"
#include "brave/components/brave_wallet/browser/fil_tx_state_manager.h"
#include "brave/components/brave_wallet/browser/hd_keyring.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/browser/tx_service.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/features.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "components/prefs/testing_pref_service.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/test_browser_context.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/l10n/l10n_util.h"

namespace brave_wallet {

namespace {

void AddUnapprovedTransactionSuccessCallback(bool* callback_called,
                                             std::string* tx_meta_id,
                                             bool success,
                                             const std::string& id,
                                             const std::string& error_message) {
  EXPECT_TRUE(success);
  EXPECT_FALSE(id.empty());
  EXPECT_TRUE(error_message.empty());
  *callback_called = true;
  *tx_meta_id = id;
}

}  // namespace

class FilTxManagerUnitTest : public testing::Test {
 public:
  FilTxManagerUnitTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME),
        browser_context_(new content::TestBrowserContext()),
        shared_url_loader_factory_(
            base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                &url_loader_factory_)) {}

  void SetUp() override {
    feature_list_.InitAndEnableFeature(
        brave_wallet::features::kBraveWalletFilecoinFeature);

    url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [&](const network::ResourceRequest& request) {
          url_loader_factory_.ClearResponses();
          base::StringPiece request_string(request.request_body->elements()
                                               ->at(0)
                                               .As<network::DataElementBytes>()
                                               .AsStringPiece());
          absl::optional<base::Value> request_value =
              base::JSONReader::Read(request_string);
          std::string* method = request_value->FindStringKey("method");
          ASSERT_TRUE(method);

          if (*method == "Filecoin.MpoolGetNonce") {
            url_loader_factory_.AddResponse(
                request.url.spec(),
                "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\": 1 }");
          } else if (*method == "Filecoin.GasEstimateMessageGas") {
            url_loader_factory_.AddResponse(request.url.spec(),
                                            R"(
                    {
                      jsonrpc: '2.0',
                      result: {
                        Version: 0,
                        To: 't1typmdwecdcidnwbrj67ogxut3kqcz57cb32o3iy',
                        From: 't1edrnbgw6rlj6hvwlzhmabszw7owrzrpm5ra22ry',
                        Nonce: 5,
                        Value: '42',
                        GasLimit: 598585,
                        GasFeeCap: '100820',
                        GasPremium: '99766',
                        Method: 0,
                        Params: '',
                        CID: {
                          '/': 'bafy2bzacedkdoldmztwjwi3jvxhxo4qqp7haufuifpqzregfqkthlyhhf2lfu'
                        }
                      },
                      id: 1
                    }
                  )");
          }
        }));
    brave_wallet::RegisterProfilePrefs(prefs_.registry());
    json_rpc_service_.reset(
        new JsonRpcService(shared_url_loader_factory_, &prefs_));
    keyring_service_.reset(new KeyringService(&prefs_));
    tx_service_.reset(new TxService(json_rpc_service_.get(),
                                    keyring_service_.get(), &prefs_));

    base::RunLoop run_loop;
    json_rpc_service_->SetNetwork(brave_wallet::mojom::kLocalhostChainId,
                                  mojom::CoinType::FIL,
                                  base::BindLambdaForTesting([&](bool success) {
                                    EXPECT_TRUE(success);
                                    run_loop.Quit();
                                  }));
    run_loop.Run();
    keyring_service_->CreateWallet("testing123", base::DoNothing());
    base::RunLoop().RunUntilIdle();
    keyring_service_->AddAccount("Account 1", mojom::CoinType::FIL,
                                 base::DoNothing());
    base::RunLoop().RunUntilIdle();
  }

  std::string from() {
    return keyring_service_
        ->GetHDKeyringById(brave_wallet::mojom::kFilecoinKeyringId)
        ->GetAddress(0);
  }

  FilTxManager* fil_tx_manager() { return tx_service_->GetFilTxManager(); }

  PrefService* GetPrefs() { return &prefs_; }

  void AddUnapprovedTransaction(
      mojom::FilTxDataPtr tx_data,
      const std::string& from,
      FilTxManager::AddUnapprovedTransactionCallback callback) {
    fil_tx_manager()->AddUnapprovedTransaction(std::move(tx_data), from,
                                               std::move(callback));
  }

 protected:
  base::test::ScopedFeatureList feature_list_;
  content::BrowserTaskEnvironment task_environment_;
  std::unique_ptr<content::TestBrowserContext> browser_context_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;

  std::unique_ptr<JsonRpcService> json_rpc_service_;
  std::unique_ptr<KeyringService> keyring_service_;
  std::unique_ptr<TxService> tx_service_;
};

TEST_F(FilTxManagerUnitTest,
       AddUnapprovedTransactionWithoutGasPriceAndGasLimit) {
  auto tx_data = mojom::FilTxData::New(
      "" /* nonce */, "" /* gas_premium */, "" /* gas_fee_cap */,
      "" /* gas_limit */, "" /* max_fee */,
      "t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q", "0x11",
      mojom::kLocalhostChainId);
  bool callback_called = false;
  std::string tx_meta_id;

  AddUnapprovedTransaction(
      std::move(tx_data), from(),
      base::BindOnce(&AddUnapprovedTransactionSuccessCallback, &callback_called,
                     &tx_meta_id));

  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);
  auto tx_meta = fil_tx_manager()->GetTxForTesting(tx_meta_id);
  EXPECT_TRUE(tx_meta);
}

}  //  namespace brave_wallet
