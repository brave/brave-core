/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot/polkadot_wallet_service.h"

#include "base/functional/callback_forward.h"
#include "base/test/bind.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "base/test/values_test_util.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/network_manager.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_keyring.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_substrate_rpc.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "brave/components/brave_wallet/common/features.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

namespace {

// Use the BOB account here:
// https://westend.subscan.io/account/5FHneW46xGXgs5mUiveU4sbTyGBzmstUspZC92UhjJM694ty
inline constexpr const char kBob[] =
    "8eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a48";

}  // namespace

class PolkadotWalletServiceUnitTest : public testing::Test {
 public:
  PolkadotWalletServiceUnitTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME),
        shared_url_loader_factory_(
            base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                &url_loader_factory_)) {}

  ~PolkadotWalletServiceUnitTest() override = default;

  void SetUp() override {
    feature_list_.InitAndEnableFeature(
        brave_wallet::features::kBraveWalletPolkadotFeature);

    brave_wallet::RegisterProfilePrefs(prefs_.registry());
    brave_wallet::RegisterLocalStatePrefs(local_state_.registry());

    network_manager_ = std::make_unique<NetworkManager>(&prefs_);
    keyring_service_ =
        std::make_unique<KeyringService>(nullptr, &prefs_, &local_state_);

    polkadot_substrate_rpc_ = std::make_unique<PolkadotSubstrateRpc>(
        *network_manager_, shared_url_loader_factory_);

    GetAccountUtils().CreateWallet(kMnemonicDivideCruise, kTestWalletPassword);
    polkadot_testnet_account_ =
        GetAccountUtils().EnsureAccount(mojom::KeyringId::kPolkadotTestnet, 0);
    ASSERT_TRUE(polkadot_testnet_account_);
  }

  void UnlockWallet() {
    keyring_service_->Unlock(
        kTestWalletPassword,
        base::BindOnce(
            [](base::RepeatingClosure quit_closure, bool unlocked) {
              EXPECT_TRUE(unlocked);
              quit_closure.Run();
            },
            task_environment_.QuitClosure()));

    task_environment_.RunUntilQuit();

    keyring_service_
        ->GetKeyring<PolkadotKeyring>(mojom::KeyringId::kPolkadotTestnet)
        ->SetSignatureRngForTesting();
  }

  AccountUtils GetAccountUtils() {
    return AccountUtils(keyring_service_.get());
  }

 protected:
  base::test::TaskEnvironment task_environment_;

  base::test::ScopedFeatureList feature_list_;
  mojom::AccountInfoPtr polkadot_testnet_account_;

  sync_preferences::TestingPrefServiceSyncable prefs_;
  sync_preferences::TestingPrefServiceSyncable local_state_;

  std::unique_ptr<NetworkManager> network_manager_;
  std::unique_ptr<KeyringService> keyring_service_;

  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;

  std::unique_ptr<PolkadotSubstrateRpc> polkadot_substrate_rpc_;
};

void VerifyTestnet(const PolkadotChainMetadata& metadata) {
  std::array<uint8_t, kPolkadotSubstrateAccountIdSize> pubkey = {};
  base::HexStringToSpan(kBob, pubkey);

  PolkadotUnsignedTransfer transfer_extrinsic(pubkey, 1234);
  const char* testnet_extrinsic =
      R"(98040400008eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a484913)";

  EXPECT_EQ(transfer_extrinsic.send_amount(), 1234u);
  EXPECT_EQ(base::HexEncodeLower(transfer_extrinsic.recipient()), kBob);
  EXPECT_EQ(transfer_extrinsic.Encode(metadata), testnet_extrinsic);
}

void VerifyMainnet(const PolkadotChainMetadata& metadata) {
  std::array<uint8_t, kPolkadotSubstrateAccountIdSize> pubkey = {};
  base::HexStringToSpan(kBob, pubkey);

  PolkadotUnsignedTransfer transfer_extrinsic(pubkey, 1234);
  const char* mainnet_extrinsic =
      R"(98040500008eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a484913)";

  EXPECT_EQ(transfer_extrinsic.send_amount(), 1234u);
  EXPECT_EQ(base::HexEncodeLower(transfer_extrinsic.recipient()), kBob);
  EXPECT_EQ(transfer_extrinsic.Encode(metadata), mainnet_extrinsic);
}

TEST_F(PolkadotWalletServiceUnitTest, Constructor) {
  // Basic Hello, World style test for getting chain data from the constructor
  // calls.

  url_loader_factory_.ClearResponses();

  std::string testnet_url =
      network_manager_
          ->GetKnownChain(mojom::kPolkadotTestnet, mojom::CoinType::DOT)
          ->rpc_endpoints.front()
          .spec();

  std::string mainnet_url =
      network_manager_
          ->GetKnownChain(mojom::kPolkadotMainnet, mojom::CoinType::DOT)
          ->rpc_endpoints.front()
          .spec();

  EXPECT_EQ(testnet_url, "https://polkadot-westend.wallet.brave.com/");
  EXPECT_EQ(mainnet_url, "https://polkadot-mainnet.wallet.brave.com/");

  {
    // Both requests in the constructor complete successfully.

    auto polkadot_wallet_service = std::make_unique<PolkadotWalletService>(
        *keyring_service_, *network_manager_, shared_url_loader_factory_);

    UnlockWallet();
    EXPECT_EQ(url_loader_factory_.NumPending(), 2);

    url_loader_factory_.AddResponse(testnet_url, R"(
    { "jsonrpc": "2.0",
      "result": "Westend",
      "id": 1 })");

    url_loader_factory_.AddResponse(mainnet_url, R"(
    { "jsonrpc": "2.0",
      "result": "Polkadot",
      "id": 1 })");

    // Do this twice to prove our fetching and caching layers work.
    for (int i = 0; i < 2; ++i) {
      // Testnet chain metadata.

      polkadot_wallet_service->GetChainMetadata(
          mojom::kPolkadotTestnet,
          base::BindOnce(
              [](base::RepeatingClosure quit_closure,
                 base::expected<PolkadotChainMetadata, std::string> metadata) {
                EXPECT_TRUE(metadata.has_value());
                VerifyTestnet(*metadata);
                std::move(quit_closure).Run();
              },
              task_environment_.QuitClosure()));

      task_environment_.RunUntilQuit();
    }

    // Do this twice to prove our fetching and caching layers work.
    for (int i = 0; i < 2; ++i) {
      // Mainnet chain metadata.

      polkadot_wallet_service->GetChainMetadata(
          mojom::kPolkadotMainnet,
          base::BindOnce(
              [](base::RepeatingClosure quit_closure,
                 base::expected<PolkadotChainMetadata, std::string> metadata) {
                EXPECT_TRUE(metadata.has_value());
                VerifyMainnet(*metadata);
                std::move(quit_closure).Run();
              },
              task_environment_.QuitClosure()));

      task_environment_.RunUntilQuit();
    }
  }

  url_loader_factory_.ClearResponses();

  {
    // Both responses are invalid.

    auto polkadot_wallet_service = std::make_unique<PolkadotWalletService>(
        *keyring_service_, *network_manager_, shared_url_loader_factory_);

    UnlockWallet();
    EXPECT_EQ(url_loader_factory_.NumPending(), 2);

    url_loader_factory_.AddResponse(testnet_url, R"(
    { "jsonrpc": "2.0",
      "result": "hello",
      "id": 1 })");

    url_loader_factory_.AddResponse(mainnet_url, R"(
    { "jsonrpc": "2.0",
      "result": "world",
      "id": 1 })");

    {
      // Testnet chain metadata.

      polkadot_wallet_service->GetChainMetadata(
          mojom::kPolkadotTestnet,
          base::BindOnce(
              [](base::RepeatingCallback<void()> quit_closure,
                 base::expected<PolkadotChainMetadata, std::string> metadata) {
                EXPECT_FALSE(metadata.has_value());
                std::move(quit_closure).Run();
              },
              task_environment_.QuitClosure()));

      task_environment_.RunUntilQuit();
    }

    {
      // Mainnet chain metadata.

      polkadot_wallet_service->GetChainMetadata(
          mojom::kPolkadotMainnet,
          base::BindOnce(
              [](base::RepeatingCallback<void()> quit_closure,
                 base::expected<PolkadotChainMetadata, std::string> metadata) {
                EXPECT_FALSE(metadata.has_value());
                std::move(quit_closure).Run();
              },
              task_environment_.QuitClosure()));

      task_environment_.RunUntilQuit();
    }
  }
}

TEST_F(PolkadotWalletServiceUnitTest, ConcurrentChainNameFetches) {
  // Test callback caching for getting chain names.

  url_loader_factory_.ClearResponses();

  std::string testnet_url =
      network_manager_
          ->GetKnownChain(mojom::kPolkadotTestnet, mojom::CoinType::DOT)
          ->rpc_endpoints.front()
          .spec();

  std::string mainnet_url =
      network_manager_
          ->GetKnownChain(mojom::kPolkadotMainnet, mojom::CoinType::DOT)
          ->rpc_endpoints.front()
          .spec();

  EXPECT_EQ(testnet_url, "https://polkadot-westend.wallet.brave.com/");
  EXPECT_EQ(mainnet_url, "https://polkadot-mainnet.wallet.brave.com/");

  auto polkadot_wallet_service = std::make_unique<PolkadotWalletService>(
      *keyring_service_, *network_manager_, shared_url_loader_factory_);

  UnlockWallet();
  EXPECT_EQ(url_loader_factory_.NumPending(), 2);

  url_loader_factory_.AddResponse(testnet_url, R"(
    { "jsonrpc": "2.0",
      "result": "Westend",
      "id": 1 })");

  url_loader_factory_.AddResponse(mainnet_url, R"(
    { "jsonrpc": "2.0",
      "result": "Polkadot",
      "id": 1 })");

  int num_requests = 5;
  for (int i = 0; i < num_requests; ++i) {
    // Testnet chain metadata.
    polkadot_wallet_service->GetChainMetadata(
        mojom::kPolkadotTestnet,
        base::BindOnce(
            [](base::RepeatingClosure quit_closure, int* num_reqs,
               base::expected<PolkadotChainMetadata, std::string> metadata) {
              EXPECT_TRUE(*num_reqs > 0);
              EXPECT_TRUE(metadata.has_value());
              VerifyTestnet(*metadata);
              if (--*num_reqs == 0) {
                std::move(quit_closure).Run();
              }
            },
            task_environment_.QuitClosure(), &num_requests));
  }
  task_environment_.RunUntilQuit();

  num_requests = 5;
  for (int i = 0; i < num_requests; ++i) {
    // Mainnet chain metadata.
    polkadot_wallet_service->GetChainMetadata(
        mojom::kPolkadotMainnet,
        base::BindOnce(
            [](base::RepeatingClosure quit_closure, int* num_reqs,
               base::expected<PolkadotChainMetadata, std::string> metadata) {
              EXPECT_TRUE(*num_reqs > 0);
              EXPECT_TRUE(metadata.has_value());
              VerifyMainnet(*metadata);
              if (--*num_reqs == 0) {
                std::move(quit_closure).Run();
              }
            },
            task_environment_.QuitClosure(), &num_requests));
  }
  task_environment_.RunUntilQuit();
}

namespace {

base::DictValue RequestBodyToJsonDict(const network::ResourceRequest& req) {
  const auto* body_elems = req.request_body->elements();
  CHECK(body_elems->size() == 1u);

  const auto& element = body_elems->at(0);
  auto sv = element.As<network::DataElementBytes>().AsStringView();
  return base::test::ParseJsonDict(sv);
}

}  // namespace

