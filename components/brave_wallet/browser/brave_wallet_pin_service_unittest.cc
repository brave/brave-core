// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_wallet/browser/brave_wallet_pin_service.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/json/json_reader.h"
#include "base/json/values_util.h"
#include "base/test/bind.h"
#include "base/time/time_override.h"
#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/ipfs/pin/ipfs_local_pin_service.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "components/prefs/testing_pref_service.h"
#include "content/public/test/browser_task_environment.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using base::subtle::ScopedTimeClockOverrides;
using testing::_;

namespace brave_wallet {
namespace {
const char kMonkey1Path[] =
    "nft.local.60.0x1.0xbc4ca0eda7647a8ab7c2061c2e118a18a936f13d.0x1";
const char kMonkey2Path[] =
    "nft.local.60.0x1.0xbc4ca0eda7647a8ab7c2061c2e118a18a936f13d.0x2";
const char kMonkey3Path[] =
    "nft.nftstorage.60.0x1.0xbc4ca0eda7647a8ab7c2061c2e118a18a936f13d.0x2";

const char kMonkey1Url[] =
    "ipfs://QmeSjSinHpPnmXmspMjwiXyN6zS4E9zccariGR3jxcaWtq/2413";
const char kMonkey1[] =
    R"({"image":"ipfs://Qmcyc7tm9sZB9JnvLgejPTwdzjjNjDMiRWCUvaZAfp6cUg",
                        "attributes":[
                        {"trait_type":"Mouth","value":"Bored Cigarette"},
                        {"trait_type":"Fur","value":"Zombie"},
                        {"trait_type":"Background","value":"Purple"},
                        {"trait_type":"Eyes","value":"Closed"},
                        {"trait_type":"Clothes","value":"Toga"},
                        {"trait_type":"Hat","value":"Cowboy Hat"}]})";

base::Time g_overridden_now;
std::unique_ptr<ScopedTimeClockOverrides> OverrideWithTimeNow(
    const base::Time& overridden_now) {
  g_overridden_now = overridden_now;
  return std::make_unique<ScopedTimeClockOverrides>(
      []() { return g_overridden_now; }, nullptr, nullptr);
}

class MockIpfsLocalPinService : public ipfs::IpfsLocalPinService {
 public:
  MockIpfsLocalPinService() {}

  ~MockIpfsLocalPinService() override {}

  MOCK_METHOD3(AddPins,
               void(const std::string& prefix,
                    const std::vector<std::string>& cids,
                    ipfs::AddPinCallback callback));
  MOCK_METHOD2(RemovePins,
               void(const std::string& prefix,
                    ipfs::RemovePinCallback callback));
  MOCK_METHOD3(ValidatePins,
               void(const std::string& prefix,
                    const std::vector<std::string>& cids,
                    ipfs::ValidatePinsCallback callback));
};

class MockJsonRpcService : public JsonRpcService {
 public:
  MockJsonRpcService() : JsonRpcService() {}

  MOCK_METHOD4(GetERC721Metadata,
               void(const std::string& contract_address,
                    const std::string& token_id,
                    const std::string& chain_id,
                    GetERC721MetadataCallback callback));

  ~MockJsonRpcService() override {}
};

class MockIpfsService : public IpfsService {
 public:
  MockIpfsService() = default;
  ~MockIpfsService() override = default;

  MOCK_METHOD1(AddObserver, void(ipfs::IpfsServiceObserver* observer));
  MOCK_METHOD0(IsDaemonLaunched, bool());
};

}  // namespace

class BraveWalletPinServiceTest : public testing::Test {
 public:
  BraveWalletPinServiceTest() = default;

  BraveWalletPinService* service() { return brave_wallet_pin_service_.get(); }

 protected:
  void SetUp() override {
    auto* registry = pref_service_.registry();
    registry->RegisterDictionaryPref(kPinnedNFTAssets);
    brave_wallet_pin_service_ = std::make_unique<BraveWalletPinService>(
        GetPrefs(), GetJsonRpcService(), GetIpfsLocalPinService(),
        GetIpfsService());
  }

  PrefService* GetPrefs() { return &pref_service_; }

