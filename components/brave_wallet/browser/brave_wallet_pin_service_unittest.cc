/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/brave_wallet_pin_service.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/json/json_reader.h"
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
                        "attributes":[{"trait_type":"Mouth","value":"Bored Cigarette"},
                        {"trait_type":"Fur","value":"Zombie"},{"trait_type":"Background","value":"Purple"},
                        {"trait_type":"Eyes","value":"Closed"},{"trait_type":"Clothes","value":"Toga"},
                        {"trait_type":"Hat","value":"Cowboy Hat"}]})";

base::Time g_overridden_now;
std::unique_ptr<ScopedTimeClockOverrides> OverrideWithTimeNow(
    const base::Time& overridden_now) {
  g_overridden_now = overridden_now;
  return std::make_unique<ScopedTimeClockOverrides>(
      []() { return g_overridden_now; }, nullptr, nullptr);
}

}  // namespace

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
  MockJsonRpcService() {}

  MOCK_METHOD4(GetERC721Metadata,
               void(const std::string& contract_address,
                    const std::string& token_id,
                    const std::string& chain_id,
                    GetTokenMetadataCallback callback));

  ~MockJsonRpcService() override {}
};

class BraveWalletPinServiceTest : public testing::Test {
 public:
  BraveWalletPinServiceTest() = default;

  BraveWalletPinService* service() { return brave_wallet_pin_service_.get(); }

 protected:
  void SetUp() override {
    auto* registry = pref_service_.registry();
    registry->RegisterDictionaryPref(kPinnedErc721Assets);
    brave_wallet_pin_service_ = std::make_unique<BraveWalletPinService>(
        GetPrefs(), GetJsonRpcService(), GetIpfsLocalPinService());
  }

  PrefService* GetPrefs() { return &pref_service_; }

  testing::NiceMock<MockJsonRpcService>* GetJsonRpcService() {
    return &json_rpc_service_;
  }

  testing::NiceMock<MockIpfsLocalPinService>* GetIpfsLocalPinService() {
    return &ipfs_local_pin_service_;
  }

  testing::NiceMock<MockIpfsLocalPinService> ipfs_local_pin_service_;
  testing::NiceMock<MockJsonRpcService> json_rpc_service_;

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
               MockJsonRpcService::GetTokenMetadataCallback callback) {
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

    auto scoped_override =
        OverrideWithTimeNow(base::Time::FromInternalValue(123u));

    mojom::BlockchainTokenPtr token =
        BraveWalletPinService::TokenFromPath(kMonkey1Path);

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
            ->GetDict(kPinnedErc721Assets)
            .FindDictByDottedPath(kMonkey1Path);

    base::Value::List expected_cids;
    expected_cids.Append("QmeSjSinHpPnmXmspMjwiXyN6zS4E9zccariGR3jxcaWtq");
    expected_cids.Append("Qmcyc7tm9sZB9JnvLgejPTwdzjjNjDMiRWCUvaZAfp6cUg");

    EXPECT_EQ(StatusToString(mojom::TokenPinStatusCode::STATUS_PINNED),
              *(token_record->FindString("status")));
    EXPECT_EQ(nullptr, token_record->FindDict("error"));
    EXPECT_EQ(expected_cids, *(token_record->FindList("cids")));
    EXPECT_EQ("123", *(token_record->FindString("validate_timestamp")));
  }

  {
    ON_CALL(*GetJsonRpcService(), GetERC721Metadata(_, _, _, _))
        .WillByDefault(::testing::Invoke(
            [](const std::string& contract_address, const std::string& token_id,
               const std::string& chain_id,
               MockJsonRpcService::GetTokenMetadataCallback callback) {
              EXPECT_EQ("0x1", chain_id);
              EXPECT_EQ("0xbc4ca0eda7647a8ab7c2061c2e118a18a936f13d",
                        contract_address);
              EXPECT_EQ("0x2", token_id);
              std::move(callback).Run(
                  "", "", mojom::ProviderError::kParsingError, "Parsing error");
            }));

    mojom::BlockchainTokenPtr token =
        BraveWalletPinService::TokenFromPath(kMonkey2Path);

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
            ->GetDict(kPinnedErc721Assets)
            .FindDictByDottedPath(kMonkey2Path);

    EXPECT_EQ(StatusToString(mojom::TokenPinStatusCode::STATUS_PINNING_FAILED),
              *(token_record->FindString("status")));
    EXPECT_EQ(ErrorCodeToString(
                  mojom::WalletPinServiceErrorCode::ERR_FETCH_METADATA_FAILED),
              token_record->FindByDottedPath("error.error_code")->GetString());
  }
}