TEST_F(PolkadotWalletServiceUnitTest, SignTransferExtrinsic) {
  url_loader_factory_.ClearResponses();

  std::string testnet_url =
      network_manager_
          ->GetKnownChain(mojom::kPolkadotTestnet, mojom::CoinType::DOT)
          ->rpc_endpoints.front()
          .spec();

  std::string mainnet_url =
      network_manager_
          ->GetKnownChain(mojom::kPolkadotMainnet, mojom::CoinType::DOT)
          ->rpc_endpoints.front()
          .spec();

  EXPECT_EQ(testnet_url, "https://polkadot-westend.wallet.brave.com/");
  EXPECT_EQ(mainnet_url, "https://polkadot-mainnet.wallet.brave.com/");

  auto polkadot_wallet_service = std::make_unique<PolkadotWalletService>(
      *keyring_service_, *network_manager_, shared_url_loader_factory_);

  UnlockWallet();
  EXPECT_EQ(url_loader_factory_.NumPending(), 2);

  std::array<uint8_t, kPolkadotSubstrateAccountIdSize> recipient_pubkey = {};
  EXPECT_TRUE(base::HexStringToSpan(kBob, recipient_pubkey));

  // For the two initial network calls that fetch the chain metadata.
  url_loader_factory_.AddResponse(testnet_url, R"(
    { "jsonrpc": "2.0",
      "result": "Westend",
      "id": 1 })");

  url_loader_factory_.AddResponse(mainnet_url, R"(
    { "jsonrpc": "2.0",
      "result": "Polkadot",
      "id": 1 })");

  url_loader_factory_.ClearResponses();

  base::flat_map<base::DictValue, std::string_view> req_res_pairs;
  // Our initial call to get account information so we have a usable nonce for
  // extinsic creation.
  req_res_pairs.emplace(base::test::ParseJsonDict(
                            R"(
            {
              "id":1,
              "jsonrpc":"2.0",
              "method":"state_queryStorageAt",
              "params":[
                [
                  "0x26AA394EEA5630E07C48AE0C9558CEF7B99D880EC681799C0CF30E8886371DA9DB9D3491A2D745CFE5E957DAA0A734BCD6B2A5CC606EA86342001DD036B301C15A5CBA63C413CAD5CA0E8F47E6FA9516"
                ]
              ]
            })"),
                        R"(
            {
              "jsonrpc":"2.0",
              "id":8,
              "result":[
                {
                  "block":"0xdcc9741f0258ede18a1684ff787c1591db8585f3154481cd162378fdd6677056",
                  "changes":[
                    [
                      "0x26aa394eea5630e07c48ae0c9558cef7b99d880ec681799c0cf30e8886371da9c6282eb06d1994674b75538b85244e4952707850d9298f5dfb0a3e5b23fcca39ea286c6def2db5716c996fb39db6477c",
                      "0x11000000000000000100000000000000be2bcb22d90800000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000080"
                    ]
                  ]
                }
              ]
            })");

  // Our initial call to get the most recent block header in the chain.
  req_res_pairs.emplace(
      base::test::ParseJsonDict(
          R"({"id":1,"jsonrpc":"2.0","method":"chain_getHeader","params":[]})"),
      R"(
            {
              "jsonrpc":"2.0",
              "id":10,
              "result": {
                "parentHash": "0x5834828e919dc0eccce83080104cc14f51f81330451bdf74bbc9bc1edba618f2",
                "number": "0x1c06358",
                "stateRoot": "0xc7ce642fe3c31724fc2808e179d807e5b0ba38a40eb5c08d98a41bd22eeeb9b5",
                "extrinsicsRoot": "0xba8c3f44dbde4e14831d61cb349f808f902bf4474112cf45c2c7d92bef4e754e",
                "digest": {
                  "logs": [
                    "0x0642414245b501030d0000002a92911100000000641ad61d0d848c4b374270406d092e8e5ca294e78ab6ea47d6da8ffe51e207758c49a32cac0d5d1e1ca56e4996b2136f42602debdf22478aff56b359adeadf06912cba0095e5b5abc3d7c0e03d9b628587010e6b44c6f9f40419b16bec4f5204",
                    "0x04424545468403b61c769585188ea959dc10c9434bfa46d57e818c7f17e047c75c42b3b1389c11",
                    "0x0542414245010186630051f53fb7ce6e02d1e020383357327c9e28ebfc399a96a98ad42528290a3f57cd65de0688f232da0b9db4a5007e69da55e2e13146c4f5c60983fc9c138e"
                  ]
                }
              }
            })");

  // Chained call from grabbing the most recent block header in the chain. Grab
  // the header of the corresponding parent hash for mortality calculations,
  // like how polkadot-js's api package does.
  // Implement the same algorithm as here:
  // https://github.com/polkadot-js/api/blob/eb34741c871ca8d029a9706ae989ba8ce865db0f/packages/api-derive/src/tx/signingInfo.ts#L41-L71
  req_res_pairs.emplace(
      base::test::ParseJsonDict(
          R"({"id":1,"jsonrpc":"2.0","method":"chain_getHeader","params":["5834828E919DC0ECCCE83080104CC14F51F81330451BDF74BBC9BC1EDBA618F2"]})"),
      R"(
            {
              "jsonrpc":"2.0",
              "id":12,
              "result":{
                "parentHash":"0x08cf134277f266acbce9727a13e7675ef472eb5932c693d7e924a5e12ca89b66",
                "number":"0x1c06357",
                "stateRoot":"0x6ac80fab8d74177b34675338269aad0d19d61dbe9adc2f421f77742bc4153d2d",
                "extrinsicsRoot":"0x2ee277dc4f23c4b3ab4e84cd7bb060cceca658933aed2735699e6d434cea39bc",
                "digest":{
                  "logs":[
                    "0x0642414245b50103000000002992911100000000d0053711418b7b2b943547844945a8eba27aa6163575a5b3aadd7cb78c878f6e2c8f4af3018fd5ad87d36dcca26914a9324722334311c8055e3889db61d5e60fa2eb601a422f67f1daf0a08d434c151430d1467855699ddb294e347e5ecdf90e",
                    "0x04424545468403ffb231bc483aba17682e4b1968b90559cab35ee078c021899f6751b2fc845085",
                    "0x05424142450101407020be76e604fdbb36bc1cd07dc71aa9eeddd537c26fec73178e48effde4607c728bb10f06c94299af0cb1a5835bb60bab5d9c3cc6eff8a0ade2a9a13f1780"
                  ]
                }
              }
            })");

  // Our initial call to get the hash of the last finalized block in the canon
  // chain.
  req_res_pairs.emplace(
      base::test::ParseJsonDict(
          R"({"id":1,"jsonrpc":"2.0","method":"chain_getFinalizedHead","params":[]})"),
      R"({"jsonrpc":"2.0","id":11,"result":"0x46e5afe42b1ff0c40ecc18d7ff97974f3bdf5dfda1e21d779644a7ea30a97d21"})");

  // Chained call, grab the block header using the hash of the finalized head.
  req_res_pairs.emplace(
      base::test::ParseJsonDict(
          R"({"id":1,"jsonrpc":"2.0","method":"chain_getHeader","params":["46E5AFE42B1FF0C40ECC18D7FF97974F3BDF5DFDA1E21D779644A7EA30A97D21"]})"),
      R"(
            {
              "jsonrpc":"2.0",
              "id":13,
              "result":{
                "parentHash":"0xcf424e463b14b26905d4e2aaff455a3c149c3ccff5a1fc62203c0c07b711e3f4",
                "number":"0x1c06355",
                "stateRoot":"0x3a501ddbfc394d859401cd6d55f5743461ddb3a5aecfebb31f587c16ad23f505",
                "extrinsicsRoot":"0x8fc47b641e793ed938eae4d793636b2feb657bca97726a43ee3375a8e5b321a6",
                "digest":{
                  "logs":[
                    "0x0642414245b501030200000027929111000000008038b165beaf68d4ae8b7a3eae2055ecdfde0a0462993a43e522c709773da51a550d604eb90a671b88437f7f0d5e7f2e4efe323e2cee3992ffa2bcd3e5e10d07ff37c43e11e82263d2bc774942196e96c05a38bbbd820eff1cbf2441b2c59307",
                    "0x04424545468403cfdc267eac55b3225fe8d581f3d2f7d9ece28a564bb70b50dd04b829e893b78a",
                    "0x05424142450101fc0b1a7fcff42ffb1fcb8166843fb9b9eded36f64891deea28eea90da9215e70c605638b274f0c8517fc70d0c2b1442fd50ad933ee6cf7ceba600f762e2bd682"
                  ]
                }
              }
            })");

  // Grab the block hash of whichever block header we're using for signing the
  // extrinsic. The polkadot-js algorithm selects between either the finalized
  // head or the parent of the current head. In this case, the finalized block
  // wound up winning out. Theoretically, this can be replaced by just directly
  // hashing the block headers locally.
  req_res_pairs.emplace(
      base::test::ParseJsonDict(
          R"({"id":1,"jsonrpc":"2.0","method":"chain_getBlockHash","params":["01C06355"]})"),
      R"({"jsonrpc":"2.0","id":1,"result":"0x46e5afe42b1ff0c40ecc18d7ff97974f3bdf5dfda1e21d779644a7ea30a97d21"})");

  // Grab the runtime version of whichever block we're going to use for signing.
  req_res_pairs.emplace(
      base::test::ParseJsonDict(
          R"({"id":1,"jsonrpc":"2.0","method":"state_getRuntimeVersion","params":["cf424e463b14b26905d4e2aaff455a3c149c3ccff5a1fc62203c0c07b711e3f4"]})"),
      R"(
            {
              "jsonrpc":"2.0",
              "id":15,
              "result":{
                "specName":"westend",
                "implName":"parity-westend",
                "authoringVersion":2,
                "specVersion":1021000,
                "implVersion":0,
                "apis":[
                  ["0xdf6acb689907609b",5],["0x37e397fc7c91f5e4",2],["0xccd9de6396c899ca",1],["0x40fe3ad401f8959a",6],
                  ["0xd2bc9897eed08f15",3],["0xf78b278be53f454c",2],["0xaf2c0297a23e6d3d",15],["0x49eaaf1b548a0cb0",6],
                  ["0x91d5df18b0d2cf58",3],["0x2a5e924655399e60",1],["0xed99c5acb25eedf5",3],["0xcbca25e39f142387",2],
                  ["0x687ad44ad37f03c2",1],["0xab3c0572291feb8b",1],["0xbc9d89904f5b923f",1],["0x37c8bb1350a9a2a8",4],
                  ["0xf3ff14d5ab527059",3],["0x6ff52ee858e6c5bd",2],["0x91b1c8b16328eb92",2],["0x9ffb505aa738d69c",1],
                  ["0x17a6bc0d0062aeb3",1],["0x18ef58a3b67ba770",1],["0xfbc577b9d747efd6",1],["0x2609be83ac4468dc",1]
                ],
                "transactionVersion":27,
                "systemVersion":1,
                "stateVersion":1
              }
            })");

  // We need to grab the genesis block hash.
  req_res_pairs.emplace(
      base::test::ParseJsonDict(
          R"({"id":1,"jsonrpc":"2.0","method":"chain_getBlockHash","params":["00000000"]})"),
      R"({"jsonrpc":"2.0","id":1,"result":"0xe143f23803ac50e8f6f8e62695d1ce9e4e1d68aa36c1cd2cfd15340213f3423e"})");

  url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
      [&, self = this](const network::ResourceRequest& req) {
        auto pos = req_res_pairs.find(RequestBodyToJsonDict(req));
        if (pos != req_res_pairs.end()) {
          self->url_loader_factory_.AddResponse(testnet_url, pos->second);
        }
      }));

  auto pubkey = base::HexEncodeLower(
      keyring_service_->GetPolkadotPubKey(polkadot_testnet_account_->account_id)
          .value());

  EXPECT_EQ(pubkey,
            "d6b2a5cc606ea86342001dd036b301c15a5cba63c413cad5ca0e8f47e6fa9516");

  // clang-format off
  constexpr static std::string_view kExpectedExtrinsic =
      "3502"  // Length prefix.
      "84"    // 0x80 => signed, 0x04 => extrinsic version v4
      "00"    // Address type.
      "d6b2a5cc606ea86342001dd036b301c15a5cba63c413cad5ca0e8f47e6fa9516" // Sender pubkey.
      "01"  // Signature type (0x01 => Sr25519)
      "36376696cfb3470732527fff9fe146da430ec75a3bb3236b8720e2e19df0c170" // Signature
      "9fcb3667049de2ff68185770bd9221a9b2e9a650d962d7015a6ee91e5de2d586"
      "5501"    // Mortal era.
      "440000"  // Nonce is 17, SCALE-encoded as 0x44; tip = 0; mode = 0.
      "0400"    // Pallet index, call index.
      "00"      // Address type
      "8eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a48" // Recipient address
      "4913"  // Send amount.
      ;
  // clang-format on

  polkadot_wallet_service->GenerateSignedTransferExtrinsic(
      mojom::kPolkadotTestnet, polkadot_testnet_account_->account_id.Clone(),
      uint128_t{1234}, recipient_pubkey,
      base::BindOnce(
          [](base::RepeatingClosure quit_closure,
             base::expected<std::string, std::string> signed_extrinsic) {
            EXPECT_EQ(signed_extrinsic.value(), kExpectedExtrinsic);
            quit_closure.Run();
          },
          task_environment_.QuitClosure()));

  task_environment_.RunUntilQuit();
}