  testing::NiceMock<MockJsonRpcService>* GetJsonRpcService() {
    return &json_rpc_service_;
  }

  testing::NiceMock<MockIpfsLocalPinService>* GetIpfsLocalPinService() {
    return &ipfs_local_pin_service_;
  }

  testing::NiceMock<MockIpfsService>* GetIpfsService() {
    return &ipfs_service_;
  }

  testing::NiceMock<MockIpfsLocalPinService> ipfs_local_pin_service_;
  testing::NiceMock<MockJsonRpcService> json_rpc_service_;
  testing::NiceMock<MockIpfsService> ipfs_service_;

  std::unique_ptr<BraveWalletPinService> brave_wallet_pin_service_;
  TestingPrefServiceSimple pref_service_;
  content::BrowserTaskEnvironment task_environment_;
};

TEST_F(BraveWalletPinServiceTest, AddPin) {
  {
    ON_CALL(*GetJsonRpcService(), GetERC721Metadata(_, _, _, _))
        .WillByDefault(::testing::Invoke(
            [](const std::string& contract_address, const std::string& token_id,
               const std::string& chain_id,
               MockJsonRpcService::GetERC721MetadataCallback callback) {
              EXPECT_EQ("0x1", chain_id);
              EXPECT_EQ("0xbc4ca0eda7647a8ab7c2061c2e118a18a936f13d",
                        contract_address);
              EXPECT_EQ("0x1", token_id);
              std::move(callback).Run(kMonkey1Url, kMonkey1,
                                      mojom::ProviderError::kSuccess, "");
            }));
    ON_CALL(*GetIpfsLocalPinService(), AddPins(_, _, _))
        .WillByDefault(::testing::Invoke(
            [](const std::string& prefix, const std::vector<std::string>& cids,
               ipfs::AddPinCallback callback) {
              EXPECT_EQ(kMonkey1Path, prefix);
              EXPECT_EQ("QmeSjSinHpPnmXmspMjwiXyN6zS4E9zccariGR3jxcaWtq",
                        cids.at(0));
              EXPECT_EQ("Qmcyc7tm9sZB9JnvLgejPTwdzjjNjDMiRWCUvaZAfp6cUg",
                        cids.at(1));
              std::move(callback).Run(true);
            }));

    auto scoped_override = OverrideWithTimeNow(base::Time::FromTimeT(123u));

    mojom::BlockchainTokenPtr token =
        BraveWalletPinService::TokenFromPrefPath(kMonkey1Path);
    token->is_erc721 = true;
    absl::optional<bool> call_status;
    service()->AddPin(
        std::move(token), absl::nullopt,
        base::BindLambdaForTesting(
            [&call_status](bool result, mojom::PinErrorPtr error) {
              call_status = result;
              EXPECT_FALSE(error);
            }));
    EXPECT_TRUE(call_status.value());

    const base::Value::Dict* token_record =
        GetPrefs()
            ->GetDict(kPinnedNFTAssets)
            .FindDictByDottedPath(kMonkey1Path);

    base::Value::List expected_cids;
    expected_cids.Append("QmeSjSinHpPnmXmspMjwiXyN6zS4E9zccariGR3jxcaWtq");
    expected_cids.Append("Qmcyc7tm9sZB9JnvLgejPTwdzjjNjDMiRWCUvaZAfp6cUg");

    EXPECT_EQ(BraveWalletPinService::StatusToString(
                  mojom::TokenPinStatusCode::STATUS_PINNED),
              *(token_record->FindString("status")));
    EXPECT_EQ(nullptr, token_record->FindDict("error"));
    EXPECT_EQ(expected_cids, *(token_record->FindList("cids")));
    EXPECT_EQ(base::Time::FromTimeT(123u),
              base::ValueToTime(token_record->Find("validate_timestamp")));
  }

  {
    ON_CALL(*GetJsonRpcService(), GetERC721Metadata(_, _, _, _))
        .WillByDefault(::testing::Invoke(
            [](const std::string& contract_address, const std::string& token_id,
               const std::string& chain_id,
               MockJsonRpcService::GetERC721MetadataCallback callback) {
              EXPECT_EQ("0x1", chain_id);
              EXPECT_EQ("0xbc4ca0eda7647a8ab7c2061c2e118a18a936f13d",
                        contract_address);
              EXPECT_EQ("0x2", token_id);
              std::move(callback).Run(
                  "", "", mojom::ProviderError::kParsingError, "Parsing error");
            }));

    mojom::BlockchainTokenPtr token =
        BraveWalletPinService::TokenFromPrefPath(kMonkey2Path);
    token->is_erc721 = true;
    absl::optional<bool> call_status;
    service()->AddPin(
        std::move(token), absl::nullopt,
        base::BindLambdaForTesting(
            [&call_status](bool result, mojom::PinErrorPtr error) {
              call_status = result;
              EXPECT_TRUE(error);
            }));

    EXPECT_FALSE(call_status.value());

    const base::Value::Dict* token_record =
        GetPrefs()
            ->GetDict(kPinnedNFTAssets)
            .FindDictByDottedPath(kMonkey2Path);

    EXPECT_EQ(BraveWalletPinService::StatusToString(
                  mojom::TokenPinStatusCode::STATUS_PINNING_FAILED),
              *(token_record->FindString("status")));
    EXPECT_EQ(BraveWalletPinService::ErrorCodeToString(
                  mojom::WalletPinServiceErrorCode::ERR_FETCH_METADATA_FAILED),
              token_record->FindByDottedPath("error.error_code")->GetString());
  }
}