TEST_F(BraveWalletPinServiceTest, RemovePin) {
  {
    DictionaryPrefUpdate update(GetPrefs(), kPinnedErc721Assets);
    base::Value::Dict& update_dict = update->GetDict();

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
        BraveWalletPinService::TokenFromPath(kMonkey1Path);

    absl::optional<bool> remove_status;
    service()->RemovePin(
        std::move(token), absl::nullopt,
        base::BindLambdaForTesting(
            [&remove_status](bool status, mojom::PinErrorPtr error) {
              remove_status = status;
            }));
    EXPECT_FALSE(remove_status.value());
    EXPECT_EQ(
        StatusToString(mojom::TokenPinStatusCode::STATUS_UNPINNING_FAILED),
        *(GetPrefs()
              ->GetDict(kPinnedErc721Assets)
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
        BraveWalletPinService::TokenFromPath(kMonkey1Path);

    absl::optional<bool> remove_status;
    service()->RemovePin(
        std::move(token), absl::nullopt,
        base::BindLambdaForTesting(
            [&remove_status](bool status, mojom::PinErrorPtr error) {
              remove_status = status;
            }));
    EXPECT_TRUE(remove_status.value());
    EXPECT_EQ(nullptr, GetPrefs()
                           ->GetDict(kPinnedErc721Assets)
                           .FindDictByDottedPath(kMonkey1Path));
  }
}

TEST_F(BraveWalletPinServiceTest, ValidatePin) {
  {
    DictionaryPrefUpdate update(GetPrefs(), kPinnedErc721Assets);
    base::Value::Dict& update_dict = update->GetDict();

    base::Value::Dict item;
    item.Set("status", "pinned");
    item.Set("validate_timestamp", "123");
    base::Value::List cids;
    cids.Append("QmeSjSinHpPnmXmspMjwiXyN6zS4E9zccariGR3jxcaWtq");
    cids.Append("Qmcyc7tm9sZB9JnvLgejPTwdzjjNjDMiRWCUvaZAfp6cUg");
    item.Set("cids", std::move(cids));

    update_dict.SetByDottedPath(kMonkey1Path, std::move(item));
  }

  {
    auto scoped_override =
        OverrideWithTimeNow(base::Time::FromInternalValue(345u));

    mojom::BlockchainTokenPtr token =
        BraveWalletPinService::TokenFromPath(kMonkey1Path);

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
            ->GetDict(kPinnedErc721Assets)
            .FindDictByDottedPath(kMonkey1Path);
    EXPECT_EQ("345", *(token_record->FindString("validate_timestamp")));
  }

  {
    mojom::BlockchainTokenPtr token =
        BraveWalletPinService::TokenFromPath(kMonkey1Path);

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
            ->GetDict(kPinnedErc721Assets)
            .FindDictByDottedPath(kMonkey1Path);
    EXPECT_EQ("345", *(token_record->FindString("validate_timestamp")));
  }

  {
    mojom::BlockchainTokenPtr token =
        BraveWalletPinService::TokenFromPath(kMonkey1Path);

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
            ->GetDict(kPinnedErc721Assets)
            .FindDictByDottedPath(kMonkey1Path);

    EXPECT_EQ(nullptr, token_record->FindString("validate_timestamp"));
  }
}

TEST_F(BraveWalletPinServiceTest, GetTokenStatus) {
  {
    DictionaryPrefUpdate update(GetPrefs(), kPinnedErc721Assets);
    base::Value::Dict& update_dict = update->GetDict();

    base::Value::Dict item;
    item.Set("status", "pinned");
    item.Set("validate_timestamp", "123");
    base::Value::List cids;
    cids.Append("QmeSjSinHpPnmXmspMjwiXyN6zS4E9zccariGR3jxcaWtq");
    cids.Append("Qmcyc7tm9sZB9JnvLgejPTwdzjjNjDMiRWCUvaZAfp6cUg");
    item.Set("cids", std::move(cids));

    update_dict.SetByDottedPath(kMonkey1Path, std::move(item));
  }

  {
    DictionaryPrefUpdate update(GetPrefs(), kPinnedErc721Assets);
    base::Value::Dict& update_dict = update->GetDict();

    base::Value::Dict item;
    item.Set("status",
             StatusToString(mojom::TokenPinStatusCode::STATUS_PINNING_FAILED));
    base::Value::Dict error;
    error.Set("error_code",
              ErrorCodeToString(
                  mojom::WalletPinServiceErrorCode::ERR_FETCH_METADATA_FAILED));
    error.Set("error_message", "Fail to fetch metadata");
    item.Set("error", std::move(error));

    update_dict.SetByDottedPath(kMonkey2Path, std::move(item));
  }

  mojom::BlockchainTokenPtr token1 =
      BraveWalletPinService::TokenFromPath(kMonkey1Path);

  {
    mojom::TokenPinStatusPtr status =
        service()->GetTokenStatus(absl::nullopt, token1);
    EXPECT_EQ(mojom::TokenPinStatusCode::STATUS_PINNED, status->code);
    EXPECT_TRUE(status->error.is_null());
    EXPECT_EQ(base::Time::FromInternalValue(123u),
              status->validate_time.value());
  }

  {
    mojom::TokenPinStatusPtr status =
        service()->GetTokenStatus("nft.storage", token1);
    EXPECT_TRUE(status.is_null());
  }

  mojom::BlockchainTokenPtr token2 =
      BraveWalletPinService::TokenFromPath(kMonkey2Path);

  {
    mojom::TokenPinStatusPtr status =
        service()->GetTokenStatus(absl::nullopt, token2);
    EXPECT_EQ(mojom::TokenPinStatusCode::STATUS_PINNING_FAILED, status->code);
    EXPECT_EQ(mojom::WalletPinServiceErrorCode::ERR_FETCH_METADATA_FAILED,
              status->error->error_code);
    EXPECT_FALSE(status->validate_time.has_value());
  }
}

TEST_F(BraveWalletPinServiceTest, GetLastValidateTime) {
  {
    DictionaryPrefUpdate update(GetPrefs(), kPinnedErc721Assets);
    base::Value::Dict& update_dict = update->GetDict();

    base::Value::Dict item;
    item.Set("status", "pinned");
    item.Set("validate_timestamp", "123");
    base::Value::List cids;
    cids.Append("QmeSjSinHpPnmXmspMjwiXyN6zS4E9zccariGR3jxcaWtq");
    cids.Append("Qmcyc7tm9sZB9JnvLgejPTwdzjjNjDMiRWCUvaZAfp6cUg");
    item.Set("cids", std::move(cids));

    update_dict.SetByDottedPath(kMonkey1Path, std::move(item));
  }

  mojom::BlockchainTokenPtr token =
      BraveWalletPinService::TokenFromPath(kMonkey1Path);

  {
    base::Time last_validate_time =
        service()->GetLastValidateTime(absl::nullopt, token).value();
    EXPECT_EQ(base::Time::FromInternalValue(123u), last_validate_time);
  }

  {
    EXPECT_FALSE(
        service()->GetLastValidateTime("nft.storage", token).has_value());
  }
}

TEST_F(BraveWalletPinServiceTest, TokenFromPath) {
  mojom::BlockchainTokenPtr token =
      BraveWalletPinService::TokenFromPath(kMonkey1Path);
  EXPECT_TRUE(token->is_erc721);
  EXPECT_EQ(mojom::CoinType::ETH, static_cast<mojom::CoinType>(token->coin));
  EXPECT_EQ("0x1", token->chain_id);
  EXPECT_EQ("0xbc4ca0eda7647a8ab7c2061c2e118a18a936f13d",
            token->contract_address);
  EXPECT_EQ("0x1", token->token_id);
}

TEST_F(BraveWalletPinServiceTest, ServiceFromPath) {
  EXPECT_FALSE(
      BraveWalletPinService::ServiceFromPath(kMonkey1Path).has_value());

  EXPECT_EQ("nftstorage", BraveWalletPinService::ServiceFromPath(
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
    auto path = BraveWalletPinService::GetPath(absl::nullopt, token);
    EXPECT_EQ("nft.local.60.mainnet.abc.0x2", path);
  }

  {
    mojom::BlockchainTokenPtr token = mojom::BlockchainToken::New();
    token->coin = mojom::CoinType::ETH;
    token->contract_address = "abc";
    token->token_id = "0x2";
    token->chain_id = "mainnet";
    auto path = BraveWalletPinService::GetPath("nftstorage", token);
    EXPECT_EQ("nft.nftstorage.60.mainnet.abc.0x2", path);
  }
}

TEST_F(BraveWalletPinServiceTest, GetTokens) {
  {
    DictionaryPrefUpdate update(GetPrefs(), kPinnedErc721Assets);
    base::Value::Dict& update_dict = update->GetDict();

    base::Value::Dict item;
    item.Set("status", "pinned");

    update_dict.SetByDottedPath(kMonkey1Path, std::move(item));
  }

  {
    DictionaryPrefUpdate update(GetPrefs(), kPinnedErc721Assets);
    base::Value::Dict& update_dict = update->GetDict();

    base::Value::Dict item;
    item.Set("status", "pinning_failed");

    update_dict.SetByDottedPath(kMonkey2Path, std::move(item));
  }

  {
    DictionaryPrefUpdate update(GetPrefs(), kPinnedErc721Assets);
    base::Value::Dict& update_dict = update->GetDict();

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