TEST_F(PolkadotWalletServiceUnitTest, SignTransferExtrinsic_NoChainMetadata) {
  url_loader_factory_.ClearResponses();

  std::string testnet_url =
      network_manager_
          ->GetKnownChain(mojom::kPolkadotTestnet, mojom::CoinType::DOT)
          ->rpc_endpoints.front()
          .spec();

  std::string mainnet_url =
      network_manager_
          ->GetKnownChain(mojom::kPolkadotMainnet, mojom::CoinType::DOT)
          ->rpc_endpoints.front()
          .spec();

  EXPECT_EQ(testnet_url, "https://polkadot-westend.wallet.brave.com/");
  EXPECT_EQ(mainnet_url, "https://polkadot-mainnet.wallet.brave.com/");

  auto polkadot_wallet_service = std::make_unique<PolkadotWalletService>(
      *keyring_service_, *network_manager_, shared_url_loader_factory_);

  UnlockWallet();
  EXPECT_EQ(url_loader_factory_.NumPending(), 2);

  std::array<uint8_t, kPolkadotSubstrateAccountIdSize> recipient_pubkey = {};
  EXPECT_TRUE(base::HexStringToSpan(kBob, recipient_pubkey));

  // For the two initial network calls that fetch the chain metadata.
  url_loader_factory_.AddResponse(testnet_url, R"(
    { "jsonrpc": "2.0",
      "error": {
        "code": 1234
      },
      "id": 1 })");

  url_loader_factory_.AddResponse(mainnet_url, R"(
    { "jsonrpc": "2.0",
      "result": "Polkadot",
      "id": 1 })");

  url_loader_factory_.ClearResponses();

  polkadot_wallet_service->GenerateSignedTransferExtrinsic(
      mojom::kPolkadotTestnet, polkadot_testnet_account_->account_id.Clone(),
      uint128_t{1234}, recipient_pubkey,
      base::BindOnce(
          [](base::RepeatingClosure quit_closure,
             base::expected<std::string, std::string> signed_extrinsic) {
            EXPECT_EQ(signed_extrinsic.error(), WalletInternalErrorMessage());
            quit_closure.Run();
          },
          task_environment_.QuitClosure()));

  task_environment_.RunUntilQuit();
}

TEST_F(PolkadotWalletServiceUnitTest, SignTransferExtrinsic_NoAccountInfo) {
  url_loader_factory_.ClearResponses();

  std::string testnet_url =
      network_manager_
          ->GetKnownChain(mojom::kPolkadotTestnet, mojom::CoinType::DOT)
          ->rpc_endpoints.front()
          .spec();

  std::string mainnet_url =
      network_manager_
          ->GetKnownChain(mojom::kPolkadotMainnet, mojom::CoinType::DOT)
          ->rpc_endpoints.front()
          .spec();

  EXPECT_EQ(testnet_url, "https://polkadot-westend.wallet.brave.com/");
  EXPECT_EQ(mainnet_url, "https://polkadot-mainnet.wallet.brave.com/");

  auto polkadot_wallet_service = std::make_unique<PolkadotWalletService>(
      *keyring_service_, *network_manager_, shared_url_loader_factory_);

  UnlockWallet();
  EXPECT_EQ(url_loader_factory_.NumPending(), 2);

  std::array<uint8_t, kPolkadotSubstrateAccountIdSize> recipient_pubkey = {};
  EXPECT_TRUE(base::HexStringToSpan(kBob, recipient_pubkey));

  // For the two initial network calls that fetch the chain metadata.
  url_loader_factory_.AddResponse(testnet_url, R"(
    { "jsonrpc": "2.0",
      "result": "Westend",
      "id": 1 })");

  url_loader_factory_.AddResponse(mainnet_url, R"(
    { "jsonrpc": "2.0",
      "result": "Polkadot",
      "id": 1 })");

  url_loader_factory_.ClearResponses();

  base::flat_map<base::DictValue, std::string_view> req_res_pairs;
  // Our initial call to get account information so we have a usable nonce for
  // extinsic creation.
  req_res_pairs.emplace(base::test::ParseJsonDict(
                            R"(
            {
              "id":1,
              "jsonrpc":"2.0",
              "method":"state_queryStorageAt",
              "params":[
                [
                  "0x26AA394EEA5630E07C48AE0C9558CEF7B99D880EC681799C0CF30E8886371DA9DB9D3491A2D745CFE5E957DAA0A734BCD6B2A5CC606EA86342001DD036B301C15A5CBA63C413CAD5CA0E8F47E6FA9516"
                ]
              ]
            })"),
                        R"(
            {
              "jsonrpc":"2.0",
              "id":8,
              "error": {
                "code": 1234
              }
            })");

  url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
      [&, self = this](const network::ResourceRequest& req) {
        auto pos = req_res_pairs.find(RequestBodyToJsonDict(req));
        if (pos != req_res_pairs.end()) {
          self->url_loader_factory_.AddResponse(testnet_url, pos->second);
        }
      }));

  auto pubkey = base::HexEncodeLower(
      keyring_service_->GetPolkadotPubKey(polkadot_testnet_account_->account_id)
          .value());

  EXPECT_EQ(pubkey,
            "d6b2a5cc606ea86342001dd036b301c15a5cba63c413cad5ca0e8f47e6fa9516");

  polkadot_wallet_service->GenerateSignedTransferExtrinsic(
      mojom::kPolkadotTestnet, polkadot_testnet_account_->account_id.Clone(),
      uint128_t{1234}, recipient_pubkey,
      base::BindOnce(
          [](base::RepeatingClosure quit_closure,
             base::expected<std::string, std::string> signed_extrinsic) {
            EXPECT_EQ(signed_extrinsic.error(), WalletInternalErrorMessage());
            quit_closure.Run();
          },
          task_environment_.QuitClosure()));

  task_environment_.RunUntilQuit();
}

TEST_F(PolkadotWalletServiceUnitTest, SignTransferExtrinsic_NoChainHeader) {
  url_loader_factory_.ClearResponses();

  std::string testnet_url =
      network_manager_
          ->GetKnownChain(mojom::kPolkadotTestnet, mojom::CoinType::DOT)
          ->rpc_endpoints.front()
          .spec();

  std::string mainnet_url =
      network_manager_
          ->GetKnownChain(mojom::kPolkadotMainnet, mojom::CoinType::DOT)
          ->rpc_endpoints.front()
          .spec();

  EXPECT_EQ(testnet_url, "https://polkadot-westend.wallet.brave.com/");
  EXPECT_EQ(mainnet_url, "https://polkadot-mainnet.wallet.brave.com/");

  auto polkadot_wallet_service = std::make_unique<PolkadotWalletService>(
      *keyring_service_, *network_manager_, shared_url_loader_factory_);

  UnlockWallet();
  EXPECT_EQ(url_loader_factory_.NumPending(), 2);

  std::array<uint8_t, kPolkadotSubstrateAccountIdSize> recipient_pubkey = {};
  EXPECT_TRUE(base::HexStringToSpan(kBob, recipient_pubkey));

  // For the two initial network calls that fetch the chain metadata.
  url_loader_factory_.AddResponse(testnet_url, R"(
    { "jsonrpc": "2.0",
      "result": "Westend",
      "id": 1 })");

  url_loader_factory_.AddResponse(mainnet_url, R"(
    { "jsonrpc": "2.0",
      "result": "Polkadot",
      "id": 1 })");

  url_loader_factory_.ClearResponses();

  base::flat_map<base::DictValue, std::string_view> req_res_pairs;
  // Our initial call to get account information so we have a usable nonce for
  // extinsic creation.
  req_res_pairs.emplace(base::test::ParseJsonDict(
                            R"(
            {
              "id":1,
              "jsonrpc":"2.0",
              "method":"state_queryStorageAt",
              "params":[
                [
                  "0x26AA394EEA5630E07C48AE0C9558CEF7B99D880EC681799C0CF30E8886371DA9DB9D3491A2D745CFE5E957DAA0A734BCD6B2A5CC606EA86342001DD036B301C15A5CBA63C413CAD5CA0E8F47E6FA9516"
                ]
              ]
            })"),
                        R"(
            {
              "jsonrpc":"2.0",
              "id":8,
              "result":[
                {
                  "block":"0xdcc9741f0258ede18a1684ff787c1591db8585f3154481cd162378fdd6677056",
                  "changes":[
                    [
                      "0x26aa394eea5630e07c48ae0c9558cef7b99d880ec681799c0cf30e8886371da9c6282eb06d1994674b75538b85244e4952707850d9298f5dfb0a3e5b23fcca39ea286c6def2db5716c996fb39db6477c",
                      "0x11000000000000000100000000000000be2bcb22d90800000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000080"
                    ]
                  ]
                }
              ]
            })");

  // Our initial call to get the most recent block header in the chain.
  req_res_pairs.emplace(
      base::test::ParseJsonDict(
          R"({"id":1,"jsonrpc":"2.0","method":"chain_getHeader","params":[]})"),
      R"(
            {
              "jsonrpc":"2.0",
              "id":10,
              "error": {
                "code": 1234
              }
            })");

  // Our initial call to get the hash of the last finalized block in the canon
  // chain.
  req_res_pairs.emplace(
      base::test::ParseJsonDict(
          R"({"id":1,"jsonrpc":"2.0","method":"chain_getFinalizedHead","params":[]})"),
      R"({"jsonrpc":"2.0","id":11,"result":"0x46e5afe42b1ff0c40ecc18d7ff97974f3bdf5dfda1e21d779644a7ea30a97d21"})");

  url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
      [&, self = this](const network::ResourceRequest& req) {
        auto pos = req_res_pairs.find(RequestBodyToJsonDict(req));
        if (pos != req_res_pairs.end()) {
          self->url_loader_factory_.AddResponse(testnet_url, pos->second);
        }
      }));

  auto pubkey = base::HexEncodeLower(
      keyring_service_->GetPolkadotPubKey(polkadot_testnet_account_->account_id)
          .value());

  EXPECT_EQ(pubkey,
            "d6b2a5cc606ea86342001dd036b301c15a5cba63c413cad5ca0e8f47e6fa9516");

  polkadot_wallet_service->GenerateSignedTransferExtrinsic(
      mojom::kPolkadotTestnet, polkadot_testnet_account_->account_id.Clone(),
      uint128_t{1234}, recipient_pubkey,
      base::BindOnce(
          [](base::RepeatingClosure quit_closure,
             base::expected<std::string, std::string> signed_extrinsic) {
            EXPECT_EQ(signed_extrinsic.error(), WalletInternalErrorMessage());
            quit_closure.Run();
          },
          task_environment_.QuitClosure()));

  task_environment_.RunUntilQuit();
}