TEST_F(BraveWalletPinServiceTest, RemovePin) {
  {
    ScopedDictPrefUpdate update(GetPrefs(), kPinnedNFTAssets);
    base::Value::Dict& update_dict = update.Get();

    base::Value::Dict item;
    item.Set("status", "pinned");
    item.Set("validation_timestamp", "123");
    base::Value::List cids;
    cids.Append("QmeSjSinHpPnmXmspMjwiXyN6zS4E9zccariGR3jxcaWtq");
    cids.Append("Qmcyc7tm9sZB9JnvLgejPTwdzjjNjDMiRWCUvaZAfp6cUg");

    update_dict.SetByDottedPath(kMonkey1Path, std::move(item));
  }

  {
    ON_CALL(*GetIpfsLocalPinService(), RemovePins(_, _))
        .WillByDefault(::testing::Invoke(
            [](const std::string& prefix, ipfs::RemovePinCallback callback) {
              EXPECT_EQ(kMonkey1Path, prefix);
              std::move(callback).Run(false);
            }));

    mojom::BlockchainTokenPtr token =
        BraveWalletPinService::TokenFromPrefPath(kMonkey1Path);
    token->is_erc721 = true;
    absl::optional<bool> remove_status;
    service()->RemovePin(
        std::move(token), absl::nullopt,
        base::BindLambdaForTesting(
            [&remove_status](bool status, mojom::PinErrorPtr error) {
              remove_status = status;
            }));
    EXPECT_FALSE(remove_status.value());
    EXPECT_EQ(BraveWalletPinService::StatusToString(
                  mojom::TokenPinStatusCode::STATUS_UNPINNING_FAILED),
              *(GetPrefs()
                    ->GetDict(kPinnedNFTAssets)
                    .FindByDottedPath(kMonkey1Path)
                    ->FindStringKey("status")));
  }

  {
    ON_CALL(*GetIpfsLocalPinService(), RemovePins(_, _))
        .WillByDefault(::testing::Invoke(
            [](const std::string& prefix, ipfs::RemovePinCallback callback) {
              EXPECT_EQ(kMonkey1Path, prefix);
              std::move(callback).Run(true);
            }));

    mojom::BlockchainTokenPtr token =
        BraveWalletPinService::TokenFromPrefPath(kMonkey1Path);
    token->is_erc721 = true;
    absl::optional<bool> remove_status;
    service()->RemovePin(
        std::move(token), absl::nullopt,
        base::BindLambdaForTesting(
            [&remove_status](bool status, mojom::PinErrorPtr error) {
              remove_status = status;
            }));
    EXPECT_TRUE(remove_status.value());
    EXPECT_EQ(nullptr, GetPrefs()
                           ->GetDict(kPinnedNFTAssets)
                           .FindDictByDottedPath(kMonkey1Path));
  }
}