TEST_F(PolkadotWalletServiceUnitTest, SignTransferExtrinsic_NoParentHeader) {
  url_loader_factory_.ClearResponses();

  std::string testnet_url =
      network_manager_
          ->GetKnownChain(mojom::kPolkadotTestnet, mojom::CoinType::DOT)
          ->rpc_endpoints.front()
          .spec();

  std::string mainnet_url =
      network_manager_
          ->GetKnownChain(mojom::kPolkadotMainnet, mojom::CoinType::DOT)
          ->rpc_endpoints.front()
          .spec();

  EXPECT_EQ(testnet_url, "https://polkadot-westend.wallet.brave.com/");
  EXPECT_EQ(mainnet_url, "https://polkadot-mainnet.wallet.brave.com/");

  auto polkadot_wallet_service = std::make_unique<PolkadotWalletService>(
      *keyring_service_, *network_manager_, shared_url_loader_factory_);

  UnlockWallet();
  EXPECT_EQ(url_loader_factory_.NumPending(), 2);

  std::array<uint8_t, kPolkadotSubstrateAccountIdSize> recipient_pubkey = {};
  EXPECT_TRUE(base::HexStringToSpan(kBob, recipient_pubkey));

  // For the two initial network calls that fetch the chain metadata.
  url_loader_factory_.AddResponse(testnet_url, R"(
    { "jsonrpc": "2.0",
      "result": "Westend",
      "id": 1 })");

  url_loader_factory_.AddResponse(mainnet_url, R"(
    { "jsonrpc": "2.0",
      "result": "Polkadot",
      "id": 1 })");

  url_loader_factory_.ClearResponses();

  base::flat_map<base::DictValue, std::string_view> req_res_pairs;
  // Our initial call to get account information so we have a usable nonce for
  // extinsic creation.
  req_res_pairs.emplace(base::test::ParseJsonDict(
                            R"(
            {
              "id":1,
              "jsonrpc":"2.0",
              "method":"state_queryStorageAt",
              "params":[
                [
                  "0x26AA394EEA5630E07C48AE0C9558CEF7B99D880EC681799C0CF30E8886371DA9DB9D3491A2D745CFE5E957DAA0A734BCD6B2A5CC606EA86342001DD036B301C15A5CBA63C413CAD5CA0E8F47E6FA9516"
                ]
              ]
            })"),
                        R"(
            {
              "jsonrpc":"2.0",
              "id":8,
              "result":[
                {
                  "block":"0xdcc9741f0258ede18a1684ff787c1591db8585f3154481cd162378fdd6677056",
                  "changes":[
                    [
                      "0x26aa394eea5630e07c48ae0c9558cef7b99d880ec681799c0cf30e8886371da9c6282eb06d1994674b75538b85244e4952707850d9298f5dfb0a3e5b23fcca39ea286c6def2db5716c996fb39db6477c",
                      "0x11000000000000000100000000000000be2bcb22d90800000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000080"
                    ]
                  ]
                }
              ]
            })");

  // Our initial call to get the most recent block header in the chain.
  req_res_pairs.emplace(
      base::test::ParseJsonDict(
          R"({"id":1,"jsonrpc":"2.0","method":"chain_getHeader","params":[]})"),
      R"(
            {
              "jsonrpc":"2.0",
              "id":10,
              "result": {
                "parentHash": "0x5834828e919dc0eccce83080104cc14f51f81330451bdf74bbc9bc1edba618f2",
                "number": "0x1c06358",
                "stateRoot": "0xc7ce642fe3c31724fc2808e179d807e5b0ba38a40eb5c08d98a41bd22eeeb9b5",
                "extrinsicsRoot": "0xba8c3f44dbde4e14831d61cb349f808f902bf4474112cf45c2c7d92bef4e754e",
                "digest": {
                  "logs": [
                    "0x0642414245b501030d0000002a92911100000000641ad61d0d848c4b374270406d092e8e5ca294e78ab6ea47d6da8ffe51e207758c49a32cac0d5d1e1ca56e4996b2136f42602debdf22478aff56b359adeadf06912cba0095e5b5abc3d7c0e03d9b628587010e6b44c6f9f40419b16bec4f5204",
                    "0x04424545468403b61c769585188ea959dc10c9434bfa46d57e818c7f17e047c75c42b3b1389c11",
                    "0x0542414245010186630051f53fb7ce6e02d1e020383357327c9e28ebfc399a96a98ad42528290a3f57cd65de0688f232da0b9db4a5007e69da55e2e13146c4f5c60983fc9c138e"
                  ]
                }
              }
            })");

  // Chained call from grabbing the most recent block header in the chain. Grab
  // the header of the corresponding parent hash for mortality calculations,
  // like how polkadot-js's api package does.
  // Implement the same algorithm as here:
  // https://github.com/polkadot-js/api/blob/eb34741c871ca8d029a9706ae989ba8ce865db0f/packages/api-derive/src/tx/signingInfo.ts#L41-L71
  req_res_pairs.emplace(
      base::test::ParseJsonDict(
          R"({"id":1,"jsonrpc":"2.0","method":"chain_getHeader","params":["5834828E919DC0ECCCE83080104CC14F51F81330451BDF74BBC9BC1EDBA618F2"]})"),
      R"(
            {
              "jsonrpc":"2.0",
              "id":12,
              "error":{
                "code": 1234
              }
            })");

  // Our initial call to get the hash of the last finalized block in the canon
  // chain.
  req_res_pairs.emplace(
      base::test::ParseJsonDict(
          R"({"id":1,"jsonrpc":"2.0","method":"chain_getFinalizedHead","params":[]})"),
      R"({"jsonrpc":"2.0","id":11,"result":"0x46e5afe42b1ff0c40ecc18d7ff97974f3bdf5dfda1e21d779644a7ea30a97d21"})");

  // We need to grab the genesis block hash.
  req_res_pairs.emplace(
      base::test::ParseJsonDict(
          R"({"id":1,"jsonrpc":"2.0","method":"chain_getBlockHash","params":["00000000"]})"),
      R"({"jsonrpc":"2.0","id":1,"result":"0xe143f23803ac50e8f6f8e62695d1ce9e4e1d68aa36c1cd2cfd15340213f3423e"})");

  url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
      [&, self = this](const network::ResourceRequest& req) {
        auto pos = req_res_pairs.find(RequestBodyToJsonDict(req));
        if (pos != req_res_pairs.end()) {
          self->url_loader_factory_.AddResponse(testnet_url, pos->second);
        }
      }));

  auto pubkey = base::HexEncodeLower(
      keyring_service_->GetPolkadotPubKey(polkadot_testnet_account_->account_id)
          .value());

  EXPECT_EQ(pubkey,
            "d6b2a5cc606ea86342001dd036b301c15a5cba63c413cad5ca0e8f47e6fa9516");

  polkadot_wallet_service->GenerateSignedTransferExtrinsic(
      mojom::kPolkadotTestnet, polkadot_testnet_account_->account_id.Clone(),
      uint128_t{1234}, recipient_pubkey,
      base::BindOnce(
          [](base::RepeatingClosure quit_closure,
             base::expected<std::string, std::string> signed_extrinsic) {
            EXPECT_EQ(signed_extrinsic.error(), WalletInternalErrorMessage());
            quit_closure.Run();
          },
          task_environment_.QuitClosure()));

  task_environment_.RunUntilQuit();
}

TEST_F(PolkadotWalletServiceUnitTest, SignTransferExtrinsic_NoFinalizedHead) {
  url_loader_factory_.ClearResponses();

  std::string testnet_url =
      network_manager_
          ->GetKnownChain(mojom::kPolkadotTestnet, mojom::CoinType::DOT)
          ->rpc_endpoints.front()
          .spec();

  std::string mainnet_url =
      network_manager_
          ->GetKnownChain(mojom::kPolkadotMainnet, mojom::CoinType::DOT)
          ->rpc_endpoints.front()
          .spec();

  EXPECT_EQ(testnet_url, "https://polkadot-westend.wallet.brave.com/");
  EXPECT_EQ(mainnet_url, "https://polkadot-mainnet.wallet.brave.com/");

  auto polkadot_wallet_service = std::make_unique<PolkadotWalletService>(
      *keyring_service_, *network_manager_, shared_url_loader_factory_);

  UnlockWallet();
  EXPECT_EQ(url_loader_factory_.NumPending(), 2);

  std::array<uint8_t, kPolkadotSubstrateAccountIdSize> recipient_pubkey = {};
  EXPECT_TRUE(base::HexStringToSpan(kBob, recipient_pubkey));

  // For the two initial network calls that fetch the chain metadata.
  url_loader_factory_.AddResponse(testnet_url, R"(
    { "jsonrpc": "2.0",
      "result": "Westend",
      "id": 1 })");

  url_loader_factory_.AddResponse(mainnet_url, R"(
    { "jsonrpc": "2.0",
      "result": "Polkadot",
      "id": 1 })");

  url_loader_factory_.ClearResponses();

  base::flat_map<base::DictValue, std::string_view> req_res_pairs;
  // Our initial call to get account information so we have a usable nonce for
  // extinsic creation.
  req_res_pairs.emplace(base::test::ParseJsonDict(
                            R"(
            {
              "id":1,
              "jsonrpc":"2.0",
              "method":"state_queryStorageAt",
              "params":[
                [
                  "0x26AA394EEA5630E07C48AE0C9558CEF7B99D880EC681799C0CF30E8886371DA9DB9D3491A2D745CFE5E957DAA0A734BCD6B2A5CC606EA86342001DD036B301C15A5CBA63C413CAD5CA0E8F47E6FA9516"
                ]
              ]
            })"),
                        R"(
            {
              "jsonrpc":"2.0",
              "id":8,
              "result":[
                {
                  "block":"0xdcc9741f0258ede18a1684ff787c1591db8585f3154481cd162378fdd6677056",
                  "changes":[
                    [
                      "0x26aa394eea5630e07c48ae0c9558cef7b99d880ec681799c0cf30e8886371da9c6282eb06d1994674b75538b85244e4952707850d9298f5dfb0a3e5b23fcca39ea286c6def2db5716c996fb39db6477c",
                      "0x11000000000000000100000000000000be2bcb22d90800000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000080"
                    ]
                  ]
                }
              ]
            })");

  // Our initial call to get the most recent block header in the chain.
  req_res_pairs.emplace(
      base::test::ParseJsonDict(
          R"({"id":1,"jsonrpc":"2.0","method":"chain_getHeader","params":[]})"),
      R"(
            {
              "jsonrpc":"2.0",
              "id":10,
              "result": {
                "parentHash": "0x5834828e919dc0eccce83080104cc14f51f81330451bdf74bbc9bc1edba618f2",
                "number": "0x1c06358",
                "stateRoot": "0xc7ce642fe3c31724fc2808e179d807e5b0ba38a40eb5c08d98a41bd22eeeb9b5",
                "extrinsicsRoot": "0xba8c3f44dbde4e14831d61cb349f808f902bf4474112cf45c2c7d92bef4e754e",
                "digest": {
                  "logs": [
                    "0x0642414245b501030d0000002a92911100000000641ad61d0d848c4b374270406d092e8e5ca294e78ab6ea47d6da8ffe51e207758c49a32cac0d5d1e1ca56e4996b2136f42602debdf22478aff56b359adeadf06912cba0095e5b5abc3d7c0e03d9b628587010e6b44c6f9f40419b16bec4f5204",
                    "0x04424545468403b61c769585188ea959dc10c9434bfa46d57e818c7f17e047c75c42b3b1389c11",
                    "0x0542414245010186630051f53fb7ce6e02d1e020383357327c9e28ebfc399a96a98ad42528290a3f57cd65de0688f232da0b9db4a5007e69da55e2e13146c4f5c60983fc9c138e"
                  ]
                }
              }
            })");

  // Our initial call to get the hash of the last finalized block in the canon
  // chain.
  req_res_pairs.emplace(
      base::test::ParseJsonDict(
          R"({"id":1,"jsonrpc":"2.0","method":"chain_getFinalizedHead","params":[]})"),
      R"({"jsonrpc":"2.0","id":11,"error":{"code": 1234}})");

  // We need to grab the genesis block hash.
  req_res_pairs.emplace(
      base::test::ParseJsonDict(
          R"({"id":1,"jsonrpc":"2.0","method":"chain_getBlockHash","params":["00000000"]})"),
      R"({"jsonrpc":"2.0","id":1,"result":"0xe143f23803ac50e8f6f8e62695d1ce9e4e1d68aa36c1cd2cfd15340213f3423e"})");

  url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
      [&, self = this](const network::ResourceRequest& req) {
        auto pos = req_res_pairs.find(RequestBodyToJsonDict(req));
        if (pos != req_res_pairs.end()) {
          self->url_loader_factory_.AddResponse(testnet_url, pos->second);
        }
      }));

  auto pubkey = base::HexEncodeLower(
      keyring_service_->GetPolkadotPubKey(polkadot_testnet_account_->account_id)
          .value());

  EXPECT_EQ(pubkey,
            "d6b2a5cc606ea86342001dd036b301c15a5cba63c413cad5ca0e8f47e6fa9516");

  polkadot_wallet_service->GenerateSignedTransferExtrinsic(
      mojom::kPolkadotTestnet, polkadot_testnet_account_->account_id.Clone(),
      uint128_t{1234}, recipient_pubkey,
      base::BindOnce(
          [](base::RepeatingClosure quit_closure,
             base::expected<std::string, std::string> signed_extrinsic) {
            EXPECT_EQ(signed_extrinsic.error(), WalletInternalErrorMessage());
            quit_closure.Run();
          },
          task_environment_.QuitClosure()));

  task_environment_.RunUntilQuit();
}

TEST_F(PolkadotWalletServiceUnitTest,
       SignTransferExtrinsic_NoFinalizedBlockHeader) {
  url_loader_factory_.ClearResponses();

  std::string testnet_url =
      network_manager_
          ->GetKnownChain(mojom::kPolkadotTestnet, mojom::CoinType::DOT)
          ->rpc_endpoints.front()
          .spec();

  std::string mainnet_url =
      network_manager_
          ->GetKnownChain(mojom::kPolkadotMainnet, mojom::CoinType::DOT)
          ->rpc_endpoints.front()
          .spec();

  EXPECT_EQ(testnet_url, "https://polkadot-westend.wallet.brave.com/");
  EXPECT_EQ(mainnet_url, "https://polkadot-mainnet.wallet.brave.com/");

  auto polkadot_wallet_service = std::make_unique<PolkadotWalletService>(
      *keyring_service_, *network_manager_, shared_url_loader_factory_);

  UnlockWallet();
  EXPECT_EQ(url_loader_factory_.NumPending(), 2);

  std::array<uint8_t, kPolkadotSubstrateAccountIdSize> recipient_pubkey = {};
  EXPECT_TRUE(base::HexStringToSpan(kBob, recipient_pubkey));

  // For the two initial network calls that fetch the chain metadata.
  url_loader_factory_.AddResponse(testnet_url, R"(
    { "jsonrpc": "2.0",
      "result": "Westend",
      "id": 1 })");

  url_loader_factory_.AddResponse(mainnet_url, R"(
    { "jsonrpc": "2.0",
      "result": "Polkadot",
      "id": 1 })");

  url_loader_factory_.ClearResponses();

  base::flat_map<base::DictValue, std::string_view> req_res_pairs;
  // Our initial call to get account information so we have a usable nonce for
  // extinsic creation.
  req_res_pairs.emplace(base::test::ParseJsonDict(
                            R"(
            {
              "id":1,
              "jsonrpc":"2.0",
              "method":"state_queryStorageAt",
              "params":[
                [
                  "0x26AA394EEA5630E07C48AE0C9558CEF7B99D880EC681799C0CF30E8886371DA9DB9D3491A2D745CFE5E957DAA0A734BCD6B2A5CC606EA86342001DD036B301C15A5CBA63C413CAD5CA0E8F47E6FA9516"
                ]
              ]
            })"),
                        R"(
            {
              "jsonrpc":"2.0",
              "id":8,
              "result":[
                {
                  "block":"0xdcc9741f0258ede18a1684ff787c1591db8585f3154481cd162378fdd6677056",
                  "changes":[
                    [
                      "0x26aa394eea5630e07c48ae0c9558cef7b99d880ec681799c0cf30e8886371da9c6282eb06d1994674b75538b85244e4952707850d9298f5dfb0a3e5b23fcca39ea286c6def2db5716c996fb39db6477c",
                      "0x11000000000000000100000000000000be2bcb22d90800000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000080"
                    ]
                  ]
                }
              ]
            })");

  // Our initial call to get the most recent block header in the chain.
  req_res_pairs.emplace(
      base::test::ParseJsonDict(
          R"({"id":1,"jsonrpc":"2.0","method":"chain_getHeader","params":[]})"),
      R"(
            {
              "jsonrpc":"2.0",
              "id":10,
              "result": {
                "parentHash": "0x5834828e919dc0eccce83080104cc14f51f81330451bdf74bbc9bc1edba618f2",
                "number": "0x1c06358",
                "stateRoot": "0xc7ce642fe3c31724fc2808e179d807e5b0ba38a40eb5c08d98a41bd22eeeb9b5",
                "extrinsicsRoot": "0xba8c3f44dbde4e14831d61cb349f808f902bf4474112cf45c2c7d92bef4e754e",
                "digest": {
                  "logs": [
                    "0x0642414245b501030d0000002a92911100000000641ad61d0d848c4b374270406d092e8e5ca294e78ab6ea47d6da8ffe51e207758c49a32cac0d5d1e1ca56e4996b2136f42602debdf22478aff56b359adeadf06912cba0095e5b5abc3d7c0e03d9b628587010e6b44c6f9f40419b16bec4f5204",
                    "0x04424545468403b61c769585188ea959dc10c9434bfa46d57e818c7f17e047c75c42b3b1389c11",
                    "0x0542414245010186630051f53fb7ce6e02d1e020383357327c9e28ebfc399a96a98ad42528290a3f57cd65de0688f232da0b9db4a5007e69da55e2e13146c4f5c60983fc9c138e"
                  ]
                }
              }
            })");

  // Chained call from grabbing the most recent block header in the chain. Grab
  // the header of the corresponding parent hash for mortality calculations,
  // like how polkadot-js's api package does.
  // Implement the same algorithm as here:
  // https://github.com/polkadot-js/api/blob/eb34741c871ca8d029a9706ae989ba8ce865db0f/packages/api-derive/src/tx/signingInfo.ts#L41-L71
  req_res_pairs.emplace(
      base::test::ParseJsonDict(
          R"({"id":1,"jsonrpc":"2.0","method":"chain_getHeader","params":["5834828E919DC0ECCCE83080104CC14F51F81330451BDF74BBC9BC1EDBA618F2"]})"),
      R"(
            {
              "jsonrpc":"2.0",
              "id":12,
              "result":{
                "parentHash":"0x08cf134277f266acbce9727a13e7675ef472eb5932c693d7e924a5e12ca89b66",
                "number":"0x1c06357",
                "stateRoot":"0x6ac80fab8d74177b34675338269aad0d19d61dbe9adc2f421f77742bc4153d2d",
                "extrinsicsRoot":"0x2ee277dc4f23c4b3ab4e84cd7bb060cceca658933aed2735699e6d434cea39bc",
                "digest":{
                  "logs":[
                    "0x0642414245b50103000000002992911100000000d0053711418b7b2b943547844945a8eba27aa6163575a5b3aadd7cb78c878f6e2c8f4af3018fd5ad87d36dcca26914a9324722334311c8055e3889db61d5e60fa2eb601a422f67f1daf0a08d434c151430d1467855699ddb294e347e5ecdf90e",
                    "0x04424545468403ffb231bc483aba17682e4b1968b90559cab35ee078c021899f6751b2fc845085",
                    "0x05424142450101407020be76e604fdbb36bc1cd07dc71aa9eeddd537c26fec73178e48effde4607c728bb10f06c94299af0cb1a5835bb60bab5d9c3cc6eff8a0ade2a9a13f1780"
                  ]
                }
              }
            })");

  // Our initial call to get the hash of the last finalized block in the canon
  // chain.
  req_res_pairs.emplace(
      base::test::ParseJsonDict(
          R"({"id":1,"jsonrpc":"2.0","method":"chain_getFinalizedHead","params":[]})"),
      R"({"jsonrpc":"2.0","id":11,"result":"0x46e5afe42b1ff0c40ecc18d7ff97974f3bdf5dfda1e21d779644a7ea30a97d21"})");

  // Chained call, grab the block header using the hash of the finalized head.
  req_res_pairs.emplace(
      base::test::ParseJsonDict(
          R"({"id":1,"jsonrpc":"2.0","method":"chain_getHeader","params":["46E5AFE42B1FF0C40ECC18D7FF97974F3BDF5DFDA1E21D779644A7EA30A97D21"]})"),
      R"(
            {
              "jsonrpc":"2.0",
              "id":13,
              "error":{
                "code": 1234
              }
            })");

  // We need to grab the genesis block hash.
  req_res_pairs.emplace(
      base::test::ParseJsonDict(
          R"({"id":1,"jsonrpc":"2.0","method":"chain_getBlockHash","params":["00000000"]})"),
      R"({"jsonrpc":"2.0","id":1,"result":"0xe143f23803ac50e8f6f8e62695d1ce9e4e1d68aa36c1cd2cfd15340213f3423e"})");

  url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
      [&, self = this](const network::ResourceRequest& req) {
        auto pos = req_res_pairs.find(RequestBodyToJsonDict(req));
        if (pos != req_res_pairs.end()) {
          self->url_loader_factory_.AddResponse(testnet_url, pos->second);
        }
      }));

  auto pubkey = base::HexEncodeLower(
      keyring_service_->GetPolkadotPubKey(polkadot_testnet_account_->account_id)
          .value());

  EXPECT_EQ(pubkey,
            "d6b2a5cc606ea86342001dd036b301c15a5cba63c413cad5ca0e8f47e6fa9516");

  polkadot_wallet_service->GenerateSignedTransferExtrinsic(
      mojom::kPolkadotTestnet, polkadot_testnet_account_->account_id.Clone(),
      uint128_t{1234}, recipient_pubkey,
      base::BindOnce(
          [](base::RepeatingClosure quit_closure,
             base::expected<std::string, std::string> signed_extrinsic) {
            EXPECT_EQ(signed_extrinsic.error(), WalletInternalErrorMessage());
            quit_closure.Run();
          },
          task_environment_.QuitClosure()));

  task_environment_.RunUntilQuit();
}