TEST_F(BraveWalletPinServiceTest, ValidatePin) {
  {
    ScopedDictPrefUpdate update(GetPrefs(), kPinnedNFTAssets);
    base::Value::Dict& update_dict = update.Get();

    base::Value::Dict item;
    item.Set("status", "pinned");
    item.Set("validate_timestamp", base::TimeToValue(base::Time::Now()));
    base::Value::List cids;
    cids.Append("QmeSjSinHpPnmXmspMjwiXyN6zS4E9zccariGR3jxcaWtq");
    cids.Append("Qmcyc7tm9sZB9JnvLgejPTwdzjjNjDMiRWCUvaZAfp6cUg");
    item.Set("cids", std::move(cids));

    update_dict.SetByDottedPath(kMonkey1Path, std::move(item));
  }

  {
    auto scoped_override = OverrideWithTimeNow(base::Time::FromTimeT(345u));

    mojom::BlockchainTokenPtr token =
        BraveWalletPinService::TokenFromPrefPath(kMonkey1Path);
    token->is_erc721 = true;
    ON_CALL(*GetIpfsLocalPinService(), ValidatePins(_, _, _))
        .WillByDefault(::testing::Invoke(
            [](const std::string& prefix, const std::vector<std::string>& cids,
               ipfs::ValidatePinsCallback callback) {
              EXPECT_EQ("QmeSjSinHpPnmXmspMjwiXyN6zS4E9zccariGR3jxcaWtq",
                        cids.at(0));
              EXPECT_EQ("Qmcyc7tm9sZB9JnvLgejPTwdzjjNjDMiRWCUvaZAfp6cUg",
                        cids.at(1));
              EXPECT_EQ(kMonkey1Path, prefix);
              std::move(callback).Run(true);
            }));

    absl::optional<bool> validate_status;
    service()->Validate(
        std::move(token), absl::nullopt,
        base::BindLambdaForTesting(
            [&validate_status](bool status, mojom::PinErrorPtr error) {
              validate_status = status;
            }));
    EXPECT_TRUE(validate_status.value());

    const base::Value::Dict* token_record =
        GetPrefs()
            ->GetDict(kPinnedNFTAssets)
            .FindDictByDottedPath(kMonkey1Path);
    EXPECT_EQ(base::Time::FromTimeT(345u),
              base::ValueToTime(token_record->Find("validate_timestamp")));
  }

  {
    mojom::BlockchainTokenPtr token =
        BraveWalletPinService::TokenFromPrefPath(kMonkey1Path);
    token->is_erc721 = true;
    ON_CALL(*GetIpfsLocalPinService(), ValidatePins(_, _, _))
        .WillByDefault(::testing::Invoke(
            [](const std::string& prefix, const std::vector<std::string>& cids,
               ipfs::ValidatePinsCallback callback) {
              std::move(callback).Run(absl::nullopt);
            }));

    absl::optional<bool> validate_status;
    service()->Validate(
        std::move(token), absl::nullopt,
        base::BindLambdaForTesting(
            [&validate_status](bool status, mojom::PinErrorPtr error) {
              validate_status = status;
            }));

    EXPECT_FALSE(validate_status.value());

    const base::Value::Dict* token_record =
        GetPrefs()
            ->GetDict(kPinnedNFTAssets)
            .FindDictByDottedPath(kMonkey1Path);
    EXPECT_EQ(base::Time::FromTimeT(345u),
              base::ValueToTime(token_record->Find("validate_timestamp")));
  }

  {
    mojom::BlockchainTokenPtr token =
        BraveWalletPinService::TokenFromPrefPath(kMonkey1Path);
    token->is_erc721 = true;
    ON_CALL(*GetIpfsLocalPinService(), ValidatePins(_, _, _))
        .WillByDefault(::testing::Invoke(
            [](const std::string& prefix, const std::vector<std::string>& cids,
               ipfs::ValidatePinsCallback callback) {
              std::move(callback).Run(false);
            }));

    absl::optional<bool> validate_status;
    service()->Validate(
        std::move(token), absl::nullopt,
        base::BindLambdaForTesting(
            [&validate_status](bool status, mojom::PinErrorPtr error) {
              validate_status = status;
            }));

    EXPECT_TRUE(validate_status.value());

    const base::Value::Dict* token_record =
        GetPrefs()
            ->GetDict(kPinnedNFTAssets)
            .FindDictByDottedPath(kMonkey1Path);

    EXPECT_EQ(nullptr, token_record->Find("validate_timestamp"));
  }
}