TEST_F(PolkadotWalletServiceUnitTest, SignTransferExtrinsic_NoGenesisHash) {
  url_loader_factory_.ClearResponses();

  std::string testnet_url =
      network_manager_
          ->GetKnownChain(mojom::kPolkadotTestnet, mojom::CoinType::DOT)
          ->rpc_endpoints.front()
          .spec();

  std::string mainnet_url =
      network_manager_
          ->GetKnownChain(mojom::kPolkadotMainnet, mojom::CoinType::DOT)
          ->rpc_endpoints.front()
          .spec();

  EXPECT_EQ(testnet_url, "https://polkadot-westend.wallet.brave.com/");
  EXPECT_EQ(mainnet_url, "https://polkadot-mainnet.wallet.brave.com/");

  auto polkadot_wallet_service = std::make_unique<PolkadotWalletService>(
      *keyring_service_, *network_manager_, shared_url_loader_factory_);

  UnlockWallet();
  EXPECT_EQ(url_loader_factory_.NumPending(), 2);

  std::array<uint8_t, kPolkadotSubstrateAccountIdSize> recipient_pubkey = {};
  EXPECT_TRUE(base::HexStringToSpan(kBob, recipient_pubkey));

  // For the two initial network calls that fetch the chain metadata.
  url_loader_factory_.AddResponse(testnet_url, R"(
    { "jsonrpc": "2.0",
      "result": "Westend",
      "id": 1 })");

  url_loader_factory_.AddResponse(mainnet_url, R"(
    { "jsonrpc": "2.0",
      "result": "Polkadot",
      "id": 1 })");

  url_loader_factory_.ClearResponses();

  base::flat_map<base::DictValue, std::string_view> req_res_pairs;

  // Our initial call to get account information so we have a usable nonce for
  // extinsic creation.
  req_res_pairs.emplace(base::test::ParseJsonDict(
                            R"(
            {
              "id":1,
              "jsonrpc":"2.0",
              "method":"state_queryStorageAt",
              "params":[
                [
                  "0x26AA394EEA5630E07C48AE0C9558CEF7B99D880EC681799C0CF30E8886371DA9DB9D3491A2D745CFE5E957DAA0A734BCD6B2A5CC606EA86342001DD036B301C15A5CBA63C413CAD5CA0E8F47E6FA9516"
                ]
              ]
            })"),
                        R"(
            {
              "jsonrpc":"2.0",
              "id":8,
              "result":[
                {
                  "block":"0xdcc9741f0258ede18a1684ff787c1591db8585f3154481cd162378fdd6677056",
                  "changes":[
                    [
                      "0x26aa394eea5630e07c48ae0c9558cef7b99d880ec681799c0cf30e8886371da9c6282eb06d1994674b75538b85244e4952707850d9298f5dfb0a3e5b23fcca39ea286c6def2db5716c996fb39db6477c",
                      "0x11000000000000000100000000000000be2bcb22d90800000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000080"
                    ]
                  ]
                }
              ]
            })");

  // Our initial call to get the most recent block header in the chain.
  req_res_pairs.emplace(
      base::test::ParseJsonDict(
          R"({"id":1,"jsonrpc":"2.0","method":"chain_getHeader","params":[]})"),
      R"(
            {
              "jsonrpc":"2.0",
              "id":10,
              "result": {
                "parentHash": "0x5834828e919dc0eccce83080104cc14f51f81330451bdf74bbc9bc1edba618f2",
                "number": "0x1c06358",
                "stateRoot": "0xc7ce642fe3c31724fc2808e179d807e5b0ba38a40eb5c08d98a41bd22eeeb9b5",
                "extrinsicsRoot": "0xba8c3f44dbde4e14831d61cb349f808f902bf4474112cf45c2c7d92bef4e754e",
                "digest": {
                  "logs": [
                    "0x0642414245b501030d0000002a92911100000000641ad61d0d848c4b374270406d092e8e5ca294e78ab6ea47d6da8ffe51e207758c49a32cac0d5d1e1ca56e4996b2136f42602debdf22478aff56b359adeadf06912cba0095e5b5abc3d7c0e03d9b628587010e6b44c6f9f40419b16bec4f5204",
                    "0x04424545468403b61c769585188ea959dc10c9434bfa46d57e818c7f17e047c75c42b3b1389c11",
                    "0x0542414245010186630051f53fb7ce6e02d1e020383357327c9e28ebfc399a96a98ad42528290a3f57cd65de0688f232da0b9db4a5007e69da55e2e13146c4f5c60983fc9c138e"
                  ]
                }
              }
            })");

  // Chained call from grabbing the most recent block header in the chain. Grab
  // the header of the corresponding parent hash for mortality calculations,
  // like how polkadot-js's api package does.
  // Implement the same algorithm as here:
  // https://github.com/polkadot-js/api/blob/eb34741c871ca8d029a9706ae989ba8ce865db0f/packages/api-derive/src/tx/signingInfo.ts#L41-L71
  req_res_pairs.emplace(
      base::test::ParseJsonDict(
          R"({"id":1,"jsonrpc":"2.0","method":"chain_getHeader","params":["5834828E919DC0ECCCE83080104CC14F51F81330451BDF74BBC9BC1EDBA618F2"]})"),
      R"(
            {
              "jsonrpc":"2.0",
              "id":12,
              "result":{
                "parentHash":"0x08cf134277f266acbce9727a13e7675ef472eb5932c693d7e924a5e12ca89b66",
                "number":"0x1c06357",
                "stateRoot":"0x6ac80fab8d74177b34675338269aad0d19d61dbe9adc2f421f77742bc4153d2d",
                "extrinsicsRoot":"0x2ee277dc4f23c4b3ab4e84cd7bb060cceca658933aed2735699e6d434cea39bc",
                "digest":{
                  "logs":[
                    "0x0642414245b50103000000002992911100000000d0053711418b7b2b943547844945a8eba27aa6163575a5b3aadd7cb78c878f6e2c8f4af3018fd5ad87d36dcca26914a9324722334311c8055e3889db61d5e60fa2eb601a422f67f1daf0a08d434c151430d1467855699ddb294e347e5ecdf90e",
                    "0x04424545468403ffb231bc483aba17682e4b1968b90559cab35ee078c021899f6751b2fc845085",
                    "0x05424142450101407020be76e604fdbb36bc1cd07dc71aa9eeddd537c26fec73178e48effde4607c728bb10f06c94299af0cb1a5835bb60bab5d9c3cc6eff8a0ade2a9a13f1780"
                  ]
                }
              }
            })");

  // Our initial call to get the hash of the last finalized block in the canon
  // chain.
  req_res_pairs.emplace(
      base::test::ParseJsonDict(
          R"({"id":1,"jsonrpc":"2.0","method":"chain_getFinalizedHead","params":[]})"),
      R"({"jsonrpc":"2.0","id":11,"result":"0x46e5afe42b1ff0c40ecc18d7ff97974f3bdf5dfda1e21d779644a7ea30a97d21"})");

  // Chained call, grab the block header using the hash of the finalized head.
  req_res_pairs.emplace(
      base::test::ParseJsonDict(
          R"({"id":1,"jsonrpc":"2.0","method":"chain_getHeader","params":["46E5AFE42B1FF0C40ECC18D7FF97974F3BDF5DFDA1E21D779644A7EA30A97D21"]})"),
      R"(
            {
              "jsonrpc":"2.0",
              "id":13,
              "result":{
                "parentHash":"0xcf424e463b14b26905d4e2aaff455a3c149c3ccff5a1fc62203c0c07b711e3f4",
                "number":"0x1c06355",
                "stateRoot":"0x3a501ddbfc394d859401cd6d55f5743461ddb3a5aecfebb31f587c16ad23f505",
                "extrinsicsRoot":"0x8fc47b641e793ed938eae4d793636b2feb657bca97726a43ee3375a8e5b321a6",
                "digest":{
                  "logs":[
                    "0x0642414245b501030200000027929111000000008038b165beaf68d4ae8b7a3eae2055ecdfde0a0462993a43e522c709773da51a550d604eb90a671b88437f7f0d5e7f2e4efe323e2cee3992ffa2bcd3e5e10d07ff37c43e11e82263d2bc774942196e96c05a38bbbd820eff1cbf2441b2c59307",
                    "0x04424545468403cfdc267eac55b3225fe8d581f3d2f7d9ece28a564bb70b50dd04b829e893b78a",
                    "0x05424142450101fc0b1a7fcff42ffb1fcb8166843fb9b9eded36f64891deea28eea90da9215e70c605638b274f0c8517fc70d0c2b1442fd50ad933ee6cf7ceba600f762e2bd682"
                  ]
                }
              }
            })");

  // Grab the block hash of whichever block header we're using for signing the
  // extrinsic. The polkadot-js algorithm selects between either the finalized
  // head or the parent of the current head. In this case, the finalized block
  // wound up winning out. Theoretically, this can be replaced by just directly
  // hashing the block headers locally.
  req_res_pairs.emplace(
      base::test::ParseJsonDict(
          R"({"id":1,"jsonrpc":"2.0","method":"chain_getBlockHash","params":["01C06355"]})"),
      R"({"jsonrpc":"2.0","id":1,"result":"0x46e5afe42b1ff0c40ecc18d7ff97974f3bdf5dfda1e21d779644a7ea30a97d21"})");

  // Grab the runtime version of whichever block we're going to use for signing.
  req_res_pairs.emplace(
      base::test::ParseJsonDict(
          R"({"id":1,"jsonrpc":"2.0","method":"state_getRuntimeVersion","params":["cf424e463b14b26905d4e2aaff455a3c149c3ccff5a1fc62203c0c07b711e3f4"]})"),
      R"(
            {
              "jsonrpc":"2.0",
              "id":15,
              "result":{
                "specName":"westend",
                "implName":"parity-westend",
                "authoringVersion":2,
                "specVersion":1021000,
                "implVersion":0,
                "apis":[
                  ["0xdf6acb689907609b",5],["0x37e397fc7c91f5e4",2],["0xccd9de6396c899ca",1],["0x40fe3ad401f8959a",6],
                  ["0xd2bc9897eed08f15",3],["0xf78b278be53f454c",2],["0xaf2c0297a23e6d3d",15],["0x49eaaf1b548a0cb0",6],
                  ["0x91d5df18b0d2cf58",3],["0x2a5e924655399e60",1],["0xed99c5acb25eedf5",3],["0xcbca25e39f142387",2],
                  ["0x687ad44ad37f03c2",1],["0xab3c0572291feb8b",1],["0xbc9d89904f5b923f",1],["0x37c8bb1350a9a2a8",4],
                  ["0xf3ff14d5ab527059",3],["0x6ff52ee858e6c5bd",2],["0x91b1c8b16328eb92",2],["0x9ffb505aa738d69c",1],
                  ["0x17a6bc0d0062aeb3",1],["0x18ef58a3b67ba770",1],["0xfbc577b9d747efd6",1],["0x2609be83ac4468dc",1]
                ],
                "transactionVersion":27,
                "systemVersion":1,
                "stateVersion":1
              }
            })");

  // We need to grab the genesis block hash.
  req_res_pairs.emplace(
      base::test::ParseJsonDict(
          R"({"id":1,"jsonrpc":"2.0","method":"chain_getBlockHash","params":["00000000"]})"),
      R"({"jsonrpc":"2.0","id":1,"error":{"code":1234}})");

  url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
      [&, self = this](const network::ResourceRequest& req) {
        auto pos = req_res_pairs.find(RequestBodyToJsonDict(req));
        if (pos != req_res_pairs.end()) {
          self->url_loader_factory_.AddResponse(testnet_url, pos->second);
        }
      }));

  auto pubkey = base::HexEncodeLower(
      keyring_service_->GetPolkadotPubKey(polkadot_testnet_account_->account_id)
          .value());

  EXPECT_EQ(pubkey,
            "d6b2a5cc606ea86342001dd036b301c15a5cba63c413cad5ca0e8f47e6fa9516");

  polkadot_wallet_service->GenerateSignedTransferExtrinsic(
      mojom::kPolkadotTestnet, polkadot_testnet_account_->account_id.Clone(),
      uint128_t{1234}, recipient_pubkey,
      base::BindOnce(
          [](base::RepeatingClosure quit_closure,
             base::expected<std::string, std::string> signed_extrinsic) {
            EXPECT_EQ(signed_extrinsic.error(), WalletInternalErrorMessage());
            quit_closure.Run();
          },
          task_environment_.QuitClosure()));

  task_environment_.RunUntilQuit();
}

TEST_F(PolkadotWalletServiceUnitTest,
       SignTransferExtrinsic_NoSigningBlockHash) {
  url_loader_factory_.ClearResponses();

  std::string testnet_url =
      network_manager_
          ->GetKnownChain(mojom::kPolkadotTestnet, mojom::CoinType::DOT)
          ->rpc_endpoints.front()
          .spec();

  std::string mainnet_url =
      network_manager_
          ->GetKnownChain(mojom::kPolkadotMainnet, mojom::CoinType::DOT)
          ->rpc_endpoints.front()
          .spec();

  EXPECT_EQ(testnet_url, "https://polkadot-westend.wallet.brave.com/");
  EXPECT_EQ(mainnet_url, "https://polkadot-mainnet.wallet.brave.com/");

  auto polkadot_wallet_service = std::make_unique<PolkadotWalletService>(
      *keyring_service_, *network_manager_, shared_url_loader_factory_);

  UnlockWallet();
  EXPECT_EQ(url_loader_factory_.NumPending(), 2);

  std::array<uint8_t, kPolkadotSubstrateAccountIdSize> recipient_pubkey = {};
  EXPECT_TRUE(base::HexStringToSpan(kBob, recipient_pubkey));

  // For the two initial network calls that fetch the chain metadata.
  url_loader_factory_.AddResponse(testnet_url, R"(
    { "jsonrpc": "2.0",
      "result": "Westend",
      "id": 1 })");

  url_loader_factory_.AddResponse(mainnet_url, R"(
    { "jsonrpc": "2.0",
      "result": "Polkadot",
      "id": 1 })");

  url_loader_factory_.ClearResponses();

  base::flat_map<base::DictValue, std::string_view> req_res_pairs;
  // Our initial call to get account information so we have a usable nonce for
  // extinsic creation.
  req_res_pairs.emplace(base::test::ParseJsonDict(
                            R"(
            {
              "id":1,
              "jsonrpc":"2.0",
              "method":"state_queryStorageAt",
              "params":[
                [
                  "0x26AA394EEA5630E07C48AE0C9558CEF7B99D880EC681799C0CF30E8886371DA9DB9D3491A2D745CFE5E957DAA0A734BCD6B2A5CC606EA86342001DD036B301C15A5CBA63C413CAD5CA0E8F47E6FA9516"
                ]
              ]
            })"),
                        R"(
            {
              "jsonrpc":"2.0",
              "id":8,
              "result":[
                {
                  "block":"0xdcc9741f0258ede18a1684ff787c1591db8585f3154481cd162378fdd6677056",
                  "changes":[
                    [
                      "0x26aa394eea5630e07c48ae0c9558cef7b99d880ec681799c0cf30e8886371da9c6282eb06d1994674b75538b85244e4952707850d9298f5dfb0a3e5b23fcca39ea286c6def2db5716c996fb39db6477c",
                      "0x11000000000000000100000000000000be2bcb22d90800000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000080"
                    ]
                  ]
                }
              ]
            })");

  // Our initial call to get the most recent block header in the chain.
  req_res_pairs.emplace(
      base::test::ParseJsonDict(
          R"({"id":1,"jsonrpc":"2.0","method":"chain_getHeader","params":[]})"),
      R"(
            {
              "jsonrpc":"2.0",
              "id":10,
              "result": {
                "parentHash": "0x5834828e919dc0eccce83080104cc14f51f81330451bdf74bbc9bc1edba618f2",
                "number": "0x1c06358",
                "stateRoot": "0xc7ce642fe3c31724fc2808e179d807e5b0ba38a40eb5c08d98a41bd22eeeb9b5",
                "extrinsicsRoot": "0xba8c3f44dbde4e14831d61cb349f808f902bf4474112cf45c2c7d92bef4e754e",
                "digest": {
                  "logs": [
                    "0x0642414245b501030d0000002a92911100000000641ad61d0d848c4b374270406d092e8e5ca294e78ab6ea47d6da8ffe51e207758c49a32cac0d5d1e1ca56e4996b2136f42602debdf22478aff56b359adeadf06912cba0095e5b5abc3d7c0e03d9b628587010e6b44c6f9f40419b16bec4f5204",
                    "0x04424545468403b61c769585188ea959dc10c9434bfa46d57e818c7f17e047c75c42b3b1389c11",
                    "0x0542414245010186630051f53fb7ce6e02d1e020383357327c9e28ebfc399a96a98ad42528290a3f57cd65de0688f232da0b9db4a5007e69da55e2e13146c4f5c60983fc9c138e"
                  ]
                }
              }
            })");

  // Chained call from grabbing the most recent block header in the chain. Grab
  // the header of the corresponding parent hash for mortality calculations,
  // like how polkadot-js's api package does.
  // Implement the same algorithm as here:
  // https://github.com/polkadot-js/api/blob/eb34741c871ca8d029a9706ae989ba8ce865db0f/packages/api-derive/src/tx/signingInfo.ts#L41-L71
  req_res_pairs.emplace(
      base::test::ParseJsonDict(
          R"({"id":1,"jsonrpc":"2.0","method":"chain_getHeader","params":["5834828E919DC0ECCCE83080104CC14F51F81330451BDF74BBC9BC1EDBA618F2"]})"),
      R"(
            {
              "jsonrpc":"2.0",
              "id":12,
              "result":{
                "parentHash":"0x08cf134277f266acbce9727a13e7675ef472eb5932c693d7e924a5e12ca89b66",
                "number":"0x1c06357",
                "stateRoot":"0x6ac80fab8d74177b34675338269aad0d19d61dbe9adc2f421f77742bc4153d2d",
                "extrinsicsRoot":"0x2ee277dc4f23c4b3ab4e84cd7bb060cceca658933aed2735699e6d434cea39bc",
                "digest":{
                  "logs":[
                    "0x0642414245b50103000000002992911100000000d0053711418b7b2b943547844945a8eba27aa6163575a5b3aadd7cb78c878f6e2c8f4af3018fd5ad87d36dcca26914a9324722334311c8055e3889db61d5e60fa2eb601a422f67f1daf0a08d434c151430d1467855699ddb294e347e5ecdf90e",
                    "0x04424545468403ffb231bc483aba17682e4b1968b90559cab35ee078c021899f6751b2fc845085",
                    "0x05424142450101407020be76e604fdbb36bc1cd07dc71aa9eeddd537c26fec73178e48effde4607c728bb10f06c94299af0cb1a5835bb60bab5d9c3cc6eff8a0ade2a9a13f1780"
                  ]
                }
              }
            })");

  // Our initial call to get the hash of the last finalized block in the canon
  // chain.
  req_res_pairs.emplace(
      base::test::ParseJsonDict(
          R"({"id":1,"jsonrpc":"2.0","method":"chain_getFinalizedHead","params":[]})"),
      R"({"jsonrpc":"2.0","id":11,"result":"0x46e5afe42b1ff0c40ecc18d7ff97974f3bdf5dfda1e21d779644a7ea30a97d21"})");

  // Chained call, grab the block header using the hash of the finalized head.
  req_res_pairs.emplace(
      base::test::ParseJsonDict(
          R"({"id":1,"jsonrpc":"2.0","method":"chain_getHeader","params":["46E5AFE42B1FF0C40ECC18D7FF97974F3BDF5DFDA1E21D779644A7EA30A97D21"]})"),
      R"(
            {
              "jsonrpc":"2.0",
              "id":13,
              "result":{
                "parentHash":"0xcf424e463b14b26905d4e2aaff455a3c149c3ccff5a1fc62203c0c07b711e3f4",
                "number":"0x1c06355",
                "stateRoot":"0x3a501ddbfc394d859401cd6d55f5743461ddb3a5aecfebb31f587c16ad23f505",
                "extrinsicsRoot":"0x8fc47b641e793ed938eae4d793636b2feb657bca97726a43ee3375a8e5b321a6",
                "digest":{
                  "logs":[
                    "0x0642414245b501030200000027929111000000008038b165beaf68d4ae8b7a3eae2055ecdfde0a0462993a43e522c709773da51a550d604eb90a671b88437f7f0d5e7f2e4efe323e2cee3992ffa2bcd3e5e10d07ff37c43e11e82263d2bc774942196e96c05a38bbbd820eff1cbf2441b2c59307",
                    "0x04424545468403cfdc267eac55b3225fe8d581f3d2f7d9ece28a564bb70b50dd04b829e893b78a",
                    "0x05424142450101fc0b1a7fcff42ffb1fcb8166843fb9b9eded36f64891deea28eea90da9215e70c605638b274f0c8517fc70d0c2b1442fd50ad933ee6cf7ceba600f762e2bd682"
                  ]
                }
              }
            })");

  // Grab the block hash of whichever block header we're using for signing the
  // extrinsic. The polkadot-js algorithm selects between either the finalized
  // head or the parent of the current head. In this case, the finalized block
  // wound up winning out. Theoretically, this can be replaced by just directly
  // hashing the block headers locally.
  req_res_pairs.emplace(
      base::test::ParseJsonDict(
          R"({"id":1,"jsonrpc":"2.0","method":"chain_getBlockHash","params":["01C06355"]})"),
      R"({"jsonrpc":"2.0","id":1,"error":{"code":1234}})");

  // We need to grab the genesis block hash.
  req_res_pairs.emplace(
      base::test::ParseJsonDict(
          R"({"id":1,"jsonrpc":"2.0","method":"chain_getBlockHash","params":["00000000"]})"),
      R"({"jsonrpc":"2.0","id":1,"result":"0xe143f23803ac50e8f6f8e62695d1ce9e4e1d68aa36c1cd2cfd15340213f3423e"})");

  url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
      [&, self = this](const network::ResourceRequest& req) {
        auto pos = req_res_pairs.find(RequestBodyToJsonDict(req));
        if (pos != req_res_pairs.end()) {
          self->url_loader_factory_.AddResponse(testnet_url, pos->second);
        }
      }));

  auto pubkey = base::HexEncodeLower(
      keyring_service_->GetPolkadotPubKey(polkadot_testnet_account_->account_id)
          .value());

  EXPECT_EQ(pubkey,
            "d6b2a5cc606ea86342001dd036b301c15a5cba63c413cad5ca0e8f47e6fa9516");

  polkadot_wallet_service->GenerateSignedTransferExtrinsic(
      mojom::kPolkadotTestnet, polkadot_testnet_account_->account_id.Clone(),
      uint128_t{1234}, recipient_pubkey,
      base::BindOnce(
          [](base::RepeatingClosure quit_closure,
             base::expected<std::string, std::string> signed_extrinsic) {
            EXPECT_EQ(signed_extrinsic.error(), WalletInternalErrorMessage());
            quit_closure.Run();
          },
          task_environment_.QuitClosure()));

  task_environment_.RunUntilQuit();
}