TEST_F(BraveWalletPinServiceTest, GetTokenStatus) {
  {
    ScopedDictPrefUpdate update(GetPrefs(), kPinnedNFTAssets);
    base::Value::Dict& update_dict = update.Get();

    base::Value::Dict item;
    item.Set("status", "pinned");
    item.Set("validate_timestamp",
             base::TimeToValue(base::Time::FromTimeT(123u)));
    base::Value::List cids;
    cids.Append("QmeSjSinHpPnmXmspMjwiXyN6zS4E9zccariGR3jxcaWtq");
    cids.Append("Qmcyc7tm9sZB9JnvLgejPTwdzjjNjDMiRWCUvaZAfp6cUg");
    item.Set("cids", std::move(cids));

    update_dict.SetByDottedPath(kMonkey1Path, std::move(item));
  }

  {
    ScopedDictPrefUpdate update(GetPrefs(), kPinnedNFTAssets);
    base::Value::Dict& update_dict = update.Get();

    base::Value::Dict item;
    item.Set("status", BraveWalletPinService::StatusToString(
                           mojom::TokenPinStatusCode::STATUS_PINNING_FAILED));
    base::Value::Dict error;
    error.Set("error_code",
              BraveWalletPinService::ErrorCodeToString(
                  mojom::WalletPinServiceErrorCode::ERR_FETCH_METADATA_FAILED));
    error.Set("error_message", "Fail to fetch metadata");
    item.Set("error", std::move(error));

    update_dict.SetByDottedPath(kMonkey2Path, std::move(item));
  }

  mojom::BlockchainTokenPtr token1 =
      BraveWalletPinService::TokenFromPrefPath(kMonkey1Path);
  token1->is_erc721 = true;
  {
    mojom::TokenPinStatusPtr status =
        service()->GetTokenStatus(absl::nullopt, token1);
    EXPECT_EQ(mojom::TokenPinStatusCode::STATUS_PINNED, status->code);
    EXPECT_TRUE(status->error.is_null());
    EXPECT_EQ(base::Time::FromTimeT(123u), status->validate_time);
  }

  {
    mojom::TokenPinStatusPtr status =
        service()->GetTokenStatus("nft.storage", token1);
    EXPECT_EQ(mojom::TokenPinStatusCode::STATUS_NOT_PINNED, status->code);
  }

  mojom::BlockchainTokenPtr token2 =
      BraveWalletPinService::TokenFromPrefPath(kMonkey2Path);
  token2->is_erc721 = true;
  {
    mojom::TokenPinStatusPtr status =
        service()->GetTokenStatus(absl::nullopt, token2);
    EXPECT_EQ(mojom::TokenPinStatusCode::STATUS_PINNING_FAILED, status->code);
    EXPECT_EQ(mojom::WalletPinServiceErrorCode::ERR_FETCH_METADATA_FAILED,
              status->error->error_code);
    EXPECT_EQ(base::Time(), status->validate_time);
  }
}

TEST_F(BraveWalletPinServiceTest, GetLastValidateTime) {
  {
    ScopedDictPrefUpdate update(GetPrefs(), kPinnedNFTAssets);
    base::Value::Dict& update_dict = update.Get();

    base::Value::Dict item;
    item.Set("status", "pinned");
    item.Set("validate_timestamp",
             base::TimeToValue(base::Time::FromTimeT(123u)));
    base::Value::List cids;
    cids.Append("QmeSjSinHpPnmXmspMjwiXyN6zS4E9zccariGR3jxcaWtq");
    cids.Append("Qmcyc7tm9sZB9JnvLgejPTwdzjjNjDMiRWCUvaZAfp6cUg");
    item.Set("cids", std::move(cids));

    update_dict.SetByDottedPath(kMonkey1Path, std::move(item));
  }

  mojom::BlockchainTokenPtr token =
      BraveWalletPinService::TokenFromPrefPath(kMonkey1Path);
  token->is_erc721 = true;
  {
    base::Time last_validate_time =
        service()->GetLastValidateTime(absl::nullopt, token).value();
    EXPECT_EQ(base::Time::FromTimeT(123u), last_validate_time);
  }

  {
    EXPECT_FALSE(
        service()->GetLastValidateTime("nft.storage", token).has_value());
  }
}