TEST_F(PolkadotWalletServiceUnitTest, SignTransferExtrinsic_NoRuntimeVersion) {
  url_loader_factory_.ClearResponses();

  std::string testnet_url =
      network_manager_
          ->GetKnownChain(mojom::kPolkadotTestnet, mojom::CoinType::DOT)
          ->rpc_endpoints.front()
          .spec();

  std::string mainnet_url =
      network_manager_
          ->GetKnownChain(mojom::kPolkadotMainnet, mojom::CoinType::DOT)
          ->rpc_endpoints.front()
          .spec();

  EXPECT_EQ(testnet_url, "https://polkadot-westend.wallet.brave.com/");
  EXPECT_EQ(mainnet_url, "https://polkadot-mainnet.wallet.brave.com/");

  auto polkadot_wallet_service = std::make_unique<PolkadotWalletService>(
      *keyring_service_, *network_manager_, shared_url_loader_factory_);

  UnlockWallet();
  EXPECT_EQ(url_loader_factory_.NumPending(), 2);

  std::array<uint8_t, kPolkadotSubstrateAccountIdSize> recipient_pubkey = {};
  EXPECT_TRUE(base::HexStringToSpan(kBob, recipient_pubkey));

  // For the two initial network calls that fetch the chain metadata.
  url_loader_factory_.AddResponse(testnet_url, R"(
    { "jsonrpc": "2.0",
      "result": "Westend",
      "id": 1 })");

  url_loader_factory_.AddResponse(mainnet_url, R"(
    { "jsonrpc": "2.0",
      "result": "Polkadot",
      "id": 1 })");

  url_loader_factory_.ClearResponses();

  base::flat_map<base::DictValue, std::string_view> req_res_pairs;
  // Our initial call to get account information so we have a usable nonce for
  // extinsic creation.
  req_res_pairs.emplace(base::test::ParseJsonDict(
                            R"(
            {
              "id":1,
              "jsonrpc":"2.0",
              "method":"state_queryStorageAt",
              "params":[
                [
                  "0x26AA394EEA5630E07C48AE0C9558CEF7B99D880EC681799C0CF30E8886371DA9DB9D3491A2D745CFE5E957DAA0A734BCD6B2A5CC606EA86342001DD036B301C15A5CBA63C413CAD5CA0E8F47E6FA9516"
                ]
              ]
            })"),
                        R"(
            {
              "jsonrpc":"2.0",
              "id":8,
              "result":[
                {
                  "block":"0xdcc9741f0258ede18a1684ff787c1591db8585f3154481cd162378fdd6677056",
                  "changes":[
                    [
                      "0x26aa394eea5630e07c48ae0c9558cef7b99d880ec681799c0cf30e8886371da9c6282eb06d1994674b75538b85244e4952707850d9298f5dfb0a3e5b23fcca39ea286c6def2db5716c996fb39db6477c",
                      "0x11000000000000000100000000000000be2bcb22d90800000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000080"
                    ]
                  ]
                }
              ]
            })");

  // Our initial call to get the most recent block header in the chain.
  req_res_pairs.emplace(
      base::test::ParseJsonDict(
          R"({"id":1,"jsonrpc":"2.0","method":"chain_getHeader","params":[]})"),
      R"(
            {
              "jsonrpc":"2.0",
              "id":10,
              "result": {
                "parentHash": "0x5834828e919dc0eccce83080104cc14f51f81330451bdf74bbc9bc1edba618f2",
                "number": "0x1c06358",
                "stateRoot": "0xc7ce642fe3c31724fc2808e179d807e5b0ba38a40eb5c08d98a41bd22eeeb9b5",
                "extrinsicsRoot": "0xba8c3f44dbde4e14831d61cb349f808f902bf4474112cf45c2c7d92bef4e754e",
                "digest": {
                  "logs": [
                    "0x0642414245b501030d0000002a92911100000000641ad61d0d848c4b374270406d092e8e5ca294e78ab6ea47d6da8ffe51e207758c49a32cac0d5d1e1ca56e4996b2136f42602debdf22478aff56b359adeadf06912cba0095e5b5abc3d7c0e03d9b628587010e6b44c6f9f40419b16bec4f5204",
                    "0x04424545468403b61c769585188ea959dc10c9434bfa46d57e818c7f17e047c75c42b3b1389c11",
                    "0x0542414245010186630051f53fb7ce6e02d1e020383357327c9e28ebfc399a96a98ad42528290a3f57cd65de0688f232da0b9db4a5007e69da55e2e13146c4f5c60983fc9c138e"
                  ]
                }
              }
            })");

  // Chained call from grabbing the most recent block header in the chain. Grab
  // the header of the corresponding parent hash for mortality calculations,
  // like how polkadot-js's api package does.
  // Implement the same algorithm as here:
  // https://github.com/polkadot-js/api/blob/eb34741c871ca8d029a9706ae989ba8ce865db0f/packages/api-derive/src/tx/signingInfo.ts#L41-L71
  req_res_pairs.emplace(
      base::test::ParseJsonDict(
          R"({"id":1,"jsonrpc":"2.0","method":"chain_getHeader","params":["5834828E919DC0ECCCE83080104CC14F51F81330451BDF74BBC9BC1EDBA618F2"]})"),
      R"(
            {
              "jsonrpc":"2.0",
              "id":12,
              "result":{
                "parentHash":"0x08cf134277f266acbce9727a13e7675ef472eb5932c693d7e924a5e12ca89b66",
                "number":"0x1c06357",
                "stateRoot":"0x6ac80fab8d74177b34675338269aad0d19d61dbe9adc2f421f77742bc4153d2d",
                "extrinsicsRoot":"0x2ee277dc4f23c4b3ab4e84cd7bb060cceca658933aed2735699e6d434cea39bc",
                "digest":{
                  "logs":[
                    "0x0642414245b50103000000002992911100000000d0053711418b7b2b943547844945a8eba27aa6163575a5b3aadd7cb78c878f6e2c8f4af3018fd5ad87d36dcca26914a9324722334311c8055e3889db61d5e60fa2eb601a422f67f1daf0a08d434c151430d1467855699ddb294e347e5ecdf90e",
                    "0x04424545468403ffb231bc483aba17682e4b1968b90559cab35ee078c021899f6751b2fc845085",
                    "0x05424142450101407020be76e604fdbb36bc1cd07dc71aa9eeddd537c26fec73178e48effde4607c728bb10f06c94299af0cb1a5835bb60bab5d9c3cc6eff8a0ade2a9a13f1780"
                  ]
                }
              }
            })");

  // Our initial call to get the hash of the last finalized block in the canon
  // chain.
  req_res_pairs.emplace(
      base::test::ParseJsonDict(
          R"({"id":1,"jsonrpc":"2.0","method":"chain_getFinalizedHead","params":[]})"),
      R"({"jsonrpc":"2.0","id":11,"result":"0x46e5afe42b1ff0c40ecc18d7ff97974f3bdf5dfda1e21d779644a7ea30a97d21"})");

  // Chained call, grab the block header using the hash of the finalized head.
  req_res_pairs.emplace(
      base::test::ParseJsonDict(
          R"({"id":1,"jsonrpc":"2.0","method":"chain_getHeader","params":["46E5AFE42B1FF0C40ECC18D7FF97974F3BDF5DFDA1E21D779644A7EA30A97D21"]})"),
      R"(
            {
              "jsonrpc":"2.0",
              "id":13,
              "result":{
                "parentHash":"0xcf424e463b14b26905d4e2aaff455a3c149c3ccff5a1fc62203c0c07b711e3f4",
                "number":"0x1c06355",
                "stateRoot":"0x3a501ddbfc394d859401cd6d55f5743461ddb3a5aecfebb31f587c16ad23f505",
                "extrinsicsRoot":"0x8fc47b641e793ed938eae4d793636b2feb657bca97726a43ee3375a8e5b321a6",
                "digest":{
                  "logs":[
                    "0x0642414245b501030200000027929111000000008038b165beaf68d4ae8b7a3eae2055ecdfde0a0462993a43e522c709773da51a550d604eb90a671b88437f7f0d5e7f2e4efe323e2cee3992ffa2bcd3e5e10d07ff37c43e11e82263d2bc774942196e96c05a38bbbd820eff1cbf2441b2c59307",
                    "0x04424545468403cfdc267eac55b3225fe8d581f3d2f7d9ece28a564bb70b50dd04b829e893b78a",
                    "0x05424142450101fc0b1a7fcff42ffb1fcb8166843fb9b9eded36f64891deea28eea90da9215e70c605638b274f0c8517fc70d0c2b1442fd50ad933ee6cf7ceba600f762e2bd682"
                  ]
                }
              }
            })");

  // Grab the block hash of whichever block header we're using for signing the
  // extrinsic. The polkadot-js algorithm selects between either the finalized
  // head or the parent of the current head. In this case, the finalized block
  // wound up winning out. Theoretically, this can be replaced by just directly
  // hashing the block headers locally.
  req_res_pairs.emplace(
      base::test::ParseJsonDict(
          R"({"id":1,"jsonrpc":"2.0","method":"chain_getBlockHash","params":["01C06355"]})"),
      R"({"jsonrpc":"2.0","id":1,"result":"0x46e5afe42b1ff0c40ecc18d7ff97974f3bdf5dfda1e21d779644a7ea30a97d21"})");

  // Grab the runtime version of whichever block we're going to use for signing.
  req_res_pairs.emplace(
      base::test::ParseJsonDict(
          R"({"id":1,"jsonrpc":"2.0","method":"state_getRuntimeVersion","params":["cf424e463b14b26905d4e2aaff455a3c149c3ccff5a1fc62203c0c07b711e3f4"]})"),
      R"(
            {
              "jsonrpc":"2.0",
              "id":15,
              "error":{
                "code":1234
              }
            })");

  // We need to grab the genesis block hash.
  req_res_pairs.emplace(
      base::test::ParseJsonDict(
          R"({"id":1,"jsonrpc":"2.0","method":"chain_getBlockHash","params":["00000000"]})"),
      R"({"jsonrpc":"2.0","id":1,"result":"0xe143f23803ac50e8f6f8e62695d1ce9e4e1d68aa36c1cd2cfd15340213f3423e"})");

  url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
      [&, self = this](const network::ResourceRequest& req) {
        auto pos = req_res_pairs.find(RequestBodyToJsonDict(req));
        if (pos != req_res_pairs.end()) {
          self->url_loader_factory_.AddResponse(testnet_url, pos->second);
        }
      }));

  auto pubkey = base::HexEncodeLower(
      keyring_service_->GetPolkadotPubKey(polkadot_testnet_account_->account_id)
          .value());

  EXPECT_EQ(pubkey,
            "d6b2a5cc606ea86342001dd036b301c15a5cba63c413cad5ca0e8f47e6fa9516");

  polkadot_wallet_service->GenerateSignedTransferExtrinsic(
      mojom::kPolkadotTestnet, polkadot_testnet_account_->account_id.Clone(),
      uint128_t{1234}, recipient_pubkey,
      base::BindOnce(
          [](base::RepeatingClosure quit_closure,
             base::expected<std::string, std::string> signed_extrinsic) {
            EXPECT_EQ(signed_extrinsic.error(), WalletInternalErrorMessage());
            quit_closure.Run();
          },
          task_environment_.QuitClosure()));

  task_environment_.RunUntilQuit();
}

}  // namespace brave_wallet