TEST_F(BraveWalletPinServiceTest, TokenFromPrefPath) {
  mojom::BlockchainTokenPtr token =
      BraveWalletPinService::TokenFromPrefPath(kMonkey1Path);
  EXPECT_EQ(mojom::CoinType::ETH, static_cast<mojom::CoinType>(token->coin));
  EXPECT_EQ("0x1", token->chain_id);
  EXPECT_EQ("0xbc4ca0eda7647a8ab7c2061c2e118a18a936f13d",
            token->contract_address);
  EXPECT_EQ("0x1", token->token_id);
}

TEST_F(BraveWalletPinServiceTest, ServiceFromPath) {
  EXPECT_FALSE(
      BraveWalletPinService::ServiceFromPrefPath(kMonkey1Path).has_value());

  EXPECT_EQ("nftstorage", BraveWalletPinService::ServiceFromPrefPath(
                              "nft.nftstorage.60.0x1."
                              "0xbc4ca0eda7647a8ab7c2061c2e118a18a936f13d.0x1")
                              .value());
}

TEST_F(BraveWalletPinServiceTest, GetPath) {
  {
    mojom::BlockchainTokenPtr token = mojom::BlockchainToken::New();
    token->coin = mojom::CoinType::ETH;
    token->contract_address = "abc";
    token->token_id = "0x2";
    token->chain_id = "mainnet";
    auto path = BraveWalletPinService::GetTokenPrefPath(absl::nullopt, token);
    EXPECT_EQ("nft.local.60.mainnet.abc.0x2", path.value());
  }

  {
    mojom::BlockchainTokenPtr token = mojom::BlockchainToken::New();
    token->coin = mojom::CoinType::ETH;
    token->contract_address = "abc";
    token->token_id = "0x2";
    token->chain_id = "mainnet";
    auto path = BraveWalletPinService::GetTokenPrefPath("nftstorage", token);
    EXPECT_EQ("nft.nftstorage.60.mainnet.abc.0x2", path.value());
  }
}

TEST_F(BraveWalletPinServiceTest, GetTokens) {
  {
    ScopedDictPrefUpdate update(GetPrefs(), kPinnedNFTAssets);
    base::Value::Dict& update_dict = update.Get();

    base::Value::Dict item;
    item.Set("status", "pinned");

    update_dict.SetByDottedPath(kMonkey1Path, std::move(item));
  }

  {
    ScopedDictPrefUpdate update(GetPrefs(), kPinnedNFTAssets);
    base::Value::Dict& update_dict = update.Get();

    base::Value::Dict item;
    item.Set("status", "pinning_failed");

    update_dict.SetByDottedPath(kMonkey2Path, std::move(item));
  }

  {
    ScopedDictPrefUpdate update(GetPrefs(), kPinnedNFTAssets);
    base::Value::Dict& update_dict = update.Get();

    base::Value::Dict item;
    item.Set("status", "pinned");

    update_dict.SetByDottedPath(kMonkey3Path, std::move(item));
  }

  {
    auto tokens = service()->GetTokens(absl::nullopt);
    EXPECT_EQ(2u, tokens.size());
    EXPECT_TRUE(tokens.contains(kMonkey1Path));
    EXPECT_TRUE(tokens.contains(kMonkey2Path));
  }

  {
    auto tokens = service()->GetTokens("nftstorage");
    EXPECT_EQ(1u, tokens.size());
    EXPECT_TRUE(tokens.contains(kMonkey3Path));
  }

  {
    auto tokens = service()->GetTokens("non_existing_storage");
    EXPECT_EQ(0u, tokens.size());
  }
}

}  // namespace brave_wallet
