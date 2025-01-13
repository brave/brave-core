/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/json_rpc_service.h"

#include <stdint.h>

#include <algorithm>
#include <map>
#include <memory>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

#include "base/base64.h"
#include "base/containers/contains.h"
#include "base/containers/span.h"
#include "base/containers/span_writer.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/functional/callback_helpers.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/notreached.h"
#include "base/numerics/byte_conversions.h"
#include "base/ranges/algorithm.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/bind.h"
#include "base/test/mock_callback.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/blockchain_list_parser.h"
#include "brave/components/brave_wallet/browser/blockchain_registry.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/ens_resolver_task.h"
#include "brave/components/brave_wallet/browser/json_rpc_service_test_utils.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/browser/sns_resolver_task.h"
#include "brave/components/brave_wallet/browser/unstoppable_domains_dns_resolve.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/brave_wallet_constants.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/brave_wallet/common/encoding_utils.h"
#include "brave/components/brave_wallet/common/eth_abi_utils.h"
#include "brave/components/brave_wallet/common/eth_address.h"
#include "brave/components/brave_wallet/common/features.h"
#include "brave/components/brave_wallet/common/hash_utils.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "brave/components/brave_wallet/common/solana_address.h"
#include "brave/components/brave_wallet/common/solana_utils.h"
#include "brave/components/brave_wallet/common/test_utils.h"
#include "brave/components/brave_wallet/common/value_conversion_utils.h"
#include "brave/components/constants/brave_services_key.h"
#include "brave/components/decentralized_dns/core/constants.h"
#include "brave/components/decentralized_dns/core/utils.h"
#include "brave/components/ipfs/ipfs_utils.h"
#include "build/build_config.h"
#include "components/grit/brave_components_strings.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "mojo/public/cpp/bindings/self_owned_receiver.h"
#include "services/data_decoder/public/cpp/test_support/in_process_data_decoder.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/boringssl/src/include/openssl/curve25519.h"
#include "ui/base/l10n/l10n_util.h"
#include "url/gurl.h"
#include "url/origin.h"

using testing::_;
using testing::Contains;
using testing::ElementsAreArray;
using testing::Eq;
using testing::Not;

MATCHER_P(MatchesCIDv1URL, ipfs_url, "") {
  return ipfs::ContentHashToCIDv1URL(arg).spec() == ipfs_url;
}

namespace brave_wallet {

namespace {

// Compare two JSON strings, ignoring the order of the keys and other
// insignificant whitespace differences.
void CompareJSON(const std::string& response,
                 const std::string& expected_response) {
  auto response_val = base::JSONReader::Read(response);
  auto expected_response_val = base::JSONReader::Read(expected_response);
  EXPECT_EQ(response_val, expected_response_val);
  if (response_val) {
    // If the JSON is valid, compare the parsed values.
    EXPECT_EQ(*response_val, *expected_response_val);
  } else {
    // If the JSON is invalid, compare the raw strings.
    EXPECT_EQ(response, expected_response);
  }
}

void GetErrorCodeMessage(base::Value formed_response,
                         mojom::ProviderError* error,
                         std::string* error_message) {
  if (!formed_response.is_dict()) {
    *error = mojom::ProviderError::kSuccess;
    error_message->clear();
    return;
  }
  auto code = formed_response.GetDict().FindInt("code");
  if (code) {
    *error = static_cast<mojom::ProviderError>(*code);
  }
  const std::string* message = formed_response.GetDict().FindString("message");
  if (message) {
    *error_message = *message;
  }
}

std::string GetGasFilEstimateResponse(int64_t value) {
  std::string response =
      R"({
          "id": 1,
          "jsonrpc": "2.0",
          "result": {
              "CID": {
                "/": "bafy2bzacebefvj6623fkmfwazpvg7qxgomhicefeb6tunc7wbvd2ee4uppfkw"
              },
              "From": "t1h5tg3bhp5r56uzgjae2373znti6ygq4agkx4hzq",
              "GasFeeCap": "101520",
              "GasLimit": {gas_limit},
              "GasPremium": "100466",
              "Method": 0,
              "Nonce": 1,
              "Params": "",
              "To": "t1tquwkjo6qvweah2g2yikewr7y5dyjds42pnrn3a",
              "Value": "1000000000000000000",
              "Version": 0
          }
      })";
  base::ReplaceSubstringsAfterOffset(&response, 0, "{gas_limit}",
                                     std::to_string(value));
  return response;
}

std::string GetFilStateSearchMsgLimitedResponse(int64_t value) {
  std::string response =
      R"({
        "id": 1,
        "jsonrpc": "2.0",
        "result":{
            "Height": 22389,
            "Message":
            {
                "/": "bafy2bzacebundyopm3trenj47hxkwiqn2cbvvftz3fss4dxuttu2u6xbbtkqy"
            },
            "Receipt":
            {
                "ExitCode": {exit_code},
                "GasUsed": 1749648,
                "Return": null
            },
            "ReturnDec": null,
            "TipSet":
            [
                {
                    "/": "bafy2bzacednkg6htmwwlkewl5wr2nezsovfgx5xb56l2uthz32uraqlmtsuzc"
                }
            ]
        }
      }
    )";
  base::ReplaceSubstringsAfterOffset(&response, 0, "{exit_code}",
                                     std::to_string(value));
  return response;
}

void UpdateCustomNetworks(PrefService* prefs,
                          std::vector<base::Value::Dict>* values) {
  ScopedDictPrefUpdate update(prefs, kBraveWalletCustomNetworks);
  base::Value::List* list = update->EnsureList(kEthereumPrefKey);
  list->clear();
  for (auto& it : *values) {
    list->Append(std::move(it));
  }
}

void OnRequestResponse(bool* callback_called,
                       bool expected_success,
                       const std::string& expected_response,
                       base::Value id,
                       base::Value formed_response,
                       const bool reject,
                       const std::string& first_allowed_account,
                       const bool update_bind_js_properties) {
  *callback_called = true;
  std::string response;
  base::JSONWriter::Write(formed_response, &response);
  mojom::ProviderError error = mojom::ProviderError::kUnknown;
  std::string error_message;
  GetErrorCodeMessage(std::move(formed_response), &error, &error_message);
  bool success = error == brave_wallet::mojom::ProviderError::kSuccess;
  EXPECT_EQ(expected_success, success);
  if (!success) {
    response = "";
  }
  EXPECT_EQ(expected_response, response);
}

void OnStringResponse(bool* callback_called,
                      brave_wallet::mojom::ProviderError expected_error,
                      const std::string& expected_error_message,
                      const std::string& expected_response,
                      const std::string& response,
                      brave_wallet::mojom::ProviderError error,
                      const std::string& error_message) {
  *callback_called = true;
  EXPECT_EQ(expected_response, response);
  EXPECT_EQ(expected_error, error);
  EXPECT_EQ(expected_error_message, error_message);
}

void OnBoolResponse(bool* callback_called,
                    brave_wallet::mojom::ProviderError expected_error,
                    const std::string& expected_error_message,
                    bool expected_response,
                    bool response,
                    brave_wallet::mojom::ProviderError error,
                    const std::string& error_message) {
  *callback_called = true;
  EXPECT_EQ(expected_response, response);
  EXPECT_EQ(expected_error, error);
  EXPECT_EQ(expected_error_message, error_message);
}

void OnEthUint256Response(bool* callback_called,
                          brave_wallet::mojom::ProviderError expected_error,
                          const std::string& expected_error_message,
                          uint256_t expected_response,
                          uint256_t response,
                          brave_wallet::mojom::ProviderError error,
                          const std::string& error_message) {
  *callback_called = true;
  EXPECT_EQ(expected_response, response);
  EXPECT_EQ(expected_error, error);
  EXPECT_EQ(expected_error_message, error_message);
}

void OnFilUint256Response(
    bool* callback_called,
    brave_wallet::mojom::FilecoinProviderError expected_error,
    const std::string& expected_error_message,
    uint256_t expected_response,
    uint256_t response,
    brave_wallet::mojom::FilecoinProviderError error,
    const std::string& error_message) {
  *callback_called = true;
  EXPECT_EQ(expected_response, response);
  EXPECT_EQ(expected_error, error);
  EXPECT_EQ(expected_error_message, error_message);
}

class TestJsonRpcServiceObserver
    : public brave_wallet::mojom::JsonRpcServiceObserver {
 public:
  TestJsonRpcServiceObserver() = default;
  TestJsonRpcServiceObserver(base::OnceClosure callback,
                             const std::string& expected_chain_id,
                             const std::string& expected_error) {
    callback_ = std::move(callback);
    expected_chain_id_ = expected_chain_id;
    expected_error_ = expected_error;
  }

  void OnAddEthereumChainRequestCompleted(const std::string& chain_id,
                                          const std::string& error) override {
    EXPECT_EQ(chain_id, expected_chain_id_);
    EXPECT_EQ(error, expected_error_);
    std::move(callback_).Run();
  }

  MOCK_METHOD3(ChainChangedEvent,
               void(const std::string&,
                    mojom::CoinType,
                    const std::optional<url::Origin>& origin));

  ::mojo::PendingRemote<brave_wallet::mojom::JsonRpcServiceObserver>
  GetReceiver() {
    return observer_receiver_.BindNewPipeAndPassRemote();
  }

  base::OnceClosure callback_;
  std::string expected_chain_id_;
  std::string expected_error_;
  mojo::Receiver<brave_wallet::mojom::JsonRpcServiceObserver>
      observer_receiver_{this};
};

constexpr char https_metadata_response[] =
    R"({"attributes":[{"trait_type":"Feet","value":"Green Shoes"},{"trait_type":"Legs","value":"Tan Pants"},{"trait_type":"Suspenders","value":"White Suspenders"},{"trait_type":"Upper Body","value":"Indigo Turtleneck"},{"trait_type":"Sleeves","value":"Long Sleeves"},{"trait_type":"Hat","value":"Yellow / Blue Pointy Beanie"},{"trait_type":"Eyes","value":"White Nerd Glasses"},{"trait_type":"Mouth","value":"Toothpick"},{"trait_type":"Ears","value":"Bing Bong Stick"},{"trait_type":"Right Arm","value":"Swinging"},{"trait_type":"Left Arm","value":"Diamond Hand"},{"trait_type":"Background","value":"Blue"}],"description":"5,000 animated Invisible Friends hiding in the metaverse. A collection by Markus Magnusson & Random Character Collective.","image":"https://rcc.mypinata.cloud/ipfs/QmXmuSenZRnofhGMz2NyT3Yc4Zrty1TypuiBKDcaBsNw9V/1817.gif","name":"Invisible Friends #1817"})";

std::optional<base::Value> ToValue(const network::ResourceRequest& request) {
  std::string_view request_string(request.request_body->elements()
                                      ->at(0)
                                      .As<network::DataElementBytes>()
                                      .AsStringPiece());
  return base::JSONReader::Read(request_string,
                                base::JSONParserOptions::JSON_PARSE_RFC);
}

class EthCallHandler {
 public:
  EthCallHandler(const EthAddress& to, const eth_abi::Bytes4& selector)
      : to_(to), selectors_({selector}) {}
  EthCallHandler(const EthAddress& to,
                 const std::vector<eth_abi::Bytes4>& selectors)
      : to_(to), selectors_(selectors) {}
  virtual ~EthCallHandler() = default;

  bool CallSupported(const EthAddress& to, eth_abi::Span call_data) {
    if (to != to_) {
      return false;
    }

    auto [selector, _] =
        *eth_abi::ExtractFunctionSelectorAndArgsFromCall(call_data);

    for (const auto& s : selectors_) {
      if (base::ranges::equal(s, selector)) {
        return true;
      }
    }
    return false;
  }

  const EthAddress& to() const { return to_; }

  virtual std::optional<std::string> HandleEthCall(eth_abi::Span call_data) = 0;

 protected:
  EthAddress to_;
  std::vector<eth_abi::Bytes4> selectors_;
};

class SolRpcCallHandler {
 public:
  virtual ~SolRpcCallHandler() = default;

  virtual bool CallSupported(const base::Value::Dict& dict) = 0;
  virtual std::optional<std::string> HandleCall(
      const base::Value::Dict& dict) = 0;

  void FailWithTimeout(bool fail_with_timeout = true) {
    fail_with_timeout_ = fail_with_timeout;
  }
  void Disable(bool disabled = true) { disabled_ = disabled; }
  void Enable() { disabled_ = false; }

  std::optional<SolanaAddress> AddressFromParams(
      const base::Value::Dict& dict) {
    auto* params_list = dict.FindList("params");
    if (!params_list || params_list->size() == 0) {
      return std::nullopt;
    }

    return SolanaAddress::FromBase58((*params_list)[0].GetString());
  }

 protected:
  bool fail_with_timeout_ = false;
  bool disabled_ = false;
};

class GetAccountInfoHandler : public SolRpcCallHandler {
 public:
  GetAccountInfoHandler() = default;
  GetAccountInfoHandler(const SolanaAddress& account_address,
                        const SolanaAddress& owner,
                        std::vector<uint8_t> data)
      : account_address_(account_address), owner_(owner), data_(data) {}

  void Reset(const SolanaAddress& account_address,
             const SolanaAddress& owner,
             std::vector<uint8_t> data) {
    account_address_ = account_address;
    owner_ = owner;
    data_ = std::move(data);
  }

  bool CallSupported(const base::Value::Dict& dict) override {
    if (disabled_) {
      return false;
    }
    auto* method = dict.FindString("method");
    if (!method || *method != "getAccountInfo") {
      return false;
    }
    if (!account_address_.IsValid()) {
      return true;
    }
    return AddressFromParams(dict) == account_address_;
  }

  static std::vector<uint8_t> MakeMintData(int supply) {
    std::vector<uint8_t> data(82);
    base::span(data).subspan(36u).first<8u>().copy_from(
        base::U64ToLittleEndian(supply));
    return data;
  }

  static std::vector<uint8_t> MakeNameRegistryStateData(
      const SolanaAddress& owner,
      const std::vector<uint8_t>& data = {}) {
    std::vector<uint8_t> result(96 + data.size());
    auto result_span = base::span(result);
    // Header.
    result_span.subspan<32, 32>().copy_from(owner.bytes());

    // Data.
    result_span.last(data.size()).copy_from(data);

    return result;
  }

  static std::vector<uint8_t> MakeSolRecordV1PayloadData(
      const SolanaAddress& sol_record_payload_address,
      const SolanaAddress& sol_record_address,
      const std::vector<uint8_t>& signer_key) {
    std::vector<uint8_t> result(32 + 64);  // payload_address + signature.
    auto result_span = base::span(result);

    result_span.copy_prefix_from(sol_record_payload_address.bytes());

    std::vector<uint8_t> message;
    message.insert(message.end(), sol_record_payload_address.bytes().begin(),
                   sol_record_payload_address.bytes().end());
    message.insert(message.end(), sol_record_address.bytes().begin(),
                   sol_record_address.bytes().end());
    std::string hex_message = base::ToLowerASCII(base::HexEncode(message));
    ED25519_sign(result_span.subspan(32u).data(),
                 reinterpret_cast<const uint8_t*>(hex_message.data()),
                 hex_message.length(), signer_key.data());

    return result;
  }

  static std::vector<uint8_t> MakeTextRecordV1PayloadData(
      const std::string& text) {
    return std::vector<uint8_t>(text.begin(), text.end());
  }

  template <class T>
  static void PushAsLE(std::vector<uint8_t>& to, T value) {
    to.resize(to.size() + sizeof(T));
    auto* insert_to = &to[to.size() - sizeof(T)];

#if defined(ARCH_CPU_LITTLE_ENDIAN)
    T in_le = value;
#else
    T in_le = ByteSwap(value);
#endif

    (*reinterpret_cast<T*>(insert_to)) = in_le;
  }

  static std::vector<uint8_t> MakeTextRecordV2PayloadData(
      SnsRecordV2ValidationType staleness_validation_type,
      const std::optional<SolanaAddress>& solana_validation_id,
      const std::string& content) {
    std::vector<uint8_t> result;
    result.reserve(300);

    // Staleness validation type.
    PushAsLE(result, uint16_t(staleness_validation_type));
    // ROA validation type. (only kNone for test records supported)
    PushAsLE(result, uint16_t(SnsRecordV2ValidationType::kNone));
    // content length.
    PushAsLE(result, uint32_t(content.size()));

    // staleness id.
    if (staleness_validation_type == SnsRecordV2ValidationType::kSolana) {
      CHECK(solana_validation_id);
      result.insert(result.end(), solana_validation_id->bytes().begin(),
                    solana_validation_id->bytes().end());
    } else if (staleness_validation_type ==
               SnsRecordV2ValidationType::kEthereum) {
      result.resize(result.size() + kEthAddressLength);
    } else if (staleness_validation_type ==
               SnsRecordV2ValidationType::kSolanaUnverified) {
      result.resize(result.size() + kSolanaPubkeySize);
    }

    // content
    auto content_span = base::as_byte_span(content);
    result.insert(result.end(), content_span.begin(), content_span.end());

    return result;
  }

  static std::vector<uint8_t> MakeSolRecordV2PayloadData(
      SnsRecordV2ValidationType staleness_validation_type,
      const std::optional<SolanaAddress>& solana_validation_id,
      SnsRecordV2ValidationType roa_validation_type,
      const std::optional<SolanaAddress>& solana_roa_id,
      const SolanaAddress& content) {
    std::vector<uint8_t> result;
    result.reserve(300);

    // Staleness validation type.
    PushAsLE(result, uint16_t(staleness_validation_type));
    // ROA validation type.
    PushAsLE(result, uint16_t(roa_validation_type));
    // content length.
    PushAsLE(result, uint32_t(content.bytes().size()));

    // staleness id.
    if (staleness_validation_type == SnsRecordV2ValidationType::kSolana) {
      CHECK(solana_validation_id);
      result.insert(result.end(), solana_validation_id->bytes().begin(),
                    solana_validation_id->bytes().end());
    } else if (staleness_validation_type ==
               SnsRecordV2ValidationType::kEthereum) {
      result.resize(result.size() + kEthAddressLength);
    } else if (staleness_validation_type ==
               SnsRecordV2ValidationType::kSolanaUnverified) {
      result.resize(result.size() + kSolanaPubkeySize);
    }

    // roa id.
    if (roa_validation_type == SnsRecordV2ValidationType::kSolana) {
      CHECK(solana_roa_id);
      result.insert(result.end(), solana_roa_id->bytes().begin(),
                    solana_roa_id->bytes().end());
    } else if (roa_validation_type == SnsRecordV2ValidationType::kEthereum) {
      result.resize(result.size() + kEthAddressLength);
    } else if (roa_validation_type ==
               SnsRecordV2ValidationType::kSolanaUnverified) {
      result.resize(result.size() + kSolanaPubkeySize);
    }

    // content
    auto content_span = base::as_byte_span(content.bytes());
    result.insert(result.end(), content_span.begin(), content_span.end());

    return result;
  }

  std::optional<std::string> HandleCall(
      const base::Value::Dict& dict) override {
    if (fail_with_timeout_) {
      return "timeout";
    }

    if (!account_address_.IsValid()) {
      return MakeJsonRpcValueResponse(base::Value());
    }

    base::Value::Dict value;
    base::Value::List data_array;
    data_array.Append(base::Value(base::Base64Encode(data_)));
    data_array.Append(base::Value("base64"));
    value.Set("data", std::move(data_array));
    value.Set("executable", false);
    value.Set("lamports", 123);
    value.Set("owner", owner_.ToBase58());
    value.Set("rentEpoch", 123);

    return MakeJsonRpcValueResponse(base::Value(std::move(value)));
  }

  std::vector<uint8_t>& data() { return data_; }

 protected:
  SolanaAddress account_address_;
  SolanaAddress owner_;
  std::vector<uint8_t> data_;
};

class GetProgramAccountsHandler : public SolRpcCallHandler {
 public:
  explicit GetProgramAccountsHandler(
      const SolanaAddress& target,
      const SolanaAddress& token_account_address,
      const std::vector<uint8_t>& token_account_data)
      : target_(target),
        token_account_address_(token_account_address),
        token_account_data_(token_account_data) {}

  bool CallSupported(const base::Value::Dict& dict) override {
    if (disabled_) {
      return false;
    }

    auto* method = dict.FindString("method");
    if (!method || *method != "getProgramAccounts") {
      return false;
    }
    return AddressFromParams(dict) == target_;
  }

  static std::vector<uint8_t> MakeTokenAccountData(const SolanaAddress& mint,
                                                   const SolanaAddress& owner) {
    std::vector<uint8_t> data(165);

    auto span_writer = base::SpanWriter(base::span(data));
    span_writer.Write(mint.bytes());
    span_writer.Write(owner.bytes());
    span_writer.WriteU8LittleEndian(1);

    return data;
  }

  std::optional<std::string> HandleCall(
      const base::Value::Dict& dict) override {
    if (fail_with_timeout_) {
      return "timeout";
    }

    auto* filters = (*dict.FindList("params"))[1].GetDict().FindList("filters");
    EXPECT_TRUE(filters);

    auto data_span = base::span(token_account_data_);
    base::Value::List expected_filters;
    expected_filters.Append(base::Value::Dict());
    expected_filters.back().GetDict().SetByDottedPath("memcmp.offset", 0);
    expected_filters.back().GetDict().SetByDottedPath(
        "memcmp.bytes", Base58Encode(data_span.first<32>()));
    expected_filters.Append(base::Value::Dict());
    expected_filters.back().GetDict().SetByDottedPath("memcmp.offset", 64);
    expected_filters.back().GetDict().SetByDottedPath(
        "memcmp.bytes", Base58Encode(data_span.subspan(64u, 1u)));
    expected_filters.Append(base::Value::Dict());
    expected_filters.back().GetDict().Set("dataSize", 165);

    EXPECT_EQ(expected_filters, *filters);

    base::Value::Dict item;

    base::Value::Dict account_dict;

    base::Value::List data_array;
    data_array.Append(base::Base64Encode(token_account_data_));
    data_array.Append("base64");
    account_dict.Set("data", std::move(data_array));

    account_dict.Set("executable", false);
    account_dict.Set("lamports", 123);
    account_dict.Set("owner", target_.ToBase58());
    account_dict.Set("rentEpoch", 11);

    item.Set("account", std::move(account_dict));

    item.Set("pubkey", token_account_address_.ToBase58());

    base::Value::List items;
    items.Append(std::move(item));

    return MakeJsonRpcResultResponse(base::Value(std::move(items)));
  }

 protected:
  SolanaAddress target_;
  SolanaAddress token_account_address_;
  std::vector<uint8_t> token_account_data_;
};

class JsonRpcEndpointHandler {
 public:
  explicit JsonRpcEndpointHandler(const GURL& endpoint) : endpoint_(endpoint) {}

  std::optional<std::string> HandleRequest(
      const network::ResourceRequest& request) {
    if (request.url != endpoint_) {
      return std::nullopt;
    }

    auto value = ToValue(request);
    if (value && value->is_dict()) {
      auto response = HandleCall(value->GetDict());
      if (response) {
        return response;
      }
    }

    return std::nullopt;
  }

  void AddEthCallHandler(EthCallHandler* handler) {
    eth_call_handlers_.push_back(handler);
  }

  void AddSolRpcCallHandler(SolRpcCallHandler* handler) {
    sol_rpc_call_handlers_.push_back(handler);
  }

 protected:
  std::optional<std::string> HandleCall(const base::Value::Dict& dict) {
    auto* method = dict.FindString("method");
    if (!method) {
      return std::nullopt;
    }
    if (*method == "eth_call") {
      return HandleEthCall(dict);
    }

    return HandleSolRpcCall(dict);
  }

  std::optional<std::string> HandleEthCall(const base::Value::Dict& dict) {
    auto* params_list = dict.FindList("params");
    if (!params_list || params_list->size() == 0 ||
        !params_list->begin()->is_dict()) {
      return std::nullopt;
    }

    auto& transaction_params = params_list->begin()->GetDict();
    auto* data_param = transaction_params.FindString("data");
    auto* to_param = transaction_params.FindString("to");
    if (!data_param || !to_param || !EthAddress::FromHex(*to_param).IsValid()) {
      return std::nullopt;
    }

    auto call_data = PrefixedHexStringToBytes(*data_param);
    if (!call_data) {
      return std::nullopt;
    }

    for (auto* handler : eth_call_handlers_) {
      if (!handler->CallSupported(EthAddress::FromHex(*to_param), *call_data)) {
        continue;
      }

      auto response = handler->HandleEthCall(*call_data);
      if (response) {
        return response;
      }
    }
    return std::nullopt;
  }

  std::optional<std::string> HandleSolRpcCall(const base::Value::Dict& dict) {
    for (auto* handler : sol_rpc_call_handlers_) {
      if (!handler->CallSupported(dict)) {
        continue;
      }

      auto response = handler->HandleCall(dict);
      if (response) {
        return response;
      }
    }
    return std::nullopt;
  }

 private:
  GURL endpoint_;
  std::vector<EthCallHandler*> eth_call_handlers_;
  std::vector<SolRpcCallHandler*> sol_rpc_call_handlers_;
};

constexpr char kJsonRpcResponseTemplate[] = R"({
      "jsonrpc":"2.0",
      "id":1,
      "result":"$1"
  })";

std::string FormatJsonRpcResponse(const std::string& value) {
  return base::ReplaceStringPlaceholders(kJsonRpcResponseTemplate, {value},
                                         nullptr);
}

}  // namespace

class JsonRpcServiceUnitTest : public testing::Test {
 public:
  JsonRpcServiceUnitTest() = default;

  void SetUp() override {
    Test::SetUp();
    shared_url_loader_factory_ =
        base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
            &url_loader_factory_);
    url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [this](const network::ResourceRequest& request) {
          url_loader_factory_.ClearResponses();
          url_loader_factory_.AddResponse(
              network_manager_
                  ->GetNetworkURL(mojom::kLocalhostChainId,
                                  mojom::CoinType::ETH)
                  .spec(),
              "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":"
              "\"0x000000000000000000000000000000000000000000000000000000000000"
              "0020000000000000000000000000000000000000000000000000000000000000"
              "0026e3010170122008ab7bf21b73828364305ef6b7c676c1f5a73e18ab4f93be"
              "ec7e21e0bc84010e000000000000000000000000000000000000000000000000"
              "0000\"}");
        }));

    decentralized_dns::RegisterLocalStatePrefs(local_state_prefs_.registry());
    brave_wallet::RegisterProfilePrefs(prefs_.registry());
    brave_wallet::RegisterProfilePrefsForMigration(prefs_.registry());
    network_manager_ = std::make_unique<NetworkManager>(&prefs_);
    json_rpc_service_ = std::make_unique<JsonRpcService>(
        shared_url_loader_factory_, network_manager_.get(), &prefs_,
        &local_state_prefs_);
    SetNetwork(mojom::kLocalhostChainId, mojom::CoinType::ETH, std::nullopt);
    SetNetwork(mojom::kLocalhostChainId, mojom::CoinType::SOL, std::nullopt);
    SetNetwork(mojom::kLocalhostChainId, mojom::CoinType::FIL, std::nullopt);
  }

  ~JsonRpcServiceUnitTest() override = default;

  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory() {
    return shared_url_loader_factory_;
  }

  PrefService* prefs() { return &prefs_; }
  PrefService* local_state_prefs() { return &local_state_prefs_; }

  GURL GetNetwork(const std::string& chain_id, mojom::CoinType coin) {
    return network_manager_->GetNetworkURL(chain_id, coin);
  }

  std::vector<mojom::NetworkInfoPtr> GetAllEthCustomChains() {
    return network_manager_->GetAllCustomChains(mojom::CoinType::ETH);
  }

  bool GetIsEip1559FromPrefs(const std::string& chain_id) {
    return network_manager_->IsEip1559Chain(chain_id).value_or(false);
  }

  void SetEthTokenInfoInterceptor(const GURL& network_url,
                                  const std::string& chain_id,
                                  const std::string& symbol,
                                  const std::string& name,
                                  const std::string& decimals) {
    url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [=, this](const network::ResourceRequest& request) {
          std::string_view request_string(request.request_body->elements()
                                              ->at(0)
                                              .As<network::DataElementBytes>()
                                              .AsStringPiece());
          url_loader_factory_.ClearResponses();
          if (request_string.find("0x95d89b41") != std::string::npos) {
            url_loader_factory_.AddResponse(
                network_url.spec(),
                "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":\"" + symbol + "\"}");
          }

          if (request_string.find("0x06fdde03") != std::string::npos) {
            url_loader_factory_.AddResponse(
                network_url.spec(),
                "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":\"" + name + "\"}");
          }

          if (request_string.find("0x313ce567") != std::string::npos) {
            url_loader_factory_.AddResponse(
                network_url.spec(),
                "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":\"" + decimals +
                    "\"}");
          }
        }));
  }

  void SetEthChainIdInterceptor(const GURL& network_url,
                                const std::string& chain_id) {
    url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [=, this](const network::ResourceRequest& request) {
          std::string_view request_string(request.request_body->elements()
                                              ->at(0)
                                              .As<network::DataElementBytes>()
                                              .AsStringPiece());
          url_loader_factory_.ClearResponses();
          if (request_string.find("eth_chainId") != std::string::npos) {
            url_loader_factory_.AddResponse(
                network_url.spec(),
                "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":\"" + chain_id +
                    "\"}");
          }
        }));
  }
  void SetEthChainIdInterceptorWithBrokenResponse(const GURL& network_url) {
    url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [=, this](const network::ResourceRequest& request) {
          std::string_view request_string(request.request_body->elements()
                                              ->at(0)
                                              .As<network::DataElementBytes>()
                                              .AsStringPiece());
          url_loader_factory_.ClearResponses();
          if (request_string.find("eth_chainId") != std::string::npos) {
            url_loader_factory_.AddResponse(network_url.spec(),
                                            "{\"jsonrpc\":\"");
          }
        }));
  }

  void SetUDENSInterceptor(const std::string& chain_id) {
    GURL network_url =
        network_manager_->GetNetworkURL(chain_id, mojom::CoinType::ETH);
    ASSERT_TRUE(network_url.is_valid());
    url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [=, this](const network::ResourceRequest& request) {
          std::string_view request_string(request.request_body->elements()
                                              ->at(0)
                                              .As<network::DataElementBytes>()
                                              .AsStringPiece());
          url_loader_factory_.ClearResponses();
          if (request_string.find(GetFunctionHash("resolver(bytes32)")) !=
              std::string::npos) {
            url_loader_factory_.AddResponse(
                network_url.spec(),
                "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":"
                "\"0x0000000000000000000000004976fb03c32e5b8cfe2b6ccb31c09ba78e"
                "baba41\"}");
          } else if (request_string.find(GetFunctionHash(
                         "contenthash(bytes32)")) != std::string::npos) {
            url_loader_factory_.AddResponse(
                network_url.spec(),
                "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":"
                "\"0x0000000000000000000000000000000000000000000000000000000000"
                "00002000000000000000000000000000000000000000000000000000000000"
                "00000026e3010170122023e0160eec32d7875c19c5ac7c03bc1f306dc26008"
                "0d621454bc5f631e7310a70000000000000000000000000000000000000000"
                "000000000000\"}");
          } else if (request_string.find(GetFunctionHash("addr(bytes32)")) !=
                     std::string::npos) {
            url_loader_factory_.AddResponse(
                network_url.spec(),
                "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":"
                "\"0x000000000000000000000000983110309620d911731ac0932219af0609"
                "1b6744\"}");
          } else if (request_string.find(GetFunctionHash(
                         "get(string,uint256)")) != std::string::npos) {
            url_loader_factory_.AddResponse(
                network_url.spec(),
                "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":"
                "\"0x0000000000000000000000000000000000000000000000000000000000"
                "00002000000000000000000000000000000000000000000000000000000000"
                "0000002a307838616144343433323141383662313730383739643741323434"
                "63316538643336306339394464413800000000000000000000000000000000"
                "000000000000\"}");
          } else {
            url_loader_factory_.AddResponse(request.url.spec(), "",
                                            net::HTTP_REQUEST_TIMEOUT);
          }
        }));
  }

  void SetENSZeroAddressInterceptor(const std::string& chain_id) {
    GURL network_url =
        network_manager_->GetNetworkURL(chain_id, mojom::CoinType::ETH);
    ASSERT_TRUE(network_url.is_valid());
    url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [=, this](const network::ResourceRequest& request) {
          std::string_view request_string(request.request_body->elements()
                                              ->at(0)
                                              .As<network::DataElementBytes>()
                                              .AsStringPiece());
          url_loader_factory_.ClearResponses();
          if (request_string.find(GetFunctionHash("resolver(bytes32)")) !=
              std::string::npos) {
            url_loader_factory_.AddResponse(
                network_url.spec(),
                "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":"
                "\"0x0000000000000000000000004976fb03c32e5b8cfe2b6ccb31c09ba78e"
                "baba41\"}");
          } else if (request_string.find(GetFunctionHash("addr(bytes32)")) !=
                     std::string::npos) {
            url_loader_factory_.AddResponse(
                network_url.spec(),
                "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":"
                "\"0x0000000000000000000000000000000000000000000000000000000000"
                "000000\"}");
          } else {
            url_loader_factory_.AddResponse(request.url.spec(), "",
                                            net::HTTP_REQUEST_TIMEOUT);
          }
        }));
  }

  void SetTokenMetadataInterceptor(
      const std::string& interface_id,
      const std::string& chain_id,
      const std::string& supports_interface_provider_response,
      const std::string& token_uri_provider_response = "",
      const std::string& metadata_response = "",
      net::HttpStatusCode supports_interface_status = net::HTTP_OK,
      net::HttpStatusCode token_uri_status = net::HTTP_OK,
      net::HttpStatusCode metadata_status = net::HTTP_OK) {
    GURL network_url =
        network_manager_->GetNetworkURL(chain_id, mojom::CoinType::ETH);
    ASSERT_TRUE(network_url.is_valid());
    url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [=, this](const network::ResourceRequest& request) {
          url_loader_factory_.ClearResponses();
          if (request.method ==
              "POST") {  // An eth_call, either to supportsInterface or tokenURI
            std::string_view request_string(request.request_body->elements()
                                                ->at(0)
                                                .As<network::DataElementBytes>()
                                                .AsStringPiece());
            bool is_supports_interface_req =
                request_string.find(GetFunctionHash(
                    "supportsInterface(bytes4)")) != std::string::npos;
            if (is_supports_interface_req) {
              ASSERT_NE(request_string.find(interface_id.substr(2)),
                        std::string::npos);
              EXPECT_EQ(request.url.spec(), network_url);
              url_loader_factory_.AddResponse(
                  network_url.spec(), supports_interface_provider_response,
                  supports_interface_status);
              return;
            } else {
              std::string function_hash;
              if (interface_id == kERC721MetadataInterfaceId) {
                function_hash = GetFunctionHash("tokenURI(uint256)");
              } else {
                function_hash = GetFunctionHash("uri(uint256)");
              }
              ASSERT_NE(request_string.find(function_hash), std::string::npos);
              url_loader_factory_.AddResponse(network_url.spec(),
                                              token_uri_provider_response,
                                              token_uri_status);
              return;
            }
          } else {  // A HTTP GET to fetch the metadata json from the web
            url_loader_factory_.AddResponse(request.url.spec(),
                                            metadata_response, metadata_status);
            return;
          }
        }));
  }

  void SetGetEthNftStandardInterceptor(
      const GURL& expected_url,
      const std::map<std::string, std::string>& interface_id_to_response) {
    url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [=, this](const network::ResourceRequest& request) {
          EXPECT_EQ(request.url, expected_url);
          std::string_view request_string(request.request_body->elements()
                                              ->at(0)
                                              .As<network::DataElementBytes>()
                                              .AsStringPiece());
          // Check if any of the interface ids are in the request
          // if so, return the response for that interface id
          // if not, do nothing
          for (const auto& [interface_id, response] :
               interface_id_to_response) {
            bool request_is_checking_interface =
                request_string.find(interface_id.substr(2)) !=
                std::string::npos;
            if (request_is_checking_interface) {
              url_loader_factory_.ClearResponses();
              url_loader_factory_.AddResponse(expected_url.spec(), response);
              return;
            }
          }
        }));
  }

  void SetSolTokenMetadataInterceptor(
      const GURL& expected_rpc_url,
      const std::string& get_account_info_response,
      const GURL& expected_metadata_url,
      const std::string& metadata_response) {
    auto network_url =
        GetNetwork(mojom::kLocalhostChainId, mojom::CoinType::SOL);
    ASSERT_TRUE(expected_rpc_url.is_valid());
    ASSERT_TRUE(expected_metadata_url.is_valid());
    url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [=, this](const network::ResourceRequest& request) {
          url_loader_factory_.AddResponse(expected_rpc_url.spec(),
                                          get_account_info_response);
          url_loader_factory_.AddResponse(expected_metadata_url.spec(),
                                          metadata_response);
        }));
  }

  void SetInterceptor(const GURL& expected_url,
                      const std::string& expected_method,
                      const std::string& expected_cache_header,
                      const std::string& content) {
    url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [=, this](const network::ResourceRequest& request) {
          EXPECT_EQ(request.url, expected_url);
          std::string header_value =
              request.headers.GetHeader("X-Eth-Method").value_or("");
          EXPECT_EQ(expected_method, header_value);
          if (expected_method == "eth_blockNumber") {
            header_value =
                request.headers.GetHeader("X-Eth-Block").value_or("");
            EXPECT_EQ(expected_cache_header, header_value);
          } else if (expected_method == "eth_getBlockByNumber") {
            header_value =
                request.headers.GetHeader("X-eth-get-block").value_or("");
            EXPECT_EQ(expected_cache_header, header_value);
          }

          if (IsEndpointUsingBraveWalletProxy(request.url)) {
            header_value =
                request.headers.GetHeader("x-brave-key").value_or("");
            EXPECT_EQ(BUILDFLAG(BRAVE_SERVICES_KEY), header_value);
          } else {
            EXPECT_FALSE(request.headers.HasHeader("x-brave-key"));
          }

          url_loader_factory_.ClearResponses();
          url_loader_factory_.AddResponse(request.url.spec(), content);
        }));
  }

  void SetInterceptor(const GURL& expected_url,
                      const std::map<std::string, std::string>& json_rsp_map) {
    url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [=, this](const network::ResourceRequest& request) {
          EXPECT_EQ(request.url, expected_url);

          if (IsEndpointUsingBraveWalletProxy(request.url)) {
            EXPECT_EQ(BUILDFLAG(BRAVE_SERVICES_KEY),
                      request.headers.GetHeader("x-brave-key").value_or(""));
          } else {
            EXPECT_FALSE(request.headers.HasHeader("x-brave-key"));
          }

          auto header_value = request.headers.GetHeader("X-Eth-Method");
          ASSERT_TRUE(header_value);
          ASSERT_TRUE(json_rsp_map.contains(*header_value));
          url_loader_factory_.ClearResponses();
          url_loader_factory_.AddResponse(request.url.spec(),
                                          json_rsp_map.at(*header_value));
        }));
  }

  void SetOwnedTokenAccountsInterceptor(
      const GURL& expected_url,
      const std::string& token_accounts_rsp,
      const std::string& token2022_accounts_rsp) {
    url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [=, this](const network::ResourceRequest& request) {
          EXPECT_EQ(request.url, expected_url);
          std::string_view request_string(request.request_body->elements()
                                              ->at(0)
                                              .As<network::DataElementBytes>()
                                              .AsStringPiece());
          bool is_token = request_string.find(mojom::kSolanaTokenProgramId) !=
                          std::string::npos;
          bool is_token2022 =
              request_string.find(mojom::kSolanaToken2022ProgramId) !=
              std::string::npos;
          ASSERT_TRUE(is_token || is_token2022);
          url_loader_factory_.ClearResponses();
          url_loader_factory_.AddResponse(
              request.url.spec(),
              is_token ? token_accounts_rsp : token2022_accounts_rsp);
        }));
  }

  void SetInvalidJsonInterceptor() {
    url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [this](const network::ResourceRequest& request) {
          url_loader_factory_.ClearResponses();
          url_loader_factory_.AddResponse(request.url.spec(), "Answer is 42");
        }));
  }

  void SetHTTPRequestTimeoutInterceptor() {
    url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [this](const network::ResourceRequest& request) {
          url_loader_factory_.ClearResponses();
          url_loader_factory_.AddResponse(request.url.spec(), "",
                                          net::HTTP_REQUEST_TIMEOUT);
        }));
  }

  void SetFilecoinActorErrorJsonErrorResponse() {
    url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [this](const network::ResourceRequest& request) {
          url_loader_factory_.ClearResponses();
          url_loader_factory_.AddResponse(request.url.spec(),
                                          R"({
            "jsonrpc":"2.0",
            "id":1,
            "error": {
              "code": 1,
              "message": "resolution lookup failed"
            }
          })");
        }));
  }

  void SetLimitExceededJsonErrorResponse() {
    url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [this](const network::ResourceRequest& request) {
          url_loader_factory_.ClearResponses();
          url_loader_factory_.AddResponse(request.url.spec(),
                                          R"({
            "jsonrpc":"2.0",
            "id":1,
            "error": {
              "code":-32005,
              "message": "Request exceeds defined limit"
            }
          })");
        }));
  }

  void SetIsEip1559Interceptor(const GURL& expected_network, bool is_eip1559) {
    if (is_eip1559) {
      SetInterceptor(
          expected_network, "eth_getBlockByNumber", "latest,false",
          "{\"jsonrpc\":\"2.0\",\"id\": \"0\",\"result\": "
          "{\"baseFeePerGas\":\"0x181f22e7a9\", \"gasLimit\":\"0x6691b8\"}}");
    } else {
      SetInterceptor(expected_network, "eth_getBlockByNumber", "latest,false",
                     "{\"jsonrpc\":\"2.0\",\"id\": \"0\",\"result\": "
                     "{\"gasLimit\":\"0x6691b8\"}}");
    }
  }

  void SetInterceptor(const std::string& content) {
    url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [=, this](const network::ResourceRequest& request) {
          url_loader_factory_.ClearResponses();
          url_loader_factory_.AddResponse(request.url.spec(), content);
        }));
  }

  void SetInterceptors(std::map<GURL, std::string> responses) {
    url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [&, responses](const network::ResourceRequest& request) {
          auto it = responses.find(request.url);
          if (it != responses.end()) {
            std::string response = it->second;
            url_loader_factory_.ClearResponses();
            url_loader_factory_.AddResponse(request.url.spec(), response);
          }
        }));
  }

  bool SetNetwork(const std::string& chain_id,
                  mojom::CoinType coin,
                  const std::optional<::url::Origin>& origin) {
    return json_rpc_service_->SetNetwork(chain_id, coin, origin);
  }

  std::string GetChainId(mojom::CoinType coin,
                         const std::optional<::url::Origin>& origin) {
    std::string chain_id_out;
    base::RunLoop run_loop;
    if (!origin) {
      json_rpc_service_->GetDefaultChainId(
          coin, base::BindLambdaForTesting([&](const std::string& chain_id) {
            chain_id_out = chain_id;
            run_loop.Quit();
          }));
    } else {
      json_rpc_service_->GetChainIdForOrigin(
          coin, *origin,
          base::BindLambdaForTesting([&](const std::string& chain_id) {
            chain_id_out = chain_id;
            run_loop.Quit();
          }));
    }
    run_loop.Run();
    return chain_id_out;
  }

  void TestGetCode(const std::string& address,
                   mojom::CoinType coin,
                   const std::string& chain_id,
                   const std::string& expected_bytecode,
                   mojom::ProviderError expected_error,
                   const std::string& expected_error_message) {
    std::optional<std::string> result;
    base::RunLoop run_loop;
    json_rpc_service_->GetCode(
        address, coin, chain_id,
        base::BindLambdaForTesting([&](const std::string& bytecode,
                                       mojom::ProviderError error,
                                       const std::string& error_message) {
          EXPECT_EQ(error, expected_error);
          EXPECT_EQ(error_message, expected_error_message);
          EXPECT_EQ(bytecode, expected_bytecode);
          run_loop.Quit();
        }));
    run_loop.Run();
  }

  void TestGetERC1155TokenBalance(const std::string& contract,
                                  const std::string& token_id,
                                  const std::string& account_address,
                                  const std::string& chain_id,
                                  const std::string& expected_response,
                                  mojom::ProviderError expected_error,
                                  const std::string& expected_error_message) {
    base::RunLoop run_loop;
    json_rpc_service_->GetERC1155TokenBalance(
        contract, token_id, account_address, chain_id,
        base::BindLambdaForTesting([&](const std::string& response,
                                       mojom::ProviderError error,
                                       const std::string& error_message) {
          EXPECT_EQ(response, expected_response);
          EXPECT_EQ(error, expected_error);
          EXPECT_EQ(error_message, expected_error_message);
          run_loop.Quit();
        }));
    run_loop.Run();
  }

  void TestGetERC721Metadata(const std::string& contract,
                             const std::string& token_id,
                             const std::string& chain_id,
                             const std::string& expected_response,
                             mojom::ProviderError expected_error,
                             const std::string& expected_error_message) {
    base::RunLoop run_loop;
    json_rpc_service_->GetERC721Metadata(
        contract, token_id, chain_id,
        base::BindLambdaForTesting(
            [&](const std::string& token_url, const std::string& response,
                mojom::ProviderError error, const std::string& error_message) {
              EXPECT_EQ(response, expected_response);
              EXPECT_EQ(error, expected_error);
              EXPECT_EQ(error_message, expected_error_message);
              run_loop.Quit();
            }));
    run_loop.Run();
  }

  void TestGetERC1155Metadata(const std::string& contract,
                              const std::string& token_id,
                              const std::string& chain_id,
                              const std::string& expected_response,
                              mojom::ProviderError expected_error,
                              const std::string& expected_error_message) {
    base::RunLoop run_loop;
    json_rpc_service_->GetERC1155Metadata(
        contract, token_id, chain_id,
        base::BindLambdaForTesting(
            [&](const std::string& token_url, const std::string& response,
                mojom::ProviderError error, const std::string& error_message) {
              EXPECT_EQ(response, expected_response);
              EXPECT_EQ(error, expected_error);
              EXPECT_EQ(error_message, expected_error_message);
              run_loop.Quit();
            }));
    run_loop.Run();
  }

  void TestGetEthTokenUri(const std::string& contract,
                          const std::string& token_id,
                          const std::string& chain_id,
                          const std::string& interface_id,
                          const GURL& expected_uri,
                          mojom::ProviderError expected_error,
                          const std::string& expected_error_message) {
    base::RunLoop run_loop;
    json_rpc_service_->GetEthTokenUri(
        chain_id, contract, token_id, interface_id,
        base::BindLambdaForTesting([&](const GURL& uri,
                                       mojom::ProviderError error,
                                       const std::string& error_message) {
          EXPECT_EQ(uri, expected_uri);
          EXPECT_EQ(error, expected_error);
          EXPECT_EQ(error_message, expected_error_message);
          run_loop.Quit();
        }));
    run_loop.Run();
  }

  void TestEthGetLogs(const std::string& chain_id,
                      const std::string& from_block,
                      const std::string& to_block,
                      base::Value::List contract_addresses,
                      base::Value::List topics,
                      const std::vector<Log>& expected_logs,
                      mojom::ProviderError expected_error,
                      const std::string& expected_error_message) {
    base::RunLoop run_loop;
    base::Value::Dict params;
    params.Set("fromBlock", from_block);
    params.Set("toBlock", to_block);
    params.Set("address", std::move(contract_addresses));
    params.Set("topics", std::move(topics));
    json_rpc_service_->EthGetLogs(
        chain_id, std::move(params),
        base::BindLambdaForTesting(
            [&](const std::vector<Log>& logs, base::Value rawlogs,
                mojom::ProviderError error, const std::string& error_message) {
              EXPECT_EQ(logs, expected_logs);
              EXPECT_EQ(error, expected_error);
              EXPECT_EQ(error_message, expected_error_message);
              run_loop.Quit();
            }));
    run_loop.Run();
  }

  void TestGetERC20TokenBalances(
      const std::vector<std::string>& token_contract_addresses,
      const std::string& user_address,
      const std::string& chain_id,
      std::vector<mojom::ERC20BalanceResultPtr> expected_results,
      mojom::ProviderError expected_error,
      const std::string& expected_error_message) {
    base::RunLoop run_loop;
    json_rpc_service_->GetERC20TokenBalances(
        token_contract_addresses, user_address, chain_id,
        base::BindLambdaForTesting(
            [&](std::vector<mojom::ERC20BalanceResultPtr> results,
                mojom::ProviderError error, const std::string& error_message) {
              EXPECT_EQ(results.size(), expected_results.size());
              for (size_t i = 0; i < results.size(); i++) {
                EXPECT_EQ(results[i]->contract_address,
                          expected_results[i]->contract_address);
                EXPECT_EQ(results[i]->balance, expected_results[i]->balance);
              }
              EXPECT_EQ(error, expected_error);
              EXPECT_EQ(error_message, expected_error_message);
              run_loop.Quit();
            }));
    run_loop.Run();
  }

  void TestGetEthNftStandard(
      const std::string& contract_address,
      const std::string& chain_id,
      std::vector<std::string>& interfaces,
      const std::optional<std::string>& expected_standard,
      mojom::ProviderError expected_error,
      const std::string& expected_error_message) {
    base::RunLoop run_loop;
    json_rpc_service_->GetEthNftStandard(
        contract_address, chain_id, interfaces,
        base::BindLambdaForTesting(
            [&](const std::optional<std::string>& standard,
                mojom::ProviderError error, const std::string& error_message) {
              EXPECT_EQ(standard, expected_standard);
              EXPECT_EQ(error, expected_error);
              EXPECT_EQ(error_message, expected_error_message);
              run_loop.Quit();
            }));
    run_loop.Run();
  }

  void TestGetEthTokenSymbol(const std::string& contract_address,
                             const std::string& chain_id,
                             const std::string& expected_symbol,
                             mojom::ProviderError expected_error,
                             const std::string& expected_error_message) {
    base::RunLoop run_loop;
    json_rpc_service_->GetEthTokenSymbol(
        contract_address, chain_id,
        base::BindLambdaForTesting([&](const std::string& symbol,
                                       mojom::ProviderError error,
                                       const std::string& error_message) {
          EXPECT_EQ(symbol, expected_symbol);
          EXPECT_EQ(error, expected_error);
          EXPECT_EQ(error_message, expected_error_message);
          run_loop.Quit();
        }));
    run_loop.Run();
  }

  void TestGetEthTokenDecimals(const std::string& contract_address,
                               const std::string& chain_id,
                               const std::string& expected_decimals,
                               mojom::ProviderError expected_error,
                               const std::string& expected_error_message) {
    base::RunLoop run_loop;
    json_rpc_service_->GetEthTokenDecimals(
        contract_address, chain_id,
        base::BindLambdaForTesting([&](const std::string& decimals,
                                       mojom::ProviderError error,
                                       const std::string& error_message) {
          EXPECT_EQ(decimals, expected_decimals);
          EXPECT_EQ(error, expected_error);
          EXPECT_EQ(error_message, expected_error_message);
          run_loop.Quit();
        }));
    run_loop.Run();
  }

  void TestGetEthTokenInfo(const std::string& contract_address,
                           const std::string& chain_id,
                           mojom::BlockchainTokenPtr expected_token,
                           mojom::ProviderError expected_error,
                           const std::string& expected_error_message) {
    base::RunLoop run_loop;
    json_rpc_service_->GetEthTokenInfo(
        contract_address, chain_id,
        base::BindLambdaForTesting([&](mojom::BlockchainTokenPtr token,
                                       mojom::ProviderError error,
                                       const std::string& error_message) {
          EXPECT_EQ(token, expected_token);
          EXPECT_EQ(error, expected_error);
          EXPECT_EQ(error_message, expected_error_message);
          run_loop.Quit();
        }));
    run_loop.Run();
  }

  void TestGetSolanaBalance(uint64_t expected_balance,
                            mojom::SolanaProviderError expected_error,
                            const std::string& expected_error_message) {
    base::RunLoop run_loop;
    json_rpc_service_->GetSolanaBalance(
        "test_public_key", mojom::kSolanaMainnet,
        base::BindLambdaForTesting([&](uint64_t balance,
                                       mojom::SolanaProviderError error,
                                       const std::string& error_message) {
          EXPECT_EQ(balance, expected_balance);
          EXPECT_EQ(error, expected_error);
          EXPECT_EQ(error_message, expected_error_message);
          run_loop.Quit();
        }));
    run_loop.Run();
  }
  void GetFilBlockHeight(const std::string& chain_id,
                         uint64_t expected_height,
                         mojom::FilecoinProviderError expected_error,
                         const std::string& expected_error_message) {
    bool callback_called = false;
    base::RunLoop run_loop;
    json_rpc_service_->GetFilBlockHeight(
        chain_id, base::BindLambdaForTesting(
                      [&](uint64_t height, mojom::FilecoinProviderError error,
                          const std::string& error_message) {
                        EXPECT_EQ(height, expected_height);
                        EXPECT_EQ(error, expected_error);
                        EXPECT_EQ(error_message, expected_error_message);
                        callback_called = true;
                        run_loop.Quit();
                      }));
    run_loop.Run();
    EXPECT_TRUE(callback_called);
  }
  void GetFilStateSearchMsgLimited(const std::string& chain_id,
                                   const std::string& cid,
                                   uint64_t period,
                                   int64_t expected_exit_code,
                                   mojom::FilecoinProviderError expected_error,
                                   const std::string& expected_error_message) {
    bool callback_called = false;
    base::RunLoop run_loop;
    json_rpc_service_->GetFilStateSearchMsgLimited(
        chain_id, cid, period,
        base::BindLambdaForTesting([&](int64_t exit_code,
                                       mojom::FilecoinProviderError error,
                                       const std::string& error_message) {
          EXPECT_EQ(exit_code, expected_exit_code);
          EXPECT_EQ(error, expected_error);
          EXPECT_EQ(error_message, expected_error_message);
          callback_called = true;
          run_loop.Quit();
        }));
    run_loop.Run();
    EXPECT_TRUE(callback_called);
  }
  void GetSendFilecoinTransaction(const std::string& chain_id,
                                  const std::string& signed_tx,
                                  const std::string& expected_cid,
                                  mojom::FilecoinProviderError expected_error,
                                  const std::string& expected_error_message) {
    base::RunLoop run_loop;
    json_rpc_service_->SendFilecoinTransaction(
        chain_id, signed_tx,
        base::BindLambdaForTesting([&](const std::string& cid,
                                       mojom::FilecoinProviderError error,
                                       const std::string& error_message) {
          EXPECT_EQ(cid, expected_cid);
          EXPECT_EQ(error, expected_error);
          EXPECT_EQ(error_message, expected_error_message);

          run_loop.Quit();
        }));
    run_loop.Run();
  }
  void TestGetSPLTokenAccountBalance(
      const std::string& expected_amount,
      uint8_t expected_decimals,
      const std::string& expected_ui_amount_string,
      mojom::SolanaProviderError expected_error,
      const std::string& expected_error_message) {
    base::RunLoop run_loop;
    json_rpc_service_->GetSPLTokenAccountBalance(
        "BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8",
        "AQoKYV7tYpTrFZN6P5oUufbQKAUr9mNYGe1TTJC9wajM", mojom::kSolanaMainnet,
        base::BindLambdaForTesting([&](const std::string& amount,
                                       uint8_t decimals,
                                       const std::string& ui_amount_string,
                                       mojom::SolanaProviderError error,
                                       const std::string& error_message) {
          EXPECT_EQ(amount, expected_amount);
          EXPECT_EQ(decimals, expected_decimals);
          EXPECT_EQ(ui_amount_string, expected_ui_amount_string);
          EXPECT_EQ(error, expected_error);
          EXPECT_EQ(error_message, expected_error_message);
          run_loop.Quit();
        }));
    run_loop.Run();
  }

  void TestSendSolanaTransaction(const std::string& chain_id,
                                 const std::string& expected_tx_id,
                                 mojom::SolanaProviderError expected_error,
                                 const std::string& expected_error_message,
                                 const std::string& signed_tx = "signed_tx") {
    base::RunLoop run_loop;
    json_rpc_service_->SendSolanaTransaction(
        chain_id, signed_tx, std::nullopt,
        base::BindLambdaForTesting([&](const std::string& tx_id,
                                       mojom::SolanaProviderError error,
                                       const std::string& error_message) {
          EXPECT_EQ(tx_id, expected_tx_id);
          EXPECT_EQ(error, expected_error);
          EXPECT_EQ(error_message, expected_error_message);
          run_loop.Quit();
        }));
    run_loop.Run();
  }

  void TestSimulateSolanaTransaction(
      const std::string& chain_id,
      uint64_t expected_compute_units,
      mojom::SolanaProviderError expected_error,
      const std::string& expected_error_message,
      const std::string& unsigned_tx = "unsigned_tx") {
    base::RunLoop run_loop;
    json_rpc_service_->SimulateSolanaTransaction(
        chain_id, unsigned_tx,
        base::BindLambdaForTesting([&](uint64_t compute_units,
                                       mojom::SolanaProviderError error,
                                       const std::string& error_message) {
          EXPECT_EQ(compute_units, expected_compute_units);
          EXPECT_EQ(error, expected_error);
          EXPECT_EQ(error_message, expected_error_message);
          run_loop.Quit();
        }));
    run_loop.Run();
  }

  void TestGetSolanaLatestBlockhash(const std::string& chain_id,
                                    const std::string& expected_hash,
                                    uint64_t expected_last_valid_block_height,
                                    mojom::SolanaProviderError expected_error,
                                    const std::string& expected_error_message) {
    base::RunLoop run_loop;
    json_rpc_service_->GetSolanaLatestBlockhash(
        chain_id,
        base::BindLambdaForTesting([&](const std::string& hash,
                                       uint64_t last_valid_block_height,
                                       mojom::SolanaProviderError error,
                                       const std::string& error_message) {
          EXPECT_EQ(hash, expected_hash);
          EXPECT_EQ(last_valid_block_height, expected_last_valid_block_height);
          EXPECT_EQ(error, expected_error);
          EXPECT_EQ(error_message, expected_error_message);
          run_loop.Quit();
        }));
    run_loop.Run();
  }

  void TestGetSolanaSignatureStatuses(
      const std::string& chain_id,
      const std::vector<std::string>& tx_signatures,
      const std::vector<std::optional<SolanaSignatureStatus>>& expected_stats,
      mojom::SolanaProviderError expected_error,
      const std::string& expected_error_message) {
    base::RunLoop run_loop;
    json_rpc_service_->GetSolanaSignatureStatuses(
        chain_id, tx_signatures,
        base::BindLambdaForTesting(
            [&](const std::vector<std::optional<SolanaSignatureStatus>>& stats,
                mojom::SolanaProviderError error,
                const std::string& error_message) {
              EXPECT_EQ(stats, expected_stats);
              EXPECT_EQ(error, expected_error);
              EXPECT_EQ(error_message, expected_error_message);
              run_loop.Quit();
            }));
    run_loop.Run();
  }

  void TestGetSolanaAccountInfo(
      const std::string& chain_id,
      std::optional<SolanaAccountInfo> expected_account_info,
      mojom::SolanaProviderError expected_error,
      const std::string& expected_error_message) {
    base::RunLoop run_loop;
    json_rpc_service_->GetSolanaAccountInfo(
        chain_id, "vines1vzrYbzLMRdu58ou5XTby4qAqVRLmqo36NKPTg",
        base::BindLambdaForTesting(
            [&](std::optional<SolanaAccountInfo> account_info,
                mojom::SolanaProviderError error,
                const std::string& error_message) {
              EXPECT_EQ(account_info, expected_account_info);
              EXPECT_EQ(error, expected_error);
              EXPECT_EQ(error_message, expected_error_message);
              run_loop.Quit();
            }));
    run_loop.Run();
  }

  void TestGetSolanaFeeForMessage(const std::string& chain_id,
                                  const std::string& message,
                                  uint64_t expected_tx_fee,
                                  mojom::SolanaProviderError expected_error,
                                  const std::string& expected_error_message) {
    base::RunLoop run_loop;
    json_rpc_service_->GetSolanaFeeForMessage(
        chain_id, message,
        base::BindLambdaForTesting([&](uint64_t tx_fee,
                                       mojom::SolanaProviderError error,
                                       const std::string& error_message) {
          EXPECT_EQ(tx_fee, expected_tx_fee);
          EXPECT_EQ(error, expected_error);
          EXPECT_EQ(error_message, expected_error_message);
          run_loop.Quit();
        }));
    run_loop.Run();
  }

  void TestGetSolanaBlockHeight(const std::string& chain_id,
                                uint64_t expected_block_height,
                                mojom::SolanaProviderError expected_error,
                                const std::string& expected_error_message) {
    base::RunLoop run_loop;
    json_rpc_service_->GetSolanaBlockHeight(
        chain_id,
        base::BindLambdaForTesting([&](uint64_t block_height,
                                       mojom::SolanaProviderError error,
                                       const std::string& error_message) {
          EXPECT_EQ(block_height, expected_block_height);
          EXPECT_EQ(error, expected_error);
          EXPECT_EQ(error_message, expected_error_message);
          run_loop.Quit();
        }));
    run_loop.Run();
  }

  void TestGetSolanaTokenAccountsByOwner(
      const SolanaAddress& solana_address,
      const std::string& chain_id,
      const std::vector<SolanaAccountInfo>& expected_token_accounts,
      mojom::SolanaProviderError expected_error,
      const std::string& expected_error_message) {
    base::RunLoop run_loop;
    json_rpc_service_->GetSolanaTokenAccountsByOwner(
        solana_address, chain_id,
        base::BindLambdaForTesting(
            [&](std::vector<SolanaAccountInfo> token_accounts,
                mojom::SolanaProviderError error,
                const std::string& error_message) {
              EXPECT_EQ(token_accounts, expected_token_accounts);
              EXPECT_EQ(error, expected_error);
              EXPECT_EQ(error_message, expected_error_message);
              run_loop.Quit();
            }));
    run_loop.Run();
  }

  void TestIsSolanaBlockhashValid(bool expected_is_valid,
                                  mojom::SolanaProviderError expected_error,
                                  const std::string& expected_error_message) {
    base::RunLoop run_loop;
    json_rpc_service_->IsSolanaBlockhashValid(
        mojom::kSolanaMainnet, "J7rBdM6AecPDEZp8aPq5iPSNKVkU5Q76F3oAV4eW5wsW",
        std::nullopt,
        base::BindLambdaForTesting([&](bool is_valid,
                                       mojom::SolanaProviderError error,
                                       const std::string& error_message) {
          EXPECT_EQ(is_valid, expected_is_valid);
          EXPECT_EQ(error, expected_error);
          EXPECT_EQ(error_message, expected_error_message);
          run_loop.Quit();
        }));
    run_loop.Run();
  }

  void TestGetSPLTokenBalances(
      const std::string& pubkey,
      const std::string& chain_id,
      std::vector<mojom::SPLTokenAmountPtr> expected_results,
      mojom::SolanaProviderError expected_error,
      const std::string& expected_error_message) {
    base::RunLoop run_loop;
    json_rpc_service_->GetSPLTokenBalances(
        pubkey, chain_id,
        base::BindLambdaForTesting(
            [&](std::vector<mojom::SPLTokenAmountPtr> results,
                mojom::SolanaProviderError error,
                const std::string& error_message) {
              EXPECT_EQ(results.size(), expected_results.size());
              for (size_t i = 0; i < results.size(); i++) {
                EXPECT_EQ(results[i]->mint, expected_results[i]->mint);
                EXPECT_EQ(results[i]->amount, expected_results[i]->amount);
                EXPECT_EQ(results[i]->decimals, expected_results[i]->decimals);
                EXPECT_EQ(results[i]->ui_amount,
                          expected_results[i]->ui_amount);
              }
              EXPECT_EQ(error, expected_error);
              EXPECT_EQ(error_message, expected_error_message);
              run_loop.Quit();
            }));
    run_loop.Run();
  }

  void TestGetSPLTokenProgramByMint(const base::Location& location,
                                    const std::string& mint,
                                    const std::string& chain_id,
                                    mojom::SPLTokenProgram expected_program,
                                    mojom::SolanaProviderError expected_error,
                                    const std::string& expected_error_message) {
    SCOPED_TRACE(testing::Message() << location.ToString());
    base::RunLoop run_loop;
    json_rpc_service_->GetSPLTokenProgramByMint(
        chain_id, mint,
        base::BindLambdaForTesting([&](mojom::SPLTokenProgram program,
                                       mojom::SolanaProviderError error,
                                       const std::string& error_message) {
          EXPECT_EQ(program, expected_program);
          EXPECT_EQ(error, expected_error);
          EXPECT_EQ(error_message, expected_error_message);
          run_loop.Quit();
        }));
    run_loop.Run();
  }

  void TestGetRecentSolanaPrioritizationFees(
      const std::string& chain_id,
      const std::vector<std::pair<uint64_t, uint64_t>>& expected_recent_fees,
      mojom::SolanaProviderError expected_error,
      const std::string& expected_error_message) {
    base::RunLoop run_loop;
    json_rpc_service_->GetRecentSolanaPrioritizationFees(
        chain_id,
        base::BindLambdaForTesting(
            [&](std::vector<std::pair<uint64_t, uint64_t>>& recent_fees,
                mojom::SolanaProviderError error,
                const std::string& error_message) {
              EXPECT_EQ(error, expected_error);
              EXPECT_EQ(error_message, expected_error_message);
              EXPECT_EQ(expected_recent_fees, recent_fees);
              run_loop.Quit();
            }));
    run_loop.Run();
  }

  void GetFilEstimateGas(const std::string& chain_id,
                         const std::string& from,
                         const std::string& to,
                         const std::string& value,
                         const std::string& expected_gas_premium,
                         const std::string& expected_gas_fee_cap,
                         int64_t expected_gas_limit,
                         mojom::FilecoinProviderError expected_error) {
    base::RunLoop loop;
    json_rpc_service_->GetFilEstimateGas(
        chain_id, from, to, "", "", 0, 0, "", value,
        base::BindLambdaForTesting(
            [&](const std::string& gas_premium, const std::string& gas_fee_cap,
                int64_t gas_limit, mojom::FilecoinProviderError error,
                const std::string& error_message) {
              EXPECT_EQ(gas_premium, expected_gas_premium);
              EXPECT_EQ(gas_fee_cap, expected_gas_fee_cap);
              EXPECT_EQ(gas_limit, expected_gas_limit);
              EXPECT_EQ(error, expected_error);
              bool success =
                  mojom::FilecoinProviderError::kSuccess == expected_error;
              EXPECT_EQ(error_message.empty(), success);
              loop.Quit();
            }));
    loop.Run();
  }

  void AddEthereumChainForOrigin(mojom::NetworkInfoPtr chain,
                                 const url::Origin& origin,
                                 const std::string& expected_error_message) {
    EXPECT_EQ(
        expected_error_message,
        json_rpc_service_->AddEthereumChainForOrigin(std::move(chain), origin));
  }

  void TestGetSolTokenMetadata(const std::string& token_mint_address,
                               const std::string& expected_response,
                               mojom::SolanaProviderError expected_error,
                               const std::string& expected_error_message) {
    base::RunLoop loop;
    json_rpc_service_->GetSolTokenMetadata(
        mojom::kSolanaMainnet, token_mint_address,
        base::BindLambdaForTesting([&](const std::string& token_url,
                                       const std::string& response,
                                       mojom::SolanaProviderError error,
                                       const std::string& error_message) {
          CompareJSON(response, expected_response);
          EXPECT_EQ(error, expected_error);
          EXPECT_EQ(error_message, expected_error_message);
          loop.Quit();
        }));
    loop.Run();
  }

  void TestGetNftMetadatas(
      mojom::CoinType coin,
      std::vector<mojom::NftIdentifierPtr> nft_identifiers,
      std::vector<mojom::NftMetadataPtr> expected_metadatas,
      const std::string& expected_error_message) {
    base::RunLoop run_loop;
    json_rpc_service_->GetNftMetadatas(
        coin, std::move(nft_identifiers),
        base::BindLambdaForTesting(
            [&](std::vector<mojom::NftMetadataPtr> metadatas,
                const std::string& error_message) {
              EXPECT_EQ(metadatas, expected_metadatas);
              EXPECT_EQ(error_message, expected_error_message);
              run_loop.Quit();
            }));
    run_loop.Run();
  }

  void TestGetNftBalances(const std::string& wallet_address,
                          std::vector<mojom::NftIdentifierPtr> nft_identifiers,
                          mojom::CoinType coin,
                          const std::vector<uint64_t>& expected_balances,
                          const std::string& expected_error_message) {
    base::RunLoop run_loop;
    json_rpc_service_->GetNftBalances(
        wallet_address, std::move(nft_identifiers), coin,
        base::BindLambdaForTesting([&](const std::vector<uint64_t>& balances,
                                       const std::string& error_message) {
          EXPECT_EQ(balances, expected_balances);
          EXPECT_EQ(error_message, expected_error_message);
          run_loop.Quit();
        }));
    run_loop.Run();
  }

  template <class T>
  void WaitAndVerify(base::MockCallback<T>* callback) {
    task_environment_.RunUntilIdle();
    testing::Mock::VerifyAndClearExpectations(callback);
  }

 protected:
  std::unique_ptr<NetworkManager> network_manager_;
  std::unique_ptr<JsonRpcService> json_rpc_service_;
  network::TestURLLoaderFactory url_loader_factory_;
  base::test::TaskEnvironment task_environment_;

 private:
  sync_preferences::TestingPrefServiceSyncable prefs_;
  sync_preferences::TestingPrefServiceSyncable local_state_prefs_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  data_decoder::test::InProcessDataDecoder in_process_data_decoder_;
};

TEST_F(JsonRpcServiceUnitTest, SetNetwork) {
  const auto& origin_a = url::Origin::Create(GURL("https://a.com"));
  const auto& origin_b = url::Origin::Create(GURL("https://b.com"));
  for (const auto& network :
       network_manager_->GetAllKnownChains(mojom::CoinType::ETH)) {
    SCOPED_TRACE(network->chain_id);
    EXPECT_TRUE(
        SetNetwork(network->chain_id, mojom::CoinType::ETH, std::nullopt));
    EXPECT_TRUE(
        SetNetwork(mojom::kSepoliaChainId, mojom::CoinType::ETH, origin_a));

    EXPECT_EQ(network->chain_id, network_manager_->GetCurrentChainId(
                                     mojom::CoinType::ETH, std::nullopt));
    EXPECT_EQ(mojom::kSepoliaChainId, network_manager_->GetCurrentChainId(
                                          mojom::CoinType::ETH, origin_a));
    EXPECT_EQ(network->chain_id, network_manager_->GetCurrentChainId(
                                     mojom::CoinType::ETH, origin_b));

    EXPECT_EQ(GetChainId(mojom::CoinType::ETH, std::nullopt),
              network->chain_id);
    EXPECT_EQ(GetChainId(mojom::CoinType::ETH, origin_a),
              mojom::kSepoliaChainId);
    EXPECT_EQ(GetChainId(mojom::CoinType::ETH, origin_b), network->chain_id);

    EXPECT_EQ(url::Origin::Create(network_manager_->GetNetworkURL(
                  mojom::CoinType::ETH, std::nullopt)),
              url::Origin::Create(GetActiveEndpointUrl(*network)));
    EXPECT_EQ(
        url::Origin::Create(
            network_manager_->GetNetworkURL(mojom::CoinType::ETH, origin_a)),
        url::Origin::Create(GURL("https://ethereum-sepolia.wallet.brave.com")));
    EXPECT_EQ(url::Origin::Create(network_manager_->GetNetworkURL(
                  mojom::CoinType::ETH, origin_b)),
              url::Origin::Create(GetActiveEndpointUrl(*network)));
  }

  // Solana
  EXPECT_TRUE(
      SetNetwork(mojom::kSolanaMainnet, mojom::CoinType::SOL, std::nullopt));
  EXPECT_FALSE(SetNetwork("0x1234", mojom::CoinType::SOL, std::nullopt));
  EXPECT_TRUE(
      SetNetwork(mojom::kSolanaTestnet, mojom::CoinType::SOL, origin_a));

  EXPECT_EQ(mojom::kSolanaMainnet, network_manager_->GetCurrentChainId(
                                       mojom::CoinType::SOL, std::nullopt));
  EXPECT_EQ(mojom::kSolanaTestnet, network_manager_->GetCurrentChainId(
                                       mojom::CoinType::SOL, origin_a));
  EXPECT_EQ(mojom::kSolanaMainnet, network_manager_->GetCurrentChainId(
                                       mojom::CoinType::SOL, origin_b));

  EXPECT_EQ(GetChainId(mojom::CoinType::SOL, std::nullopt),
            mojom::kSolanaMainnet);
  EXPECT_EQ(GetChainId(mojom::CoinType::SOL, origin_a), mojom::kSolanaTestnet);
  EXPECT_EQ(GetChainId(mojom::CoinType::SOL, origin_b), mojom::kSolanaMainnet);

  EXPECT_EQ(
      url::Origin::Create(GURL(
          network_manager_->GetNetworkURL(mojom::CoinType::SOL, std::nullopt))),
      url::Origin::Create(GURL("https://solana-mainnet.wallet.brave.com")));
  EXPECT_EQ(url::Origin::Create(GURL(network_manager_->GetNetworkURL(
                mojom::CoinType::SOL, origin_a))),
            url::Origin::Create(GURL("https://api.testnet.solana.com")));
  EXPECT_EQ(
      url::Origin::Create(GURL(
          network_manager_->GetNetworkURL(mojom::CoinType::SOL, origin_b))),
      url::Origin::Create(GURL("https://solana-mainnet.wallet.brave.com")));
}

TEST_F(JsonRpcServiceUnitTest, SetCustomNetwork) {
  const auto& origin_a = url::Origin::Create(GURL("https://a.com"));
  const auto& origin_b = url::Origin::Create(GURL("https://b.com"));

  std::vector<base::Value::Dict> values;
  mojom::NetworkInfo chain1 = GetTestNetworkInfo1();
  values.push_back(NetworkInfoToValue(chain1));

  mojom::NetworkInfo chain2 = GetTestNetworkInfo2();
  values.push_back(NetworkInfoToValue(chain2));
  UpdateCustomNetworks(prefs(), &values);

  EXPECT_TRUE(SetNetwork(chain1.chain_id, mojom::CoinType::ETH, std::nullopt));
  EXPECT_TRUE(SetNetwork(chain2.chain_id, mojom::CoinType::ETH, origin_a));

  EXPECT_EQ(GetChainId(mojom::CoinType::ETH, std::nullopt), chain1.chain_id);
  EXPECT_EQ(GetChainId(mojom::CoinType::ETH, origin_a), chain2.chain_id);
  EXPECT_EQ(GetChainId(mojom::CoinType::ETH, origin_b), chain1.chain_id);

  EXPECT_EQ(network_manager_->GetNetworkURL(mojom::CoinType::ETH, std::nullopt),
            GetActiveEndpointUrl(chain1));
  EXPECT_EQ(network_manager_->GetNetworkURL(mojom::CoinType::ETH, origin_a),
            GetActiveEndpointUrl(chain2));
  EXPECT_EQ(network_manager_->GetNetworkURL(mojom::CoinType::ETH, origin_b),
            GetActiveEndpointUrl(chain1));
}

TEST_F(JsonRpcServiceUnitTest, GetAllNetworks) {
  std::vector<base::Value::Dict> values;
  const auto& origin_a = url::Origin::Create(GURL("https://a.com"));
  const auto& origin_b = url::Origin::Create(GURL("https://b.com"));
  mojom::NetworkInfo chain1 = GetTestNetworkInfo1();
  values.push_back(NetworkInfoToValue(chain1));

  mojom::NetworkInfo chain2 = GetTestNetworkInfo2();
  values.push_back(NetworkInfoToValue(chain2));
  UpdateCustomNetworks(prefs(), &values);

  std::vector<mojom::NetworkInfoPtr> expected_chains =
      network_manager_->GetAllChains();
  bool callback_is_called = false;
  json_rpc_service_->GetAllNetworks(base::BindLambdaForTesting(
      [&callback_is_called,
       &expected_chains](std::vector<mojom::NetworkInfoPtr> chains) {
        EXPECT_EQ(expected_chains.size(), chains.size());

        for (size_t i = 0; i < chains.size(); i++) {
          ASSERT_TRUE(chains.at(i).Equals(expected_chains.at(i)));
        }
        callback_is_called = true;
      }));
  task_environment_.RunUntilIdle();
  ASSERT_TRUE(callback_is_called);
}

TEST_F(JsonRpcServiceUnitTest, GetCustomNetworks) {
  base::MockCallback<mojom::JsonRpcService::GetCustomNetworksCallback> callback;
  std::vector<base::Value::Dict> values;
  mojom::NetworkInfo chain1 = GetTestNetworkInfo1(mojom::kMainnetChainId);
  values.push_back(NetworkInfoToValue(chain1));

  mojom::NetworkInfo chain2 = GetTestNetworkInfo1("0x123456");
  values.push_back(NetworkInfoToValue(chain2));
  EXPECT_CALL(callback, Run(ElementsAreArray(std::vector<std::string>{})));
  json_rpc_service_->GetCustomNetworks(mojom::CoinType::ETH, callback.Get());
  testing::Mock::VerifyAndClearExpectations(&callback);
  UpdateCustomNetworks(prefs(), &values);

  EXPECT_CALL(callback, Run(ElementsAreArray({"0x1", "0x123456"})));
  json_rpc_service_->GetCustomNetworks(mojom::CoinType::ETH, callback.Get());
  testing::Mock::VerifyAndClearExpectations(&callback);
}

TEST_F(JsonRpcServiceUnitTest, GetKnownNetworks) {
  base::MockCallback<mojom::JsonRpcService::GetKnownNetworksCallback> callback;
  std::vector<base::Value::Dict> values;
  mojom::NetworkInfo chain1 = GetTestNetworkInfo1(mojom::kMainnetChainId);
  values.push_back(NetworkInfoToValue(chain1));
  UpdateCustomNetworks(prefs(), &values);

  EXPECT_CALL(callback,
              Run(ElementsAreArray({"0x1", "0x2105", "0x89", "0x38", "0xa",
                                    "0xa86a", "0x13a", "0xe9ac0d6", "0xaa36a7",
                                    "0x4cb2f", "0x539"})));
  json_rpc_service_->GetKnownNetworks(mojom::CoinType::ETH, callback.Get());
  testing::Mock::VerifyAndClearExpectations(&callback);
}

TEST_F(JsonRpcServiceUnitTest, GetHiddenNetworks) {
  base::MockCallback<mojom::JsonRpcService::GetHiddenNetworksCallback> callback;

  // Test networks are hidden by default.
  // kLocalhostChainId is active so not listed as hidden.
  EXPECT_CALL(callback,
              Run(ElementsAreArray({mojom::kSepoliaChainId,
                                    mojom::kFilecoinEthereumTestnetChainId})));
  json_rpc_service_->GetHiddenNetworks(mojom::CoinType::ETH, callback.Get());
  testing::Mock::VerifyAndClearExpectations(&callback);

  // Remove network hidden by default.
  network_manager_->RemoveHiddenNetwork(mojom::CoinType::ETH,
                                        mojom::kSepoliaChainId);
  EXPECT_CALL(callback,
              Run(ElementsAreArray({mojom::kFilecoinEthereumTestnetChainId})));
  json_rpc_service_->GetHiddenNetworks(mojom::CoinType::ETH, callback.Get());
  testing::Mock::VerifyAndClearExpectations(&callback);

  // Making custom network hidden.
  network_manager_->AddHiddenNetwork(mojom::CoinType::ETH, "0x123");
  EXPECT_CALL(
      callback,
      Run(ElementsAreArray({mojom::kFilecoinEthereumTestnetChainId, "0x123"})));
  json_rpc_service_->GetHiddenNetworks(mojom::CoinType::ETH, callback.Get());
  testing::Mock::VerifyAndClearExpectations(&callback);

  // Making custom network visible.
  network_manager_->RemoveHiddenNetwork(mojom::CoinType::ETH, "0x123");
  EXPECT_CALL(callback,
              Run(ElementsAreArray({mojom::kFilecoinEthereumTestnetChainId})));
  json_rpc_service_->GetHiddenNetworks(mojom::CoinType::ETH, callback.Get());
  testing::Mock::VerifyAndClearExpectations(&callback);

  // Change active network so kLocalhostChainId becomes hidden.
  SetNetwork(mojom::kMainnetChainId, mojom::CoinType::ETH, std::nullopt);
  EXPECT_CALL(callback,
              Run(ElementsAreArray({mojom::kLocalhostChainId,
                                    mojom::kFilecoinEthereumTestnetChainId})));
  json_rpc_service_->GetHiddenNetworks(mojom::CoinType::ETH, callback.Get());
  testing::Mock::VerifyAndClearExpectations(&callback);

  // Remove all hidden networks.
  network_manager_->RemoveHiddenNetwork(mojom::CoinType::ETH,
                                        mojom::kSepoliaChainId);
  network_manager_->RemoveHiddenNetwork(mojom::CoinType::ETH,
                                        mojom::kLocalhostChainId);
  network_manager_->RemoveHiddenNetwork(mojom::CoinType::ETH,
                                        mojom::kFilecoinEthereumTestnetChainId);
  EXPECT_CALL(callback, Run(ElementsAreArray<std::string>({})));
  json_rpc_service_->GetHiddenNetworks(mojom::CoinType::ETH, callback.Get());
  testing::Mock::VerifyAndClearExpectations(&callback);
}

TEST_F(JsonRpcServiceUnitTest, AddEthereumChainApproved) {
  auto expected_token = mojom::BlockchainToken::New();
  expected_token->coin = mojom::CoinType::ETH;
  expected_token->chain_id = "0x111";
  expected_token->name = "symbol_name";
  expected_token->symbol = "symbol";
  expected_token->decimals = 11;
  expected_token->logo = "https://url1.com";
  expected_token->visible = true;
  expected_token->spl_token_program = mojom::SPLTokenProgram::kUnsupported;

  EXPECT_THAT(GetAllUserAssets(prefs()),
              Not(Contains(Eq(std::ref(expected_token)))));

  mojom::NetworkInfo chain = GetTestNetworkInfo1("0x111");
  bool callback_is_called = false;
  mojom::ProviderError expected = mojom::ProviderError::kSuccess;
  ASSERT_FALSE(network_manager_->GetNetworkURL("0x111", mojom::CoinType::ETH)
                   .is_valid());
  SetEthChainIdInterceptor(GetActiveEndpointUrl(chain), "0x111");
  json_rpc_service_->AddChain(
      chain.Clone(),
      base::BindLambdaForTesting(
          [&callback_is_called, &expected](const std::string& chain_id,
                                           mojom::ProviderError error,
                                           const std::string& error_message) {
            ASSERT_FALSE(chain_id.empty());
            EXPECT_EQ(error, expected);
            ASSERT_TRUE(error_message.empty());
            callback_is_called = true;
          }));
  task_environment_.RunUntilIdle();

  EXPECT_THAT(GetAllUserAssets(prefs()),
              Contains(Eq(std::ref(expected_token))));

  bool failed_callback_is_called = false;
  mojom::ProviderError expected_error = mojom::ProviderError::kInvalidParams;
  json_rpc_service_->AddChain(
      chain.Clone(),
      base::BindLambdaForTesting([&failed_callback_is_called, &expected_error](
                                     const std::string& chain_id,
                                     mojom::ProviderError error,
                                     const std::string& error_message) {
        ASSERT_FALSE(chain_id.empty());
        EXPECT_EQ(error, expected_error);
        ASSERT_FALSE(error_message.empty());
        failed_callback_is_called = true;
      }));
  task_environment_.RunUntilIdle();
  ASSERT_TRUE(failed_callback_is_called);

  json_rpc_service_->AddEthereumChainRequestCompleted("0x111", true);

  ASSERT_TRUE(callback_is_called);
  ASSERT_TRUE(network_manager_->GetNetworkURL("0x111", mojom::CoinType::ETH)
                  .is_valid());

  // Prefs should be updated.
  ASSERT_EQ(GetAllEthCustomChains().size(), 1u);
  EXPECT_EQ(GetAllEthCustomChains()[0], chain.Clone());

  callback_is_called = false;
  json_rpc_service_->AddEthereumChainRequestCompleted("0x111", true);
  ASSERT_FALSE(callback_is_called);
}

TEST_F(JsonRpcServiceUnitTest, AddEthereumChainApprovedForOrigin) {
  auto expected_token = mojom::BlockchainToken::New();
  expected_token->coin = mojom::CoinType::ETH;
  expected_token->chain_id = "0x111";
  expected_token->name = "symbol_name";
  expected_token->symbol = "symbol";
  expected_token->decimals = 11;
  expected_token->logo = "https://url1.com";
  expected_token->visible = true;
  expected_token->spl_token_program = mojom::SPLTokenProgram::kUnsupported;

  EXPECT_THAT(GetAllUserAssets(prefs()),
              Not(Contains(Eq(std::ref(expected_token)))));

  mojom::NetworkInfo chain = GetTestNetworkInfo1("0x111");

  base::RunLoop loop;
  std::unique_ptr<TestJsonRpcServiceObserver> observer(
      new TestJsonRpcServiceObserver(loop.QuitClosure(), "0x111", ""));

  json_rpc_service_->AddObserver(observer->GetReceiver());

  mojo::PendingRemote<brave_wallet::mojom::JsonRpcServiceObserver> receiver;
  mojo::MakeSelfOwnedReceiver(std::move(observer),
                              receiver.InitWithNewPipeAndPassReceiver());

  ASSERT_FALSE(network_manager_->GetNetworkURL("0x111", mojom::CoinType::ETH)
                   .is_valid());
  SetEthChainIdInterceptor(GetActiveEndpointUrl(chain), "0x111");
  EXPECT_EQ("",
            json_rpc_service_->AddEthereumChainForOrigin(
                chain.Clone(), url::Origin::Create(GURL("https://brave.com"))));
  json_rpc_service_->AddEthereumChainRequestCompleted("0x111", true);
  loop.Run();

  EXPECT_THAT(GetAllUserAssets(prefs()),
              Contains(Eq(std::ref(expected_token))));

  ASSERT_TRUE(network_manager_->GetNetworkURL("0x111", mojom::CoinType::ETH)
                  .is_valid());

  // Prefs should be updated.
  ASSERT_EQ(GetAllEthCustomChains().size(), 1u);
  EXPECT_EQ(GetAllEthCustomChains()[0], chain.Clone());

  json_rpc_service_->AddEthereumChainRequestCompleted("0x111", true);
}

TEST_F(JsonRpcServiceUnitTest, AddEthereumChainForOriginRejected) {
  mojom::NetworkInfo chain = GetTestNetworkInfo1("0x111");

  base::RunLoop loop;
  std::unique_ptr<TestJsonRpcServiceObserver> observer(
      new TestJsonRpcServiceObserver(
          loop.QuitClosure(), "0x111",
          l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST)));

  json_rpc_service_->AddObserver(observer->GetReceiver());

  mojo::PendingRemote<brave_wallet::mojom::JsonRpcServiceObserver> receiver;
  mojo::MakeSelfOwnedReceiver(std::move(observer),
                              receiver.InitWithNewPipeAndPassReceiver());

  ASSERT_FALSE(network_manager_->GetNetworkURL("0x111", mojom::CoinType::ETH)
                   .is_valid());
  SetEthChainIdInterceptor(GetActiveEndpointUrl(chain), "0x111");
  EXPECT_EQ("",
            json_rpc_service_->AddEthereumChainForOrigin(
                chain.Clone(), url::Origin::Create(GURL("https://brave.com"))));
  json_rpc_service_->AddEthereumChainRequestCompleted("0x111", false);
  loop.Run();
  ASSERT_FALSE(network_manager_->GetNetworkURL("0x111", mojom::CoinType::ETH)
                   .is_valid());
}

TEST_F(JsonRpcServiceUnitTest, AddChain) {
  {
    mojom::NetworkInfo chain = GetTestNetworkInfo1("0x111");
    ASSERT_FALSE(
        network_manager_->GetNetworkURL(chain.chain_id, mojom::CoinType::ETH)
            .is_valid());
    SetEthChainIdInterceptor(GetActiveEndpointUrl(chain), chain.chain_id);

    base::MockCallback<mojom::JsonRpcService::AddChainCallback> callback;
    EXPECT_CALL(callback, Run("0x111", mojom::ProviderError::kSuccess, ""));

    json_rpc_service_->AddChain(chain.Clone(), callback.Get());
    task_environment_.RunUntilIdle();
    EXPECT_EQ(GURL("https://url1.com"),
              network_manager_->GetChain("0x111", mojom::CoinType::ETH)
                  ->rpc_endpoints[0]);
  }

  {
    mojom::NetworkInfo chain = GetTestNetworkInfo1(mojom::kFilecoinTestnet);
    chain.coin = mojom::CoinType::FIL;

    base::MockCallback<mojom::JsonRpcService::AddChainCallback> callback;
    EXPECT_CALL(callback, Run(mojom::kFilecoinTestnet,
                              mojom::ProviderError::kSuccess, ""));

    json_rpc_service_->AddChain(chain.Clone(), callback.Get());
    // No need to RunUntilIdle, callback is resolved synchronously.
    EXPECT_EQ(GURL("https://url1.com"),
              network_manager_
                  ->GetChain(mojom::kFilecoinTestnet, mojom::CoinType::FIL)
                  ->rpc_endpoints[0]);
  }

  {
    // Only known networks are allowed.
    mojom::NetworkInfo chain = GetTestNetworkInfo1("0x123");
    chain.coin = mojom::CoinType::FIL;

    base::MockCallback<mojom::JsonRpcService::AddChainCallback> callback;
    EXPECT_CALL(callback,
                Run("0x123", mojom::ProviderError::kInternalError,
                    l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)));

    json_rpc_service_->AddChain(chain.Clone(), callback.Get());
    // No need to RunUntilIdle, callback is resolved synchronously.
  }

  {
    mojom::NetworkInfo chain = GetTestNetworkInfo1(mojom::kSolanaMainnet);
    chain.coin = mojom::CoinType::SOL;

    base::MockCallback<mojom::JsonRpcService::AddChainCallback> callback;
    EXPECT_CALL(callback,
                Run(mojom::kSolanaMainnet, mojom::ProviderError::kSuccess, ""));

    json_rpc_service_->AddChain(chain.Clone(), callback.Get());
    // No need to RunUntilIdle, callback is resolved synchronously.
    EXPECT_EQ(
        GURL("https://url1.com"),
        network_manager_->GetChain(mojom::kSolanaMainnet, mojom::CoinType::SOL)
            ->rpc_endpoints[0]);
  }

  {
    // Only known networks are allowed.
    mojom::NetworkInfo chain = GetTestNetworkInfo1("0x123");
    chain.coin = mojom::CoinType::SOL;

    base::MockCallback<mojom::JsonRpcService::AddChainCallback> callback;
    EXPECT_CALL(callback,
                Run("0x123", mojom::ProviderError::kInternalError,
                    l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)));

    json_rpc_service_->AddChain(chain.Clone(), callback.Get());
    // No need to RunUntilIdle, callback is resolved synchronously.
  }

  // HTTP localhost URL is okay.
  {
    mojom::NetworkInfo chain = GetTestNetworkInfo1("0x3344");
    ASSERT_FALSE(
        network_manager_->GetNetworkURL(chain.chain_id, mojom::CoinType::ETH)
            .is_valid());
    SetEthChainIdInterceptor(GetActiveEndpointUrl(chain), chain.chain_id);

    base::MockCallback<mojom::JsonRpcService::AddChainCallback> callback;
    EXPECT_CALL(callback, Run("0x3344", mojom::ProviderError::kSuccess, ""));

    chain.rpc_endpoints.push_back(GURL("http://localhost:8545"));
    json_rpc_service_->AddChain(chain.Clone(), callback.Get());
    task_environment_.RunUntilIdle();
    EXPECT_THAT(network_manager_->GetChain("0x3344", mojom::CoinType::ETH)
                    ->rpc_endpoints,
                ElementsAreArray(
                    {GURL("https://url1.com"), GURL("http://localhost:8545")}));
  }

  // HTTP URL that's not localhost is not valid.
  {
    mojom::NetworkInfo chain = GetTestNetworkInfoWithHttpURL("0x5566");
    ASSERT_FALSE(
        network_manager_->GetNetworkURL(chain.chain_id, mojom::CoinType::ETH)
            .is_valid());

    base::MockCallback<mojom::JsonRpcService::AddChainCallback> callback;
    EXPECT_CALL(
        callback,
        Run("0x5566", mojom::ProviderError::kInvalidParams,
            l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_ADD_CHAIN_INVALID_URL)));

    json_rpc_service_->AddChain(chain.Clone(), callback.Get());
    // No need to RunUntilIdle, callback is resolved synchronously.
  }
}

TEST_F(JsonRpcServiceUnitTest, AddEthereumChainError) {
  mojom::NetworkInfo chain = GetTestNetworkInfo1("0x111");

  bool callback_is_called = false;
  mojom::ProviderError expected = mojom::ProviderError::kSuccess;
  ASSERT_FALSE(
      network_manager_->GetNetworkURL(chain.chain_id, mojom::CoinType::ETH)
          .is_valid());
  SetEthChainIdInterceptor(GetActiveEndpointUrl(chain), chain.chain_id);
  json_rpc_service_->AddChain(
      chain.Clone(),
      base::BindLambdaForTesting(
          [&callback_is_called, &expected](const std::string& chain_id,
                                           mojom::ProviderError error,
                                           const std::string& error_message) {
            ASSERT_FALSE(chain_id.empty());
            EXPECT_EQ(error, expected);
            ASSERT_TRUE(error_message.empty());
            callback_is_called = true;
          }));
  task_environment_.RunUntilIdle();
  ASSERT_TRUE(callback_is_called);

  // Add a same chain.
  bool third_callback_is_called = false;
  mojom::ProviderError third_expected = mojom::ProviderError::kInvalidParams;
  json_rpc_service_->AddChain(
      chain.Clone(),
      base::BindLambdaForTesting([&third_callback_is_called, &third_expected](
                                     const std::string& chain_id,
                                     mojom::ProviderError error,
                                     const std::string& error_message) {
        ASSERT_FALSE(chain_id.empty());
        EXPECT_EQ(error, third_expected);
        EXPECT_EQ(error_message, l10n_util::GetStringUTF8(
                                     IDS_SETTINGS_WALLET_NETWORKS_EXISTS));
        third_callback_is_called = true;
      }));
  task_environment_.RunUntilIdle();
  ASSERT_TRUE(third_callback_is_called);

  // new chain, not valid rpc url
  mojom::NetworkInfo chain4("0x444", "chain_name4", {"https://url4.com"},
                            {"https://url4.com"}, 0, {GURL("https://url4.com")},
                            "symbol_name", "symbol", 11, mojom::CoinType::ETH,
                            {mojom::KeyringId::kDefault});
  bool fourth_callback_is_called = false;
  mojom::ProviderError fourth_expected =
      mojom::ProviderError::kUserRejectedRequest;
  auto network_url = GetActiveEndpointUrl(chain4);
  SetEthChainIdInterceptor(network_url, "0x555");
  json_rpc_service_->AddChain(
      chain4.Clone(),
      base::BindLambdaForTesting(
          [&fourth_callback_is_called, &fourth_expected, &network_url](
              const std::string& chain_id, mojom::ProviderError error,
              const std::string& error_message) {
            ASSERT_FALSE(chain_id.empty());
            EXPECT_EQ(error, fourth_expected);
            EXPECT_EQ(error_message,
                      l10n_util::GetStringFUTF8(
                          IDS_BRAVE_WALLET_ETH_CHAIN_ID_FAILED,
                          base::ASCIIToUTF16(network_url.spec())));
            fourth_callback_is_called = true;
          }));
  task_environment_.RunUntilIdle();
  ASSERT_TRUE(fourth_callback_is_called);

  // new chain, broken validation response
  mojom::NetworkInfo chain5("0x444", "chain_name5", {"https://url5.com"},
                            {"https://url5.com"}, 0, {GURL("https://url5.com")},
                            "symbol_name", "symbol", 11, mojom::CoinType::ETH,
                            {mojom::KeyringId::kDefault});
  bool fifth_callback_is_called = false;
  mojom::ProviderError fifth_expected =
      mojom::ProviderError::kUserRejectedRequest;
  network_url = GetActiveEndpointUrl(chain5);
  SetEthChainIdInterceptorWithBrokenResponse(network_url);
  json_rpc_service_->AddChain(
      chain5.Clone(),
      base::BindLambdaForTesting(
          [&fifth_callback_is_called, &fifth_expected, &network_url](
              const std::string& chain_id, mojom::ProviderError error,
              const std::string& error_message) {
            ASSERT_FALSE(chain_id.empty());
            EXPECT_EQ(error, fifth_expected);
            EXPECT_EQ(error_message,
                      l10n_util::GetStringFUTF8(
                          IDS_BRAVE_WALLET_ETH_CHAIN_ID_FAILED,
                          base::ASCIIToUTF16(GURL(network_url).spec())));
            fifth_callback_is_called = true;
          }));
  task_environment_.RunUntilIdle();
  ASSERT_TRUE(fifth_callback_is_called);
}

TEST_F(JsonRpcServiceUnitTest, AddEthereumChainForOriginError) {
  mojom::NetworkInfo chain = GetTestNetworkInfo1("0x1");
  auto origin = url::Origin::Create(GURL("https://brave.com"));

  // Known eth chain should be rejected.
  ASSERT_TRUE(
      network_manager_->GetNetworkURL(chain.chain_id, mojom::CoinType::ETH)
          .is_valid());
  AddEthereumChainForOrigin(
      chain.Clone(), origin,
      l10n_util::GetStringUTF8(IDS_SETTINGS_WALLET_NETWORKS_EXISTS));

  // Try to add a custom chain.
  chain.chain_id = "0x111";
  ASSERT_FALSE(
      network_manager_->GetNetworkURL(chain.chain_id, mojom::CoinType::ETH)
          .is_valid());
  SetEthChainIdInterceptor(GetActiveEndpointUrl(chain), chain.chain_id);
  AddEthereumChainForOrigin(chain.Clone(), origin, "");

  // Other chain with same origin that has a pending request should be rejected.
  auto chain2 = chain.Clone();
  chain2->chain_id = "0x222";
  AddEthereumChainForOrigin(
      chain2->Clone(), origin,
      l10n_util::GetStringUTF8(IDS_WALLET_ALREADY_IN_PROGRESS_ERROR));

  // Try to add same chain with other origin should get rejected.
  AddEthereumChainForOrigin(
      chain.Clone(), url::Origin::Create(GURL("https://others.com")),
      l10n_util::GetStringUTF8(IDS_WALLET_ALREADY_IN_PROGRESS_ERROR));

  auto network_url = GetActiveEndpointUrl(chain);
  // New chain, not valid rpc url.
  {
    base::RunLoop loop;
    std::unique_ptr<TestJsonRpcServiceObserver> observer(
        new TestJsonRpcServiceObserver(
            loop.QuitClosure(), "0x333",
            l10n_util::GetStringFUTF8(IDS_BRAVE_WALLET_ETH_CHAIN_ID_FAILED,
                                      base::ASCIIToUTF16(network_url.spec()))));

    json_rpc_service_->AddObserver(observer->GetReceiver());

    mojo::PendingRemote<brave_wallet::mojom::JsonRpcServiceObserver> receiver;
    mojo::MakeSelfOwnedReceiver(std::move(observer),
                                receiver.InitWithNewPipeAndPassReceiver());

    chain.chain_id = "0x333";
    AddEthereumChainForOrigin(
        chain.Clone(), url::Origin::Create(GURL("https://others2.com")), "");
    SetEthChainIdInterceptor(GetActiveEndpointUrl(chain), "0x555");
    json_rpc_service_->AddEthereumChainRequestCompleted(chain.chain_id, true);
    loop.Run();
  }

  // New chain, broken validation response.
  {
    base::RunLoop loop;
    std::unique_ptr<TestJsonRpcServiceObserver> observer(
        new TestJsonRpcServiceObserver(
            loop.QuitClosure(), "0x444",
            l10n_util::GetStringFUTF8(
                IDS_BRAVE_WALLET_ETH_CHAIN_ID_FAILED,
                base::ASCIIToUTF16(GURL(network_url).spec()))));

    json_rpc_service_->AddObserver(observer->GetReceiver());

    mojo::PendingRemote<brave_wallet::mojom::JsonRpcServiceObserver> receiver;
    mojo::MakeSelfOwnedReceiver(std::move(observer),
                                receiver.InitWithNewPipeAndPassReceiver());

    chain.chain_id = "0x444";
    AddEthereumChainForOrigin(
        chain.Clone(), url::Origin::Create(GURL("https://others3.com")), "");
    SetEthChainIdInterceptorWithBrokenResponse(GetActiveEndpointUrl(chain));
    json_rpc_service_->AddEthereumChainRequestCompleted(chain.chain_id, true);
    loop.Run();
  }
}

TEST_F(JsonRpcServiceUnitTest, Request) {
  bool callback_called = false;
  std::string request =
      "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"eth_blockNumber\",\"params\":"
      "[]}";
  std::string result = "\"0xb539d5\"";
  std::string expected_response =
      "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":" + result + "}";
  SetInterceptor(GetNetwork(mojom::kLocalhostChainId, mojom::CoinType::ETH),
                 "eth_blockNumber", "true", expected_response);
  json_rpc_service_->Request(
      mojom::kLocalhostChainId, request, true, base::Value(),
      mojom::CoinType::ETH,
      base::BindOnce(&OnRequestResponse, &callback_called, true /* success */,
                     result));
  task_environment_.RunUntilIdle();
  EXPECT_TRUE(callback_called);

  callback_called = false;
  request =
      "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"eth_getBlockByNumber\","
      "\"params\":"
      "[\"0x5BAD55\",true]}";
  result = "\"0xb539d5\"";
  expected_response =
      "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":" + result + "}";
  SetInterceptor(GetNetwork(mojom::kLocalhostChainId, mojom::CoinType::ETH),
                 "eth_getBlockByNumber", "0x5BAD55,true", expected_response);
  json_rpc_service_->Request(
      mojom::kLocalhostChainId, request, true, base::Value(),
      mojom::CoinType::ETH,
      base::BindOnce(&OnRequestResponse, &callback_called, true /* success */,
                     result));
  task_environment_.RunUntilIdle();
  EXPECT_TRUE(callback_called);

  callback_called = false;
  SetHTTPRequestTimeoutInterceptor();
  json_rpc_service_->Request(
      mojom::kLocalhostChainId, request, true, base::Value(),
      mojom::CoinType::ETH,
      base::BindOnce(&OnRequestResponse, &callback_called, false /* success */,
                     ""));
  task_environment_.RunUntilIdle();
  EXPECT_TRUE(callback_called);
}

TEST_F(JsonRpcServiceUnitTest, Request_BadHeaderValues) {
  std::string request =
      "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"eth_blockNumber\n\","
      "\"params\":"
      "[]}";
  std::string mock_response =
      R"({"jsonrpc":"2.0",
          "id":1,
          "error":": {
            "code": -32601,
            "message": "unsupported method"
          }})";
  SetInterceptor(GetNetwork(mojom::kLocalhostChainId, mojom::CoinType::ETH), "",
                 "", mock_response);
  bool callback_called = false;
  json_rpc_service_->Request(
      mojom::kLocalhostChainId, request, true, base::Value(),
      mojom::CoinType::ETH,
      base::BindOnce(&OnRequestResponse, &callback_called, false, ""));
  task_environment_.RunUntilIdle();
  EXPECT_TRUE(callback_called);
}

TEST_F(JsonRpcServiceUnitTest, GetCode) {
  // Contract code response
  SetInterceptor(GetNetwork(mojom::kMainnetChainId, mojom::CoinType::ETH),
                 "eth_getCode", "",
                 // Result has code that was intentionally truncated
                 "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":\"0x6060604\"}");
  TestGetCode("0x0d8775f648430679a709e98d2b0cb6250d2887ef",
              mojom::CoinType::ETH, mojom::kMainnetChainId, "0x6060604",
              mojom::ProviderError::kSuccess, "");

  // EOA response
  SetInterceptor(GetNetwork(mojom::kMainnetChainId, mojom::CoinType::ETH),
                 "eth_getCode", "",
                 "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":\"0x\"}");
  TestGetCode("0x0d8775f648430679a709e98d2b0cb6250d2887ef",
              mojom::CoinType::ETH, mojom::kMainnetChainId, "0x",
              mojom::ProviderError::kSuccess, "");

  // Processes error results OK
  SetInterceptor(GetNetwork(mojom::kMainnetChainId, mojom::CoinType::ETH),
                 "eth_getCode", "",
                 MakeJsonRpcErrorResponse(
                     static_cast<int>(mojom::ProviderError::kInternalError),
                     l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)));
  TestGetCode("0x0d8775f648430679a709e98d2b0cb6250d2887ef",
              mojom::CoinType::ETH, mojom::kMainnetChainId, "",
              mojom::ProviderError::kInternalError,
              l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));

  // Processes invalid chain IDs OK
  SetInterceptor(GetNetwork(mojom::kMainnetChainId, mojom::CoinType::SOL),
                 "eth_getCode", "",
                 "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":\"0x\"}");
  TestGetCode("0x0d8775f648430679a709e98d2b0cb6250d2887ef",
              mojom::CoinType::SOL, mojom::kLocalhostChainId, "",
              mojom::ProviderError::kInvalidParams,
              l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
}

TEST_F(JsonRpcServiceUnitTest, GetBalance) {
  bool callback_called = false;
  SetInterceptor(GetNetwork(mojom::kMainnetChainId, mojom::CoinType::ETH),
                 "eth_getBalance", "",
                 "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":\"0xb539d5\"}");
  json_rpc_service_->GetBalance(
      "0x4e02f254184E904300e0775E4b8eeCB1", mojom::CoinType::ETH,
      mojom::kMainnetChainId,
      base::BindOnce(&OnStringResponse, &callback_called,
                     mojom::ProviderError::kSuccess, "", "0xb539d5"));
  task_environment_.RunUntilIdle();
  EXPECT_TRUE(callback_called);

  callback_called = false;
  SetHTTPRequestTimeoutInterceptor();
  json_rpc_service_->GetBalance(
      "0x4e02f254184E904300e0775E4b8eeCB1", mojom::CoinType::ETH,
      mojom::kMainnetChainId,
      base::BindOnce(&OnStringResponse, &callback_called,
                     mojom::ProviderError::kInternalError,
                     l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR), ""));
  task_environment_.RunUntilIdle();
  EXPECT_TRUE(callback_called);

  callback_called = false;
  SetInvalidJsonInterceptor();
  json_rpc_service_->GetBalance(
      "0x4e02f254184E904300e0775E4b8eeCB1", mojom::CoinType::ETH,
      mojom::kMainnetChainId,
      base::BindOnce(&OnStringResponse, &callback_called,
                     mojom::ProviderError::kParsingError,
                     l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR), ""));
  task_environment_.RunUntilIdle();
  EXPECT_TRUE(callback_called);

  callback_called = false;
  json_rpc_service_->GetBalance(
      "0x4e02f254184E904300e0775E4b8eeCB1", mojom::CoinType::ETH, "",
      base::BindOnce(&OnStringResponse, &callback_called,
                     mojom::ProviderError::kInvalidParams,
                     l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS),
                     ""));
  task_environment_.RunUntilIdle();
  EXPECT_TRUE(callback_called);

  callback_called = false;
  SetLimitExceededJsonErrorResponse();
  json_rpc_service_->GetBalance(
      "0x4e02f254184E904300e0775E4b8eeCB1", mojom::CoinType::ETH,
      mojom::kMainnetChainId,
      base::BindOnce(&OnStringResponse, &callback_called,
                     mojom::ProviderError::kLimitExceeded,
                     "Request exceeds defined limit", ""));
  task_environment_.RunUntilIdle();
  EXPECT_TRUE(callback_called);

  callback_called = false;
  std::string json = R"({"jsonrpc":"2.0","id":1,"result":"100000"})";
  SetInterceptor(GetNetwork(mojom::kFilecoinMainnet, mojom::CoinType::FIL),
                 "Filecoin.WalletBalance", "", json);
  json_rpc_service_->GetBalance(
      "addr", mojom::CoinType::FIL, mojom::kFilecoinMainnet,
      base::BindOnce(&OnStringResponse, &callback_called,
                     mojom::ProviderError::kSuccess, "", "100000"));
  task_environment_.RunUntilIdle();
  EXPECT_TRUE(callback_called);

  callback_called = false;
  SetInterceptor(GetNetwork(mojom::kFilecoinTestnet, mojom::CoinType::FIL),
                 "Filecoin.WalletBalance", "", json);
  json_rpc_service_->GetBalance(
      "addr", mojom::CoinType::FIL, mojom::kFilecoinTestnet,
      base::BindOnce(&OnStringResponse, &callback_called,
                     mojom::ProviderError::kSuccess, "", "100000"));
  task_environment_.RunUntilIdle();
  EXPECT_TRUE(callback_called);
}

TEST_F(JsonRpcServiceUnitTest, GetFeeHistory) {
  std::string json =
      R"(
      {
        "jsonrpc":"2.0",
        "id":1,
        "result": {
          "baseFeePerGas": [
            "0x215d00b8c8",
            "0x24beaded75"
          ],
          "gasUsedRatio": [
            0.020687709938714324
          ],
          "oldestBlock": "0xd6b1b0",
          "reward": [
            [
              "0x77359400",
              "0x77359400",
              "0x2816a6cfb"
            ]
          ]
        }
      })";

  SetInterceptor(GetNetwork(mojom::kLocalhostChainId, mojom::CoinType::ETH),
                 "eth_feeHistory", "", json);
  base::RunLoop run_loop;
  json_rpc_service_->GetFeeHistory(
      mojom::kLocalhostChainId,
      base::BindLambdaForTesting(
          [&](const std::vector<std::string>& base_fee_per_gas,
              const std::vector<double>& gas_used_ratio,
              const std::string& oldest_block,
              const std::vector<std::vector<std::string>>& reward,
              mojom::ProviderError error, const std::string& error_message) {
            EXPECT_EQ(error, mojom::ProviderError::kSuccess);
            EXPECT_TRUE(error_message.empty());
            EXPECT_EQ(base_fee_per_gas, (std::vector<std::string>{
                                            "0x215d00b8c8", "0x24beaded75"}));
            EXPECT_EQ(gas_used_ratio,
                      (std::vector<double>{0.020687709938714324}));
            EXPECT_EQ(oldest_block, "0xd6b1b0");
            EXPECT_EQ(reward,
                      (std::vector<std::vector<std::string>>{
                          {"0x77359400", "0x77359400", "0x2816a6cfb"}}));
            run_loop.Quit();
          }));
  run_loop.Run();

  // OK: valid response
  SetHTTPRequestTimeoutInterceptor();
  base::RunLoop run_loop2;
  json_rpc_service_->GetFeeHistory(
      mojom::kLocalhostChainId,
      base::BindLambdaForTesting(
          [&](const std::vector<std::string>& base_fee_per_gas,
              const std::vector<double>& gas_used_ratio,
              const std::string& oldest_block,
              const std::vector<std::vector<std::string>>& reward,
              mojom::ProviderError error, const std::string& error_message) {
            EXPECT_EQ(error, mojom::ProviderError::kInternalError);
            EXPECT_EQ(error_message,
                      l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
            run_loop2.Quit();
          }));
  run_loop2.Run();

  // KO: invalid JSON
  SetInvalidJsonInterceptor();
  base::RunLoop run_loop3;
  json_rpc_service_->GetFeeHistory(
      mojom::kLocalhostChainId,
      base::BindLambdaForTesting(
          [&](const std::vector<std::string>& base_fee_per_gas,
              const std::vector<double>& gas_used_ratio,
              const std::string& oldest_block,
              const std::vector<std::vector<std::string>>& reward,
              mojom::ProviderError error, const std::string& error_message) {
            EXPECT_EQ(error, mojom::ProviderError::kInternalError);
            EXPECT_EQ(error_message,
                      l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
            run_loop3.Quit();
          }));
  run_loop3.Run();

  // KO: valid JSON but unexpected response
  SetInterceptor(GetNetwork(mojom::kLocalhostChainId, mojom::CoinType::ETH),
                 "eth_feeHistory", "", "{\"foo\":0}");
  base::RunLoop run_loop4;
  json_rpc_service_->GetFeeHistory(
      mojom::kLocalhostChainId,
      base::BindLambdaForTesting(
          [&](const std::vector<std::string>& base_fee_per_gas,
              const std::vector<double>& gas_used_ratio,
              const std::string& oldest_block,
              const std::vector<std::vector<std::string>>& reward,
              mojom::ProviderError error, const std::string& error_message) {
            EXPECT_EQ(error, mojom::ProviderError::kParsingError);
            EXPECT_EQ(error_message,
                      l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));
            run_loop4.Quit();
          }));
  run_loop4.Run();

  // KO: valid error response
  SetLimitExceededJsonErrorResponse();
  base::RunLoop run_loop5;
  json_rpc_service_->GetFeeHistory(
      mojom::kLocalhostChainId,
      base::BindLambdaForTesting(
          [&](const std::vector<std::string>& base_fee_per_gas,
              const std::vector<double>& gas_used_ratio,
              const std::string& oldest_block,
              const std::vector<std::vector<std::string>>& reward,
              mojom::ProviderError error, const std::string& error_message) {
            EXPECT_EQ(error, mojom::ProviderError::kLimitExceeded);
            EXPECT_EQ(error_message, "Request exceeds defined limit");
            run_loop5.Quit();
          }));
  run_loop5.Run();
}

TEST_F(JsonRpcServiceUnitTest, GetERC20TokenBalance) {
  bool callback_called = false;
  SetInterceptor(
      GetNetwork(mojom::kMainnetChainId, mojom::CoinType::ETH), "eth_call", "",
      "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":"
      "\"0x00000000000000000000000000000000000000000000000166e12cfce39a0000\""
      "}");

  json_rpc_service_->GetERC20TokenBalance(
      "0x0D8775F648430679A709E98d2b0Cb6250d2887EF",
      "0x4e02f254184E904300e0775E4b8eeCB1", mojom::kMainnetChainId,
      base::BindOnce(&OnStringResponse, &callback_called,
                     mojom::ProviderError::kSuccess, "",
                     "0x166e12cfce39a0000"));
  task_environment_.RunUntilIdle();
  EXPECT_TRUE(callback_called);

  callback_called = false;
  SetHTTPRequestTimeoutInterceptor();
  json_rpc_service_->GetERC20TokenBalance(
      "0x0D8775F648430679A709E98d2b0Cb6250d2887EF",
      "0x4e02f254184E904300e0775E4b8eeCB1", mojom::kMainnetChainId,
      base::BindOnce(&OnStringResponse, &callback_called,
                     mojom::ProviderError::kInternalError,
                     l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR), ""));
  task_environment_.RunUntilIdle();
  EXPECT_TRUE(callback_called);

  callback_called = false;
  SetInvalidJsonInterceptor();
  json_rpc_service_->GetERC20TokenBalance(
      "0x0d8775f648430679a709e98d2b0cb6250d2887ef",
      "0x4e02f254184E904300e0775E4b8eeCB1", mojom::kMainnetChainId,
      base::BindOnce(&OnStringResponse, &callback_called,
                     mojom::ProviderError::kParsingError,
                     l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR), ""));
  task_environment_.RunUntilIdle();
  EXPECT_TRUE(callback_called);

  callback_called = false;
  SetLimitExceededJsonErrorResponse();
  json_rpc_service_->GetERC20TokenBalance(
      "0x0d8775f648430679a709e98d2b0cb6250d2887ef",
      "0x4e02f254184E904300e0775E4b8eeCB1", mojom::kMainnetChainId,
      base::BindOnce(&OnStringResponse, &callback_called,
                     mojom::ProviderError::kLimitExceeded,
                     "Request exceeds defined limit", ""));
  task_environment_.RunUntilIdle();
  EXPECT_TRUE(callback_called);

  // Invalid input should fail.
  callback_called = false;
  json_rpc_service_->GetERC20TokenBalance(
      "", "", mojom::kMainnetChainId,
      base::BindOnce(&OnStringResponse, &callback_called,
                     mojom::ProviderError::kInvalidParams,
                     l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS),
                     ""));
  task_environment_.RunUntilIdle();
  EXPECT_TRUE(callback_called);

  callback_called = false;
  json_rpc_service_->GetERC20TokenBalance(
      "0x0d8775f648430679a709e98d2b0cb6250d2887ef",
      "0x4e02f254184E904300e0775E4b8eeCB1", "",
      base::BindOnce(&OnStringResponse, &callback_called,
                     mojom::ProviderError::kInvalidParams,
                     l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS),
                     ""));
  task_environment_.RunUntilIdle();
  EXPECT_TRUE(callback_called);
}

TEST_F(JsonRpcServiceUnitTest, GetERC20TokenAllowance) {
  bool callback_called = false;
  SetInterceptor(
      GetNetwork(mojom::kMainnetChainId, mojom::CoinType::ETH), "eth_call", "",
      "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":"
      "\"0x00000000000000000000000000000000000000000000000166e12cfce39a0000\""
      "}");

  json_rpc_service_->GetERC20TokenAllowance(
      "0x0d8775f648430679a709e98d2b0cb6250d2887ef",
      "0xBFb30a082f650C2A15D0632f0e87bE4F8e64460f",
      "0xBFb30a082f650C2A15D0632f0e87bE4F8e64460a", mojom::kMainnetChainId,
      base::BindOnce(&OnStringResponse, &callback_called,
                     mojom::ProviderError::kSuccess, "",
                     "0x166e12cfce39a0000"));
  task_environment_.RunUntilIdle();
  EXPECT_TRUE(callback_called);

  callback_called = false;
  SetHTTPRequestTimeoutInterceptor();
  json_rpc_service_->GetERC20TokenAllowance(
      "0x0d8775f648430679a709e98d2b0cb6250d2887ef",
      "0xBFb30a082f650C2A15D0632f0e87bE4F8e64460f",
      "0xBFb30a082f650C2A15D0632f0e87bE4F8e64460a", mojom::kMainnetChainId,
      base::BindOnce(&OnStringResponse, &callback_called,
                     mojom::ProviderError::kInternalError,
                     l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR), ""));
  task_environment_.RunUntilIdle();
  EXPECT_TRUE(callback_called);

  callback_called = false;
  SetInvalidJsonInterceptor();
  json_rpc_service_->GetERC20TokenAllowance(
      "0x0d8775f648430679a709e98d2b0cb6250d2887ef",
      "0xBFb30a082f650C2A15D0632f0e87bE4F8e64460f",
      "0xBFb30a082f650C2A15D0632f0e87bE4F8e64460a", mojom::kMainnetChainId,
      base::BindOnce(&OnStringResponse, &callback_called,
                     mojom::ProviderError::kParsingError,
                     l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR), ""));
  task_environment_.RunUntilIdle();
  EXPECT_TRUE(callback_called);

  callback_called = false;
  SetLimitExceededJsonErrorResponse();
  json_rpc_service_->GetERC20TokenAllowance(
      "0x0d8775f648430679a709e98d2b0cb6250d2887ef",
      "0xBFb30a082f650C2A15D0632f0e87bE4F8e64460f",
      "0xBFb30a082f650C2A15D0632f0e87bE4F8e64460a", mojom::kMainnetChainId,
      base::BindOnce(&OnStringResponse, &callback_called,
                     mojom::ProviderError::kLimitExceeded,
                     "Request exceeds defined limit", ""));
  task_environment_.RunUntilIdle();
  EXPECT_TRUE(callback_called);

  // Invalid input should fail.
  callback_called = false;
  json_rpc_service_->GetERC20TokenAllowance(
      "", "", "", mojom::kMainnetChainId,
      base::BindOnce(&OnStringResponse, &callback_called,
                     mojom::ProviderError::kInvalidParams,
                     l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS),
                     ""));
  task_environment_.RunUntilIdle();
  EXPECT_TRUE(callback_called);
}

TEST_F(JsonRpcServiceUnitTest, GetERC20TokenBalances) {
  // Invalid token contract addresses yields invalid params
  TestGetERC20TokenBalances(
      std::vector<std::string>(), "0xB4B2802129071b2B9eBb8cBB01EA1E4D14B34961",
      mojom::kMainnetChainId, {}, mojom::ProviderError::kInvalidParams,
      l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));

  // Unsupported chain ID yields invalid params
  TestGetERC20TokenBalances(
      std::vector<std::string>({"0x0d8775f648430679a709e98d2b0cb6250d2887ef"}),
      "0xB4B2802129071b2B9eBb8cBB01EA1E4D14B34961", mojom::kSepoliaChainId, {},
      mojom::ProviderError::kInvalidParams,
      l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));

  // Invalid user address yields invalid calldata, which yields invalid params
  TestGetERC20TokenBalances(
      std::vector<std::string>({"0x0d8775f648430679a709e98d2b0cb6250d2887ef"}),
      "", mojom::kMainnetChainId, {}, mojom::ProviderError::kInvalidParams,
      l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));

  // Valid input should succeed.
  // 1. Test with 1 token contract address that successfully fetches a balance
  // (0x0d8775f648430679a709e98d2b0cb6250d2887ef BAT)
  SetInterceptor(GetNetwork(mojom::kMainnetChainId, mojom::CoinType::ETH),
                 "eth_call", "", R"({
      "jsonrpc":"2.0",
      "id":1,
      "result":"0x000000000000000000000000000000000000000000000000000000000000002000000000000000000000000000000000000000000000000000000000000000010000000000000000000000000000000000000000000000000000000000000020000000000000000000000000000000000000000000000000000000000000000100000000000000000000000000000000000000000000000000000000000000400000000000000000000000000000000000000000000000000000000000000020000000000000000000000000000000000000000000000006e83695ab1f893c00"
  })");
  std::vector<mojom::ERC20BalanceResultPtr> expected_results;
  mojom::ERC20BalanceResultPtr result = mojom::ERC20BalanceResult::New();
  result->contract_address = "0x0d8775f648430679a709e98d2b0cb6250d2887ef";
  result->balance =
      "0x000000000000000000000000000000000000000000000006e83695ab1f893c00";
  expected_results.push_back(std::move(result));
  TestGetERC20TokenBalances(
      std::vector<std::string>({"0x0d8775f648430679a709e98d2b0cb6250d2887ef"}),
      "0xB4B2802129071b2B9eBb8cBB01EA1E4D14B34961", mojom::kMainnetChainId,
      std::move(expected_results), mojom::ProviderError::kSuccess, "");

  // Valid request leading to timeout yields internal error
  SetHTTPRequestTimeoutInterceptor();
  TestGetERC20TokenBalances(
      std::vector<std::string>({"0x0d8775f648430679a709e98d2b0cb6250d2887ef"}),
      "0xB4B2802129071b2B9eBb8cBB01EA1E4D14B34961", mojom::kMainnetChainId, {},
      mojom::ProviderError::kInternalError,
      l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));

  // Valid request yields invalid json response yields parsing error
  SetInvalidJsonInterceptor();
  TestGetERC20TokenBalances(
      std::vector<std::string>({"0x0d8775f648430679a709e98d2b0cb6250d2887ef"}),
      "0xB4B2802129071b2B9eBb8cBB01EA1E4D14B34961", mojom::kMainnetChainId, {},
      mojom::ProviderError::kParsingError,
      l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));

  // Valid request, valid json response, but invalid RLP encoded data yields
  // parsing error
  SetInterceptor(GetNetwork(mojom::kMainnetChainId, mojom::CoinType::ETH),
                 "eth_call", "", R"({
      "jsonrpc":"2.0",
      "id":1,
      "result":"0xinvalid"
  })");
  TestGetERC20TokenBalances(
      std::vector<std::string>({"0x0d8775f648430679a709e98d2b0cb6250d2887ef"}),
      "0xB4B2802129071b2B9eBb8cBB01EA1E4D14B34961", mojom::kMainnetChainId, {},
      mojom::ProviderError::kParsingError,
      l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));

  // Valid request, valid json response, but unexpected RLP encoded data
  // (mismatch between provided contract addresses supplied (1) in params vs.
  // returned balances (3)) yields internal error
  SetInterceptor(GetNetwork(mojom::kMainnetChainId, mojom::CoinType::ETH),
                 "eth_call", "", R"({
      "jsonrpc":"2.0",
      "id":1,
      "result":"0x00000000000000000000000000000000000000000000000000000000000000200000000000000000000000000000000000000000000000000000000000000003000000000000000000000000000000000000000000000000000000000000006000000000000000000000000000000000000000000000000000000000000000e00000000000000000000000000000000000000000000000000000000000000140000000000000000000000000000000000000000000000000000000000000000100000000000000000000000000000000000000000000000000000000000000400000000000000000000000000000000000000000000000000000000000000020000000000000000000000000000000000000000000000006e83695ab1f893c000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000004000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000001000000000000000000000000000000000000000000000000000000000000004000000000000000000000000000000000000000000000000000000000000000200000000000000000000000000000000000000000000000000000000000000000"
  })");
  TestGetERC20TokenBalances(
      std::vector<std::string>({"0x0d8775f648430679a709e98d2b0cb6250d2887ef"}),
      "0xB4B2802129071b2B9eBb8cBB01EA1E4D14B34961", mojom::kMainnetChainId, {},
      mojom::ProviderError::kInternalError,
      l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
}

class UDGetManyCallHandler : public EthCallHandler {
 public:
  explicit UDGetManyCallHandler(const EthAddress& contract_address)
      : EthCallHandler(contract_address,
                       GetFunctionHashBytes4("getMany(string[],uint256)")) {}
  ~UDGetManyCallHandler() override = default;

  std::optional<std::string> HandleEthCall(eth_abi::Span call_data) override {
    auto [_, args] =
        *eth_abi::ExtractFunctionSelectorAndArgsFromCall(call_data);
    auto keys_array = eth_abi::ExtractStringArrayFromTuple(args, 0);
    auto namehash_bytes = eth_abi::ExtractFixedBytesFromTuple<32>(args, 1);
    EXPECT_TRUE(keys_array);
    EXPECT_TRUE(namehash_bytes);

    calls_number_++;

    if (!raw_response_.empty()) {
      return raw_response_;
    }

    std::vector<std::string> result_strings;
    for (auto& key : *keys_array) {
      std::string result_value;
      for (auto& item : items_) {
        if (base::ranges::equal(Namehash(item.domain), *namehash_bytes) &&
            key == item.key) {
          result_value = item.value;
          break;
        }
      }
      result_strings.push_back(result_value);
    }
    return MakeJsonRpcTupleResponse(
        eth_abi::TupleEncoder().AddStringArray(result_strings));
  }

  void AddItem(std::string_view domain,
               std::string_view key,
               std::string_view value) {
    items_.push_back(
        Item{std::string(domain), std::string(key), std::string(value)});
  }

  void Reset() {
    items_.clear();
    raw_response_.clear();
  }

  void SetRawResponse(std::string response) {
    raw_response_ = std::move(response);
  }

  int calls_number() const { return calls_number_; }

 private:
  struct Item {
    std::string domain;
    std::string key;
    std::string value;
  };
  std::vector<Item> items_;
  std::string raw_response_;
  int calls_number_ = 0;
};

class UnstoppableDomainsUnitTest : public JsonRpcServiceUnitTest {
 public:
  using GetWalletAddrCallback =
      mojom::JsonRpcService::UnstoppableDomainsGetWalletAddrCallback;
  using ResolveDnsCallback =
      JsonRpcService::UnstoppableDomainsResolveDnsCallback;

  void SetUp() override {
    JsonRpcServiceUnitTest::SetUp();
    eth_mainnet_endpoint_handler_ = std::make_unique<JsonRpcEndpointHandler>(
        NetworkManager::GetUnstoppableDomainsRpcUrl(mojom::kMainnetChainId));
    eth_mainnet_getmany_call_handler_ = std::make_unique<UDGetManyCallHandler>(
        EthAddress::FromHex(GetUnstoppableDomainsProxyReaderContractAddress(
            mojom::kMainnetChainId)));
    eth_mainnet_endpoint_handler_->AddEthCallHandler(
        eth_mainnet_getmany_call_handler_.get());

    polygon_endpoint_handler_ = std::make_unique<JsonRpcEndpointHandler>(
        NetworkManager::GetUnstoppableDomainsRpcUrl(
            mojom::kPolygonMainnetChainId));
    polygon_getmany_call_handler_ = std::make_unique<UDGetManyCallHandler>(
        EthAddress::FromHex(GetUnstoppableDomainsProxyReaderContractAddress(
            mojom::kPolygonMainnetChainId)));
    polygon_endpoint_handler_->AddEthCallHandler(
        polygon_getmany_call_handler_.get());

    base_endpoint_handler_ = std::make_unique<JsonRpcEndpointHandler>(
        NetworkManager::GetUnstoppableDomainsRpcUrl(
            mojom::kBaseMainnetChainId));
    base_getmany_call_handler_ = std::make_unique<UDGetManyCallHandler>(
        EthAddress::FromHex(GetUnstoppableDomainsProxyReaderContractAddress(
            mojom::kBaseMainnetChainId)));
    base_endpoint_handler_->AddEthCallHandler(base_getmany_call_handler_.get());

    url_loader_factory_.SetInterceptor(base::BindRepeating(
        &UnstoppableDomainsUnitTest::HandleRequest, base::Unretained(this)));
  }

  // Eth Mainnet: brad.crypto -> 0x8aaD44321A86b170879d7A244c1e8d360c99DdA8
  static constexpr char k0x8aaD44Addr[] =
      "0x8aaD44321A86b170879d7A244c1e8d360c99DdA8";

  // Polygon: javajobs.crypto -> 0x3a2f3f7aab82d69036763cfd3f755975f84496e6
  static constexpr char k0x3a2f3fAddr[] =
      "0x3a2f3f7aab82d69036763cfd3f755975f84496e6";

  // Base: test.bald -> 0x1111111111111111111111111111111111111111
  static constexpr char k0x111111Addr[] =
      "0x1111111111111111111111111111111111111111";

  void SetEthResponse(const std::string& domain, const std::string& response) {
    eth_mainnet_getmany_call_handler_->Reset();
    eth_mainnet_getmany_call_handler_->AddItem(domain, "crypto.ETH.address",
                                               response);
  }
  void SetEthRawResponse(const std::string& response) {
    eth_mainnet_getmany_call_handler_->Reset();
    eth_mainnet_getmany_call_handler_->SetRawResponse(response);
  }
  void SetEthTimeoutResponse() {
    eth_mainnet_getmany_call_handler_->Reset();
    eth_mainnet_getmany_call_handler_->SetRawResponse("timeout");
  }
  void SetPolygonResponse(const std::string& domain,
                          const std::string& response) {
    polygon_getmany_call_handler_->Reset();
    polygon_getmany_call_handler_->AddItem(domain, "crypto.ETH.address",
                                           response);
  }
  void SetPolygonRawResponse(const std::string& response) {
    polygon_getmany_call_handler_->Reset();
    polygon_getmany_call_handler_->SetRawResponse(response);
  }
  void SetPolygonTimeoutResponse() {
    polygon_getmany_call_handler_->Reset();
    polygon_getmany_call_handler_->SetRawResponse("timeout");
  }
  void SetBaseResponse(const std::string& domain, const std::string& response) {
    base_getmany_call_handler_->Reset();
    base_getmany_call_handler_->AddItem(domain, "crypto.ETH.address", response);
  }
  void SetBaseRawResponse(const std::string& response) {
    base_getmany_call_handler_->Reset();
    base_getmany_call_handler_->SetRawResponse(response);
  }
  void SetBaseTimeoutResponse() {
    base_getmany_call_handler_->Reset();
    base_getmany_call_handler_->SetRawResponse("timeout");
  }

  std::string DnsIpfsResponse() const {
    return MakeJsonRpcStringArrayResponse(
        {"QmbWqxBEKC3P8tqsKc98xmWNzrzDtRLMiMPL8wBuTGsMnR", "", "", "", "",
         "https://brave.com"});
  }

  std::string DnsBraveResponse() const {
    return MakeJsonRpcStringArrayResponse(
        {"", "", "", "", "", "https://brave.com"});
  }

  std::string DnsEmptyResponse() const {
    return MakeJsonRpcStringArrayResponse({"", "", "", "", "", ""});
  }

  mojom::BlockchainTokenPtr MakeToken() {
    auto token = mojom::BlockchainToken::New();
    token->coin = mojom::CoinType::ETH;
    token->chain_id = mojom::kMainnetChainId;
    token->symbol = "ETH";
    return token;
  }

 protected:
  std::unique_ptr<JsonRpcEndpointHandler> eth_mainnet_endpoint_handler_;
  std::unique_ptr<JsonRpcEndpointHandler> polygon_endpoint_handler_;
  std::unique_ptr<JsonRpcEndpointHandler> base_endpoint_handler_;

  std::unique_ptr<UDGetManyCallHandler> eth_mainnet_getmany_call_handler_;
  std::unique_ptr<UDGetManyCallHandler> polygon_getmany_call_handler_;
  std::unique_ptr<UDGetManyCallHandler> base_getmany_call_handler_;

  void HandleRequest(const network::ResourceRequest& request) {
    url_loader_factory_.ClearResponses();
    std::optional<std::string> response;
    if ((response = eth_mainnet_endpoint_handler_->HandleRequest(request))) {
      if (response == "timeout") {
        url_loader_factory_.AddResponse(request.url.spec(), "",
                                        net::HTTP_REQUEST_TIMEOUT);
      } else {
        url_loader_factory_.AddResponse(request.url.spec(), *response);
      }
    } else if ((response = polygon_endpoint_handler_->HandleRequest(request))) {
      if (response == "timeout") {
        url_loader_factory_.AddResponse(request.url.spec(), "",
                                        net::HTTP_REQUEST_TIMEOUT);
      } else {
        url_loader_factory_.AddResponse(request.url.spec(), *response);
      }
    } else if ((response = base_endpoint_handler_->HandleRequest(request))) {
      if (response == "timeout") {
        url_loader_factory_.AddResponse(request.url.spec(), "",
                                        net::HTTP_REQUEST_TIMEOUT);
      } else {
        url_loader_factory_.AddResponse(request.url.spec(), *response);
      }
    } else {
      url_loader_factory_.AddResponse(request.url.spec(), "",
                                      net::HTTP_INTERNAL_SERVER_ERROR);
    }
  }
};

TEST_F(UnstoppableDomainsUnitTest, GetWalletAddr_PolygonNetworkError) {
  base::MockCallback<GetWalletAddrCallback> callback;
  EXPECT_CALL(callback,
              Run("", mojom::ProviderError::kInternalError,
                  l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)));
  SetEthTimeoutResponse();
  SetPolygonTimeoutResponse();
  json_rpc_service_->UnstoppableDomainsGetWalletAddr("brad.crypto", MakeToken(),
                                                     callback.Get());
  WaitAndVerify(&callback);

  EXPECT_CALL(callback,
              Run("", mojom::ProviderError::kInternalError,
                  l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)));
  SetEthResponse("brad.crypto", k0x8aaD44Addr);
  SetPolygonTimeoutResponse();
  json_rpc_service_->UnstoppableDomainsGetWalletAddr("brad.crypto", MakeToken(),
                                                     callback.Get());
  WaitAndVerify(&callback);

  EXPECT_CALL(callback,
              Run("", mojom::ProviderError::kParsingError,
                  l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR)));
  SetEthResponse("brad.crypto", k0x8aaD44Addr);
  SetPolygonRawResponse("Not a json");
  json_rpc_service_->UnstoppableDomainsGetWalletAddr("brad.crypto", MakeToken(),
                                                     callback.Get());
  WaitAndVerify(&callback);

  EXPECT_CALL(callback,
              Run("", mojom::ProviderError::kLimitExceeded, "Error!"));
  SetEthResponse("brad.crypto", k0x8aaD44Addr);
  SetPolygonRawResponse(MakeJsonRpcErrorResponse(-32005, "Error!"));
  json_rpc_service_->UnstoppableDomainsGetWalletAddr("brad.crypto", MakeToken(),
                                                     callback.Get());
  WaitAndVerify(&callback);
}

TEST_F(UnstoppableDomainsUnitTest, GetWalletAddr_PolygonResult) {
  base::MockCallback<GetWalletAddrCallback> callback;
  EXPECT_CALL(callback, Run(k0x3a2f3fAddr, mojom::ProviderError::kSuccess, ""));
  SetEthResponse("javajobs.crypto", "");
  SetPolygonResponse("javajobs.crypto", k0x3a2f3fAddr);
  json_rpc_service_->UnstoppableDomainsGetWalletAddr(
      "javajobs.crypto", MakeToken(), callback.Get());
  WaitAndVerify(&callback);

  EXPECT_CALL(callback, Run(k0x3a2f3fAddr, mojom::ProviderError::kSuccess, ""));
  SetEthResponse("javajobs.crypto", k0x8aaD44Addr);
  SetPolygonResponse("javajobs.crypto", k0x3a2f3fAddr);
  json_rpc_service_->UnstoppableDomainsGetWalletAddr(
      "javajobs.crypto", MakeToken(), callback.Get());
  WaitAndVerify(&callback);

  EXPECT_CALL(callback, Run(k0x3a2f3fAddr, mojom::ProviderError::kSuccess, ""));
  SetEthResponse("javajobs.crypto", "");
  SetPolygonResponse("javajobs.crypto", k0x3a2f3fAddr);
  json_rpc_service_->UnstoppableDomainsGetWalletAddr(
      "javajobs.crypto", MakeToken(), callback.Get());
  task_environment_.RunUntilIdle();
}

TEST_F(UnstoppableDomainsUnitTest, GetWalletAddr_BaseResult) {
  base::MockCallback<GetWalletAddrCallback> callback;
  EXPECT_CALL(callback, Run(k0x111111Addr, mojom::ProviderError::kSuccess, ""));
  SetEthResponse("javajobs.crypto", "");
  SetPolygonResponse("javajobs.crypto", "");
  SetBaseResponse("javajobs.crypto", k0x111111Addr);
  json_rpc_service_->UnstoppableDomainsGetWalletAddr(
      "javajobs.crypto", MakeToken(), callback.Get());
  WaitAndVerify(&callback);

  EXPECT_CALL(callback, Run(k0x111111Addr, mojom::ProviderError::kSuccess, ""));
  SetEthResponse("javajobs.crypto", k0x8aaD44Addr);
  SetPolygonResponse("javajobs.crypto", "");
  SetBaseResponse("javajobs.crypto", k0x111111Addr);
  json_rpc_service_->UnstoppableDomainsGetWalletAddr(
      "javajobs.crypto", MakeToken(), callback.Get());
  WaitAndVerify(&callback);
}

TEST_F(UnstoppableDomainsUnitTest, GetWalletAddr_FallbackToEthMainnet) {
  base::MockCallback<GetWalletAddrCallback> callback;
  EXPECT_CALL(callback, Run(k0x8aaD44Addr, mojom::ProviderError::kSuccess, ""));
  SetEthResponse("brad.crypto", k0x8aaD44Addr);
  SetPolygonResponse("brad.crypto", "");
  json_rpc_service_->UnstoppableDomainsGetWalletAddr("brad.crypto", MakeToken(),
                                                     callback.Get());
  task_environment_.RunUntilIdle();
}

TEST_F(UnstoppableDomainsUnitTest, GetWalletAddr_FallbackToEthMainnetError) {
  base::MockCallback<GetWalletAddrCallback> callback;
  EXPECT_CALL(callback,
              Run("", mojom::ProviderError::kInternalError,
                  l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)));
  SetEthTimeoutResponse();
  SetPolygonResponse("brad.crypto", "");
  json_rpc_service_->UnstoppableDomainsGetWalletAddr("brad.crypto", MakeToken(),
                                                     callback.Get());
  task_environment_.RunUntilIdle();
}

TEST_F(UnstoppableDomainsUnitTest, GetWalletAddr_InvalidDomain) {
  base::MockCallback<GetWalletAddrCallback> callback;
  EXPECT_CALL(callback,
              Run("", mojom::ProviderError::kInvalidParams,
                  l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS)));
  json_rpc_service_->UnstoppableDomainsGetWalletAddr("brad.test", MakeToken(),
                                                     callback.Get());
  EXPECT_EQ(0, url_loader_factory_.NumPending());
  task_environment_.RunUntilIdle();
}

TEST_F(UnstoppableDomainsUnitTest, GetWalletAddr_ManyCalls) {
  base::MockCallback<GetWalletAddrCallback> callback1;
  EXPECT_CALL(callback1,
              Run(k0x3a2f3fAddr, mojom::ProviderError::kSuccess, ""));
  base::MockCallback<GetWalletAddrCallback> callback2;
  EXPECT_CALL(callback2,
              Run(k0x3a2f3fAddr, mojom::ProviderError::kSuccess, ""));
  base::MockCallback<GetWalletAddrCallback> callback3;
  EXPECT_CALL(callback3,
              Run(k0x8aaD44Addr, mojom::ProviderError::kSuccess, ""));

  // This will resolve javajobs.crypto requests.
  eth_mainnet_getmany_call_handler_->AddItem(
      "javajobs.crypto", "crypto.ETH.address", k0x8aaD44Addr);
  polygon_getmany_call_handler_->AddItem("javajobs.crypto",
                                         "crypto.ETH.address", k0x3a2f3fAddr);

  // This will resolve another.crypto requests.
  eth_mainnet_getmany_call_handler_->AddItem(
      "another.crypto", "crypto.ETH.address", k0x8aaD44Addr);
  polygon_getmany_call_handler_->AddItem("another.crypto", "crypto.ETH.address",
                                         "");

  EXPECT_EQ(0, eth_mainnet_getmany_call_handler_->calls_number());
  EXPECT_EQ(0, polygon_getmany_call_handler_->calls_number());
  json_rpc_service_->UnstoppableDomainsGetWalletAddr(
      "javajobs.crypto", MakeToken(), callback1.Get());
  json_rpc_service_->UnstoppableDomainsGetWalletAddr(
      "javajobs.crypto", MakeToken(), callback2.Get());
  task_environment_.RunUntilIdle();
  EXPECT_EQ(1, eth_mainnet_getmany_call_handler_->calls_number());
  EXPECT_EQ(1, polygon_getmany_call_handler_->calls_number());
  testing::Mock::VerifyAndClearExpectations(&callback1);
  testing::Mock::VerifyAndClearExpectations(&callback2);

  json_rpc_service_->UnstoppableDomainsGetWalletAddr(
      "another.crypto", MakeToken(), callback3.Get());
  task_environment_.RunUntilIdle();
  EXPECT_EQ(2, eth_mainnet_getmany_call_handler_->calls_number());
  EXPECT_EQ(2, polygon_getmany_call_handler_->calls_number());
  testing::Mock::VerifyAndClearExpectations(&callback3);
}

TEST_F(UnstoppableDomainsUnitTest, GetWalletAddr_MultipleKeys) {
  base::MockCallback<GetWalletAddrCallback> callback;
  EXPECT_CALL(callback, Run("ethaddr1", mojom::ProviderError::kSuccess, ""));

  auto token = mojom::BlockchainToken::New();
  token->chain_id = mojom::kBnbSmartChainMainnetChainId;
  token->symbol = "USDT";
  token->coin = mojom::CoinType::ETH;

  // Default fallback is always crypto.ETH.address.
  eth_mainnet_getmany_call_handler_->AddItem("test.crypto",
                                             "crypto.ETH.address", "ethaddr1");
  json_rpc_service_->UnstoppableDomainsGetWalletAddr(
      "test.crypto", token.Clone(), callback.Get());
  WaitAndVerify(&callback);

  // crypto.USDT.address is preferred over default.
  EXPECT_CALL(callback, Run("ethaddr2", mojom::ProviderError::kSuccess, ""));
  eth_mainnet_getmany_call_handler_->AddItem("test.crypto",
                                             "crypto.USDT.address", "ethaddr2");
  json_rpc_service_->UnstoppableDomainsGetWalletAddr(
      "test.crypto", token.Clone(), callback.Get());
  WaitAndVerify(&callback);

  // crypto.USDT.version.BEP20.address is the most preferred.
  EXPECT_CALL(callback, Run("ethaddr3", mojom::ProviderError::kSuccess, ""));
  eth_mainnet_getmany_call_handler_->AddItem(
      "test.crypto", "crypto.USDT.version.BEP20.address", "ethaddr3");
  json_rpc_service_->UnstoppableDomainsGetWalletAddr(
      "test.crypto", token.Clone(), callback.Get());
  WaitAndVerify(&callback);

  // Address on Polygon network takes precedence over anything on ETH mainnet.
  EXPECT_CALL(callback, Run("polyaddr", mojom::ProviderError::kSuccess, ""));
  polygon_getmany_call_handler_->AddItem("test.crypto", "crypto.USDT.address",
                                         "polyaddr");
  json_rpc_service_->UnstoppableDomainsGetWalletAddr(
      "test.crypto", token.Clone(), callback.Get());
  WaitAndVerify(&callback);
}

TEST_F(UnstoppableDomainsUnitTest, ResolveDns_PolygonNetworkError) {
  base::MockCallback<ResolveDnsCallback> callback;
  EXPECT_CALL(callback,
              Run(std::optional<GURL>(), mojom::ProviderError::kInternalError,
                  l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)));
  SetEthTimeoutResponse();
  SetPolygonTimeoutResponse();
  json_rpc_service_->UnstoppableDomainsResolveDns("brave.crypto",
                                                  callback.Get());
  WaitAndVerify(&callback);

  EXPECT_CALL(callback,
              Run(std::optional<GURL>(), mojom::ProviderError::kInternalError,
                  l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)));
  SetEthRawResponse(DnsBraveResponse());
  SetPolygonTimeoutResponse();
  json_rpc_service_->UnstoppableDomainsResolveDns("brave.crypto",
                                                  callback.Get());
  WaitAndVerify(&callback);

  EXPECT_CALL(callback,
              Run(std::optional<GURL>(), mojom::ProviderError::kParsingError,
                  l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR)));
  SetEthRawResponse(DnsBraveResponse());
  SetPolygonRawResponse("Not a json");
  json_rpc_service_->UnstoppableDomainsResolveDns("brad.crypto",
                                                  callback.Get());
  WaitAndVerify(&callback);

  EXPECT_CALL(callback, Run(std::optional<GURL>(),
                            mojom::ProviderError::kLimitExceeded, "Error!"));
  SetEthRawResponse(DnsBraveResponse());
  SetPolygonRawResponse(MakeJsonRpcErrorResponse(-32005, "Error!"));
  json_rpc_service_->UnstoppableDomainsResolveDns("brave.crypto",
                                                  callback.Get());
  WaitAndVerify(&callback);
}

TEST_F(UnstoppableDomainsUnitTest, ResolveDns_PolygonResult) {
  base::MockCallback<ResolveDnsCallback> callback;
  EXPECT_CALL(callback, Run(std::optional<GURL>("https://brave.com"),
                            mojom::ProviderError::kSuccess, ""));
  SetEthTimeoutResponse();
  SetPolygonRawResponse(DnsBraveResponse());
  json_rpc_service_->UnstoppableDomainsResolveDns("brave.crypto",
                                                  callback.Get());
  WaitAndVerify(&callback);

  EXPECT_CALL(callback, Run(std::optional<GURL>("https://brave.com"),
                            mojom::ProviderError::kSuccess, ""));
  SetEthRawResponse(DnsIpfsResponse());
  SetPolygonRawResponse(DnsBraveResponse());
  json_rpc_service_->UnstoppableDomainsResolveDns("brave.crypto",
                                                  callback.Get());
  WaitAndVerify(&callback);

  EXPECT_CALL(callback, Run(std::optional<GURL>("https://brave.com"),
                            mojom::ProviderError::kSuccess, ""));
  SetEthRawResponse(DnsEmptyResponse());
  SetPolygonRawResponse(DnsBraveResponse());
  json_rpc_service_->UnstoppableDomainsResolveDns("brave.crypto",
                                                  callback.Get());
  WaitAndVerify(&callback);
}

TEST_F(UnstoppableDomainsUnitTest, ResolveDns_BaseResult) {
  base::MockCallback<ResolveDnsCallback> callback;
  EXPECT_CALL(callback, Run(std::optional<GURL>("https://brave.com"),
                            mojom::ProviderError::kSuccess, ""));
  SetEthTimeoutResponse();
  SetPolygonRawResponse(DnsEmptyResponse());
  SetBaseRawResponse(DnsBraveResponse());
  json_rpc_service_->UnstoppableDomainsResolveDns("brave.crypto",
                                                  callback.Get());
  WaitAndVerify(&callback);

  EXPECT_CALL(callback, Run(std::optional<GURL>("https://brave.com"),
                            mojom::ProviderError::kSuccess, ""));
  SetEthRawResponse(DnsIpfsResponse());
  SetPolygonRawResponse(DnsEmptyResponse());
  SetBaseRawResponse(DnsBraveResponse());
  json_rpc_service_->UnstoppableDomainsResolveDns("brave.crypto",
                                                  callback.Get());
  WaitAndVerify(&callback);

  EXPECT_CALL(callback, Run(std::optional<GURL>("https://brave.com"),
                            mojom::ProviderError::kSuccess, ""));
  SetEthRawResponse(DnsEmptyResponse());
  SetPolygonRawResponse(DnsEmptyResponse());
  SetBaseRawResponse(DnsBraveResponse());
  json_rpc_service_->UnstoppableDomainsResolveDns("brave.crypto",
                                                  callback.Get());
  WaitAndVerify(&callback);
}

TEST_F(UnstoppableDomainsUnitTest, ResolveDns_FallbackToEthMainnet) {
  base::MockCallback<ResolveDnsCallback> callback;
  EXPECT_CALL(
      callback,
      Run(std::optional<GURL>("https://ipfs.io/ipfs/"
                              "QmbWqxBEKC3P8tqsKc98xmWNzrzDtRLMiMPL8wBuTGsMnR"),
          mojom::ProviderError::kSuccess, ""));
  SetEthRawResponse(DnsIpfsResponse());
  SetPolygonRawResponse(DnsEmptyResponse());
  json_rpc_service_->UnstoppableDomainsResolveDns("brave.crypto",
                                                  callback.Get());
  WaitAndVerify(&callback);

  EXPECT_CALL(callback, Run(std::optional<GURL>("https://brave.com"),
                            mojom::ProviderError::kSuccess, ""));
  SetEthRawResponse(DnsBraveResponse());
  SetPolygonRawResponse(
      MakeJsonRpcStringArrayResponse({"", "", "", "", "", "invalid url"}));
  json_rpc_service_->UnstoppableDomainsResolveDns("brave.crypto",
                                                  callback.Get());
  WaitAndVerify(&callback);
}

TEST_F(UnstoppableDomainsUnitTest, ResolveDns_FallbackToEthMainnetError) {
  base::MockCallback<ResolveDnsCallback> callback;
  EXPECT_CALL(callback,
              Run(std::optional<GURL>(), mojom::ProviderError::kInternalError,
                  l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)));
  SetEthTimeoutResponse();
  SetPolygonRawResponse(DnsEmptyResponse());
  json_rpc_service_->UnstoppableDomainsResolveDns("brave.crypto",
                                                  callback.Get());
  WaitAndVerify(&callback);

  EXPECT_CALL(callback,
              Run(std::optional<GURL>(), mojom::ProviderError::kSuccess, ""));
  SetEthRawResponse(
      MakeJsonRpcStringArrayResponse({"", "", "", "", "", "invalid url"}));
  SetPolygonRawResponse(DnsEmptyResponse());
  json_rpc_service_->UnstoppableDomainsResolveDns("brave.crypto",
                                                  callback.Get());
  WaitAndVerify(&callback);
}

TEST_F(UnstoppableDomainsUnitTest, ResolveDns_InvalidDomain) {
  base::MockCallback<ResolveDnsCallback> callback;
  EXPECT_CALL(callback,
              Run(std::optional<GURL>(), mojom::ProviderError::kInvalidParams,
                  l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS)));
  json_rpc_service_->UnstoppableDomainsResolveDns("brave.test", callback.Get());
  EXPECT_EQ(0, url_loader_factory_.NumPending());
  task_environment_.RunUntilIdle();
}

TEST_F(UnstoppableDomainsUnitTest, ResolveDns_ManyCalls) {
  base::MockCallback<ResolveDnsCallback> callback1;
  EXPECT_CALL(callback1, Run(std::optional<GURL>("https://brave.com"),
                             mojom::ProviderError::kSuccess, ""));
  base::MockCallback<ResolveDnsCallback> callback2;
  EXPECT_CALL(callback2, Run(std::optional<GURL>("https://brave.com"),
                             mojom::ProviderError::kSuccess, ""));
  base::MockCallback<ResolveDnsCallback> callback3;
  EXPECT_CALL(
      callback3,
      Run(std::optional<GURL>("https://ipfs.io/ipfs/"
                              "QmbWqxBEKC3P8tqsKc98xmWNzrzDtRLMiMPL8wBuTGsMnR"),
          mojom::ProviderError::kSuccess, ""));

  ASSERT_EQ(6u, unstoppable_domains::kRecordKeys.size());
  // This will resolve brave.crypto requests.
  eth_mainnet_getmany_call_handler_->AddItem(
      "brave.crypto", unstoppable_domains::kRecordKeys[0],
      "QmbWqxBEKC3P8tqsKc98xmWNzrzDtRLMiMPL8wBuTGsMnR");
  eth_mainnet_getmany_call_handler_->AddItem(
      "brave.crypto", unstoppable_domains::kRecordKeys[5], "https://brave.com");
  polygon_getmany_call_handler_->AddItem(
      "brave.crypto", unstoppable_domains::kRecordKeys[5], "https://brave.com");

  // This will resolve brave.x requests.
  polygon_getmany_call_handler_->AddItem(
      "brave.x", unstoppable_domains::kRecordKeys[0],
      "QmbWqxBEKC3P8tqsKc98xmWNzrzDtRLMiMPL8wBuTGsMnR");
  polygon_getmany_call_handler_->AddItem(
      "brave.x", unstoppable_domains::kRecordKeys[5], "https://brave.com");
  eth_mainnet_getmany_call_handler_->AddItem(
      "brave.x", unstoppable_domains::kRecordKeys[5], "https://brave.com");

  EXPECT_EQ(0, eth_mainnet_getmany_call_handler_->calls_number());
  EXPECT_EQ(0, polygon_getmany_call_handler_->calls_number());
  json_rpc_service_->UnstoppableDomainsResolveDns("brave.crypto",
                                                  callback1.Get());
  json_rpc_service_->UnstoppableDomainsResolveDns("brave.crypto",
                                                  callback2.Get());
  task_environment_.RunUntilIdle();
  EXPECT_EQ(1, eth_mainnet_getmany_call_handler_->calls_number());
  EXPECT_EQ(1, polygon_getmany_call_handler_->calls_number());
  testing::Mock::VerifyAndClearExpectations(&callback1);
  testing::Mock::VerifyAndClearExpectations(&callback2);

  json_rpc_service_->UnstoppableDomainsResolveDns("brave.x", callback3.Get());
  task_environment_.RunUntilIdle();
  EXPECT_EQ(2, eth_mainnet_getmany_call_handler_->calls_number());
  EXPECT_EQ(2, polygon_getmany_call_handler_->calls_number());
  testing::Mock::VerifyAndClearExpectations(&callback3);
}

TEST_F(JsonRpcServiceUnitTest, GetBaseFeePerGas) {
  bool callback_called = false;
  GURL expected_network =
      GetNetwork(mojom::kLocalhostChainId, mojom::CoinType::ETH);
  // Successful path when the network is EIP1559
  SetIsEip1559Interceptor(expected_network, true);
  json_rpc_service_->GetBaseFeePerGas(
      mojom::kLocalhostChainId,
      base::BindOnce(&OnStringResponse, &callback_called,
                     mojom::ProviderError::kSuccess, "", "0x181f22e7a9"));
  task_environment_.RunUntilIdle();
  EXPECT_TRUE(callback_called);

  // Successful path when the network is not EIP1559
  callback_called = false;
  SetIsEip1559Interceptor(expected_network, false);
  json_rpc_service_->GetBaseFeePerGas(
      mojom::kLocalhostChainId,
      base::BindOnce(&OnStringResponse, &callback_called,
                     mojom::ProviderError::kSuccess, "", ""));
  task_environment_.RunUntilIdle();
  EXPECT_TRUE(callback_called);

  callback_called = false;
  SetHTTPRequestTimeoutInterceptor();
  json_rpc_service_->GetBaseFeePerGas(
      mojom::kLocalhostChainId,
      base::BindOnce(&OnStringResponse, &callback_called,
                     mojom::ProviderError::kInternalError,
                     l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR), ""));
  task_environment_.RunUntilIdle();
  EXPECT_TRUE(callback_called);

  callback_called = false;
  SetInvalidJsonInterceptor();
  json_rpc_service_->GetBaseFeePerGas(
      mojom::kLocalhostChainId,
      base::BindOnce(&OnStringResponse, &callback_called,
                     mojom::ProviderError::kParsingError,
                     l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR), ""));
  task_environment_.RunUntilIdle();
  EXPECT_TRUE(callback_called);

  callback_called = false;
  SetLimitExceededJsonErrorResponse();
  json_rpc_service_->GetBaseFeePerGas(
      mojom::kLocalhostChainId,
      base::BindOnce(&OnStringResponse, &callback_called,
                     mojom::ProviderError::kLimitExceeded,
                     "Request exceeds defined limit", ""));
  task_environment_.RunUntilIdle();
  EXPECT_TRUE(callback_called);
}

TEST_F(JsonRpcServiceUnitTest, UpdateIsEip1559LocalhostChain) {
  TestJsonRpcServiceObserver observer;
  json_rpc_service_->AddObserver(observer.GetReceiver());
  GURL expected_network =
      GetNetwork(mojom::kLocalhostChainId, mojom::CoinType::ETH);
  // Switching to localhost should update is_eip1559 to true when is_eip1559 is
  // true in the RPC response.
  EXPECT_FALSE(GetIsEip1559FromPrefs(mojom::kLocalhostChainId));
  SetIsEip1559Interceptor(expected_network, true);
  EXPECT_CALL(observer,
              ChainChangedEvent(mojom::kLocalhostChainId, mojom::CoinType::ETH,
                                testing::Eq(std::nullopt)))
      .Times(1);
  EXPECT_TRUE(
      SetNetwork(mojom::kLocalhostChainId, mojom::CoinType::ETH, std::nullopt));
  task_environment_.RunUntilIdle();
  EXPECT_TRUE(testing::Mock::VerifyAndClearExpectations(&observer));
  EXPECT_TRUE(GetIsEip1559FromPrefs(mojom::kLocalhostChainId));

  // Switching to localhost should update is_eip1559 to false when is_eip1559
  // is false in the RPC response.
  SetIsEip1559Interceptor(expected_network, false);
  EXPECT_CALL(observer,
              ChainChangedEvent(mojom::kLocalhostChainId, mojom::CoinType::ETH,
                                testing::Eq(std::nullopt)))
      .Times(1);
  EXPECT_TRUE(
      SetNetwork(mojom::kLocalhostChainId, mojom::CoinType::ETH, std::nullopt));
  task_environment_.RunUntilIdle();
  EXPECT_TRUE(testing::Mock::VerifyAndClearExpectations(&observer));
  EXPECT_FALSE(GetIsEip1559FromPrefs(mojom::kLocalhostChainId));

  // Switch to localhost again without changing is_eip1559 should not trigger
  // event.
  EXPECT_FALSE(GetIsEip1559FromPrefs(mojom::kLocalhostChainId));
  SetIsEip1559Interceptor(expected_network, false);
  EXPECT_CALL(observer,
              ChainChangedEvent(mojom::kLocalhostChainId, mojom::CoinType::ETH,
                                testing::Eq(std::nullopt)))
      .Times(1);
  EXPECT_TRUE(
      SetNetwork(mojom::kLocalhostChainId, mojom::CoinType::ETH, std::nullopt));
  task_environment_.RunUntilIdle();
  EXPECT_TRUE(testing::Mock::VerifyAndClearExpectations(&observer));
  EXPECT_FALSE(GetIsEip1559FromPrefs(mojom::kLocalhostChainId));

  // OnEip1559Changed will not be called if RPC fails.
  SetHTTPRequestTimeoutInterceptor();
  EXPECT_CALL(observer,
              ChainChangedEvent(mojom::kLocalhostChainId, mojom::CoinType::ETH,
                                testing::Eq(std::nullopt)))
      .Times(1);
  EXPECT_TRUE(
      SetNetwork(mojom::kLocalhostChainId, mojom::CoinType::ETH, std::nullopt));
  task_environment_.RunUntilIdle();
  EXPECT_TRUE(testing::Mock::VerifyAndClearExpectations(&observer));
  EXPECT_FALSE(GetIsEip1559FromPrefs(mojom::kLocalhostChainId));
}

TEST_F(JsonRpcServiceUnitTest, UpdateIsEip1559CustomChain) {
  std::vector<base::Value::Dict> values;
  mojom::NetworkInfo chain1 = GetTestNetworkInfo1();
  values.push_back(brave_wallet::NetworkInfoToValue(chain1));

  mojom::NetworkInfo chain2 = GetTestNetworkInfo2();
  values.push_back(brave_wallet::NetworkInfoToValue(chain2));
  UpdateCustomNetworks(prefs(), &values);
  network_manager_->SetEip1559ForCustomChain(chain2.chain_id, true);

  // Switch to chain1 should trigger is_eip1559 being updated to true when
  // is_eip1559 is true in the RPC response.
  TestJsonRpcServiceObserver observer;
  json_rpc_service_->AddObserver(observer.GetReceiver());

  EXPECT_FALSE(GetIsEip1559FromPrefs(chain1.chain_id));
  SetIsEip1559Interceptor(GetActiveEndpointUrl(chain1), true);
  EXPECT_CALL(observer, ChainChangedEvent(chain1.chain_id, mojom::CoinType::ETH,
                                          testing::Eq(std::nullopt)))
      .Times(1);
  EXPECT_TRUE(SetNetwork(chain1.chain_id, mojom::CoinType::ETH, std::nullopt));
  task_environment_.RunUntilIdle();
  EXPECT_TRUE(testing::Mock::VerifyAndClearExpectations(&observer));
  EXPECT_TRUE(GetIsEip1559FromPrefs(chain1.chain_id));

  // Switch to chain2 should trigger is_eip1559 being updated to false when
  // is_eip1559 is false in the RPC response.
  EXPECT_TRUE(GetIsEip1559FromPrefs(chain2.chain_id));
  SetIsEip1559Interceptor(GetActiveEndpointUrl(chain2), false);
  EXPECT_CALL(observer, ChainChangedEvent(chain2.chain_id, mojom::CoinType::ETH,
                                          testing::Eq(std::nullopt)))
      .Times(1);
  EXPECT_TRUE(SetNetwork(chain2.chain_id, mojom::CoinType::ETH, std::nullopt));
  task_environment_.RunUntilIdle();
  EXPECT_TRUE(testing::Mock::VerifyAndClearExpectations(&observer));
  EXPECT_FALSE(GetIsEip1559FromPrefs(chain2.chain_id));

  // Switch to chain2 again without changing is_eip1559 should not trigger
  // event.
  EXPECT_FALSE(GetIsEip1559FromPrefs(chain2.chain_id));
  SetIsEip1559Interceptor(GetActiveEndpointUrl(chain2), false);
  EXPECT_CALL(observer, ChainChangedEvent(chain2.chain_id, mojom::CoinType::ETH,
                                          testing::Eq(std::nullopt)))
      .Times(1);
  EXPECT_TRUE(SetNetwork(chain2.chain_id, mojom::CoinType::ETH, std::nullopt));
  task_environment_.RunUntilIdle();
  EXPECT_TRUE(testing::Mock::VerifyAndClearExpectations(&observer));
  EXPECT_FALSE(GetIsEip1559FromPrefs(chain2.chain_id));

  // OnEip1559Changed will not be called if RPC fails.
  SetHTTPRequestTimeoutInterceptor();
  EXPECT_CALL(observer, ChainChangedEvent(chain2.chain_id, mojom::CoinType::ETH,
                                          testing::Eq(std::nullopt)))
      .Times(1);
  EXPECT_TRUE(SetNetwork(chain2.chain_id, mojom::CoinType::ETH, std::nullopt));
  task_environment_.RunUntilIdle();
  EXPECT_TRUE(testing::Mock::VerifyAndClearExpectations(&observer));
  EXPECT_FALSE(GetIsEip1559FromPrefs(chain2.chain_id));
}

TEST_F(JsonRpcServiceUnitTest, GetWalletAddrInvalidDomain) {
  const std::vector<std::string> invalid_domains = {"", ".eth", "-brave.eth",
                                                    "brave-.eth", "b.eth"};

  for (const auto& domain : invalid_domains) {
    {
      base::MockCallback<JsonRpcService::EnsGetEthAddrCallback> callback;
      EXPECT_CALL(callback,
                  Run("", false, mojom::ProviderError::kInvalidParams,
                      l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS)));

      json_rpc_service_->EnsGetEthAddr(domain, callback.Get());
      task_environment_.RunUntilIdle();
    }

    {
      base::MockCallback<
          JsonRpcService::UnstoppableDomainsGetWalletAddrCallback>
          callback;
      EXPECT_CALL(callback,
                  Run("", mojom::ProviderError::kInvalidParams,
                      l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS)));

      json_rpc_service_->UnstoppableDomainsGetWalletAddr(
          domain, mojom::BlockchainToken::New(), callback.Get());
      task_environment_.RunUntilIdle();
    }
  }
}

TEST_F(JsonRpcServiceUnitTest, GetWalletAddrInvalidCoin) {
  base::MockCallback<JsonRpcService::UnstoppableDomainsGetWalletAddrCallback>
      callback;

  for (auto coin : {mojom::CoinType::BTC, mojom::CoinType::ZEC}) {
    auto token = mojom::BlockchainToken::New();
    token->coin = coin;
    EXPECT_CALL(callback, Run("", mojom::ProviderError::kSuccess, ""));
    json_rpc_service_->UnstoppableDomainsGetWalletAddr(
        "brave.crypto", token.Clone(), callback.Get());
    task_environment_.RunUntilIdle();
  }

  EXPECT_TRUE(AllCoinsTested());
}

TEST_F(JsonRpcServiceUnitTest, IsValidEnsDomain) {
  std::vector<std::string> valid_domains = {"brave.eth", "test.brave.eth",
                                            "brave-test.test-dev.eth"};
  for (const auto& domain : valid_domains) {
    EXPECT_TRUE(JsonRpcService::IsValidEnsDomain(domain))
        << domain << " should be valid";
  }

  std::vector<std::string> invalid_domains = {
      "",      ".eth",    "-brave.eth",      "brave-.eth",     "brave.e-th",
      "b.eth", "brave.e", "-brave.test.eth", "brave-.test.eth"};
  for (const auto& domain : invalid_domains) {
    EXPECT_FALSE(JsonRpcService::IsValidEnsDomain(domain))
        << domain << " should be invalid";
  }
}

TEST_F(JsonRpcServiceUnitTest, IsValidSnsDomain) {
  std::vector<std::string> valid_domains = {
      "brave.sol",                //
      "test.brave.sol",           //
      "brave-test.test-dev.sol",  //
      "b.sol",                    //
      "w.sol",                    //
      "-.sol",                    //
      "-brave.sol",               //
      "brave-.sol",               //
      "---.sol",                  //
      "-.-.sol",                  //
      "-brave.test.sol",          //
      "brave-.test.sol"           //
  };
  for (const auto& domain : valid_domains) {
    EXPECT_TRUE(JsonRpcService::IsValidSnsDomain(domain))
        << domain << " should be valid";
  }

  std::vector<std::string> invalid_domains = {
      "",            //
      "b.eth",       //
      ".sol",        //
      "brave.s-ol",  //
      "B.sol",       //
      "brave.s",     //
      "b.Sol"        //
  };
  for (const auto& domain : invalid_domains) {
    EXPECT_FALSE(JsonRpcService::IsValidSnsDomain(domain))
        << domain << " should be invalid";
  }
}

TEST_F(JsonRpcServiceUnitTest, IsValidUnstoppableDomain) {
  // clang-format off
  std::vector<std::string> valid_domains = {
      "test.crypto",
      "test.x",
      "test.nft",
      "test.dao",
      "test.wallet",
      "test.blockchain",
      "test.bitcoin",
      "brave.zil",
      "brave.altimist",
      "brave.anime",
      "brave.klever",
      "brave.manga",
      "brave.polygon",
      "brave.unstoppable",
      "brave.pudgy",
      "brave.tball",
      "brave.stepn",
      "brave.secret",
      "brave.raiin",
      "brave.pog",
      "brave.clay",
      "brave.metropolis",
      "brave.witg",
      "brave.ubu",
      "brave.kryptic",
      "brave.farms",
      "brave.dfz",
      "brave.kresus",
      "brave.binanceus",
      "brave.austin",
      "brave.bitget",
      "brave.wrkx",
      "brave.bald",
      "brave.benji",
      "brave.chomp",
      "brave.dream",
      "brave.ethermail",
      "brave.lfg",
      "brave.propykeys",
      "brave.smobler",
      "a.crypto",
      "1.crypto",
      "-.crypto",
  };
  std::vector<std::string> invalid_domains = {
      "",
      ".",
      "crypto.",
      "crypto.1",
      ".crypto",
      "crypto.brave",
      "brave.crypto-",
      "brave.test.crypto",
      "test.coin",
      "test.888",
  };
  // clang-format on
  for (const auto& domain : valid_domains) {
    EXPECT_TRUE(JsonRpcService::IsValidUnstoppableDomain(domain))
        << domain << " should be valid";
  }

  for (const auto& domain : invalid_domains) {
    EXPECT_FALSE(JsonRpcService::IsValidUnstoppableDomain(domain))
        << domain << " should be invalid";
  }
}

TEST_F(JsonRpcServiceUnitTest, GetERC721OwnerOf) {
  bool callback_called = false;

  json_rpc_service_->GetERC721OwnerOf(
      "", "0x1", mojom::kMainnetChainId,
      base::BindOnce(&OnStringResponse, &callback_called,
                     mojom::ProviderError::kInvalidParams,
                     l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS),
                     ""));
  task_environment_.RunUntilIdle();
  EXPECT_TRUE(callback_called);

  callback_called = false;
  json_rpc_service_->GetERC721OwnerOf(
      "0x06012c8cf97BEaD5deAe237070F9587f8E7A266d", "", mojom::kMainnetChainId,
      base::BindOnce(&OnStringResponse, &callback_called,
                     mojom::ProviderError::kInvalidParams,
                     l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS),
                     ""));
  task_environment_.RunUntilIdle();
  EXPECT_TRUE(callback_called);

  callback_called = false;
  json_rpc_service_->GetERC721OwnerOf(
      "0x06012c8cf97BEaD5deAe237070F9587f8E7A266d", "0x1", "",
      base::BindOnce(&OnStringResponse, &callback_called,
                     mojom::ProviderError::kInvalidParams,
                     l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS),
                     ""));
  task_environment_.RunUntilIdle();
  EXPECT_TRUE(callback_called);

  SetInterceptor(
      GetNetwork(mojom::kMainnetChainId, mojom::CoinType::ETH), "eth_call", "",
      "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":"
      "\"0x000000000000000000000000983110309620d911731ac0932219af0609"
      "1b6744\"}");

  callback_called = false;
  json_rpc_service_->GetERC721OwnerOf(
      "0x06012c8cf97BEaD5deAe237070F9587f8E7A266d", "0x1",
      mojom::kMainnetChainId,
      base::BindOnce(
          &OnStringResponse, &callback_called, mojom::ProviderError::kSuccess,
          "",
          "0x983110309620D911731Ac0932219af06091b6744"));  // checksum address
  task_environment_.RunUntilIdle();
  EXPECT_TRUE(callback_called);

  SetHTTPRequestTimeoutInterceptor();
  json_rpc_service_->GetERC721OwnerOf(
      "0x06012c8cf97BEaD5deAe237070F9587f8E7A266d", "0x1",
      mojom::kMainnetChainId,
      base::BindOnce(&OnStringResponse, &callback_called,
                     mojom::ProviderError::kInternalError,
                     l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR), ""));
  task_environment_.RunUntilIdle();
  EXPECT_TRUE(callback_called);

  SetInvalidJsonInterceptor();
  json_rpc_service_->GetERC721OwnerOf(
      "0x06012c8cf97BEaD5deAe237070F9587f8E7A266d", "0x1",
      mojom::kMainnetChainId,
      base::BindOnce(&OnStringResponse, &callback_called,
                     mojom::ProviderError::kParsingError,
                     l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR), ""));
  task_environment_.RunUntilIdle();
  EXPECT_TRUE(callback_called);

  SetLimitExceededJsonErrorResponse();
  json_rpc_service_->GetERC721OwnerOf(
      "0x06012c8cf97BEaD5deAe237070F9587f8E7A266d", "0x1",
      mojom::kMainnetChainId,
      base::BindOnce(&OnStringResponse, &callback_called,
                     mojom::ProviderError::kLimitExceeded,
                     "Request exceeds defined limit", ""));
  task_environment_.RunUntilIdle();
  EXPECT_TRUE(callback_called);
}

TEST_F(JsonRpcServiceUnitTest, GetERC721Metadata) {
  // Ensure GetERC721Metadata passes the correct interface ID to
  // GetEthTokenMetadata
  SetTokenMetadataInterceptor(kERC721MetadataInterfaceId,
                              mojom::kMainnetChainId,
                              R"({
                                  "jsonrpc":"2.0",
                                  "id":1,
                                  "result": "0x0000000000000000000000000000000000000000000000000000000000000001"
                              })",
                              R"({
                                  "jsonrpc":"2.0",
                                  "id":1,
                                  "result":"0x0000000000000000000000000000000000000000000000000000000000000020000000000000000000000000000000000000000000000000000000000000002468747470733a2f2f696e76697369626c65667269656e64732e696f2f6170692f3138313700000000000000000000000000000000000000000000000000000000"
                              })",
                              https_metadata_response);
  TestGetERC721Metadata("0x59468516a8259058bad1ca5f8f4bff190d30e066", "0x719",
                        mojom::kMainnetChainId, https_metadata_response,
                        mojom::ProviderError::kSuccess, "");
}

TEST_F(JsonRpcServiceUnitTest, GetERC1155Metadata) {
  // Ensure GetERC1155Metadata passes the correct interface ID to
  // GetEthTokenMetadata
  SetTokenMetadataInterceptor(kERC1155MetadataInterfaceId,
                              mojom::kMainnetChainId,
                              R"({
                                  "jsonrpc":"2.0",
                                  "id":1,
                                  "result": "0x0000000000000000000000000000000000000000000000000000000000000001"
                              })",
                              R"({
                                  "jsonrpc":"2.0",
                                  "id":1,
                                  "result":"0x0000000000000000000000000000000000000000000000000000000000000020000000000000000000000000000000000000000000000000000000000000002468747470733a2f2f696e76697369626c65667269656e64732e696f2f6170692f3138313700000000000000000000000000000000000000000000000000000000"
                              })",
                              https_metadata_response);
  TestGetERC1155Metadata("0x59468516a8259058bad1ca5f8f4bff190d30e066", "0x719",
                         mojom::kMainnetChainId, https_metadata_response,
                         mojom::ProviderError::kSuccess, "");
}

TEST_F(JsonRpcServiceUnitTest, GetERC721Balance) {
  bool callback_called = false;

  // Invalid inputs.
  json_rpc_service_->GetERC721TokenBalance(
      "", "0x1", "0x983110309620D911731Ac0932219af06091b6744",
      mojom::kMainnetChainId,
      base::BindOnce(&OnStringResponse, &callback_called,
                     mojom::ProviderError::kInvalidParams,
                     l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS),
                     ""));
  task_environment_.RunUntilIdle();
  EXPECT_TRUE(callback_called);

  callback_called = false;
  json_rpc_service_->GetERC721TokenBalance(
      "0x06012c8cf97BEaD5deAe237070F9587f8E7A266d", "",
      "0x983110309620D911731Ac0932219af06091b6744", mojom::kMainnetChainId,
      base::BindOnce(&OnStringResponse, &callback_called,
                     mojom::ProviderError::kInvalidParams,
                     l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS),
                     ""));
  task_environment_.RunUntilIdle();
  EXPECT_TRUE(callback_called);

  callback_called = false;
  json_rpc_service_->GetERC721TokenBalance(
      "0x06012c8cf97BEaD5deAe237070F9587f8E7A266d", "0x1", "",
      mojom::kMainnetChainId,
      base::BindOnce(&OnStringResponse, &callback_called,
                     mojom::ProviderError::kInvalidParams,
                     l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS),
                     ""));
  task_environment_.RunUntilIdle();
  EXPECT_TRUE(callback_called);

  callback_called = false;
  json_rpc_service_->GetERC721TokenBalance(
      "0x06012c8cf97BEaD5deAe237070F9587f8E7A266d", "0x1",
      "0x983110309620D911731Ac0932219af06091b6744", "",
      base::BindOnce(&OnStringResponse, &callback_called,
                     mojom::ProviderError::kInvalidParams,
                     l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS),
                     ""));
  task_environment_.RunUntilIdle();
  EXPECT_TRUE(callback_called);

  SetInterceptor(
      GetNetwork(mojom::kMainnetChainId, mojom::CoinType::ETH), "eth_call", "",
      "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":"
      "\"0x000000000000000000000000983110309620d911731ac0932219af0609"
      "1b6744\"}");

  // Owner gets balance 0x1.
  callback_called = false;
  json_rpc_service_->GetERC721TokenBalance(
      "0x06012c8cf97BEaD5deAe237070F9587f8E7A266d", "0x1",
      "0x983110309620D911731Ac0932219af06091b6744", mojom::kMainnetChainId,
      base::BindOnce(&OnStringResponse, &callback_called,
                     mojom::ProviderError::kSuccess, "", "0x1"));
  task_environment_.RunUntilIdle();
  EXPECT_TRUE(callback_called);

  // Non-checksum address can get the same balance.
  callback_called = false;
  json_rpc_service_->GetERC721TokenBalance(
      "0x06012c8cf97BEaD5deAe237070F9587f8E7A266d", "0x1",
      "0x983110309620d911731ac0932219af06091b6744", mojom::kMainnetChainId,
      base::BindOnce(&OnStringResponse, &callback_called,
                     mojom::ProviderError::kSuccess, "", "0x1"));
  task_environment_.RunUntilIdle();
  EXPECT_TRUE(callback_called);

  // Non-owner gets balance 0x0.
  callback_called = false;
  json_rpc_service_->GetERC721TokenBalance(
      "0x06012c8cf97BEaD5deAe237070F9587f8E7A266d", "0x1",
      "0x983110309620d911731ac0932219af06091b7811", mojom::kMainnetChainId,
      base::BindOnce(&OnStringResponse, &callback_called,
                     mojom::ProviderError::kSuccess, "", "0x0"));
  task_environment_.RunUntilIdle();
  EXPECT_TRUE(callback_called);

  SetHTTPRequestTimeoutInterceptor();
  json_rpc_service_->GetERC721TokenBalance(
      "0x06012c8cf97BEaD5deAe237070F9587f8E7A266d", "0x1",
      "0x983110309620d911731ac0932219af06091b6744", mojom::kMainnetChainId,
      base::BindOnce(&OnStringResponse, &callback_called,
                     mojom::ProviderError::kInternalError,
                     l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR), ""));
  task_environment_.RunUntilIdle();
  EXPECT_TRUE(callback_called);

  SetInvalidJsonInterceptor();
  json_rpc_service_->GetERC721TokenBalance(
      "0x06012c8cf97BEaD5deAe237070F9587f8E7A266d", "0x1",
      "0x983110309620d911731ac0932219af06091b6744", mojom::kMainnetChainId,
      base::BindOnce(&OnStringResponse, &callback_called,
                     mojom::ProviderError::kParsingError,
                     l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR), ""));
  task_environment_.RunUntilIdle();
  EXPECT_TRUE(callback_called);

  SetLimitExceededJsonErrorResponse();
  json_rpc_service_->GetERC721TokenBalance(
      "0x06012c8cf97BEaD5deAe237070F9587f8E7A266d", "0x1",
      "0x983110309620d911731ac0932219af06091b6744", mojom::kMainnetChainId,
      base::BindOnce(&OnStringResponse, &callback_called,
                     mojom::ProviderError::kLimitExceeded,
                     "Request exceeds defined limit", ""));
  task_environment_.RunUntilIdle();
  EXPECT_TRUE(callback_called);
}

TEST_F(JsonRpcServiceUnitTest, GetERC1155TokenBalance) {
  TestGetERC1155TokenBalance(
      "", "0x0", "0x16e4476c8fddc552e3b1c4b8b56261d85977fe52",
      mojom::kMainnetChainId, "", mojom::ProviderError::kInvalidParams,
      l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));

  TestGetERC1155TokenBalance(
      "0x28472a58a490c5e09a238847f66a68a47cc76f0f", "0x0", "",
      mojom::kMainnetChainId, "", mojom::ProviderError::kInvalidParams,
      l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));

  TestGetERC1155TokenBalance(
      "0x28472a58a490c5e09a238847f66a68a47cc76f0f",
      "0x16e4476c8fddc552e3b1c4b8b56261d85977fe52", "", mojom::kMainnetChainId,
      "", mojom::ProviderError::kInvalidParams,
      l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));

  TestGetERC1155TokenBalance(
      "0x28472a58a490c5e09a238847f66a68a47cc76f0f", "0x0",
      "0x16e4476c8fddc552e3b1c4b8b56261d85977fe52", "", "",
      mojom::ProviderError::kInvalidParams,
      l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));

  SetHTTPRequestTimeoutInterceptor();
  TestGetERC1155TokenBalance(
      "0x28472a58a490c5e09a238847f66a68a47cc76f0f", "0x0",
      "0x16e4476c8fddc552e3b1c4b8b56261d85977fe52", mojom::kMainnetChainId, "",
      mojom::ProviderError::kInternalError,
      l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));

  SetInvalidJsonInterceptor();
  TestGetERC1155TokenBalance(
      "0x28472a58a490c5e09a238847f66a68a47cc76f0f", "0x0",
      "0x16e4476c8fddc552e3b1c4b8b56261d85977fe52", mojom::kMainnetChainId, "",
      mojom::ProviderError::kParsingError,
      l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));

  SetLimitExceededJsonErrorResponse();
  TestGetERC1155TokenBalance(
      "0x28472a58a490c5e09a238847f66a68a47cc76f0f", "0x0",
      "0x16e4476c8fddc552e3b1c4b8b56261d85977fe52", mojom::kMainnetChainId, "",
      mojom::ProviderError::kLimitExceeded, "Request exceeds defined limit");
  SetInterceptor(
      GetNetwork(mojom::kMainnetChainId, mojom::CoinType::ETH), "eth_call", "",
      R"({"jsonrpc":"2.0","id":1,"result":"0x0000000000000000000000000000000000000000000000000000000000000001"})");

  TestGetERC1155TokenBalance(
      "0x28472a58a490c5e09a238847f66a68a47cc76f0f", "0xf",
      "0x16e4476c8fddc552e3b1c4b8b56261d85977fe52", mojom::kMainnetChainId,
      "0x0000000000000000000000000000000000000000000000000000000000000001",
      mojom::ProviderError::kSuccess, "");
}

TEST_F(JsonRpcServiceUnitTest, GetSupportsInterface) {
  // Successful, and does support the interface
  bool callback_called = false;
  SetInterceptor(GetNetwork(mojom::kMainnetChainId, mojom::CoinType::ETH),
                 "eth_call", "",
                 "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":"
                 "\"0x000000000000000000000000000000000000000000000000000000000"
                 "0000001\"}");
  json_rpc_service_->GetSupportsInterface(
      "0x06012c8cf97BEaD5deAe237070F9587f8E7A266d", "0x80ac58cd",
      mojom::kMainnetChainId,
      base::BindOnce(&OnBoolResponse, &callback_called,
                     mojom::ProviderError::kSuccess, "", true));
  task_environment_.RunUntilIdle();
  EXPECT_TRUE(callback_called);

  // Successful, but does not support the interface
  callback_called = false;
  SetInterceptor(GetNetwork(mojom::kMainnetChainId, mojom::CoinType::ETH),
                 "eth_call", "",
                 "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":"
                 "\"0x000000000000000000000000000000000000000000000000000000000"
                 "0000000\"}");
  json_rpc_service_->GetSupportsInterface(
      "0x06012c8cf97BEaD5deAe237070F9587f8E7A266d", "0x80ac58cd",
      mojom::kMainnetChainId,
      base::BindOnce(&OnBoolResponse, &callback_called,
                     mojom::ProviderError::kSuccess, "", false));
  task_environment_.RunUntilIdle();
  EXPECT_TRUE(callback_called);

  // Invalid result, should be in hex form
  callback_called = false;
  SetInterceptor(GetNetwork(mojom::kMainnetChainId, mojom::CoinType::ETH),
                 "eth_call", "",
                 "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":\"0\"}");
  json_rpc_service_->GetSupportsInterface(
      "0x06012c8cf97BEaD5deAe237070F9587f8E7A266d", "0x80ac58cd",
      mojom::kMainnetChainId,
      base::BindOnce(&OnBoolResponse, &callback_called,
                     mojom::ProviderError::kParsingError,
                     l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR),
                     false));
  task_environment_.RunUntilIdle();
  EXPECT_TRUE(callback_called);

  callback_called = false;
  SetHTTPRequestTimeoutInterceptor();
  json_rpc_service_->GetSupportsInterface(
      "0x06012c8cf97BEaD5deAe237070F9587f8E7A266d", "0x80ac58cd",
      mojom::kMainnetChainId,
      base::BindOnce(&OnBoolResponse, &callback_called,
                     mojom::ProviderError::kInternalError,
                     l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR),
                     false));
  task_environment_.RunUntilIdle();
  EXPECT_TRUE(callback_called);

  callback_called = false;
  SetInvalidJsonInterceptor();
  json_rpc_service_->GetSupportsInterface(
      "0x06012c8cf97BEaD5deAe237070F9587f8E7A266d", "0x80ac58cd",
      mojom::kMainnetChainId,
      base::BindOnce(&OnBoolResponse, &callback_called,
                     mojom::ProviderError::kParsingError,
                     l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR),
                     false));
  task_environment_.RunUntilIdle();
  EXPECT_TRUE(callback_called);

  callback_called = false;
  SetLimitExceededJsonErrorResponse();
  json_rpc_service_->GetSupportsInterface(
      "0x06012c8cf97BEaD5deAe237070F9587f8E7A266d", "0x80ac58cd",
      mojom::kMainnetChainId,
      base::BindOnce(&OnBoolResponse, &callback_called,
                     mojom::ProviderError::kLimitExceeded,
                     "Request exceeds defined limit", false));
  task_environment_.RunUntilIdle();
  EXPECT_TRUE(callback_called);
}

TEST_F(JsonRpcServiceUnitTest, Reset) {
  std::vector<base::Value::Dict> values;
  mojom::NetworkInfo chain = GetTestNetworkInfo1("0x1");
  values.push_back(brave_wallet::NetworkInfoToValue(chain));
  UpdateCustomNetworks(prefs(), &values);

  ASSERT_FALSE(GetAllEthCustomChains().empty());
  EXPECT_TRUE(
      SetNetwork(mojom::kLocalhostChainId, mojom::CoinType::ETH, std::nullopt));
  network_manager_->SetEip1559ForCustomChain("0x1", true);
  EXPECT_TRUE(prefs()->HasPrefPath(kBraveWalletEip1559CustomChains));
  EXPECT_TRUE(prefs()->HasPrefPath(kBraveWalletCustomNetworks));
  EXPECT_EQ(
      network_manager_->GetCurrentChainId(mojom::CoinType::ETH, std::nullopt),
      mojom::kLocalhostChainId);

  auto origin = url::Origin::Create(GURL("https://brave.com"));
  json_rpc_service_->AddEthereumChainForOrigin(
      GetTestNetworkInfo1("0x123").Clone(), origin);
  json_rpc_service_->AddSwitchEthereumChainRequest(
      "0x1", origin, base::DoNothing(), base::Value());

  EXPECT_FALSE(json_rpc_service_->add_chain_pending_requests_.empty());
  EXPECT_FALSE(json_rpc_service_->pending_switch_chain_requests_.empty());

  json_rpc_service_->Reset();

  ASSERT_TRUE(GetAllEthCustomChains().empty());
  EXPECT_FALSE(prefs()->HasPrefPath(kBraveWalletCustomNetworks));
  EXPECT_EQ(
      network_manager_->GetCurrentChainId(mojom::CoinType::ETH, std::nullopt),
      mojom::kMainnetChainId);
  EXPECT_FALSE(prefs()->HasPrefPath(kBraveWalletEip1559CustomChains));
  EXPECT_TRUE(json_rpc_service_->add_chain_pending_requests_.empty());
  EXPECT_TRUE(json_rpc_service_->pending_switch_chain_requests_.empty());
}

TEST_F(JsonRpcServiceUnitTest, GetSolanaBalance) {
  auto expected_network =
      GetNetwork(mojom::kSolanaMainnet, mojom::CoinType::SOL);
  SetInterceptor(expected_network, "getBalance", "",
                 R"({"jsonrpc":"2.0","id":1,"result":{
                      "context":{"slot":106921266},"value":18446744073709551615}})");
  TestGetSolanaBalance(UINT64_MAX, mojom::SolanaProviderError::kSuccess, "");

  // Response parsing error
  SetInterceptor(expected_network, "getBalance", "",
                 R"({"jsonrpc":"2.0","id":1,"result":"0"})");
  TestGetSolanaBalance(0u, mojom::SolanaProviderError::kParsingError,
                       l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));

  // JSON RPC error
  SetInterceptor(expected_network, "getBalance", "",
                 R"({"jsonrpc":"2.0","id":1,"error":{
                      "code":-32601, "message": "method does not exist"}})");
  TestGetSolanaBalance(0u, mojom::SolanaProviderError::kMethodNotFound,
                       "method does not exist");

  // HTTP error
  SetHTTPRequestTimeoutInterceptor();
  TestGetSolanaBalance(0u, mojom::SolanaProviderError::kInternalError,
                       l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
}

TEST_F(JsonRpcServiceUnitTest, GetSPLTokenAccountBalance) {
  auto expected_network =
      GetNetwork(mojom::kSolanaMainnet, mojom::CoinType::SOL);

  std::string account_info_rsp = R"(
    {
      "jsonrpc":"2.0","id":1,
      "result": {
        "context":{"slot":123065869},
        "value":{
          "data":["SEVMTE8gV09STEQ=","base64"],
          "executable":false,
          "lamports":18446744073709551615,
          "owner":"$1",
          "rentEpoch":18446744073709551615
        }
      }
    }
  )";

  std::string balance_rsp = R"(
    {
      "jsonrpc":"2.0", "id":1,
      "result":{
        "context":{"slot":1069},
        "value":{
          "amount":"9864",
          "decimals":2,
          "uiAmount":98.64,
          "uiAmountString":"98.64"
        }
      }
    })";

  std::map<std::string, std::string> mock_rsp = {
      {"getAccountInfo",
       base::ReplaceStringPlaceholders(
           account_info_rsp, {mojom::kSolanaSystemProgramId}, nullptr)},
      {"getTokenAccountBalance", balance_rsp}};
  SetInterceptor(expected_network, mock_rsp);
  TestGetSPLTokenAccountBalance(
      "", 0u, "", mojom::SolanaProviderError::kInternalError,
      l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));

  mock_rsp["getAccountInfo"] = base::ReplaceStringPlaceholders(
      account_info_rsp, {mojom::kSolanaTokenProgramId}, nullptr);
  SetInterceptor(expected_network, mock_rsp);
  TestGetSPLTokenAccountBalance("9864", 2u, "98.64",
                                mojom::SolanaProviderError::kSuccess, "");

  mock_rsp["getAccountInfo"] = base::ReplaceStringPlaceholders(
      account_info_rsp, {mojom::kSolanaToken2022ProgramId}, nullptr);
  SetInterceptor(expected_network, mock_rsp);
  TestGetSPLTokenAccountBalance("9864", 2u, "98.64",
                                mojom::SolanaProviderError::kSuccess, "");

  // Treat non-existed account as 0 balance.
  mock_rsp["getTokenAccountBalance"] =
      R"({
            "jsonrpc": "2.0",
            "id": 1,
            "error":{
              "code": -32602,
              "message": "Invalid param: could not find account"
            }
          })";
  SetInterceptor(expected_network, mock_rsp);
  TestGetSPLTokenAccountBalance("0", 0u, "0",
                                mojom::SolanaProviderError::kSuccess, "");

  // Response parsing error
  mock_rsp["getTokenAccountBalance"] =
      R"({"jsonrpc":"2.0","id":1,"result":"0"})";
  SetInterceptor(expected_network, mock_rsp);
  TestGetSPLTokenAccountBalance(
      "", 0u, "", mojom::SolanaProviderError::kParsingError,
      l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));

  // JSON RPC error
  mock_rsp["getTokenAccountBalance"] =
      R"({
            "jsonrpc": "2.0",
            "id": 1,
            "error": {
              "code": -32601,
              "message": "method does not exist"
            }
          })";
  SetInterceptor(expected_network, mock_rsp);
  TestGetSPLTokenAccountBalance("", 0u, "",
                                mojom::SolanaProviderError::kMethodNotFound,
                                "method does not exist");

  // HTTP error
  SetHTTPRequestTimeoutInterceptor();
  TestGetSPLTokenAccountBalance(
      "", 0u, "", mojom::SolanaProviderError::kInternalError,
      l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
}

TEST_F(JsonRpcServiceUnitTest, IsSolanaBlockhashValid) {
  auto expected_network =
      GetNetwork(mojom::kSolanaMainnet, mojom::CoinType::SOL);
  SetInterceptor(expected_network, "isBlockhashValid", "",
                 R"({"jsonrpc":"2.0","id":1,"result":{
                      "context":{"slot":2483},"value":true}})");
  TestIsSolanaBlockhashValid(true, mojom::SolanaProviderError::kSuccess, "");

  SetInterceptor(expected_network, "isBlockhashValid", "",
                 R"({"jsonrpc":"2.0","id":1,"result":{
                      "context":{"slot":2483},"value":false}})");
  TestIsSolanaBlockhashValid(false, mojom::SolanaProviderError::kSuccess, "");

  // Response parsing error
  SetInterceptor(expected_network, "isBlockhashValid", "",
                 R"({"jsonrpc":"2.0","id":1,"result":"0"})");
  TestIsSolanaBlockhashValid(
      false, mojom::SolanaProviderError::kParsingError,
      l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));

  // JSON RPC error
  SetInterceptor(expected_network, "isBlockhashValid", "",
                 R"({"jsonrpc":"2.0","id":1,"error":{
                      "code":-32601, "message": "method does not exist"}})");
  TestIsSolanaBlockhashValid(false, mojom::SolanaProviderError::kMethodNotFound,
                             "method does not exist");

  // HTTP error
  SetHTTPRequestTimeoutInterceptor();
  TestIsSolanaBlockhashValid(
      false, mojom::SolanaProviderError::kInternalError,
      l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
}

TEST_F(JsonRpcServiceUnitTest, SendSolanaTransaction) {
  TestSendSolanaTransaction(
      mojom::kLocalhostChainId, "", mojom::SolanaProviderError::kInvalidParams,
      l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS),
      "" /* signed_tx */);

  auto expected_network_url =
      GetNetwork(mojom::kLocalhostChainId, mojom::CoinType::SOL);
  SetInterceptor(
      expected_network_url, "sendTransaction", "",
      "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":"
      "\"2id3YC2jK9G5Wo2phDx4gJVAew8DcY5NAojnVuao8rkxwPYPe8cSwE5GzhEgJA2y8fVjDE"
      "o6iR6ykBvDxrTQrtpb\"}");

  TestSendSolanaTransaction(
      mojom::kLocalhostChainId,
      "2id3YC2jK9G5Wo2phDx4gJVAew8DcY5NAojnVuao8rkxwPYPe8cSwE5GzhEgJA2y8fVjDEo6"
      "iR6ykBvDxrTQrtpb",
      mojom::SolanaProviderError::kSuccess, "");

  // Response parsing error
  SetInterceptor(expected_network_url, "sendTransaction", "",
                 "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":0}");
  TestSendSolanaTransaction(mojom::kLocalhostChainId, "",
                            mojom::SolanaProviderError::kParsingError,
                            l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));

  // JSON RPC error
  SetInterceptor(expected_network_url, "sendTransaction", "",
                 "{\"jsonrpc\":\"2.0\",\"id\":1,\"error\":"
                 "{\"code\":-32601, \"message\": \"method does not exist\"}}");
  TestSendSolanaTransaction(mojom::kLocalhostChainId, "",
                            mojom::SolanaProviderError::kMethodNotFound,
                            "method does not exist");

  // HTTP error
  SetHTTPRequestTimeoutInterceptor();
  TestSendSolanaTransaction(
      mojom::kLocalhostChainId, "", mojom::SolanaProviderError::kInternalError,
      l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
}

TEST_F(JsonRpcServiceUnitTest, GetSolanaLatestBlockhash) {
  auto expected_network_url =
      GetNetwork(mojom::kLocalhostChainId, mojom::CoinType::SOL);
  SetInterceptor(expected_network_url, "getLatestBlockhash", "",
                 "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":"
                 "{\"context\":{\"slot\":1069},\"value\":{\"blockhash\":"
                 "\"EkSnNWid2cvwEVnVx9aBqawnmiCNiDgp3gUdkDPTKN1N\", "
                 "\"lastValidBlockHeight\":18446744073709551615}}}");

  TestGetSolanaLatestBlockhash(
      mojom::kLocalhostChainId, "EkSnNWid2cvwEVnVx9aBqawnmiCNiDgp3gUdkDPTKN1N",
      UINT64_MAX, mojom::SolanaProviderError::kSuccess, "");

  // Response parsing error
  SetInterceptor(expected_network_url, "getLatestBlockhash", "",
                 "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":\"0\"}");
  TestGetSolanaLatestBlockhash(
      mojom::kLocalhostChainId, "", 0,
      mojom::SolanaProviderError::kParsingError,
      l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));

  // JSON RPC error
  SetInterceptor(expected_network_url, "getLatestBlockhash", "",
                 "{\"jsonrpc\":\"2.0\",\"id\":1,\"error\":"
                 "{\"code\":-32601, \"message\": \"method does not exist\"}}");
  TestGetSolanaLatestBlockhash(mojom::kLocalhostChainId, "", 0,
                               mojom::SolanaProviderError::kMethodNotFound,
                               "method does not exist");

  // HTTP error
  SetHTTPRequestTimeoutInterceptor();
  TestGetSolanaLatestBlockhash(
      mojom::kLocalhostChainId, "", 0,
      mojom::SolanaProviderError::kInternalError,
      l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
}

TEST_F(JsonRpcServiceUnitTest, GetSolanaSignatureStatuses) {
  std::string json = R"(
      {"jsonrpc":2.0, "id":1, "result":
        {
          "context": {"slot": 82},
          "value": [
            {
              "slot": 18446744073709551615,
              "confirmations": 10,
              "err": null,
              "confirmationStatus": "confirmed"
            },
            {
              "slot": 72,
              "confirmations": 18446744073709551615,
              "err": null,
              "confirmationStatus": "confirmed"
            },
            {
              "slot": 1092,
              "confirmations": null,
              "err": {"InstructionError":[0,{"Custom":1}]},
              "confirmationStatus": "finalized"
            },
            null
          ]
        }
      }
  )";
  auto expected_network_url =
      GetNetwork(mojom::kLocalhostChainId, mojom::CoinType::SOL);
  SetInterceptor(expected_network_url, "getSignatureStatuses", "", json);

  std::vector<std::string> tx_sigs = {
      "5VERv8NMvzbJMEkV8xnrLkEaWRtSz9CosKDYjCJjBRnbJLgp8uirBgmQpjKhoR4tjF3ZpRzr"
      "FmBV6UjKdiSZkQUW",
      "5j7s6NiJS3JAkvgkoc18WVAsiSaci2pxB2A6ueCJP4tprA2TFg9wSyTLeYouxPBJEMzJinEN"
      "TkpA52YStRW5Dia7",
      "4VERv8NMvzbJMEkV8xnrLkEaWRtSz9CosKDYjCJjBRnbJLgp8uirBgmQpjKhoR4tjF3ZpRzr"
      "FmBV6UjKdiSZkQUW",
      "45j7s6NiJS3JAkvgkoc18WVAsiSaci2pxB2A6ueCJP4tprA2TFg9wSyTLeYouxPBJEMzJinE"
      "NTkpA52YStRW5Dia7"};

  std::vector<std::optional<SolanaSignatureStatus>> expected_statuses(
      {SolanaSignatureStatus(UINT64_MAX, 10u, "", "confirmed"),
       SolanaSignatureStatus(72u, UINT64_MAX, "", "confirmed"),
       SolanaSignatureStatus(
           1092u, 0u, R"({"InstructionError":[0,{"Custom":1}]})", "finalized"),
       std::nullopt});
  TestGetSolanaSignatureStatuses(mojom::kLocalhostChainId, tx_sigs,
                                 expected_statuses,
                                 mojom::SolanaProviderError::kSuccess, "");

  // Response parsing error
  SetInterceptor(expected_network_url, "getSignatureStatuses", "",
                 R"({"jsonrpc":"2.0","id":1,"result":"0"})");
  TestGetSolanaSignatureStatuses(
      mojom::kLocalhostChainId, tx_sigs,
      std::vector<std::optional<SolanaSignatureStatus>>(),
      mojom::SolanaProviderError::kParsingError,
      l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));

  // JSON RPC error
  SetInterceptor(expected_network_url, "getSignatureStatuses", "",
                 R"({"jsonrpc":"2.0","id":1,"error":{
                      "code":-32601, "message": "method does not exist"}})");
  TestGetSolanaSignatureStatuses(
      mojom::kLocalhostChainId, tx_sigs,
      std::vector<std::optional<SolanaSignatureStatus>>(),
      mojom::SolanaProviderError::kMethodNotFound, "method does not exist");

  // HTTP error
  SetHTTPRequestTimeoutInterceptor();
  TestGetSolanaSignatureStatuses(
      mojom::kLocalhostChainId, tx_sigs,
      std::vector<std::optional<SolanaSignatureStatus>>(),
      mojom::SolanaProviderError::kInternalError,
      l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
}

TEST_F(JsonRpcServiceUnitTest, GetSolanaAccountInfo) {
  std::string json = R"(
    {
      "jsonrpc":"2.0","id":1,
      "result": {
        "context":{"slot":123065869},
        "value":{
          "data":["SEVMTE8gV09STEQ=","base64"],
          "executable":false,
          "lamports":18446744073709551615,
          "owner":"11111111111111111111111111111111",
          "rentEpoch":18446744073709551615
        }
      }
    }
  )";
  auto expected_network_url =
      GetNetwork(mojom::kLocalhostChainId, mojom::CoinType::SOL);

  SetInterceptor(expected_network_url, "getAccountInfo", "", json);

  SolanaAccountInfo expected_info;
  expected_info.lamports = UINT64_MAX;
  expected_info.owner = "11111111111111111111111111111111";
  expected_info.data = "SEVMTE8gV09STEQ=";
  expected_info.executable = false;
  expected_info.rent_epoch = UINT64_MAX;
  TestGetSolanaAccountInfo(mojom::kLocalhostChainId, expected_info,
                           mojom::SolanaProviderError::kSuccess, "");

  // value can be null for an account not on chain.
  SetInterceptor(
      expected_network_url, "getAccountInfo", "",
      R"({"jsonrpc":"2.0","result":{"context":{"slot":123121238},"value":null},"id":1})");
  TestGetSolanaAccountInfo(mojom::kLocalhostChainId, std::nullopt,
                           mojom::SolanaProviderError::kSuccess, "");

  // Response parsing error
  SetInterceptor(expected_network_url, "getAccountInfo", "",
                 R"({"jsonrpc":"2.0","id":1,"result":"0"})");
  TestGetSolanaAccountInfo(mojom::kLocalhostChainId, std::nullopt,
                           mojom::SolanaProviderError::kParsingError,
                           l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));

  // JSON RPC error
  SetInterceptor(expected_network_url, "getAccountInfo", "",
                 R"({"jsonrpc":"2.0","id":1,"error":{
                      "code":-32601, "message": "method does not exist"}})");
  TestGetSolanaAccountInfo(mojom::kLocalhostChainId, std::nullopt,
                           mojom::SolanaProviderError::kMethodNotFound,
                           "method does not exist");

  // HTTP error
  SetHTTPRequestTimeoutInterceptor();
  TestGetSolanaAccountInfo(mojom::kLocalhostChainId, std::nullopt,
                           mojom::SolanaProviderError::kInternalError,
                           l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
}

TEST_F(JsonRpcServiceUnitTest, GetSolanaFeeForMessage) {
  std::string json = R"(
    {
      "jsonrpc":"2.0","id":1,
      "result": {
        "context":{"slot":123065869},
        "value": 18446744073709551615
      }
    }
  )";

  auto expected_network_url =
      GetNetwork(mojom::kLocalhostChainId, mojom::CoinType::SOL);
  SetInterceptor(expected_network_url, "getFeeForMessage", "", json);
  std::string base64_encoded_string = base::Base64Encode("test");

  TestGetSolanaFeeForMessage(mojom::kLocalhostChainId, base64_encoded_string,
                             UINT64_MAX, mojom::SolanaProviderError::kSuccess,
                             "");
  std::string base58_encoded_string = "JvSKSz9YHfqEQ8j";
  // Message has to be base64 encoded string and non-empty.
  TestGetSolanaFeeForMessage(
      mojom::kLocalhostChainId, "", 0,
      mojom::SolanaProviderError::kInvalidParams,
      l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
  TestGetSolanaFeeForMessage(
      mojom::kLocalhostChainId, base58_encoded_string, 0,
      mojom::SolanaProviderError::kInvalidParams,
      l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));

  // value can be null for an account not on chain.
  SetInterceptor(expected_network_url, "getFeeForMessage", "",
                 R"({
                      "jsonrpc":"2.0",
                      "result":{
                      "context":{"slot":123121238},"value":null},"id":1
                    })");
  TestGetSolanaFeeForMessage(mojom::kLocalhostChainId, base64_encoded_string, 0,
                             mojom::SolanaProviderError::kSuccess, "");

  // Response parsing error
  SetInterceptor(expected_network_url, "getFeeForMessage", "",
                 R"({"jsonrpc":"2.0","id":1,"result":"0"})");
  TestGetSolanaFeeForMessage(
      mojom::kLocalhostChainId, base64_encoded_string, 0,
      mojom::SolanaProviderError::kParsingError,
      l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));

  // JSON RPC error
  SetInterceptor(expected_network_url, "getFeeForMessage", "",
                 R"({
                      "jsonrpc":"2.0","id":1,
                      "error":
                        {"code":-32601, "message": "method does not exist"}
                    })");
  TestGetSolanaFeeForMessage(mojom::kLocalhostChainId, base64_encoded_string, 0,
                             mojom::SolanaProviderError::kMethodNotFound,
                             "method does not exist");

  // HTTP error
  SetHTTPRequestTimeoutInterceptor();
  TestGetSolanaFeeForMessage(
      mojom::kLocalhostChainId, base64_encoded_string, 0,
      mojom::SolanaProviderError::kInternalError,
      l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
}

TEST_F(JsonRpcServiceUnitTest, GetEthTransactionCount) {
  bool callback_called = false;
  SetInterceptor(GetNetwork(mojom::kLocalhostChainId, mojom::CoinType::ETH),
                 "eth_getTransactionCount", "",
                 "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":\"0x1\"}");

  json_rpc_service_->GetEthTransactionCount(
      mojom::kLocalhostChainId, "0x4e02f254184E904300e0775E4b8eeCB1",
      base::BindOnce(&OnEthUint256Response, &callback_called,
                     mojom::ProviderError::kSuccess, "", 1));
  task_environment_.RunUntilIdle();
  EXPECT_TRUE(callback_called);

  callback_called = false;
  SetHTTPRequestTimeoutInterceptor();
  json_rpc_service_->GetEthTransactionCount(
      mojom::kLocalhostChainId, "0x4e02f254184E904300e0775E4b8eeCB1",
      base::BindOnce(&OnEthUint256Response, &callback_called,
                     mojom::ProviderError::kInternalError,
                     l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR), 0));
  task_environment_.RunUntilIdle();
  EXPECT_TRUE(callback_called);

  callback_called = false;
  SetInvalidJsonInterceptor();
  json_rpc_service_->GetEthTransactionCount(
      mojom::kLocalhostChainId, "0x4e02f254184E904300e0775E4b8eeCB1",
      base::BindOnce(&OnEthUint256Response, &callback_called,
                     mojom::ProviderError::kParsingError,
                     l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR), 0));
  task_environment_.RunUntilIdle();
  EXPECT_TRUE(callback_called);

  callback_called = false;
  SetLimitExceededJsonErrorResponse();
  json_rpc_service_->GetEthTransactionCount(
      mojom::kLocalhostChainId, "0x4e02f254184E904300e0775E4b8eeCB1",
      base::BindOnce(&OnEthUint256Response, &callback_called,
                     mojom::ProviderError::kLimitExceeded,
                     "Request exceeds defined limit", 0));
  task_environment_.RunUntilIdle();
  EXPECT_TRUE(callback_called);
}

TEST_F(JsonRpcServiceUnitTest, GetFilTransactionCount) {
  bool callback_called = false;
  SetInterceptor(GetNetwork(mojom::kLocalhostChainId, mojom::CoinType::FIL),
                 "Filecoin.MpoolGetNonce", "",
                 R"({"jsonrpc":"2.0","id":1,"result":18446744073709551615})");

  json_rpc_service_->GetFilTransactionCount(
      mojom::kLocalhostChainId, "t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q",
      base::BindOnce(&OnFilUint256Response, &callback_called,
                     mojom::FilecoinProviderError::kSuccess, "", UINT64_MAX));
  task_environment_.RunUntilIdle();
  EXPECT_TRUE(callback_called);

  callback_called = false;
  SetHTTPRequestTimeoutInterceptor();
  json_rpc_service_->GetFilTransactionCount(
      mojom::kLocalhostChainId, "t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q",
      base::BindOnce(&OnFilUint256Response, &callback_called,
                     mojom::FilecoinProviderError::kInternalError,
                     l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR), 0));
  task_environment_.RunUntilIdle();
  EXPECT_TRUE(callback_called);

  callback_called = false;
  SetInterceptor(GetNetwork(mojom::kLocalhostChainId, mojom::CoinType::FIL),
                 "Filecoin.MpoolGetNonce", "", R"({"jsonrpc":"2.0","id":1})");
  json_rpc_service_->GetFilTransactionCount(
      mojom::kLocalhostChainId, "t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q",
      base::BindOnce(&OnFilUint256Response, &callback_called,
                     mojom::FilecoinProviderError::kParsingError,
                     l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR), 0));
  task_environment_.RunUntilIdle();
  EXPECT_TRUE(callback_called);

  callback_called = false;
  SetFilecoinActorErrorJsonErrorResponse();
  json_rpc_service_->GetFilTransactionCount(
      mojom::kLocalhostChainId, "t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q",
      base::BindOnce(&OnFilUint256Response, &callback_called,
                     mojom::FilecoinProviderError::kActorNotFound,
                     "resolution lookup failed", 0));
  task_environment_.RunUntilIdle();
  EXPECT_TRUE(callback_called);
}

TEST_F(JsonRpcServiceUnitTest, GetSolanaBlockHeight) {
  auto expected_network_url =
      GetNetwork(mojom::kLocalhostChainId, mojom::CoinType::SOL);
  SetInterceptor(expected_network_url, "getBlockHeight", "",
                 R"({"jsonrpc":"2.0", "id":1, "result":18446744073709551615})");

  TestGetSolanaBlockHeight(mojom::kLocalhostChainId, UINT64_MAX,
                           mojom::SolanaProviderError::kSuccess, "");

  // Response parsing error
  SetInterceptor(expected_network_url, "getBlockHeight", "",
                 R"({"jsonrpc":"2.0","id":1})");
  TestGetSolanaBlockHeight(mojom::kLocalhostChainId, 0,
                           mojom::SolanaProviderError::kParsingError,
                           l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));

  // JSON RPC error
  SetInterceptor(expected_network_url, "getBlockHeight", "",
                 R"({"jsonrpc": "2.0", "id": 1,
                     "error": {
                       "code":-32601,
                       "message":"method does not exist"
                     }
                    })");
  TestGetSolanaBlockHeight(mojom::kLocalhostChainId, 0,
                           mojom::SolanaProviderError::kMethodNotFound,
                           "method does not exist");

  // HTTP error
  SetHTTPRequestTimeoutInterceptor();
  TestGetSolanaBlockHeight(mojom::kLocalhostChainId, 0,
                           mojom::SolanaProviderError::kInternalError,
                           l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
}

TEST_F(JsonRpcServiceUnitTest, GetSolanaTokenAccountsByOwner) {
  auto expected_network_url =
      GetNetwork(mojom::kSolanaMainnet, mojom::CoinType::SOL);

  std::string token_accounts = R"({
    "jsonrpc": "2.0",
    "result": {
      "context": {
        "apiVersion": "1.13.5",
        "slot": 166895942
      },
      "value": [
        {
          "account": {
            "data": [
              "z6cxAUoRHIupvmezOL4EAsTLlwKTgwxzCg/xcNWSEu42kEWUG3BArj8SJRSnd1faFt2Tm0Ey/qtGnPdOOlQlugEAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA",
              "base64"
            ],
            "executable": false,
            "lamports": 2039280,
            "owner": "TokenkegQfeZyiNwAJbNbGKPFXCWuBvf9Ss623VQ5DA",
            "rentEpoch": 361
          },
          "pubkey": "5gjGaTE41sPVS1Dzwg43ipdj9NTtApZLcK55ihRuVb6Y"
        },
        {
          "account": {
            "data": [
              "afxiYbRCtH5HgLYFzytARQOXmFT6HhvNzk2Baxua+lM2kEWUG3BArj8SJRSnd1faFt2Tm0Ey/qtGnPdOOlQlugEAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA",
              "base64"
            ],
            "executable": false,
            "lamports": 2039280,
            "owner": "TokenkegQfeZyiNwAJbNbGKPFXCWuBvf9Ss623VQ5DA",
            "rentEpoch": 361
          },
          "pubkey": "81ZdQjbr7FhEPmcyGJtG8BAUyWxAjb2iSiWFEQn8i8Da"
        }
      ]
    },
    "id": 1
  })";

  std::string token2022_accounts = R"({
    "jsonrpc": "2.0",
    "result": {
      "context": {
        "apiVersion": "1.13.5",
        "slot": 166895942
      },
      "value": [
        {
          "account": {
            "data": [
              "afxiYbRCtH5HgLYFzytARQOXmFT6HhvNzk2Baxua+lM2kEWUG3BArj8SJRSnd1faFt2Tm0Ey/qtGnPdOOlQlugEAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA",
              "base64"
            ],
            "executable": false,
            "lamports": 2039280,
            "owner": "TokenzQdBNbLqP5VEhdkAS6EPFLC1PHnBqCXEpPxuEb",
            "rentEpoch": 361
          },
          "pubkey": "5rUXc3r8bfHVadpvCUPLgcTphcwPMLihCJrxmBeaJEpR"
        }
      ]
    },
    "id": 1
  })";

  SetOwnedTokenAccountsInterceptor(expected_network_url, token_accounts,
                                   token2022_accounts);
  // Create expected account infos
  std::vector<SolanaAccountInfo> expected_account_infos;
  SolanaAccountInfo account_info;
  account_info.data =
      "z6cxAUoRHIupvmezOL4EAsTLlwKTgwxzCg/"
      "xcNWSEu42kEWUG3BArj8SJRSnd1faFt2Tm0Ey/"
      "qtGnPdOOlQlugEAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
      "QAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
      "AAA";
  account_info.executable = false;
  account_info.lamports = 2039280;
  account_info.owner = "TokenkegQfeZyiNwAJbNbGKPFXCWuBvf9Ss623VQ5DA";
  account_info.rent_epoch = 361;

  expected_account_infos.push_back(account_info);
  account_info.data =
      "afxiYbRCtH5HgLYFzytARQOXmFT6HhvNzk2Baxua+"
      "lM2kEWUG3BArj8SJRSnd1faFt2Tm0Ey/"
      "qtGnPdOOlQlugEAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
      "QAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
      "AAA";
  expected_account_infos.push_back(account_info);

  account_info.owner = mojom::kSolanaToken2022ProgramId;
  expected_account_infos.push_back(account_info);

  std::optional solana_address =
      SolanaAddress::FromBase58("4fzcQKyGFuk55uJaBZtvTHh42RBxbrZMuXzsGQvBJbwF");
  ASSERT_TRUE(solana_address);

  // Invalid chain ID yields invalid params error
  TestGetSolanaTokenAccountsByOwner(
      *solana_address, "999", {}, mojom::SolanaProviderError::kInvalidParams,
      l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));

  // Valid
  TestGetSolanaTokenAccountsByOwner(*solana_address, mojom::kSolanaMainnet,
                                    expected_account_infos,
                                    mojom::SolanaProviderError::kSuccess, "");

  // Response parsing error
  SetInterceptor(expected_network_url, "getTokenAccountsByOwner", "",
                 R"({"jsonrpc":"2.0","id":1})");
  TestGetSolanaTokenAccountsByOwner(
      *solana_address, mojom::kSolanaMainnet, {},
      mojom::SolanaProviderError::kParsingError,
      l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));

  // JSON RPC error
  SetInterceptor(expected_network_url, "getTokenAccountsByOwner", "",
                 R"({"jsonrpc": "2.0", "id": 1,
                     "error": {
                       "code":-32601,
                       "message":"method does not exist"
                     }
                    })");
  TestGetSolanaTokenAccountsByOwner(*solana_address, mojom::kSolanaMainnet, {},
                                    mojom::SolanaProviderError::kMethodNotFound,
                                    "method does not exist");

  // HTTP error
  SetHTTPRequestTimeoutInterceptor();
  TestGetSolanaTokenAccountsByOwner(
      *solana_address, mojom::kSolanaMainnet, {},
      mojom::SolanaProviderError::kInternalError,
      l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
}

TEST_F(JsonRpcServiceUnitTest, GetSPLTokenBalances) {
  auto expected_network_url =
      GetNetwork(mojom::kSolanaMainnet, mojom::CoinType::SOL);

  std::string token_accounts = R"(
    {
      "jsonrpc": "2.0",
      "result": {
        "context": {
          "apiVersion": "1.14.17",
          "slot": 195856971
        },
        "value": [
          {
            "account": {
              "data": {
                "parsed": {
                  "info": {
                    "isNative": false,
                    "mint": "7dHbWXmci3dT8UFYWYZweBLXgycu7Y3iL6trKn1Y7ARj",
                    "owner": "5wytVPbjLb2VCXbynhUQabEZZD2B6Wxrkvwm6v6Cuy5X",
                    "state": "initialized",
                    "tokenAmount": {
                      "amount": "898865",
                      "decimals": 9,
                      "uiAmount": 0.000898865,
                      "uiAmountString": "0.000898865"
                    }
                  },
                  "type": "account"
                },
                "program": "spl-token",
                "space": 165
              },
              "executable": false,
              "lamports": 2039280,
              "owner": "TokenkegQfeZyiNwAJbNbGKPFXCWuBvf9Ss623VQ5DA",
              "rentEpoch": 0
            },
            "pubkey": "5rUXc3r8bfHVadpvCUPLgcTphcwPMLihCJrxmBeaJEpR"
          }
        ]
      },
      "id": 1
    }
  )";

  std::string token2022_accounts = R"(
    {
      "jsonrpc": "2.0",
      "result": {
        "context": {
          "apiVersion": "1.14.17",
          "slot": 195856971
        },
        "value": [
          {
            "account": {
              "data": {
                "parsed": {
                  "info": {
                    "isNative": false,
                    "mint": "6dHbWXmci3dT8UFYWYZweBLXgycu7Y3iL6trKn1Y7ARj",
                    "owner": "5wytVPbjLb2VCXbynhUQabEZZD2B6Wxrkvwm6v6Cuy5X",
                    "state": "initialized",
                    "tokenAmount": {
                      "amount": "898843",
                      "decimals": 9,
                      "uiAmount": 0.000898843,
                      "uiAmountString": "0.000898843"
                    }
                  },
                  "type": "account"
                },
                "program": "spl-token",
                "space": 165
              },
              "executable": false,
              "lamports": 2039280,
              "owner": "TokenzQdBNbLqP5VEhdkAS6EPFLC1PHnBqCXEpPxuEb",
              "rentEpoch": 0
            },
            "pubkey": "81ZdQjbr7FhEPmcyGJtG8BAUyWxAjb2iSiWFEQn8i8Da"
          }
        ]
      },
      "id": 1
    }
  )";
  SetOwnedTokenAccountsInterceptor(
      GetNetwork(mojom::kSolanaMainnet, mojom::CoinType::SOL), token_accounts,
      token2022_accounts);
  std::vector<mojom::SPLTokenAmountPtr> expected_results;
  mojom::SPLTokenAmountPtr result = mojom::SPLTokenAmount::New();
  result->mint = "7dHbWXmci3dT8UFYWYZweBLXgycu7Y3iL6trKn1Y7ARj";
  result->amount = "898865";
  result->ui_amount = "0.000898865";
  result->decimals = 9;
  expected_results.push_back(std::move(result));

  result = mojom::SPLTokenAmount::New();
  result->mint = "6dHbWXmci3dT8UFYWYZweBLXgycu7Y3iL6trKn1Y7ARj";
  result->amount = "898843";
  result->ui_amount = "0.000898843";
  result->decimals = 9;
  expected_results.push_back(std::move(result));

  // OK: valid
  TestGetSPLTokenBalances("5wytVPbjLb2VCXbynhUQabEZZD2B6Wxrkvwm6v6Cuy5X",
                          mojom::kSolanaMainnet, std::move(expected_results),
                          mojom::SolanaProviderError::kSuccess, "");

  // KO: invalid chain id
  TestGetSPLTokenBalances(
      "5wytVPbjLb2VCXbynhUQabEZZD2B6Wxrkvwm6v6Cuy5X", "999", {},
      mojom::SolanaProviderError::kInvalidParams,
      l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));

  // KO: response parsing error
  SetInterceptor(expected_network_url, "getTokenAccountsByOwner", "",
                 R"({"jsonrpc":"2.0","id":1})");
  TestGetSPLTokenBalances("5wytVPbjLb2VCXbynhUQabEZZD2B6Wxrkvwm6v6Cuy5X",
                          mojom::kSolanaMainnet, {},
                          mojom::SolanaProviderError::kParsingError,
                          l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));

  // KO: JSON RPC error
  SetInterceptor(expected_network_url, "getTokenAccountsByOwner", "",
                 R"({"jsonrpc": "2.0", "id": 1,
                     "error": {
                       "code":-32601,
                       "message":"method does not exist"
                     }
                    })");
  TestGetSPLTokenBalances(
      "5wytVPbjLb2VCXbynhUQabEZZD2B6Wxrkvwm6v6Cuy5X", mojom::kSolanaMainnet, {},
      mojom::SolanaProviderError::kMethodNotFound, "method does not exist");

  // KO: HTTP error
  SetHTTPRequestTimeoutInterceptor();
  TestGetSPLTokenBalances("5wytVPbjLb2VCXbynhUQabEZZD2B6Wxrkvwm6v6Cuy5X",
                          mojom::kSolanaMainnet, {},
                          mojom::SolanaProviderError::kInternalError,
                          l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
}

TEST_F(JsonRpcServiceUnitTest, GetFilEstimateGas) {
  SetInterceptor(GetNetwork(mojom::kLocalhostChainId, mojom::CoinType::FIL),
                 "Filecoin.GasEstimateMessageGas", "",
                 GetGasFilEstimateResponse(INT64_MAX));

  GetFilEstimateGas(
      mojom::kLocalhostChainId, "t1tquwkjo6qvweah2g2yikewr7y5dyjds42pnrn3a",
      "t1h5tg3bhp5r56uzgjae2373znti6ygq4agkx4hzq", "1000000000000000000",
      "100466", "101520", INT64_MAX, mojom::FilecoinProviderError::kSuccess);

  SetInterceptor(GetNetwork(mojom::kLocalhostChainId, mojom::CoinType::FIL),
                 "Filecoin.GasEstimateMessageGas", "",
                 GetGasFilEstimateResponse(INT64_MIN));

  GetFilEstimateGas(
      mojom::kLocalhostChainId, "t1tquwkjo6qvweah2g2yikewr7y5dyjds42pnrn3a",
      "t1h5tg3bhp5r56uzgjae2373znti6ygq4agkx4hzq", "1000000000000000000",
      "100466", "101520", INT64_MIN, mojom::FilecoinProviderError::kSuccess);

  GetFilEstimateGas(mojom::kLocalhostChainId, "",
                    "t1h5tg3bhp5r56uzgjae2373znti6ygq4agkx4hzq",
                    "1000000000000000000", "", "", 0,
                    mojom::FilecoinProviderError::kInvalidParams);
  GetFilEstimateGas(mojom::kLocalhostChainId,
                    "t1tquwkjo6qvweah2g2yikewr7y5dyjds42pnrn3a", "",
                    "1000000000000000000", "", "", 0,
                    mojom::FilecoinProviderError::kInvalidParams);

  SetInterceptor(GetNetwork(mojom::kLocalhostChainId, mojom::CoinType::FIL),
                 "Filecoin.GasEstimateMessageGas", "", "");
  GetFilEstimateGas(
      mojom::kLocalhostChainId, "t1tquwkjo6qvweah2g2yikewr7y5dyjds42pnrn3a",
      "t1h5tg3bhp5r56uzgjae2373znti6ygq4agkx4hzq", "1000000000000000000", "",
      "", 0, mojom::FilecoinProviderError::kInternalError);
}

TEST_F(JsonRpcServiceUnitTest, GetFilChainHead) {
  std::string response = R"(
    { "id": 1, "jsonrpc": "2.0",
      "result": {
        "Blocks":[],
        "Cids": [{
              "/": "bafy2bzaceauxm7waysuftonc4vod6wk4trdjx2ibw233dos6jcvkf5nrhflju"
        }],
        "Height": 18446744073709551615
      }
    })";
  SetInterceptor(GetNetwork(mojom::kLocalhostChainId, mojom::CoinType::FIL),
                 "Filecoin.ChainHead", "", response);
  GetFilBlockHeight(mojom::kLocalhostChainId, UINT64_MAX,
                    mojom::FilecoinProviderError::kSuccess, "");
  SetInterceptor(GetNetwork(mojom::kLocalhostChainId, mojom::CoinType::FIL),
                 "Filecoin.ChainHead", "", "");
  GetFilBlockHeight(mojom::kLocalhostChainId, 0,
                    mojom::FilecoinProviderError::kInternalError,
                    l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
  SetInterceptor(GetNetwork(mojom::kLocalhostChainId, mojom::CoinType::FIL),
                 "Filecoin.ChainHead", "", R"(
    {"jsonrpc":"2.0","id":1,
      "error":{
        "code":-32602,
        "message":"wrong param count (method 'Filecoin.ChainHead'): 1 != 0"
      }
    })");
  GetFilBlockHeight(mojom::kLocalhostChainId, 0,
                    mojom::FilecoinProviderError::kInvalidParams,
                    "wrong param count (method 'Filecoin.ChainHead'): 1 != 0");
}

TEST_F(JsonRpcServiceUnitTest, GetFilStateSearchMsgLimited) {
  SetInterceptor(GetNetwork(mojom::kLocalhostChainId, mojom::CoinType::FIL),
                 "Filecoin.StateSearchMsgLimited", "",
                 GetFilStateSearchMsgLimitedResponse(0));

  GetFilStateSearchMsgLimited(
      mojom::kLocalhostChainId,
      "bafy2bzacebundyopm3trenj47hxkwiqn2cbvvftz3fss4dxuttu2u6xbbtkqy", 30, 0,
      mojom::FilecoinProviderError::kSuccess, "");

  SetInterceptor(GetNetwork(mojom::kLocalhostChainId, mojom::CoinType::FIL),
                 "Filecoin.StateSearchMsgLimited", "", R"(
    {
        "id": 1,
        "jsonrpc": "2.0",
        "error":{
          "code":-32602,
          "message":"wrong param count"
        }
  })");
  GetFilStateSearchMsgLimited(
      mojom::kLocalhostChainId,
      "bafy2bzacebundyopm3trenj47hxkwiqn2cbvvftz3fss4dxuttu2u6xbbtkqy", 30, -1,
      mojom::FilecoinProviderError::kInvalidParams, "wrong param count");

  SetInterceptor(GetNetwork(mojom::kLocalhostChainId, mojom::CoinType::FIL),
                 "Filecoin.StateSearchMsgLimited", "", R"({,})");
  GetFilStateSearchMsgLimited(
      mojom::kLocalhostChainId,
      "bafy2bzacebundyopm3trenj47hxkwiqn2cbvvftz3fss4dxuttu2u6xbbtkqy", 30, -1,
      mojom::FilecoinProviderError::kInternalError,
      l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));

  SetInterceptor(GetNetwork(mojom::kLocalhostChainId, mojom::CoinType::FIL),
                 "Filecoin.StateSearchMsgLimited", "",
                 GetFilStateSearchMsgLimitedResponse(INT64_MAX));
  GetFilStateSearchMsgLimited(
      mojom::kLocalhostChainId,
      "bafy2bzacebundyopm3trenj47hxkwiqn2cbvvftz3fss4dxuttu2u6xbbtkqy", 30,
      INT64_MAX, mojom::FilecoinProviderError::kSuccess, "");

  SetInterceptor(GetNetwork(mojom::kLocalhostChainId, mojom::CoinType::FIL),
                 "Filecoin.StateSearchMsgLimited", "",
                 GetFilStateSearchMsgLimitedResponse(INT64_MIN));
  GetFilStateSearchMsgLimited(
      mojom::kLocalhostChainId,
      "bafy2bzacebundyopm3trenj47hxkwiqn2cbvvftz3fss4dxuttu2u6xbbtkqy", 30,
      INT64_MIN, mojom::FilecoinProviderError::kSuccess, "");
}

TEST_F(JsonRpcServiceUnitTest, SendFilecoinTransaction) {
  SetInterceptor(GetNetwork(mojom::kLocalhostChainId, mojom::CoinType::FIL),
                 "Filecoin.MpoolPush", "",
                 R"({
                   "id": 1,
                   "jsonrpc": "2.0",
                   "result": {
                     "/": "cid"
                   }
                 })");
  GetSendFilecoinTransaction(mojom::kLocalhostChainId, "{}", "cid",
                             mojom::FilecoinProviderError::kSuccess, "");

  SetInterceptor(GetNetwork(mojom::kLocalhostChainId, mojom::CoinType::FIL),
                 "Filecoin.MpoolPush", "", R"(
    {
        "id": 1,
        "jsonrpc": "2.0",
        "error":{
          "code":-32602,
          "message":"wrong param count"
        }
  })");
  GetSendFilecoinTransaction(mojom::kLocalhostChainId, "{}", "",
                             mojom::FilecoinProviderError::kInvalidParams,
                             "wrong param count");

  SetInterceptor(GetNetwork(mojom::kLocalhostChainId, mojom::CoinType::FIL),
                 "Filecoin.MpoolPush", "", "");
  GetSendFilecoinTransaction(
      mojom::kLocalhostChainId, "{}", "",
      mojom::FilecoinProviderError::kParsingError,
      l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));
  GetSendFilecoinTransaction(
      mojom::kLocalhostChainId, "broken json", "",
      mojom::FilecoinProviderError::kInternalError,
      l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
  GetSendFilecoinTransaction(
      mojom::kLocalhostChainId, "", "",
      mojom::FilecoinProviderError::kInternalError,
      l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
}

TEST_F(JsonRpcServiceUnitTest, ResolverPrefs) {
  base::MockCallback<base::OnceCallback<void(mojom::ResolveMethod)>> callback;

  auto methods = {mojom::ResolveMethod::kEnabled, mojom::ResolveMethod::kAsk,
                  mojom::ResolveMethod::kDisabled};

  // Unstoppable domains.
  EXPECT_CALL(callback, Run(mojom::ResolveMethod::kAsk));
  json_rpc_service_->GetUnstoppableDomainsResolveMethod(callback.Get());
  testing::Mock::VerifyAndClearExpectations(&callback);

  for (auto m : methods) {
    json_rpc_service_->SetUnstoppableDomainsResolveMethod(m);
    EXPECT_CALL(callback, Run(m));
    json_rpc_service_->GetUnstoppableDomainsResolveMethod(callback.Get());
    testing::Mock::VerifyAndClearExpectations(&callback);
  }

  // ENS.
  EXPECT_CALL(callback, Run(mojom::ResolveMethod::kAsk));
  json_rpc_service_->GetEnsResolveMethod(callback.Get());
  testing::Mock::VerifyAndClearExpectations(&callback);

  for (auto m : methods) {
    json_rpc_service_->SetEnsResolveMethod(m);
    EXPECT_CALL(callback, Run(m));
    json_rpc_service_->GetEnsResolveMethod(callback.Get());
    testing::Mock::VerifyAndClearExpectations(&callback);
  }

  // ENS Offchain.
  EXPECT_CALL(callback, Run(mojom::ResolveMethod::kAsk));
  json_rpc_service_->GetEnsOffchainLookupResolveMethod(callback.Get());
  testing::Mock::VerifyAndClearExpectations(&callback);

  for (auto m : methods) {
    json_rpc_service_->SetEnsOffchainLookupResolveMethod(m);
    EXPECT_CALL(callback, Run(m));
    json_rpc_service_->GetEnsOffchainLookupResolveMethod(callback.Get());
    testing::Mock::VerifyAndClearExpectations(&callback);
  }

  // SNS.
  EXPECT_CALL(callback, Run(mojom::ResolveMethod::kAsk));
  json_rpc_service_->GetSnsResolveMethod(callback.Get());
  testing::Mock::VerifyAndClearExpectations(&callback);

  for (auto m : methods) {
    json_rpc_service_->SetSnsResolveMethod(m);
    EXPECT_CALL(callback, Run(m));
    json_rpc_service_->GetSnsResolveMethod(callback.Get());
    testing::Mock::VerifyAndClearExpectations(&callback);
  }
}

class EnsGetResolverHandler : public EthCallHandler {
 public:
  EnsGetResolverHandler(const std::string& host_name,
                        const EthAddress& resolver_address)
      : EthCallHandler(EthAddress::FromHex(GetEnsRegistryContractAddress(
                           mojom::kMainnetChainId)),
                       GetFunctionHashBytes4("resolver(bytes32)")),
        host_name_(host_name),
        resolver_address_(resolver_address) {}
  ~EnsGetResolverHandler() override = default;

  std::optional<std::string> HandleEthCall(eth_abi::Span call_data) override {
    auto [_, args] =
        *eth_abi::ExtractFunctionSelectorAndArgsFromCall(call_data);
    auto namehash_bytes = eth_abi::ExtractFixedBytesFromTuple<32>(args, 0);
    EXPECT_TRUE(namehash_bytes);

    if (!base::ranges::equal(*namehash_bytes, Namehash(host_name_))) {
      return MakeJsonRpcTupleResponse(
          eth_abi::TupleEncoder().AddAddress(EthAddress::ZeroAddress()));
    }

    return MakeJsonRpcTupleResponse(
        eth_abi::TupleEncoder().AddAddress(resolver_address_));
  }

 private:
  std::string host_name_;
  EthAddress resolver_address_;
};

class Ensip10SupportHandler : public EthCallHandler {
 public:
  explicit Ensip10SupportHandler(const EthAddress& resolver_address)
      : EthCallHandler(resolver_address,
                       GetFunctionHashBytes4("supportsInterface(bytes4)")) {}
  ~Ensip10SupportHandler() override = default;

  std::optional<std::string> HandleEthCall(eth_abi::Span call_data) override {
    auto [_, args] =
        *eth_abi::ExtractFunctionSelectorAndArgsFromCall(call_data);

    auto arg_selector = eth_abi::ExtractFixedBytesFromTuple<4>(args, 0);
    EXPECT_TRUE(arg_selector);
    EXPECT_TRUE(base::ranges::equal(*arg_selector, kResolveBytesBytesSelector));

    return MakeJsonRpcTupleResponse(
        eth_abi::TupleEncoder().AddUint256(uint256_t(result_value_)));
  }

  void DisableSupport() { result_value_ = 0; }

 private:
  uint256_t result_value_ = 1;
};

class EnsGetRecordHandler : public EthCallHandler {
 public:
  explicit EnsGetRecordHandler(const EthAddress& resolver_address,
                               const std::string& host_name,
                               const EthAddress& result_address,
                               const std::vector<uint8_t>& result_contenthash)
      : EthCallHandler(resolver_address,
                       {GetFunctionHashBytes4("addr(bytes32)"),
                        GetFunctionHashBytes4("contenthash(bytes32)")}),
        host_name_(host_name),
        result_address_(result_address),
        result_contenthash_(result_contenthash) {}
  ~EnsGetRecordHandler() override = default;

  std::optional<std::string> HandleEthCall(eth_abi::Span call_data) override {
    if (offchain_lookup_) {
      auto extra_data =
          eth_abi::TupleEncoder().AddString("extra data").Encode();

      // Sending `bytes` as callData argument to gateway. Gateway will decode it
      // and return requested ens record.
      auto offchain_lookup =
          eth_abi::TupleEncoder()
              .AddAddress(to())                       // address
              .AddStringArray({gateway_url_.spec()})  // urls
              .AddBytes(call_data)                    // callData
              .AddFixedBytes(GetFunctionHashBytes4(
                  "resolveCallback(bytes,bytes)"))  // callbackFunction
              .AddBytes(extra_data)                 // extraData
              .EncodeWithSelector(kOffchainLookupSelector);

      return MakeJsonRpcErrorResponseWithData(3, "execution reverted",
                                              ToHex(offchain_lookup));
    }

    auto [selector, args] =
        *eth_abi::ExtractFunctionSelectorAndArgsFromCall(call_data);

    auto namehash_bytes = eth_abi::ExtractFixedBytesFromTuple<32>(args, 0);
    EXPECT_TRUE(namehash_bytes);
    bool host_matches =
        base::ranges::equal(*namehash_bytes, Namehash(host_name_));

    if (selector == GetFunctionHashBytes4("addr(bytes32)")) {
      auto eth_address = EthAddress::ZeroAddress();
      if (host_matches) {
        eth_address = result_address_;
      }

      return MakeJsonRpcTupleResponse(
          eth_abi::TupleEncoder().AddAddress(eth_address));
    }

    if (selector == GetFunctionHashBytes4("contenthash(bytes32)")) {
      std::vector<uint8_t> contenthash;
      if (host_matches) {
        contenthash = result_contenthash_;
      }

      return MakeJsonRpcTupleResponse(
          eth_abi::TupleEncoder().AddBytes(contenthash));
    }

    return std::nullopt;
  }

  void RespondWithOffchainLookup(GURL gateway_url) {
    offchain_lookup_ = true;
    gateway_url_ = gateway_url;
  }

 private:
  std::string host_name_;
  EthAddress result_address_;
  std::vector<uint8_t> result_contenthash_;
  bool offchain_lookup_ = false;
  GURL gateway_url_;
};

class Ensip10ResolveHandler : public EthCallHandler {
 public:
  Ensip10ResolveHandler(const EthAddress& resolver_address,
                        const std::string& host_name,
                        const GURL& gateway_url)
      : EthCallHandler(resolver_address,
                       GetFunctionHashBytes4("resolve(bytes,bytes)")),
        host_name_(host_name),
        gateway_url_(gateway_url) {}

  ~Ensip10ResolveHandler() override = default;

  std::optional<std::string> HandleEthCall(eth_abi::Span call_data) override {
    auto extra_data = eth_abi::TupleEncoder().AddString("extra data").Encode();

    // Sending `bytes` as callData argument to gateway. Gateway will decode it
    // and return requested ens record.
    auto offchain_lookup =
        eth_abi::TupleEncoder()
            .AddAddress(to())                       // address
            .AddStringArray({gateway_url_.spec()})  // urls
            .AddBytes(call_data)                    // callData
            .AddFixedBytes(GetFunctionHashBytes4(
                "resolveCallback(bytes,bytes)"))  // callbackFunction
            .AddBytes(extra_data)                 // extraData
            .EncodeWithSelector(kOffchainLookupSelector);

    return MakeJsonRpcErrorResponseWithData(3, "execution reverted",
                                            ToHex(offchain_lookup));
  }

 private:
  std::string host_name_;
  GURL gateway_url_;
};

class OffchainCallbackHandler : public EthCallHandler {
 public:
  explicit OffchainCallbackHandler(const EthAddress& resolver_address)
      : EthCallHandler(resolver_address,
                       GetFunctionHashBytes4("resolveCallback(bytes,bytes)")) {}
  ~OffchainCallbackHandler() override = default;

  std::optional<std::string> HandleEthCall(eth_abi::Span call_data) override {
    auto [_, args] =
        *eth_abi::ExtractFunctionSelectorAndArgsFromCall(call_data);

    auto extra_data_bytes = eth_abi::ExtractBytesFromTuple(args, 1);
    EXPECT_EQ("extra data",
              eth_abi::ExtractStringFromTuple(*extra_data_bytes, 0));

    auto bytes_result = eth_abi::ExtractBytesFromTuple(args, 0);
    if (!bytes_result) {
      return std::nullopt;
    }

    // Just returning bytes result from gateway as is.
    return MakeJsonRpcRawBytesResponse(*bytes_result);
  }
};

class OffchainGatewayHandler {
 public:
  OffchainGatewayHandler(
      const GURL& gateway_url,
      const EthAddress& resolver_address,
      const std::map<std::string, EthAddress>& map_offchain_eth_address,
      const std::map<std::string, std::vector<uint8_t>>&
          map_offchain_contenthash)
      : gateway_url_(gateway_url),
        resolver_address_(resolver_address),
        map_offchain_eth_address_(map_offchain_eth_address),
        map_offchain_contenthash_(map_offchain_contenthash) {}

  std::optional<std::string> HandleRequest(
      const network::ResourceRequest& request) {
    if (request.url.host() != gateway_url_.host()) {
      return std::nullopt;
    }

    if (respond_with_500_) {
      return "";
    }

    auto payload = ToValue(request);
    if (!payload || !payload->is_dict()) {
      return std::nullopt;
    }
    auto* sender = payload->GetDict().FindString("sender");
    EXPECT_EQ(EthAddress::FromHex(*sender), resolver_address_);

    auto* data = payload->GetDict().FindString("data");
    auto bytes = PrefixedHexStringToBytes(*data);
    if (!bytes) {
      NOTREACHED();
    }

    auto [selector, args] =
        *eth_abi::ExtractFunctionSelectorAndArgsFromCall(*bytes);

    bool ensip10_resolve = false;
    std::optional<std::vector<uint8_t>> encoded_call;
    if (ToHex(selector) == GetFunctionHash("resolve(bytes,bytes)")) {
      auto dns_encoded_name = eth_abi::ExtractBytesFromTuple(args, 0);
      EXPECT_TRUE(dns_encoded_name);
      EXPECT_FALSE(dns_encoded_name->empty());
      encoded_call = eth_abi::ExtractBytesFromTuple(args, 1);
      ensip10_resolve = true;
    } else if (ToHex(selector) == GetFunctionHash("addr(bytes32)")) {
      encoded_call = *bytes;
    } else if (ToHex(selector) == GetFunctionHash("contenthash(bytes32)")) {
      encoded_call = *bytes;
    }

    auto [encoded_call_selector, enconed_call_args] =
        *eth_abi::ExtractFunctionSelectorAndArgsFromCall(*encoded_call);

    auto domain_namehash =
        eth_abi::ExtractFixedBytesFromTuple<32>(enconed_call_args, 0);
    EXPECT_TRUE(domain_namehash);

    std::vector<uint8_t> data_value;
    if (base::ranges::equal(encoded_call_selector, kAddrBytes32Selector)) {
      data_value = eth_abi::TupleEncoder()
                       .AddAddress(EthAddress::ZeroAddress())
                       .Encode();
      if (!respond_with_no_record_) {
        for (auto& [domain, address] : map_offchain_eth_address_) {
          if (base::ranges::equal(*domain_namehash, Namehash(domain))) {
            data_value = eth_abi::TupleEncoder().AddAddress(address).Encode();
            break;
          }
        }
      }
    } else if (base::ranges::equal(encoded_call_selector,
                                   kContentHashBytes32Selector)) {
      data_value =
          eth_abi::TupleEncoder().AddBytes(std::vector<uint8_t>()).Encode();
      if (!respond_with_no_record_) {
        for (auto& [domain, contenthash] : map_offchain_contenthash_) {
          if (base::ranges::equal(*domain_namehash, Namehash(domain))) {
            data_value = eth_abi::TupleEncoder().AddBytes(contenthash).Encode();
            break;
          }
        }
      }
    } else {
      NOTREACHED();
    }

    if (ensip10_resolve) {
      // For resolve(bytes,bytes) case need to wrap encoded response as a tuple
      // with bytes.
      data_value = eth_abi::TupleEncoder().AddBytes(data_value).Encode();
    }

    base::Value::Dict result;
    result.Set("data", ToHex(data_value));
    std::string response;
    base::JSONWriter::Write(result, &response);
    return response;
  }

  void SetRespondWith500() { respond_with_500_ = true; }
  void SetRespondWithNoRecord() { respond_with_no_record_ = true; }

 private:
  GURL gateway_url_;
  EthAddress resolver_address_;
  std::map<std::string, EthAddress> map_offchain_eth_address_;
  std::map<std::string, std::vector<uint8_t>> map_offchain_contenthash_;
  bool respond_with_500_ = false;
  bool respond_with_no_record_ = false;
};

class ENSL2JsonRpcServiceUnitTest : public JsonRpcServiceUnitTest {
 public:
  ENSL2JsonRpcServiceUnitTest() = default;

  void SetUp() override {
    JsonRpcServiceUnitTest::SetUp();

    json_rpc_endpoint_handler_ = std::make_unique<JsonRpcEndpointHandler>(
        GetNetwork(mojom::kMainnetChainId, mojom::CoinType::ETH));

    ens_resolver_handler_ =
        std::make_unique<EnsGetResolverHandler>(ens_host(), resolver_address());
    ens_get_record_handler_ = std::make_unique<EnsGetRecordHandler>(
        resolver_address(), ens_host(), onchain_eth_addr(),
        onchain_contenthash());
    ensip10_support_handler_ =
        std::make_unique<Ensip10SupportHandler>(resolver_address());
    ensip10_resolve_handler_ = std::make_unique<Ensip10ResolveHandler>(
        resolver_address(), ens_host(), gateway_url());
    ensip10_resolve_callback_handler_ =
        std::make_unique<OffchainCallbackHandler>(resolver_address());

    json_rpc_endpoint_handler_->AddEthCallHandler(ens_resolver_handler_.get());
    json_rpc_endpoint_handler_->AddEthCallHandler(
        ens_get_record_handler_.get());
    json_rpc_endpoint_handler_->AddEthCallHandler(
        ensip10_support_handler_.get());
    json_rpc_endpoint_handler_->AddEthCallHandler(
        ensip10_resolve_handler_.get());
    json_rpc_endpoint_handler_->AddEthCallHandler(
        ensip10_resolve_callback_handler_.get());

    std::map<std::string, EthAddress> map_offchain_eth_address = {
        {ens_host(), offchain_eth_addr()},
        {ens_subdomain_host(), offchain_subdomain_eth_addr()}};
    std::map<std::string, std::vector<uint8_t>> map_offchain_contenthash = {
        {ens_host(), offchain_contenthash()},
        {ens_subdomain_host(), offchain_subdomain_contenthash()}};
    offchain_gateway_handler_ = std::make_unique<OffchainGatewayHandler>(
        gateway_url(), resolver_address(), std::move(map_offchain_eth_address),
        std::move(map_offchain_contenthash));

    url_loader_factory_.SetInterceptor(base::BindRepeating(
        &ENSL2JsonRpcServiceUnitTest::HandleRequest, base::Unretained(this)));
  }

  std::string ens_host() { return "offchainexample.eth"; }
  std::string ens_subdomain_host() { return "test.offchainexample.eth"; }
  GURL gateway_url() { return GURL("https://gateway.brave.com/"); }
  EthAddress resolver_address() {
    return EthAddress::FromHex("0xc1735677a60884abbcf72295e88d47764beda282");
  }
  EthAddress offchain_eth_addr() {
    return EthAddress::FromHex("0xaabbccddeeaabbccddeeaabbccddeeaabbccddee");
  }
  EthAddress offchain_subdomain_eth_addr() {
    return EthAddress::FromHex("0xeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee");
  }

  EthAddress onchain_eth_addr() {
    return EthAddress::FromHex("0x1234567890123456789012345678901234567890");
  }

  std::vector<uint8_t> offchain_contenthash() {
    std::string contenthash =
        "e30101701220f073be187e8e06039796c432a"
        "5bdd6da3f403c2f93fa5d9dbdc5547c7fe0e3bc";
    std::vector<uint8_t> bytes;
    base::HexStringToBytes(contenthash, &bytes);
    return bytes;
  }
  std::vector<uint8_t> offchain_subdomain_contenthash() {
    std::string contenthash =
        "e30101701220f073be187e8e06039796c432a"
        "5bdd6da3f403c2f93fa5d9dbdc5547c7feeeeee";
    std::vector<uint8_t> bytes;
    base::HexStringToBytes(contenthash, &bytes);
    return bytes;
  }

  std::vector<uint8_t> onchain_contenthash() {
    std::string contenthash =
        "e50101701220f073be187e8e06039796c432a"
        "5bdd6da3f403c2f93fa5d9dbdc5547c7fe0e3bc";
    std::vector<uint8_t> bytes;
    base::HexStringToBytes(contenthash, &bytes);
    return bytes;
  }

 protected:
  void HandleRequest(const network::ResourceRequest& request) {
    url_loader_factory_.ClearResponses();
    if (auto json_response =
            json_rpc_endpoint_handler_->HandleRequest(request)) {
      url_loader_factory_.AddResponse(request.url.spec(), *json_response);
    } else if (auto offchain_response =
                   offchain_gateway_handler_->HandleRequest(request)) {
      if (offchain_response->empty()) {
        url_loader_factory_.AddResponse(request.url.spec(), "",
                                        net::HTTP_INTERNAL_SERVER_ERROR);
      } else {
        url_loader_factory_.AddResponse(request.url.spec(), *offchain_response);
      }
    }
  }

  std::unique_ptr<EnsGetResolverHandler> ens_resolver_handler_;
  std::unique_ptr<EnsGetRecordHandler> ens_get_record_handler_;
  std::unique_ptr<Ensip10SupportHandler> ensip10_support_handler_;
  std::unique_ptr<Ensip10ResolveHandler> ensip10_resolve_handler_;
  std::unique_ptr<OffchainCallbackHandler> ensip10_resolve_callback_handler_;
  std::unique_ptr<JsonRpcEndpointHandler> json_rpc_endpoint_handler_;
  std::unique_ptr<OffchainGatewayHandler> offchain_gateway_handler_;
};

TEST_F(ENSL2JsonRpcServiceUnitTest, GetWalletAddr) {
  json_rpc_service_->SetEnsOffchainLookupResolveMethod(
      mojom::ResolveMethod::kEnabled);

  base::MockCallback<JsonRpcService::EnsGetEthAddrCallback> callback;
  EXPECT_CALL(callback, Run(offchain_eth_addr().ToHex(), false,
                            mojom::ProviderError::kSuccess, ""));
  json_rpc_service_->EnsGetEthAddr(ens_host(), callback.Get());
  task_environment_.RunUntilIdle();
}

TEST_F(ENSL2JsonRpcServiceUnitTest, GetWalletAddr_Subdomain) {
  json_rpc_service_->SetEnsOffchainLookupResolveMethod(
      mojom::ResolveMethod::kEnabled);

  base::MockCallback<JsonRpcService::EnsGetEthAddrCallback> callback;
  EXPECT_CALL(callback, Run(offchain_subdomain_eth_addr().ToHex(), false,
                            mojom::ProviderError::kSuccess, ""));
  json_rpc_service_->EnsGetEthAddr(ens_subdomain_host(), callback.Get());
  task_environment_.RunUntilIdle();
}

TEST_F(ENSL2JsonRpcServiceUnitTest, GetWalletAddr_Subdomain_NoEnsip10Support) {
  json_rpc_service_->SetEnsOffchainLookupResolveMethod(
      mojom::ResolveMethod::kEnabled);

  // Turning off Ensip-10 support for resolver so addr(bytes32) is called.
  ensip10_support_handler_->DisableSupport();

  base::MockCallback<JsonRpcService::EnsGetEthAddrCallback> callback;
  EXPECT_CALL(callback,
              Run("", false, mojom::ProviderError::kInvalidParams,
                  l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS)));
  json_rpc_service_->EnsGetEthAddr(ens_subdomain_host(), callback.Get());
  task_environment_.RunUntilIdle();
}

TEST_F(ENSL2JsonRpcServiceUnitTest, GetWalletAddr_NoResolver) {
  json_rpc_service_->SetEnsOffchainLookupResolveMethod(
      mojom::ResolveMethod::kEnabled);

  base::MockCallback<JsonRpcService::EnsGetEthAddrCallback> callback;
  EXPECT_CALL(callback,
              Run("", false, mojom::ProviderError::kInternalError,
                  l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)));
  json_rpc_service_->EnsGetEthAddr("unknown-host.eth", callback.Get());
  task_environment_.RunUntilIdle();
}

TEST_F(ENSL2JsonRpcServiceUnitTest, GetWalletAddr_NoEnsip10Support) {
  json_rpc_service_->SetEnsOffchainLookupResolveMethod(
      mojom::ResolveMethod::kEnabled);

  // Turning off Ensip-10 support for resolver so addr(bytes32) is called.
  ensip10_support_handler_->DisableSupport();

  base::MockCallback<JsonRpcService::EnsGetEthAddrCallback> callback;
  EXPECT_CALL(callback, Run(onchain_eth_addr().ToHex(), false,
                            mojom::ProviderError::kSuccess, ""));
  json_rpc_service_->EnsGetEthAddr(ens_host(), callback.Get());
  task_environment_.RunUntilIdle();
}

TEST_F(ENSL2JsonRpcServiceUnitTest, GetWalletAddr_NoEnsip10Support_GoOffchain) {
  json_rpc_service_->SetEnsOffchainLookupResolveMethod(
      mojom::ResolveMethod::kEnabled);

  // Turning off Ensip-10 support for resolver so addr(bytes32) is called.
  ensip10_support_handler_->DisableSupport();
  // addr(bytes32) will go offchain.
  ens_get_record_handler_->RespondWithOffchainLookup(gateway_url());

  base::MockCallback<JsonRpcService::EnsGetEthAddrCallback> callback;
  EXPECT_CALL(callback, Run(offchain_eth_addr().ToHex(), false,
                            mojom::ProviderError::kSuccess, ""));
  json_rpc_service_->EnsGetEthAddr(ens_host(), callback.Get());
  task_environment_.RunUntilIdle();
}

TEST_F(ENSL2JsonRpcServiceUnitTest, GetWalletAddr_Gateway500Error) {
  json_rpc_service_->SetEnsOffchainLookupResolveMethod(
      mojom::ResolveMethod::kEnabled);

  // Gateway request fails.
  offchain_gateway_handler_->SetRespondWith500();

  base::MockCallback<JsonRpcService::EnsGetEthAddrCallback> callback;
  EXPECT_CALL(callback,
              Run("", false, mojom::ProviderError::kInternalError,
                  l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)));
  json_rpc_service_->EnsGetEthAddr(ens_host(), callback.Get());
  task_environment_.RunUntilIdle();
}

TEST_F(ENSL2JsonRpcServiceUnitTest, GetWalletAddr_GatewayNoRecord) {
  json_rpc_service_->SetEnsOffchainLookupResolveMethod(
      mojom::ResolveMethod::kEnabled);

  // No data record in gateway.
  offchain_gateway_handler_->SetRespondWithNoRecord();

  base::MockCallback<JsonRpcService::EnsGetEthAddrCallback> callback;
  EXPECT_CALL(callback,
              Run("", false, mojom::ProviderError::kInvalidParams,
                  l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS)));
  json_rpc_service_->EnsGetEthAddr(ens_host(), callback.Get());
  task_environment_.RunUntilIdle();
}

TEST_F(ENSL2JsonRpcServiceUnitTest, GetWalletAddr_Consent) {
  EXPECT_EQ(
      decentralized_dns::EnsOffchainResolveMethod::kAsk,
      decentralized_dns::GetEnsOffchainResolveMethod(local_state_prefs()));

  // Call with defaults.
  {
    base::MockCallback<JsonRpcService::EnsGetEthAddrCallback> callback;
    // Called with `require_offchain_consent` == true.
    EXPECT_CALL(callback, Run("", true, mojom::ProviderError::kSuccess, ""));
    json_rpc_service_->EnsGetEthAddr(ens_host(), callback.Get());
    task_environment_.RunUntilIdle();
    EXPECT_EQ(
        decentralized_dns::EnsOffchainResolveMethod::kAsk,
        decentralized_dns::GetEnsOffchainResolveMethod(local_state_prefs()));
  }

  // Allow and remember.
  {
    json_rpc_service_->SetEnsOffchainLookupResolveMethod(
        mojom::ResolveMethod::kEnabled);
    EXPECT_EQ(
        decentralized_dns::EnsOffchainResolveMethod::kEnabled,
        decentralized_dns::GetEnsOffchainResolveMethod(local_state_prefs()));

    base::MockCallback<JsonRpcService::EnsGetEthAddrCallback> callback;
    EXPECT_CALL(callback, Run(offchain_eth_addr().ToHex(), false,
                              mojom::ProviderError::kSuccess, ""));
    json_rpc_service_->EnsGetEthAddr(ens_host(), callback.Get());
    task_environment_.RunUntilIdle();
  }

  // Disable in prefs.
  decentralized_dns::SetEnsOffchainResolveMethod(
      local_state_prefs(),
      decentralized_dns::EnsOffchainResolveMethod::kDisabled);

  // Fails.
  {
    base::MockCallback<JsonRpcService::EnsGetEthAddrCallback> callback;
    EXPECT_CALL(callback,
                Run("", false, mojom::ProviderError::kInternalError,
                    l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)));
    json_rpc_service_->EnsGetEthAddr(ens_host(), callback.Get());
    task_environment_.RunUntilIdle();
  }
}

TEST_F(ENSL2JsonRpcServiceUnitTest, GetContentHash) {
  decentralized_dns::SetEnsOffchainResolveMethod(
      local_state_prefs(),
      decentralized_dns::EnsOffchainResolveMethod::kEnabled);

  base::MockCallback<JsonRpcService::EnsGetContentHashCallback> callback;
  EXPECT_CALL(callback, Run(offchain_contenthash(), false,
                            mojom::ProviderError::kSuccess, ""));
  json_rpc_service_->EnsGetContentHash(ens_host(), callback.Get());
  task_environment_.RunUntilIdle();
}

TEST_F(ENSL2JsonRpcServiceUnitTest, GetContentHash_Subdomain) {
  decentralized_dns::SetEnsOffchainResolveMethod(
      local_state_prefs(),
      decentralized_dns::EnsOffchainResolveMethod::kEnabled);

  base::MockCallback<JsonRpcService::EnsGetContentHashCallback> callback;
  EXPECT_CALL(callback, Run(offchain_subdomain_contenthash(), false,
                            mojom::ProviderError::kSuccess, ""));
  json_rpc_service_->EnsGetContentHash(ens_subdomain_host(), callback.Get());
  task_environment_.RunUntilIdle();
}

TEST_F(ENSL2JsonRpcServiceUnitTest, GetContentHash_Subdomain_NoEnsip10Support) {
  // Turning off Ensip-10 support for resolver so addr(bytes32) is called.
  ensip10_support_handler_->DisableSupport();

  decentralized_dns::SetEnsOffchainResolveMethod(
      local_state_prefs(),
      decentralized_dns::EnsOffchainResolveMethod::kEnabled);

  base::MockCallback<JsonRpcService::EnsGetContentHashCallback> callback;
  EXPECT_CALL(
      callback,
      Run(std::vector<uint8_t>(), false, mojom::ProviderError::kInvalidParams,
          l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS)));
  json_rpc_service_->EnsGetContentHash(ens_subdomain_host(), callback.Get());
  task_environment_.RunUntilIdle();
}

TEST_F(ENSL2JsonRpcServiceUnitTest, GetContentHash_NoResolver) {
  decentralized_dns::SetEnsOffchainResolveMethod(
      local_state_prefs(),
      decentralized_dns::EnsOffchainResolveMethod::kEnabled);

  base::MockCallback<JsonRpcService::EnsGetContentHashCallback> callback;
  EXPECT_CALL(
      callback,
      Run(std::vector<uint8_t>(), false, mojom::ProviderError::kInternalError,
          l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)));
  json_rpc_service_->EnsGetContentHash("unknown-host.eth", callback.Get());
  task_environment_.RunUntilIdle();
}

TEST_F(ENSL2JsonRpcServiceUnitTest, GetContentHash_NoEnsip10Support) {
  decentralized_dns::SetEnsOffchainResolveMethod(
      local_state_prefs(),
      decentralized_dns::EnsOffchainResolveMethod::kEnabled);

  // Turning off Ensip-10 support for resolver so contenthash(bytes32) is
  // called.
  ensip10_support_handler_->DisableSupport();

  base::MockCallback<JsonRpcService::EnsGetContentHashCallback> callback;
  EXPECT_CALL(callback, Run(onchain_contenthash(), false,
                            mojom::ProviderError::kSuccess, ""));
  json_rpc_service_->EnsGetContentHash(ens_host(), callback.Get());
  task_environment_.RunUntilIdle();
}

TEST_F(ENSL2JsonRpcServiceUnitTest,
       GetContentHash_NoEnsip10Support_GoOffchain) {
  decentralized_dns::SetEnsOffchainResolveMethod(
      local_state_prefs(),
      decentralized_dns::EnsOffchainResolveMethod::kEnabled);

  // Turning off Ensip-10 support for resolver so contenthash(bytes32) is
  // called.
  ensip10_support_handler_->DisableSupport();
  // contenthash(bytes32) will go offchain.
  ens_get_record_handler_->RespondWithOffchainLookup(gateway_url());

  base::MockCallback<JsonRpcService::EnsGetContentHashCallback> callback;
  EXPECT_CALL(callback, Run(offchain_contenthash(), false,
                            mojom::ProviderError::kSuccess, ""));
  json_rpc_service_->EnsGetContentHash(ens_host(), callback.Get());
  task_environment_.RunUntilIdle();
}

TEST_F(ENSL2JsonRpcServiceUnitTest, GetContentHash_Gateway500Error) {
  decentralized_dns::SetEnsOffchainResolveMethod(
      local_state_prefs(),
      decentralized_dns::EnsOffchainResolveMethod::kEnabled);

  // Gateway request fails.
  offchain_gateway_handler_->SetRespondWith500();

  base::MockCallback<JsonRpcService::EnsGetContentHashCallback> callback;
  EXPECT_CALL(
      callback,
      Run(std::vector<uint8_t>(), false, mojom::ProviderError::kInternalError,
          l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)));
  json_rpc_service_->EnsGetContentHash(ens_host(), callback.Get());
  task_environment_.RunUntilIdle();
}

TEST_F(ENSL2JsonRpcServiceUnitTest, GetContentHash_GatewayNoRecord) {
  decentralized_dns::SetEnsOffchainResolveMethod(
      local_state_prefs(),
      decentralized_dns::EnsOffchainResolveMethod::kEnabled);

  // No data record in gateway.
  offchain_gateway_handler_->SetRespondWithNoRecord();

  base::MockCallback<JsonRpcService::EnsGetContentHashCallback> callback;
  EXPECT_CALL(
      callback,
      Run(std::vector<uint8_t>(), false, mojom::ProviderError::kInvalidParams,
          l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS)));
  json_rpc_service_->EnsGetContentHash(ens_host(), callback.Get());
  task_environment_.RunUntilIdle();
}

TEST_F(ENSL2JsonRpcServiceUnitTest, GetContentHash_Consent) {
  EXPECT_EQ(
      decentralized_dns::EnsOffchainResolveMethod::kAsk,
      decentralized_dns::GetEnsOffchainResolveMethod(local_state_prefs()));

  // Ask by default.
  {
    base::MockCallback<JsonRpcService::EnsGetContentHashCallback> callback;
    EXPECT_CALL(callback, Run(std::vector<uint8_t>(), true,
                              mojom::ProviderError::kSuccess, ""));
    json_rpc_service_->EnsGetContentHash(ens_host(), callback.Get());
    task_environment_.RunUntilIdle();
  }

  decentralized_dns::SetEnsOffchainResolveMethod(
      local_state_prefs(),
      decentralized_dns::EnsOffchainResolveMethod::kEnabled);
  // Ok when enabled by prefs.
  {
    base::MockCallback<JsonRpcService::EnsGetContentHashCallback> callback;
    EXPECT_CALL(callback, Run(offchain_contenthash(), false,
                              mojom::ProviderError::kSuccess, ""));
    json_rpc_service_->EnsGetContentHash(ens_host(), callback.Get());
    task_environment_.RunUntilIdle();
  }

  // Disable in prefs.
  decentralized_dns::SetEnsOffchainResolveMethod(
      local_state_prefs(),
      decentralized_dns::EnsOffchainResolveMethod::kDisabled);

  // Fails when disabled in prefs.
  {
    base::MockCallback<JsonRpcService::EnsGetContentHashCallback> callback;
    EXPECT_CALL(
        callback,
        Run(std::vector<uint8_t>(), false, mojom::ProviderError::kInternalError,
            l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)));
    json_rpc_service_->EnsGetContentHash(ens_host(), callback.Get());
    task_environment_.RunUntilIdle();
  }
}

class SnsJsonRpcServiceUnitTest : public JsonRpcServiceUnitTest {
 public:
  SnsJsonRpcServiceUnitTest() = default;

  void SetUp() override {
    JsonRpcServiceUnitTest::SetUp();

    domain_owner_public_key_.resize(32);
    domain_owner_private_key_.resize(64);
    uint8_t seed[32] = {};
    ED25519_keypair_from_seed(domain_owner_public_key_.data(),
                              domain_owner_private_key_.data(), seed);

    InitHandlers();

    url_loader_factory_.SetInterceptor(base::BindRepeating(
        &SnsJsonRpcServiceUnitTest::HandleRequest, base::Unretained(this)));
  }

  void InitHandlers() {
    json_rpc_endpoint_handler_ = std::make_unique<JsonRpcEndpointHandler>(
        GetNetwork(mojom::kSolanaMainnet, mojom::CoinType::SOL));

    mint_address_handler_ = std::make_unique<GetAccountInfoHandler>(
        GetMintAddress(), SolanaAddress::ZeroAddress(),
        GetAccountInfoHandler::MakeMintData(1));

    get_program_accounts_handler_ = std::make_unique<GetProgramAccountsHandler>(
        *SolanaAddress::FromBase58(mojom::kSolanaTokenProgramId),
        GetTokenAccountAddress(),
        GetProgramAccountsHandler::MakeTokenAccountData(GetMintAddress(),
                                                        NftOwnerAddress()));

    domain_address_handler_ = std::make_unique<GetAccountInfoHandler>(
        GetDomainKeyAddress(), SolanaAddress::ZeroAddress(),
        GetAccountInfoHandler::MakeNameRegistryStateData(DomainOwnerAddress()));

    sol_record_v1_address_handler_ = std::make_unique<GetAccountInfoHandler>(
        GetRecordV1KeyAddress(kSnsSolRecord), SolanaAddress::ZeroAddress(),
        GetAccountInfoHandler::MakeNameRegistryStateData(
            DomainOwnerAddress(),
            GetAccountInfoHandler::MakeSolRecordV1PayloadData(
                SolRecordAddressV1(), GetRecordV1KeyAddress("SOL"),
                domain_owner_private_key_)));

    url_record_v1_address_handler_ = std::make_unique<GetAccountInfoHandler>(
        GetRecordV1KeyAddress(kSnsUrlRecord), SolanaAddress::ZeroAddress(),
        GetAccountInfoHandler::MakeNameRegistryStateData(
            DomainOwnerAddress(),
            GetAccountInfoHandler::MakeTextRecordV1PayloadData(
                UrlValueV1().spec())));

    ipfs_record_v1_address_handler_ = std::make_unique<GetAccountInfoHandler>(
        GetRecordV1KeyAddress(kSnsIpfsRecord), SolanaAddress::ZeroAddress(),
        GetAccountInfoHandler::MakeNameRegistryStateData(
            DomainOwnerAddress(),
            GetAccountInfoHandler::MakeTextRecordV1PayloadData(
                IpfsValueV1().spec())));

    sol_record_v2_address_handler_ = std::make_unique<GetAccountInfoHandler>(
        GetRecordV2KeyAddress(kSnsSolRecord), SolanaAddress::ZeroAddress(),
        GetAccountInfoHandler::MakeNameRegistryStateData(
            DomainOwnerAddress(),
            GetAccountInfoHandler::MakeSolRecordV2PayloadData(
                SnsRecordV2ValidationType::kSolana, DomainOwnerAddress(),
                SnsRecordV2ValidationType::kSolana, SolRecordAddressV2(),
                SolRecordAddressV2())));

    url_record_v2_address_handler_ = std::make_unique<GetAccountInfoHandler>(
        GetRecordV2KeyAddress(kSnsUrlRecord), SolanaAddress::ZeroAddress(),
        GetAccountInfoHandler::MakeNameRegistryStateData(
            DomainOwnerAddress(),
            GetAccountInfoHandler::MakeTextRecordV2PayloadData(
                SnsRecordV2ValidationType::kSolana, DomainOwnerAddress(),
                UrlValueV2().spec())));

    ipfs_record_v2_address_handler_ = std::make_unique<GetAccountInfoHandler>(
        GetRecordV2KeyAddress(kSnsIpfsRecord), SolanaAddress::ZeroAddress(),
        GetAccountInfoHandler::MakeNameRegistryStateData(
            DomainOwnerAddress(),
            GetAccountInfoHandler::MakeTextRecordV2PayloadData(
                SnsRecordV2ValidationType::kSolana, DomainOwnerAddress(),
                IpfsValueV2().spec())));

    default_handler_ = std::make_unique<GetAccountInfoHandler>();

    json_rpc_endpoint_handler_->AddSolRpcCallHandler(
        mint_address_handler_.get());
    json_rpc_endpoint_handler_->AddSolRpcCallHandler(
        get_program_accounts_handler_.get());

    json_rpc_endpoint_handler_->AddSolRpcCallHandler(
        domain_address_handler_.get());

    json_rpc_endpoint_handler_->AddSolRpcCallHandler(
        sol_record_v1_address_handler_.get());
    json_rpc_endpoint_handler_->AddSolRpcCallHandler(
        url_record_v1_address_handler_.get());
    json_rpc_endpoint_handler_->AddSolRpcCallHandler(
        ipfs_record_v1_address_handler_.get());

    json_rpc_endpoint_handler_->AddSolRpcCallHandler(
        sol_record_v2_address_handler_.get());
    json_rpc_endpoint_handler_->AddSolRpcCallHandler(
        url_record_v2_address_handler_.get());
    json_rpc_endpoint_handler_->AddSolRpcCallHandler(
        ipfs_record_v2_address_handler_.get());

    json_rpc_endpoint_handler_->AddSolRpcCallHandler(default_handler_.get());
  }

  SolanaAddress GetDomainKeyAddress() const {
    return *GetDomainKey(sns_host());
  }

  SolanaAddress GetRecordV1KeyAddress(const std::string& record) const {
    return *GetRecordKey(sns_host(), record, SnsRecordsVersion::kRecordsV1);
  }

  SolanaAddress GetRecordV2KeyAddress(const std::string& record) const {
    return *GetRecordKey(sns_host(), record, SnsRecordsVersion::kRecordsV2);
  }

  SolanaAddress GetMintAddress() const {
    return *brave_wallet::GetMintAddress(GetDomainKeyAddress());
  }

  SolanaAddress GetTokenAccountAddress() const {
    return *SolanaAddress::FromBase58(
        "TokentAccount111111111111111111111111111111");
  }

  SolanaAddress NftOwnerAddress() const {
    return *SolanaAddress::FromBase58(
        "NftPwner11111111111111111111111111111111111");
  }

  SolanaAddress DomainOwnerAddress() const {
    return *SolanaAddress::FromBytes(domain_owner_public_key_);
  }

  SolanaAddress SolRecordAddressV1() const {
    return *SolanaAddress::FromBase58(
        "Rec1Pwner1111111111111111111111111111111111");
  }

  SolanaAddress SolRecordAddressV2() const {
    return *SolanaAddress::FromBase58(
        "Rec2Pwner1111111111111111111111111111111111");
  }

  GURL UrlValueV1() const { return GURL("https://v1.brave.com"); }
  GURL IpfsValueV1() const {
    return GURL(
        "ipfs://v1fybeibd4ala53bs26dvygofvr6ahpa7gbw4eyaibvrbivf4l5rr44yqu4");
  }

  GURL UrlValueV2() const { return GURL("https://v2.brave.com"); }
  GURL IpfsValueV2() const {
    return GURL(
        "ipfs://v2fybeibd4ala53bs26dvygofvr6ahpa7gbw4eyaibvrbivf4l5rr44yqu4");
  }

  std::string sns_host() const { return "sub.test.sol"; }

  void DisableV2Handlers() {
    sol_record_v2_address_handler_->Disable();
    url_record_v2_address_handler_->Disable();
    ipfs_record_v2_address_handler_->Disable();
  }

 protected:
  void HandleRequest(const network::ResourceRequest& request) {
    url_loader_factory_.ClearResponses();
    if (auto json_response =
            json_rpc_endpoint_handler_->HandleRequest(request)) {
      if (*json_response == "timeout") {
        url_loader_factory_.AddResponse(request.url.spec(), "",
                                        net::HTTP_REQUEST_TIMEOUT);
      } else {
        url_loader_factory_.AddResponse(request.url.spec(), *json_response);
      }
    }
  }

  std::vector<uint8_t> domain_owner_public_key_;
  std::vector<uint8_t> domain_owner_private_key_;

  std::unique_ptr<GetAccountInfoHandler> mint_address_handler_;
  std::unique_ptr<GetProgramAccountsHandler> get_program_accounts_handler_;
  std::unique_ptr<GetAccountInfoHandler> domain_address_handler_;
  std::unique_ptr<GetAccountInfoHandler> sol_record_v1_address_handler_;
  std::unique_ptr<GetAccountInfoHandler> url_record_v1_address_handler_;
  std::unique_ptr<GetAccountInfoHandler> ipfs_record_v1_address_handler_;
  std::unique_ptr<GetAccountInfoHandler> sol_record_v2_address_handler_;
  std::unique_ptr<GetAccountInfoHandler> url_record_v2_address_handler_;
  std::unique_ptr<GetAccountInfoHandler> ipfs_record_v2_address_handler_;
  std::unique_ptr<GetAccountInfoHandler> default_handler_;

  std::unique_ptr<JsonRpcEndpointHandler> json_rpc_endpoint_handler_;
};

TEST_F(SnsJsonRpcServiceUnitTest, GetWalletAddr_NftOwner) {
  // Has nft for domain. Return nft owner.
  base::MockCallback<JsonRpcService::SnsGetSolAddrCallback> callback;
  EXPECT_CALL(callback, Run(NftOwnerAddress().ToBase58(),
                            mojom::SolanaProviderError::kSuccess, ""));
  json_rpc_service_->SnsGetSolAddr(sns_host(), callback.Get());
  WaitAndVerify(&callback);

  // HTTP error while checking nft mint. Fail resolution.
  mint_address_handler_->FailWithTimeout();
  EXPECT_CALL(callback,
              Run("", mojom::SolanaProviderError::kInternalError,
                  l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)));
  json_rpc_service_->SnsGetSolAddr(sns_host(), callback.Get());
  WaitAndVerify(&callback);
  mint_address_handler_->FailWithTimeout(false);

  // HTTP error while checking nft owner. Fail resolution.
  get_program_accounts_handler_->FailWithTimeout();
  EXPECT_CALL(callback,
              Run("", mojom::SolanaProviderError::kInternalError,
                  l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)));
  json_rpc_service_->SnsGetSolAddr(sns_host(), callback.Get());
  WaitAndVerify(&callback);
  get_program_accounts_handler_->FailWithTimeout(false);

  // Domain detokenized. Fallback to domain/SOL owner.
  mint_address_handler_->data() = GetAccountInfoHandler::MakeMintData(0);
  EXPECT_CALL(callback, Run(SolRecordAddressV2().ToBase58(),
                            mojom::SolanaProviderError::kSuccess, ""));
  json_rpc_service_->SnsGetSolAddr(sns_host(), callback.Get());
  WaitAndVerify(&callback);
}

TEST_F(SnsJsonRpcServiceUnitTest, GetWalletAddr_DomainOwner) {
  DisableV2Handlers();  // Legacy v1 records test.
  mint_address_handler_->Disable();
  sol_record_v1_address_handler_->Disable();

  // No nft, no SOL record. Return domain owner address.
  base::MockCallback<JsonRpcService::SnsGetSolAddrCallback> callback;
  EXPECT_CALL(callback, Run(DomainOwnerAddress().ToBase58(),
                            mojom::SolanaProviderError::kSuccess, ""));
  json_rpc_service_->SnsGetSolAddr(sns_host(), callback.Get());
  WaitAndVerify(&callback);

  // HTTP error for domain key account. Fail resolution.
  domain_address_handler_->FailWithTimeout();
  EXPECT_CALL(callback,
              Run("", mojom::SolanaProviderError::kInternalError,
                  l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)));
  json_rpc_service_->SnsGetSolAddr(sns_host(), callback.Get());
  WaitAndVerify(&callback);
  domain_address_handler_->FailWithTimeout(false);

  // No domain key account. Fail resolution.
  domain_address_handler_->Disable();
  EXPECT_CALL(callback,
              Run("", mojom::SolanaProviderError::kInternalError,
                  l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)));
  json_rpc_service_->SnsGetSolAddr(sns_host(), callback.Get());
  WaitAndVerify(&callback);
}

TEST_F(SnsJsonRpcServiceUnitTest, GetWalletAddr_SolRecordOwner) {
  DisableV2Handlers();  // Legacy v1 records test.
  mint_address_handler_->Disable();

  // No nft, has sol record. Return address from SOL record.
  base::MockCallback<JsonRpcService::SnsGetSolAddrCallback> callback;
  EXPECT_CALL(callback, Run(SolRecordAddressV1().ToBase58(),
                            mojom::SolanaProviderError::kSuccess, ""));
  json_rpc_service_->SnsGetSolAddr(sns_host(), callback.Get());
  WaitAndVerify(&callback);

  // Bad signature. Fallback to owner address.
  sol_record_v1_address_handler_->data()[170] ^= 123;
  EXPECT_CALL(callback, Run(DomainOwnerAddress().ToBase58(),
                            mojom::SolanaProviderError::kSuccess, ""));
  json_rpc_service_->SnsGetSolAddr(sns_host(), callback.Get());
  WaitAndVerify(&callback);
  sol_record_v1_address_handler_->data()[170] ^= 123;

  // HTTP error for SOL record key account. Fail resolution.
  sol_record_v1_address_handler_->FailWithTimeout();
  EXPECT_CALL(callback,
              Run("", mojom::SolanaProviderError::kInternalError,
                  l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)));
  json_rpc_service_->SnsGetSolAddr(sns_host(), callback.Get());
  WaitAndVerify(&callback);
  sol_record_v1_address_handler_->FailWithTimeout(false);

  // No SOL record account. Fallback to owner address.
  sol_record_v1_address_handler_->Disable();
  EXPECT_CALL(callback, Run(DomainOwnerAddress().ToBase58(),
                            mojom::SolanaProviderError::kSuccess, ""));
  json_rpc_service_->SnsGetSolAddr(sns_host(), callback.Get());
  WaitAndVerify(&callback);
}

TEST_F(SnsJsonRpcServiceUnitTest, GetWalletAddr_V2Record) {
  mint_address_handler_->Disable();

  // No nft, has sol v2 record. Return address from SOLv2 record.
  base::MockCallback<JsonRpcService::SnsGetSolAddrCallback> callback;
  EXPECT_CALL(callback, Run(SolRecordAddressV2().ToBase58(),
                            mojom::SolanaProviderError::kSuccess, ""));
  json_rpc_service_->SnsGetSolAddr(sns_host(), callback.Get());
  WaitAndVerify(&callback);

  // Disable v2 record - fallback to v1.
  sol_record_v2_address_handler_->Disable();
  EXPECT_CALL(callback, Run(SolRecordAddressV1().ToBase58(),
                            mojom::SolanaProviderError::kSuccess, ""));
  json_rpc_service_->SnsGetSolAddr(sns_host(), callback.Get());
  WaitAndVerify(&callback);

  // No SOL v1 record account. Fallback to owner address.
  sol_record_v1_address_handler_->Disable();
  EXPECT_CALL(callback, Run(DomainOwnerAddress().ToBase58(),
                            mojom::SolanaProviderError::kSuccess, ""));
  json_rpc_service_->SnsGetSolAddr(sns_host(), callback.Get());
  WaitAndVerify(&callback);
}

TEST_F(SnsJsonRpcServiceUnitTest, GetWalletAddr_V2Record_StalenessCheck) {
  mint_address_handler_->Disable();

  // Return address from SOLv2 record by default.
  base::MockCallback<JsonRpcService::SnsGetSolAddrCallback> callback;
  EXPECT_CALL(callback, Run(SolRecordAddressV2().ToBase58(),
                            mojom::SolanaProviderError::kSuccess, ""));
  json_rpc_service_->SnsGetSolAddr(sns_host(), callback.Get());
  WaitAndVerify(&callback);

  // kNone staleness - fallback to next record
  sol_record_v2_address_handler_->Reset(
      GetRecordV2KeyAddress(kSnsSolRecord), SolanaAddress::ZeroAddress(),
      GetAccountInfoHandler::MakeNameRegistryStateData(
          DomainOwnerAddress(),
          GetAccountInfoHandler::MakeSolRecordV2PayloadData(
              SnsRecordV2ValidationType::kNone, std::nullopt,
              SnsRecordV2ValidationType::kSolana, SolRecordAddressV2(),
              SolRecordAddressV2())));
  EXPECT_CALL(callback, Run(SolRecordAddressV1().ToBase58(),
                            mojom::SolanaProviderError::kSuccess, ""));
  json_rpc_service_->SnsGetSolAddr(sns_host(), callback.Get());
  WaitAndVerify(&callback);

  // kEthereum staleness - fallback to next record
  sol_record_v2_address_handler_->Reset(
      GetRecordV2KeyAddress(kSnsSolRecord), SolanaAddress::ZeroAddress(),
      GetAccountInfoHandler::MakeNameRegistryStateData(
          DomainOwnerAddress(),
          GetAccountInfoHandler::MakeSolRecordV2PayloadData(
              SnsRecordV2ValidationType::kEthereum, std::nullopt,
              SnsRecordV2ValidationType::kSolana, SolRecordAddressV2(),
              SolRecordAddressV2())));
  EXPECT_CALL(callback, Run(SolRecordAddressV1().ToBase58(),
                            mojom::SolanaProviderError::kSuccess, ""));
  json_rpc_service_->SnsGetSolAddr(sns_host(), callback.Get());
  WaitAndVerify(&callback);

  // kSolanaUnverified staleness - fallback to next record
  sol_record_v2_address_handler_->Reset(
      GetRecordV2KeyAddress(kSnsSolRecord), SolanaAddress::ZeroAddress(),
      GetAccountInfoHandler::MakeNameRegistryStateData(
          DomainOwnerAddress(),
          GetAccountInfoHandler::MakeSolRecordV2PayloadData(
              SnsRecordV2ValidationType::kSolanaUnverified, std::nullopt,
              SnsRecordV2ValidationType::kSolana, SolRecordAddressV2(),
              SolRecordAddressV2())));
  EXPECT_CALL(callback, Run(SolRecordAddressV1().ToBase58(),
                            mojom::SolanaProviderError::kSuccess, ""));
  json_rpc_service_->SnsGetSolAddr(sns_host(), callback.Get());
  WaitAndVerify(&callback);

  // kSolana staleness with invalid stalenss id - fallback to next record
  sol_record_v2_address_handler_->Reset(
      GetRecordV2KeyAddress(kSnsSolRecord), SolanaAddress::ZeroAddress(),
      GetAccountInfoHandler::MakeNameRegistryStateData(
          DomainOwnerAddress(),
          GetAccountInfoHandler::MakeSolRecordV2PayloadData(
              SnsRecordV2ValidationType::kSolana, SolanaAddress::ZeroAddress(),
              SnsRecordV2ValidationType::kSolana, SolRecordAddressV2(),
              SolRecordAddressV2())));
  EXPECT_CALL(callback, Run(SolRecordAddressV1().ToBase58(),
                            mojom::SolanaProviderError::kSuccess, ""));
  json_rpc_service_->SnsGetSolAddr(sns_host(), callback.Get());
  WaitAndVerify(&callback);
}

TEST_F(SnsJsonRpcServiceUnitTest, GetWalletAddr_V2Record_RoaCheck) {
  mint_address_handler_->Disable();

  // Return address from SOLv2 record by default.
  base::MockCallback<JsonRpcService::SnsGetSolAddrCallback> callback;
  EXPECT_CALL(callback, Run(SolRecordAddressV2().ToBase58(),
                            mojom::SolanaProviderError::kSuccess, ""));
  json_rpc_service_->SnsGetSolAddr(sns_host(), callback.Get());
  WaitAndVerify(&callback);

  // kNone roa - fallback to next record
  sol_record_v2_address_handler_->Reset(
      GetRecordV2KeyAddress(kSnsSolRecord), SolanaAddress::ZeroAddress(),
      GetAccountInfoHandler::MakeNameRegistryStateData(
          DomainOwnerAddress(),
          GetAccountInfoHandler::MakeSolRecordV2PayloadData(
              SnsRecordV2ValidationType::kSolana, DomainOwnerAddress(),
              SnsRecordV2ValidationType::kNone, std::nullopt,
              SolRecordAddressV2())));
  EXPECT_CALL(callback, Run(SolRecordAddressV1().ToBase58(),
                            mojom::SolanaProviderError::kSuccess, ""));
  json_rpc_service_->SnsGetSolAddr(sns_host(), callback.Get());
  WaitAndVerify(&callback);

  // kEthereum roa - fallback to next record
  sol_record_v2_address_handler_->Reset(
      GetRecordV2KeyAddress(kSnsSolRecord), SolanaAddress::ZeroAddress(),
      GetAccountInfoHandler::MakeNameRegistryStateData(
          DomainOwnerAddress(),
          GetAccountInfoHandler::MakeSolRecordV2PayloadData(
              SnsRecordV2ValidationType::kSolana, DomainOwnerAddress(),
              SnsRecordV2ValidationType::kEthereum, std::nullopt,
              SolRecordAddressV2())));
  EXPECT_CALL(callback, Run(SolRecordAddressV1().ToBase58(),
                            mojom::SolanaProviderError::kSuccess, ""));
  json_rpc_service_->SnsGetSolAddr(sns_host(), callback.Get());
  WaitAndVerify(&callback);

  // kSolanaUnverified roa - fallback to next record
  sol_record_v2_address_handler_->Reset(
      GetRecordV2KeyAddress(kSnsSolRecord), SolanaAddress::ZeroAddress(),
      GetAccountInfoHandler::MakeNameRegistryStateData(
          DomainOwnerAddress(),
          GetAccountInfoHandler::MakeSolRecordV2PayloadData(
              SnsRecordV2ValidationType::kSolana, DomainOwnerAddress(),
              SnsRecordV2ValidationType::kSolanaUnverified, std::nullopt,
              SolRecordAddressV2())));
  EXPECT_CALL(callback, Run(SolRecordAddressV1().ToBase58(),
                            mojom::SolanaProviderError::kSuccess, ""));
  json_rpc_service_->SnsGetSolAddr(sns_host(), callback.Get());
  WaitAndVerify(&callback);

  // kSolana roa with invalid roa id - fallback to next record
  sol_record_v2_address_handler_->Reset(
      GetRecordV2KeyAddress(kSnsSolRecord), SolanaAddress::ZeroAddress(),
      GetAccountInfoHandler::MakeNameRegistryStateData(
          DomainOwnerAddress(),
          GetAccountInfoHandler::MakeSolRecordV2PayloadData(
              SnsRecordV2ValidationType::kSolana, DomainOwnerAddress(),
              SnsRecordV2ValidationType::kSolana, SolanaAddress::ZeroAddress(),
              SolRecordAddressV2())));
  EXPECT_CALL(callback, Run(SolRecordAddressV1().ToBase58(),
                            mojom::SolanaProviderError::kSuccess, ""));
  json_rpc_service_->SnsGetSolAddr(sns_host(), callback.Get());
  WaitAndVerify(&callback);
}

TEST_F(SnsJsonRpcServiceUnitTest, ResolveHost_UrlValue) {
  DisableV2Handlers();  // Legacy v1 records test.

  base::MockCallback<JsonRpcService::SnsResolveHostCallback> callback;
  EXPECT_CALL(callback, Run(testing::Eq(UrlValueV1()),
                            mojom::SolanaProviderError::kSuccess, ""));
  json_rpc_service_->SnsResolveHost(sns_host(), callback.Get());
  WaitAndVerify(&callback);

  // HTTP error for url record account. Fail resolution.
  url_record_v1_address_handler_->FailWithTimeout();
  EXPECT_CALL(
      callback,
      Run(testing::Eq(GURL()), mojom::SolanaProviderError::kInternalError,
          l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)));
  json_rpc_service_->SnsResolveHost(sns_host(), callback.Get());
  WaitAndVerify(&callback);
  url_record_v1_address_handler_->FailWithTimeout(false);
}

TEST_F(SnsJsonRpcServiceUnitTest, ResolveHost_IpfsValue) {
  DisableV2Handlers();  // Legacy v1 records test.

  url_record_v1_address_handler_->Disable();

  // No url record. Will return ipfs record.
  base::MockCallback<JsonRpcService::SnsResolveHostCallback> callback;
  EXPECT_CALL(callback, Run(testing::Eq(IpfsValueV1()),
                            mojom::SolanaProviderError::kSuccess, ""));
  json_rpc_service_->SnsResolveHost(sns_host(), callback.Get());
  WaitAndVerify(&callback);

  // HTTP error for ipfs record account. Fail resolution.
  ipfs_record_v1_address_handler_->FailWithTimeout();
  EXPECT_CALL(
      callback,
      Run(testing::Eq(GURL()), mojom::SolanaProviderError::kInternalError,
          l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)));
  json_rpc_service_->SnsResolveHost(sns_host(), callback.Get());
  WaitAndVerify(&callback);
  ipfs_record_v1_address_handler_->FailWithTimeout(false);

  // No ipfs record account. Fail resolution.
  ipfs_record_v1_address_handler_->Disable();
  EXPECT_CALL(
      callback,
      Run(testing::Eq(GURL()), mojom::SolanaProviderError::kInternalError,
          l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)));
  json_rpc_service_->SnsResolveHost(sns_host(), callback.Get());
  WaitAndVerify(&callback);
}

TEST_F(SnsJsonRpcServiceUnitTest, ResolveHost_V2Records) {
  base::MockCallback<JsonRpcService::SnsResolveHostCallback> callback;

  // Test with nft disabled as domain owner is used as staleness id by default
  // in tests.
  mint_address_handler_->Disable();

  EXPECT_CALL(callback, Run(testing::Eq(UrlValueV2()),
                            mojom::SolanaProviderError::kSuccess, ""));
  json_rpc_service_->SnsResolveHost(sns_host(), callback.Get());
  WaitAndVerify(&callback);

  url_record_v2_address_handler_->Disable();
  EXPECT_CALL(callback, Run(testing::Eq(IpfsValueV2()),
                            mojom::SolanaProviderError::kSuccess, ""));
  json_rpc_service_->SnsResolveHost(sns_host(), callback.Get());
  WaitAndVerify(&callback);

  ipfs_record_v2_address_handler_->Disable();
  EXPECT_CALL(callback, Run(testing::Eq(UrlValueV1()),
                            mojom::SolanaProviderError::kSuccess, ""));
  json_rpc_service_->SnsResolveHost(sns_host(), callback.Get());
  WaitAndVerify(&callback);

  url_record_v1_address_handler_->Disable();
  EXPECT_CALL(callback, Run(testing::Eq(IpfsValueV1()),
                            mojom::SolanaProviderError::kSuccess, ""));
  json_rpc_service_->SnsResolveHost(sns_host(), callback.Get());
  WaitAndVerify(&callback);

  ipfs_record_v1_address_handler_->Disable();
  EXPECT_CALL(
      callback,
      Run(testing::Eq(GURL()), mojom::SolanaProviderError::kInternalError,
          l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)));
  json_rpc_service_->SnsResolveHost(sns_host(), callback.Get());
  WaitAndVerify(&callback);

  InitHandlers();
  mint_address_handler_->Enable();

  // Falls back to V1 url record as current owner is an nft owner, but record's
  // staleness id is set to domain owner.
  EXPECT_CALL(callback, Run(testing::Eq(UrlValueV1()),
                            mojom::SolanaProviderError::kSuccess, ""));
  json_rpc_service_->SnsResolveHost(sns_host(), callback.Get());
  WaitAndVerify(&callback);

  // Falls back to V1 url record as current owner is an nft owner, but record's
  // staleness id is set to domain owner.
  url_record_v2_address_handler_->Disable();
  EXPECT_CALL(callback, Run(testing::Eq(UrlValueV1()),
                            mojom::SolanaProviderError::kSuccess, ""));
  json_rpc_service_->SnsResolveHost(sns_host(), callback.Get());
  WaitAndVerify(&callback);

  ipfs_record_v2_address_handler_->Disable();
  EXPECT_CALL(callback, Run(testing::Eq(UrlValueV1()),
                            mojom::SolanaProviderError::kSuccess, ""));
  json_rpc_service_->SnsResolveHost(sns_host(), callback.Get());
  WaitAndVerify(&callback);

  url_record_v1_address_handler_->Disable();
  EXPECT_CALL(callback, Run(testing::Eq(IpfsValueV1()),
                            mojom::SolanaProviderError::kSuccess, ""));
  json_rpc_service_->SnsResolveHost(sns_host(), callback.Get());
  WaitAndVerify(&callback);

  ipfs_record_v1_address_handler_->Disable();
  EXPECT_CALL(
      callback,
      Run(testing::Eq(GURL()), mojom::SolanaProviderError::kInternalError,
          l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)));
  json_rpc_service_->SnsResolveHost(sns_host(), callback.Get());
  WaitAndVerify(&callback);

  InitHandlers();
  mint_address_handler_->Enable();
  // setup handlers to use nft owner as staleness id.
  url_record_v2_address_handler_->Reset(
      GetRecordV2KeyAddress(kSnsUrlRecord), SolanaAddress::ZeroAddress(),
      GetAccountInfoHandler::MakeNameRegistryStateData(
          DomainOwnerAddress(),
          GetAccountInfoHandler::MakeTextRecordV2PayloadData(
              SnsRecordV2ValidationType::kSolana, NftOwnerAddress(),
              UrlValueV2().spec())));
  ipfs_record_v2_address_handler_->Reset(
      GetRecordV2KeyAddress(kSnsIpfsRecord), SolanaAddress::ZeroAddress(),
      GetAccountInfoHandler::MakeNameRegistryStateData(
          DomainOwnerAddress(),
          GetAccountInfoHandler::MakeTextRecordV2PayloadData(
              SnsRecordV2ValidationType::kSolana, NftOwnerAddress(),
              IpfsValueV2().spec())));

  EXPECT_CALL(callback, Run(testing::Eq(UrlValueV2()),
                            mojom::SolanaProviderError::kSuccess, ""));
  json_rpc_service_->SnsResolveHost(sns_host(), callback.Get());
  WaitAndVerify(&callback);

  url_record_v2_address_handler_->Disable();
  EXPECT_CALL(callback, Run(testing::Eq(IpfsValueV2()),
                            mojom::SolanaProviderError::kSuccess, ""));
  json_rpc_service_->SnsResolveHost(sns_host(), callback.Get());
  WaitAndVerify(&callback);

  ipfs_record_v2_address_handler_->Disable();
  EXPECT_CALL(callback, Run(testing::Eq(UrlValueV1()),
                            mojom::SolanaProviderError::kSuccess, ""));
  json_rpc_service_->SnsResolveHost(sns_host(), callback.Get());
  WaitAndVerify(&callback);

  url_record_v1_address_handler_->Disable();
  EXPECT_CALL(callback, Run(testing::Eq(IpfsValueV1()),
                            mojom::SolanaProviderError::kSuccess, ""));
  json_rpc_service_->SnsResolveHost(sns_host(), callback.Get());
  WaitAndVerify(&callback);

  ipfs_record_v1_address_handler_->Disable();
  EXPECT_CALL(
      callback,
      Run(testing::Eq(GURL()), mojom::SolanaProviderError::kInternalError,
          l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)));
  json_rpc_service_->SnsResolveHost(sns_host(), callback.Get());
  WaitAndVerify(&callback);
}

TEST_F(SnsJsonRpcServiceUnitTest, ResolveHost_V2Records_StalenessCheck) {
  base::MockCallback<JsonRpcService::SnsResolveHostCallback> callback;
  // Test with nft disabled as domain owner is used as staleness id by default
  // in tests.
  mint_address_handler_->Disable();

  // V2 url record by default.
  EXPECT_CALL(callback, Run(testing::Eq(UrlValueV2()),
                            mojom::SolanaProviderError::kSuccess, ""));
  json_rpc_service_->SnsResolveHost(sns_host(), callback.Get());
  WaitAndVerify(&callback);

  // kNone staleness - fallback to next record
  url_record_v2_address_handler_->Reset(
      GetRecordV2KeyAddress(kSnsUrlRecord), SolanaAddress::ZeroAddress(),
      GetAccountInfoHandler::MakeNameRegistryStateData(
          DomainOwnerAddress(),
          GetAccountInfoHandler::MakeTextRecordV2PayloadData(
              SnsRecordV2ValidationType::kNone, std::nullopt,
              UrlValueV2().spec())));

  EXPECT_CALL(callback, Run(testing::Eq(IpfsValueV2()),
                            mojom::SolanaProviderError::kSuccess, ""));
  json_rpc_service_->SnsResolveHost(sns_host(), callback.Get());
  WaitAndVerify(&callback);

  // kEthereum staleness - fallback to next record
  url_record_v2_address_handler_->Reset(
      GetRecordV2KeyAddress(kSnsUrlRecord), SolanaAddress::ZeroAddress(),
      GetAccountInfoHandler::MakeNameRegistryStateData(
          DomainOwnerAddress(),
          GetAccountInfoHandler::MakeTextRecordV2PayloadData(
              SnsRecordV2ValidationType::kEthereum, std::nullopt,
              UrlValueV2().spec())));

  EXPECT_CALL(callback, Run(testing::Eq(IpfsValueV2()),
                            mojom::SolanaProviderError::kSuccess, ""));
  json_rpc_service_->SnsResolveHost(sns_host(), callback.Get());
  WaitAndVerify(&callback);

  // kSolanaUnverified staleness - fallback to next record
  url_record_v2_address_handler_->Reset(
      GetRecordV2KeyAddress(kSnsUrlRecord), SolanaAddress::ZeroAddress(),
      GetAccountInfoHandler::MakeNameRegistryStateData(
          DomainOwnerAddress(),
          GetAccountInfoHandler::MakeTextRecordV2PayloadData(
              SnsRecordV2ValidationType::kSolanaUnverified, std::nullopt,
              UrlValueV2().spec())));

  EXPECT_CALL(callback, Run(testing::Eq(IpfsValueV2()),
                            mojom::SolanaProviderError::kSuccess, ""));
  json_rpc_service_->SnsResolveHost(sns_host(), callback.Get());
  WaitAndVerify(&callback);

  // kSolana staleness, but address doesn't match owner - fallback to next
  // record
  url_record_v2_address_handler_->Reset(
      GetRecordV2KeyAddress(kSnsUrlRecord), SolanaAddress::ZeroAddress(),
      GetAccountInfoHandler::MakeNameRegistryStateData(
          DomainOwnerAddress(),
          GetAccountInfoHandler::MakeTextRecordV2PayloadData(
              SnsRecordV2ValidationType::kSolana, SolanaAddress::ZeroAddress(),
              UrlValueV2().spec())));

  EXPECT_CALL(callback, Run(testing::Eq(IpfsValueV2()),
                            mojom::SolanaProviderError::kSuccess, ""));
  json_rpc_service_->SnsResolveHost(sns_host(), callback.Get());
  WaitAndVerify(&callback);
}

TEST_F(SnsJsonRpcServiceUnitTest, ResolveHost_V2Records_NetworkError) {
  base::MockCallback<JsonRpcService::SnsResolveHostCallback> callback;
  // Test with nft disabled as domain owner is used as staleness id by default
  // in tests.
  mint_address_handler_->Disable();

  // V2 url record by default.
  EXPECT_CALL(callback, Run(testing::Eq(UrlValueV2()),
                            mojom::SolanaProviderError::kSuccess, ""));
  json_rpc_service_->SnsResolveHost(sns_host(), callback.Get());
  WaitAndVerify(&callback);

  // Network error fails whole resolve process.
  url_record_v2_address_handler_->FailWithTimeout(true);

  EXPECT_CALL(
      callback,
      Run(testing::Eq(GURL()), mojom::SolanaProviderError::kInternalError,
          l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)));
  json_rpc_service_->SnsResolveHost(sns_host(), callback.Get());
  WaitAndVerify(&callback);
}

TEST_F(JsonRpcServiceUnitTest, EthGetLogs) {
  base::Value::List contract_addresses;
  base::Value::List topics;

  // Invalid network ID yields internal error
  TestEthGetLogs("0xinvalid", "earliest", "latest", contract_addresses.Clone(),
                 topics.Clone(), {}, mojom::ProviderError::kInternalError,
                 l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));

  // Non 200 response yields internal error
  SetHTTPRequestTimeoutInterceptor();
  TestEthGetLogs(mojom::kMainnetChainId, "earliest", "latest",
                 contract_addresses.Clone(), topics.Clone(), {},
                 mojom::ProviderError::kInternalError,
                 l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));

  // Invalid response body yields parsing error
  SetInvalidJsonInterceptor();
  TestEthGetLogs(mojom::kMainnetChainId, "earliest", "latest",
                 contract_addresses.Clone(), topics.Clone(), {},
                 mojom::ProviderError::kParsingError,
                 l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));

  // Valid request yields parsed Logs
  const std::string response = R"({
      "jsonrpc":"2.0",
      "id":1,
      "result":[
        {
          "address":"0x6B175474E89094C44Da98b954EedeAC495271d0F",
          "blockHash":"0x2961ceb6c16bab72a55f79e394a35f2bf1c62b30446e3537280f7c22c3115e6e",
          "blockNumber":"0xd6464e",
          "data":"0x00000000000000000000000000000000000000000000000555aff1f0fae8c000",
          "logIndex":"0x159",
          "removed":false,
          "topics":[
            "0xddf252ad1be2c89b69c2b068fc378daa952ba7f163c4a11628f55a4df523b3ef",
            "0x000000000000000000000000503828976d22510aad0201ac7ec88293211d23da",
            "0x000000000000000000000000b4b2802129071b2b9ebb8cbb01ea1e4d14b34961"
          ],
          "transactionHash":"0x2e652b70966c6a05f4b3e68f20d6540b7a5ab712385464a7ccf62774d39b7066",
          "transactionIndex":"0x9f"
        }
      ]
    })";

  Log expected_log;
  expected_log.address = "0x6B175474E89094C44Da98b954EedeAC495271d0F";
  expected_log.block_hash =
      "0x2961ceb6c16bab72a55f79e394a35f2bf1c62b30446e3537280f7c22c3115e6e";
  uint256_t expected_block_number = 14042702;
  expected_log.block_number = expected_block_number;
  expected_log.data =
      "0x00000000000000000000000000000000000000000000000555aff1f0fae8c000";
  uint32_t expected_log_index = 345;
  expected_log.log_index = expected_log_index;
  expected_log.removed = false;
  std::vector<std::string> expected_topics = {
      "0xddf252ad1be2c89b69c2b068fc378daa952ba7f163c4a11628f55a4df523b3ef",
      "0x000000000000000000000000503828976d22510aad0201ac7ec88293211d23da",
      "0x000000000000000000000000b4b2802129071b2b9ebb8cbb01ea1e4d14b34961"};
  expected_log.topics = std::move(expected_topics);
  expected_log.transaction_hash =
      "0x2e652b70966c6a05f4b3e68f20d6540b7a5ab712385464a7ccf62774d39b7066";
  uint32_t expected_transaction_index = 159;
  expected_log.transaction_index = expected_transaction_index;
  std::vector<Log> expected_logs;
  expected_logs.push_back(std::move(expected_log));
  SetInterceptor(GetNetwork(mojom::kMainnetChainId, mojom::CoinType::ETH),
                 "eth_getLogs", "", response);
  TestEthGetLogs(mojom::kMainnetChainId, "earliest", "latest",
                 contract_addresses.Clone(), topics.Clone(),
                 std::move(expected_logs), mojom::ProviderError::kSuccess, "");
}

TEST_F(JsonRpcServiceUnitTest, GetSolTokenMetadata) {
  // Valid inputs should yield metadata JSON (happy case)
  std::string get_account_info_response = R"({
    "jsonrpc": "2.0",
    "result": {
      "context": {
        "apiVersion": "1.13.3",
        "slot": 161038284
      },
      "value": {
        "data": [
          "BGUN5hJf2zSue3S0I/fCq16UREt5NxP6mQdaq4cdGPs3Q8PG/R6KFUSgce78Nwk9Frvkd9bMbvTIKCRSDy88nZQgAAAAU1BFQ0lBTCBTQVVDRQAAAAAAAAAAAAAAAAAAAAAAAAAKAAAAAAAAAAAAAAAAAMgAAABodHRwczovL2JhZmtyZWlmNHd4NTR3anI3cGdmdWczd2xhdHIzbmZudHNmd25ndjZldXNlYmJxdWV6cnhlbmo2Y2s0LmlwZnMuZHdlYi5saW5rP2V4dD0AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAOgDAQIAAABlDeYSX9s0rnt0tCP3wqtelERLeTcT+pkHWquHHRj7NwFiDUmu+U8sXOOZQXL36xmknL+Zzd/z3uw2G0ERMo8Eth4BAgABAf8BAAEBoivvbAzLh2kD2cSu6IQIqGQDGeoh/UEDizyp6mLT1tUAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA==",
          "base64"
        ],
        "executable": false,
        "lamports": 5616720,
        "owner": "metaqbxxUerdq28cj1RbAWkYQm3ybzjb6a8bt518x1s",
        "rentEpoch": 361
      }
    },
    "id": 1
  })";
  const std::string valid_metadata_response = R"({
    "attributes": [
      {
        "trait_type": "hair",
        "value": "green & blue"
      },
      {
        "trait_type": "pontus",
        "value": "no"
      }
    ],
    "description": "",
    "external_url": "",
    "image": "https://bafkreiagsgqhjudpta6trhjuv5y2n2exsrhbkkprl64tvg2mftjsdm3vgi.ipfs.dweb.link?ext=png",
    "name": "SPECIAL SAUCE",
    "properties": {
      "category": "image",
      "creators": [
        {
          "address": "7oUUEdptZnZVhSet4qobU9PtpPfiNUEJ8ftPnrC6YEaa",
          "share": 98
        },
        {
          "address": "tsU33UT3K2JTfLgHUo7hdzRhRe4wth885cqVbM8WLiq",
          "share": 2
        }
      ],
      "files": [
        {
          "type": "image/png",
          "uri": "https://bafkreiagsgqhjudpta6trhjuv5y2n2exsrhbkkprl64tvg2mftjsdm3vgi.ipfs.dweb.link?ext=png"
        }
      ],
      "maxSupply": 0
    },
    "seller_fee_basis_points": 1000,
    "symbol": ""
  })";
  auto network_url = GetNetwork(mojom::kSolanaMainnet, mojom::CoinType::SOL);
  SetSolTokenMetadataInterceptor(
      network_url, get_account_info_response,
      GURL("https://"
           "bafkreif4wx54wjr7pgfug3wlatr3nfntsfwngv6eusebbquezrxenj6ck4.ipfs."
           "dweb.link/?ext="),
      valid_metadata_response);
  TestGetSolTokenMetadata("5ZXToo7froykjvjnpHtTLYr9u2tW3USMwPg3sNkiaQVh",
                          valid_metadata_response,
                          mojom::SolanaProviderError::kSuccess, "");

  // Invalid token_mint_address yields internal error.
  SetSolTokenMetadataInterceptor(
      network_url, get_account_info_response,
      GURL("https://"
           "bafkreif4wx54wjr7pgfug3wlatr3nfntsfwngv6eusebbquezrxenj6ck4.ipfs."
           "dweb.link/?ext="),
      valid_metadata_response);
  TestGetSolTokenMetadata("Invalid", "",
                          mojom::SolanaProviderError::kInternalError,
                          l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));

  // Non 200 getAccountInfo response of yields internal server error.
  SetHTTPRequestTimeoutInterceptor();
  TestGetSolTokenMetadata("5ZXToo7froykjvjnpHtTLYr9u2tW3USMwPg3sNkiaQVh", "",
                          mojom::SolanaProviderError::kInternalError,
                          l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));

  // Invalid getAccountInfo response JSON yields internal error
  SetSolTokenMetadataInterceptor(
      network_url, "Invalid json response",
      GURL("https://"
           "bafkreif4wx54wjr7pgfug3wlatr3nfntsfwngv6eusebbquezrxenj6ck4.ipfs."
           "dweb.link/?ext="),
      valid_metadata_response);
  TestGetSolTokenMetadata("5ZXToo7froykjvjnpHtTLYr9u2tW3USMwPg3sNkiaQVh", "",
                          mojom::SolanaProviderError::kInternalError,
                          l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));

  // Valid response JSON, invalid account info (missing result.value.owner
  // field) info yields parse error
  get_account_info_response = R"({
    "jsonrpc": "2.0",
    "result": {
      "context": {
        "apiVersion": "1.13.3",
        "slot": 161038284
      },
      "value": {
        "data": [
          "BGUN5hJf2zSue3S0I/fCq16UREt5NxP6mQdaq4cdGPs3Q8PG/R6KFUSgce78Nwk9Frvkd9bMbvTIKCRSDy88nZQgAAAAU1BFQ0lBTCBTQVVDRQAAAAAAAAAAAAAAAAAAAAAAAAAKAAAAAAAAAAAAAAAAAMgAAABodHRwczovL2JhZmtyZWlmNHd4NTR3anI3cGdmdWczd2xhdHIzbmZudHNmd25ndjZldXNlYmJxdWV6cnhlbmo2Y2s0LmlwZnMuZHdlYi5saW5rP2V4dD0AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAOgDAQIAAABlDeYSX9s0rnt0tCP3wqtelERLeTcT+pkHWquHHRj7NwFiDUmu+U8sXOOZQXL36xmknL+Zzd/z3uw2G0ERMo8Eth4BAgABAf8BAAEBoivvbAzLh2kD2cSu6IQIqGQDGeoh/UEDizyp6mLT1tUAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA==",
          "base64"
        ],
        "executable": false,
        "lamports": 5616720,
        "rentEpoch": 361
      }
    },
    "id": 1
  })";
  SetSolTokenMetadataInterceptor(
      network_url, get_account_info_response,
      GURL("https://"
           "bafkreif4wx54wjr7pgfug3wlatr3nfntsfwngv6eusebbquezrxenj6ck4.ipfs."
           "dweb.link/?ext="),
      valid_metadata_response);
  TestGetSolTokenMetadata("5ZXToo7froykjvjnpHtTLYr9u2tW3USMwPg3sNkiaQVh", "",
                          mojom::SolanaProviderError::kParsingError,
                          l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));

  // Valid response JSON, parsable account info, but invalid account info data
  // (invalid base64) yields parse error
  get_account_info_response = R"({
    "jsonrpc": "2.0",
    "result": {
      "context": {
        "apiVersion": "1.13.3",
        "slot": 161038284
      },
      "value": {
        "data": [
          "*Invalid Base64*",
          "base64"
        ],
        "executable": false,
        "lamports": 5616720,
        "owner": "metaqbxxUerdq28cj1RbAWkYQm3ybzjb6a8bt518x1s",
        "rentEpoch": 361
      }
    },
    "id": 1
  })";
  SetSolTokenMetadataInterceptor(
      network_url, get_account_info_response,
      GURL("https://"
           "bafkreif4wx54wjr7pgfug3wlatr3nfntsfwngv6eusebbquezrxenj6ck4.ipfs."
           "dweb.link/?ext="),
      valid_metadata_response);
  TestGetSolTokenMetadata("5ZXToo7froykjvjnpHtTLYr9u2tW3USMwPg3sNkiaQVh", "",
                          mojom::SolanaProviderError::kParsingError,
                          l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));

  // Valid response JSON, parsable account info, invalid account info data
  // (valid base64, but invalid borsh encoded metadata) yields parse error
  get_account_info_response = R"({
    "jsonrpc": "2.0",
    "result": {
      "context": {
        "apiVersion": "1.13.3",
        "slot": 161038284
      },
      "value": {
        "data": [
          "d2hvb3BzIQ==",
          "base64"
        ],
        "executable": false,
        "lamports": 5616720,
        "owner": "metaqbxxUerdq28cj1RbAWkYQm3ybzjb6a8bt518x1s",
        "rentEpoch": 361
      }
    },
    "id": 1
  })";
  SetSolTokenMetadataInterceptor(
      network_url, get_account_info_response,
      GURL("https://"
           "bafkreif4wx54wjr7pgfug3wlatr3nfntsfwngv6eusebbquezrxenj6ck4.ipfs."
           "dweb.link/?ext="),
      valid_metadata_response);
  TestGetSolTokenMetadata("5ZXToo7froykjvjnpHtTLYr9u2tW3USMwPg3sNkiaQVh", "",
                          mojom::SolanaProviderError::kParsingError,
                          l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));

  // Valid response JSON, parsable account info, invalid account info data
  // (valid base64, valid borsh encoding, but when decoded the URI is not a
  // valid URI)
  get_account_info_response = R"({
    "jsonrpc": "2.0",
    "result": {
      "context": {
        "apiVersion": "1.13.3",
        "slot": 161038284
      },
      "value": {
        "data": [
          "BGUN5hJf2zSue3S0I/fCq16UREt5NxP6mQdaq4cdGPs3Q8PG/R6KFUSgce78Nwk9Frvkd9bMbvTIKCRSDy88nZQgAAAAU1BFQ0lBTCBTQVVDRQAAAAAAAAAAAAAAAAAAAAAAAAAKAAAAAAAAAAAAAAAAAAsAAABpbnZhbGlkIHVybOgDAQIAAABlDeYSX9s0rnt0tCP3wqtelERLeTcT+pkHWquHHRj7NwFiDUmu+U8sXOOZQXL36xmknL+Zzd/z3uw2G0ERMo8Eth4BAgABAf8BAAEBoivvbAzLh2kD2cSu6IQIqGQDGeoh/UEDizyp6mLT1tUA",
          "base64"
        ],
        "executable": false,
        "lamports": 5616720,
        "owner": "metaqbxxUerdq28cj1RbAWkYQm3ybzjb6a8bt518x1s",
        "rentEpoch": 361
      }
    },
    "id": 1
  })";
  SetSolTokenMetadataInterceptor(
      network_url, get_account_info_response,
      GURL("https://"
           "bafkreif4wx54wjr7pgfug3wlatr3nfntsfwngv6eusebbquezrxenj6ck4.ipfs."
           "dweb.link/?ext="),
      valid_metadata_response);
  TestGetSolTokenMetadata("5ZXToo7froykjvjnpHtTLYr9u2tW3USMwPg3sNkiaQVh", "",
                          mojom::SolanaProviderError::kParsingError,
                          l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));
}

TEST_F(JsonRpcServiceUnitTest, GetEthTokenUri) {
  // Invalid contract address input
  TestGetEthTokenUri("", "0x1", mojom::kMainnetChainId,
                     kERC721MetadataInterfaceId, GURL(),
                     mojom::ProviderError::kInvalidParams,
                     l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));

  // Invalid token ID input
  TestGetEthTokenUri("0x06012c8cf97BEaD5deAe237070F9587f8E7A266d", "",
                     mojom::kMainnetChainId, kERC721MetadataInterfaceId, GURL(),
                     mojom::ProviderError::kInvalidParams,
                     l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));

  // Invalid chain ID input
  TestGetEthTokenUri("0x06012c8cf97BEaD5deAe237070F9587f8E7A266d", "0x1", "",
                     kERC721MetadataInterfaceId, GURL(),
                     mojom::ProviderError::kInvalidParams,
                     l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));

  // Unknown interfaceID input
  TestGetEthTokenUri("0x06012c8cf97BEaD5deAe237070F9587f8E7A266d", "0x1",
                     mojom::kMainnetChainId, "invalid interface", GURL(),
                     mojom::ProviderError::kInvalidParams,
                     l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));

  // Valid inputs but HTTP Timeout
  SetHTTPRequestTimeoutInterceptor();
  TestGetEthTokenUri("0x59468516a8259058bad1ca5f8f4bff190d30e066", "0x719",
                     mojom::kMainnetChainId, kERC721MetadataInterfaceId, GURL(),
                     mojom::ProviderError::kInternalError,
                     l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));

  // Valid inputs, request exceeds limit response
  SetLimitExceededJsonErrorResponse();
  TestGetEthTokenUri("0x59468516a8259058bad1ca5f8f4bff190d30e066", "0x719",
                     mojom::kMainnetChainId, kERC721MetadataInterfaceId, GURL(),
                     mojom::ProviderError::kLimitExceeded,
                     "Request exceeds defined limit");

  // Valid inputs, invalid provider JSON
  SetInvalidJsonInterceptor();
  TestGetEthTokenUri("0x59468516a8259058bad1ca5f8f4bff190d30e066", "0x719",
                     mojom::kMainnetChainId, kERC721MetadataInterfaceId, GURL(),
                     mojom::ProviderError::kParsingError,
                     l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));

  // Valid inputs, valid RPC response JSON, valid RLP encoding, invalid URI
  SetInterceptor(GetNetwork(mojom::kMainnetChainId, mojom::CoinType::ETH),
                 "eth_call", "", R"({
      "jsonrpc":"2.0",
      "id":1,
      "result":"0x0000000000000000000000000000000000000000000000000000000000000020000000000000000000000000000000000000000000000000000000000000000b696e76616c69642075726c000000000000000000000000000000000000000000"
  })");
  TestGetEthTokenUri("0x59468516a8259058bad1ca5f8f4bff190d30e066", "0x719",
                     mojom::kMainnetChainId, kERC721MetadataInterfaceId, GURL(),
                     mojom::ProviderError::kParsingError,
                     l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));

  // All valid
  SetInterceptor(GetNetwork(mojom::kMainnetChainId, mojom::CoinType::ETH),
                 "eth_call", "", R"({
      "jsonrpc":"2.0",
      "id":1,
      "result":"0x0000000000000000000000000000000000000000000000000000000000000020000000000000000000000000000000000000000000000000000000000000002468747470733a2f2f696e76697369626c65667269656e64732e696f2f6170692f3138313700000000000000000000000000000000000000000000000000000000"
  })");
  TestGetEthTokenUri("0x59468516a8259058bad1ca5f8f4bff190d30e066", "0x719",
                     mojom::kMainnetChainId, kERC721MetadataInterfaceId,
                     GURL("https://invisiblefriends.io/api/1817"),
                     mojom::ProviderError::kSuccess, "");
}

TEST_F(JsonRpcServiceUnitTest, GetEthNftStandard) {
  std::vector<std::string> interfaces;
  // Empty interface IDs yields invalid params error
  TestGetEthNftStandard(
      "0x06012c8cf97BEaD5deAe237070F9587f8E7A266d", mojom::kMainnetChainId,
      interfaces, std::nullopt, mojom::ProviderError::kInvalidParams,
      l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));

  // Empty contract address yields invalid params error
  interfaces.push_back(kERC721InterfaceId);
  TestGetEthNftStandard(
      "", mojom::kMainnetChainId, interfaces, std::nullopt,
      mojom::ProviderError::kInvalidParams,
      l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));

  // Empty chain ID yields invalid params error
  TestGetEthNftStandard(
      "0x06012c8cf97BEaD5deAe237070F9587f8E7A266d", "", interfaces,
      std::nullopt, mojom::ProviderError::kInvalidParams,
      l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));

  // Valid inputs but HTTP Timeout
  SetHTTPRequestTimeoutInterceptor();
  TestGetEthNftStandard("0x06012c8cf97BEaD5deAe237070F9587f8E7A266d",
                        mojom::kMainnetChainId, interfaces, std::nullopt,
                        mojom::ProviderError::kInternalError,
                        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));

  // Valid inputs, invalid provider JSON yields parsing error
  SetInvalidJsonInterceptor();
  TestGetEthNftStandard("0x06012c8cf97BEaD5deAe237070F9587f8E7A266d",
                        mojom::kMainnetChainId, interfaces, std::nullopt,
                        mojom::ProviderError::kParsingError,
                        l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));

  // Valid inputs, supported response returned for the first interface ID
  auto network = GetNetwork(mojom::kMainnetChainId, mojom::CoinType::ETH);
  std::map<std::string, std::string> responses;
  const std::string interface_supported_response = R"({
      "jsonrpc":"2.0",
      "id":1,
      "result":"0x0000000000000000000000000000000000000000000000000000000000000001"
  })";
  responses[kERC721InterfaceId] = interface_supported_response;
  SetGetEthNftStandardInterceptor(network, responses);
  TestGetEthNftStandard("0x06012c8cf97BEaD5deAe237070F9587f8E7A266d",
                        mojom::kMainnetChainId, interfaces, kERC721InterfaceId,
                        mojom::ProviderError::kSuccess, "");

  // Valid inputs, supported response returned for the second interface ID
  // (ERC1155)
  interfaces.clear();
  interfaces.push_back(kERC721InterfaceId);
  interfaces.push_back(kERC1155InterfaceId);
  const std::string interface_not_supported_response = R"({
      "jsonrpc":"2.0",
      "id":1,
      "result":"0x0000000000000000000000000000000000000000000000000000000000000000"
  })";
  responses[kERC721InterfaceId] = interface_not_supported_response;
  responses[kERC1155InterfaceId] = interface_supported_response;
  SetGetEthNftStandardInterceptor(network, responses);
  TestGetEthNftStandard("0x06012c8cf97BEaD5deAe237070F9587f8E7A266d",
                        mojom::kMainnetChainId, interfaces, kERC1155InterfaceId,
                        mojom::ProviderError::kSuccess, "");

  // Valid inputs, but no interfaces are supported yields success / nullopt
  interfaces.clear();
  interfaces.push_back(kERC1155InterfaceId);
  interfaces.push_back(kERC721InterfaceId);
  responses[kERC721InterfaceId] = interface_not_supported_response;
  responses[kERC1155InterfaceId] = interface_not_supported_response;
  SetGetEthNftStandardInterceptor(network, responses);
  TestGetEthNftStandard("0x06012c8cf97BEaD5deAe237070F9587f8E7A266d",
                        mojom::kMainnetChainId, interfaces, std::nullopt,
                        mojom::ProviderError::kSuccess, "");
}

TEST_F(JsonRpcServiceUnitTest, GetEthTokenSymbol) {
  // Invalid chain ID yields invalid params
  TestGetEthTokenSymbol(
      "0x0D8775F648430679A709E98d2b0Cb6250d2887EF", "", "",
      mojom::ProviderError::kInvalidParams,
      l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));

  // Valid inputs but request times out yields internal error
  SetHTTPRequestTimeoutInterceptor();
  TestGetEthTokenSymbol("0x0D8775F648430679A709E98d2b0Cb6250d2887EF",
                        mojom::kMainnetChainId, "",
                        mojom::ProviderError::kInternalError,
                        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));

  // Valid
  const std::string bat_symbol_result =
      "0x"
      "0000000000000000000000000000000000000000000000000000000000000020"
      "0000000000000000000000000000000000000000000000000000000000000003"
      "4241540000000000000000000000000000000000000000000000000000000000";
  SetInterceptor(GetNetwork(mojom::kMainnetChainId, mojom::CoinType::ETH),
                 "eth_call", "", FormatJsonRpcResponse(bat_symbol_result));
  TestGetEthTokenSymbol("0x0D8775F648430679A709E98d2b0Cb6250d2887EF",
                        mojom::kMainnetChainId, "BAT",
                        mojom::ProviderError::kSuccess, "");

  // Response parsing error yields parsing error
  SetInterceptor(GetNetwork(mojom::kMainnetChainId, mojom::CoinType::ETH),
                 "eth_call", "", R"({
      "jsonrpc": "2.0",
      "id": 1,
      "result": "0xinvalid"
  })");
  TestGetEthTokenSymbol("0x0D8775F648430679A709E98d2b0Cb6250d2887EF",
                        mojom::kMainnetChainId, "",
                        mojom::ProviderError::kParsingError,
                        l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));
}

TEST_F(JsonRpcServiceUnitTest, GetEthTokenDecimals) {
  // Invalid chain ID yields invalid params
  TestGetEthTokenDecimals(
      "0x0D8775F648430679A709E98d2b0Cb6250d2887EF", "", "",
      mojom::ProviderError::kInvalidParams,
      l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));

  // Valid inputs but request times out yields internal error
  SetHTTPRequestTimeoutInterceptor();
  TestGetEthTokenDecimals("0x0D8775F648430679A709E98d2b0Cb6250d2887EF",
                          mojom::kMainnetChainId, "",
                          mojom::ProviderError::kInternalError,
                          l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));

  // Valid
  const std::string bat_decimals_result =
      "0x"
      "0000000000000000000000000000000000000000000000000000000000000012";
  SetInterceptor(GetNetwork(mojom::kMainnetChainId, mojom::CoinType::ETH),
                 "eth_call", "", FormatJsonRpcResponse(bat_decimals_result));
  TestGetEthTokenDecimals("0x0D8775F648430679A709E98d2b0Cb6250d2887EF",
                          mojom::kMainnetChainId, "0x12",
                          mojom::ProviderError::kSuccess, "");

  // Response parsing error yields parsing error
  SetInterceptor(GetNetwork(mojom::kMainnetChainId, mojom::CoinType::ETH),
                 "eth_call", "", R"({
      "jsonrpc": "2.0",
      "id": 1,
      "result": "0xinvalid"
  })");
  TestGetEthTokenDecimals("0x0D8775F648430679A709E98d2b0Cb6250d2887EF",
                          mojom::kMainnetChainId, "",
                          mojom::ProviderError::kParsingError,
                          l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));
}

TEST_F(JsonRpcServiceUnitTest, GetEthTokenInfo) {
  const std::string bat_decimals_result =
      "0x"
      "0000000000000000000000000000000000000000000000000000000000000012";
  const std::string bat_symbol_result =
      "0x"
      "0000000000000000000000000000000000000000000000000000000000000020"
      "0000000000000000000000000000000000000000000000000000000000000003"
      "4241540000000000000000000000000000000000000000000000000000000000";
  const std::string bat_name_result =
      "0x"
      "000000000000000000000000000000000000000000000000000000000000002000"
      "000000000000000000000000000000000000000000000000000000000000154261"
      "73696320417474656e74696f6e20546f6b656e0000000000000000000000";

  SetEthTokenInfoInterceptor(
      GetNetwork(mojom::kMainnetChainId, mojom::CoinType::ETH),
      mojom::kMainnetChainId, bat_symbol_result, bat_name_result,
      bat_decimals_result);

  // Setup tokens list to populate coingecko id
  std::string coingecko_ids_json = R"({
    "0x1": {
      "0x0D8775F648430679A709E98d2b0Cb6250d2887EF": "basic-attention-token"
    }
  })";
  std::optional<CoingeckoIdsMap> coingecko_ids_map =
      ParseCoingeckoIdsMap(coingecko_ids_json);
  ASSERT_TRUE(coingecko_ids_map);
  BlockchainRegistry::GetInstance()->UpdateCoingeckoIdsMap(
      std::move(*coingecko_ids_map));

  auto bat_token = mojom::BlockchainToken::New(
      "0x0D8775F648430679A709E98d2b0Cb6250d2887EF", "Basic Attention Token", "",
      false, false, false, false, mojom::SPLTokenProgram::kUnsupported, false,
      false, "BAT", 18, true, "", "basic-attention-token", "0x1",
      mojom::CoinType::ETH, false);

  TestGetEthTokenInfo("0x0D8775F648430679A709E98d2b0Cb6250d2887EF",
                      mojom::kMainnetChainId, bat_token.Clone(),
                      mojom::ProviderError::kSuccess, "");

  // Invalid (empty) symbol response does not yield error
  bat_token->symbol = "";
  SetEthTokenInfoInterceptor(
      GetNetwork(mojom::kMainnetChainId, mojom::CoinType::ETH),
      mojom::kMainnetChainId, "", bat_name_result, bat_decimals_result);
  TestGetEthTokenInfo("0x0D8775F648430679A709E98d2b0Cb6250d2887EF",
                      mojom::kMainnetChainId, bat_token.Clone(),
                      mojom::ProviderError::kSuccess, "");
  bat_token->symbol = "BAT";

  // Invalid (empty) name response does not yield error
  bat_token->name = "";
  SetEthTokenInfoInterceptor(
      GetNetwork(mojom::kMainnetChainId, mojom::CoinType::ETH),
      mojom::kMainnetChainId, bat_symbol_result, "", bat_decimals_result);
  TestGetEthTokenInfo("0x0D8775F648430679A709E98d2b0Cb6250d2887EF",
                      mojom::kMainnetChainId, bat_token.Clone(),
                      mojom::ProviderError::kSuccess, "");
  bat_token->name = "Basic Attention Token";

  // Empty decimals response does not yield error
  bat_token->decimals = 0;
  SetEthTokenInfoInterceptor(
      GetNetwork(mojom::kMainnetChainId, mojom::CoinType::ETH),
      mojom::kMainnetChainId, bat_symbol_result, bat_name_result, "");
  TestGetEthTokenInfo("0x0D8775F648430679A709E98d2b0Cb6250d2887EF",
                      mojom::kMainnetChainId, bat_token.Clone(),
                      mojom::ProviderError::kSuccess, "");

  // Invalid decimals response does not yield error
  SetEthTokenInfoInterceptor(
      GetNetwork(mojom::kMainnetChainId, mojom::CoinType::ETH),
      mojom::kMainnetChainId, bat_symbol_result, bat_name_result, "invalid");
  TestGetEthTokenInfo("0x0D8775F648430679A709E98d2b0Cb6250d2887EF",
                      mojom::kMainnetChainId, bat_token.Clone(),
                      mojom::ProviderError::kSuccess, "");
}

TEST_F(JsonRpcServiceUnitTest, AnkrGetAccountBalances) {
  // Ensure MethodNotFound error is returned if feature is disabled
  base::RunLoop run_loop_1;
  json_rpc_service_->AnkrGetAccountBalances(
      "0xa92d461a9a988a7f11ec285d39783a637fdd6ba4",
      {mojom::kPolygonMainnetChainId},
      base::BindLambdaForTesting(
          [&](std::vector<mojom::AnkrAssetBalancePtr> response,
              mojom::ProviderError error, const std::string& error_string) {
            EXPECT_EQ(response.size(), 0u);
            EXPECT_EQ(error, mojom::ProviderError::kMethodNotFound);
            EXPECT_EQ(error_string, l10n_util::GetStringUTF8(
                                        IDS_WALLET_REQUEST_PROCESSING_ERROR));

            run_loop_1.Quit();
          }));
  run_loop_1.Run();

  // Enable feature
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeature(features::kBraveWalletAnkrBalancesFeature);

  SetInterceptor(R"(
    {
      "jsonrpc": "2.0",
      "id": 1,
      "result": {
        "totalBalanceUsd": "4915134435857.581297310767673907",
        "assets": [
          {
            "blockchain": "polygon",
            "tokenName": "Matic",
            "tokenSymbol": "MATIC",
            "tokenDecimals": "18",
            "tokenType": "NATIVE",
            "holderAddress": "0xa92d461a9a988a7f11ec285d39783a637fdd6ba4",
            "balance": "120.275036899888325666",
            "balanceRawInteger": "120275036899888325666",
            "balanceUsd": "66.534394147826631446",
            "tokenPrice": "0.553185397924316979",
            "thumbnail": "polygon.svg"
          },
          {
            "blockchain": "polygon",
            "tokenName": "Malformed USDC",
            "tokenSymbol": "USDC",
            "tokenDecimals": "-6",
            "tokenType": "ERC20",
            "contractAddress": "0x2791bca1f2de4661ed88a30c99a7a9449aa84174",
            "holderAddress": "0xa92d461a9a988a7f11ec285d39783a637fdd6ba4",
            "balance": "8.202765",
            "balanceRawInteger": "8202765",
            "balanceUsd": "8.202765",
            "tokenPrice": "1",
            "thumbnail": "usdc.png"
          },
          {
            "blockchain": "polygon",
            "tokenName": "USD Coin",
            "tokenSymbol": "USDC",
            "tokenDecimals": "6",
            "tokenType": "ERC20",
            "contractAddress": "0x2791bca1f2de4661ed88a30c99a7a9449aa84174",
            "holderAddress": "0xa92d461a9a988a7f11ec285d39783a637fdd6ba4",
            "balance": "8.202765",
            "balanceRawInteger": "8202765",
            "balanceUsd": "8.202765",
            "tokenPrice": "1",
            "thumbnail": "usdc.png"
          },
          {
            "blockchain": "polygon",
            "tokenName": "Malformed USDC",
            "tokenSymbol": "USDC",
            "tokenDecimals": "6",
            "tokenType": "ERC20",
            "holderAddress": "0xa92d461a9a988a7f11ec285d39783a637fdd6ba4",
            "balance": "8.202765",
            "balanceRawInteger": "8202765",
            "balanceUsd": "8.202765",
            "tokenPrice": "1",
            "thumbnail": "usdc.png"
          }
        ]
      }
    }
  )");

  // Setup tokens list to populate coingecko id
  std::string coingecko_ids_json = R"({
    "0x89": {
      "0x2791bca1f2de4661ed88a30c99a7a9449aa84174": "usd-coin"
    }
  })";
  std::optional<CoingeckoIdsMap> coingecko_ids_map =
      ParseCoingeckoIdsMap(coingecko_ids_json);
  ASSERT_TRUE(coingecko_ids_map);
  BlockchainRegistry::GetInstance()->UpdateCoingeckoIdsMap(
      std::move(*coingecko_ids_map));

  base::RunLoop run_loop_2;
  json_rpc_service_->AnkrGetAccountBalances(
      "0xa92d461a9a988a7f11ec285d39783a637fdd6ba4",
      {mojom::kPolygonMainnetChainId},
      base::BindLambdaForTesting(
          [&](std::vector<mojom::AnkrAssetBalancePtr> response,
              mojom::ProviderError error, const std::string& error_string) {
            ASSERT_EQ(response.size(), 2u);
            EXPECT_EQ(response.at(0)->asset->contract_address, "");
            EXPECT_EQ(response.at(0)->asset->name, "Matic");
            EXPECT_EQ(response.at(0)->asset->logo, "polygon.svg");
            EXPECT_FALSE(response.at(0)->asset->is_erc20);
            EXPECT_FALSE(response.at(0)->asset->is_erc721);
            EXPECT_FALSE(response.at(0)->asset->is_erc1155);
            EXPECT_FALSE(response.at(0)->asset->is_nft);
            EXPECT_FALSE(response.at(0)->asset->is_spam);
            EXPECT_EQ(response.at(0)->asset->symbol, "MATIC");
            EXPECT_EQ(response.at(0)->asset->decimals, 18);
            EXPECT_TRUE(response.at(0)->asset->visible);
            EXPECT_EQ(response.at(0)->asset->token_id, "");
            EXPECT_EQ(response.at(0)->asset->coingecko_id, "");
            EXPECT_EQ(response.at(0)->asset->chain_id,
                      mojom::kPolygonMainnetChainId);
            EXPECT_EQ(response.at(0)->asset->coin, mojom::CoinType::ETH);
            EXPECT_EQ(response.at(0)->balance, "120275036899888325666");
            EXPECT_EQ(response.at(0)->formatted_balance,
                      "120.275036899888325666");
            EXPECT_EQ(response.at(0)->balance_usd, "66.534394147826631446");
            EXPECT_EQ(response.at(0)->price_usd, "0.553185397924316979");

            EXPECT_EQ(response.at(1)->asset->contract_address,
                      "0x2791bca1f2de4661ed88a30c99a7a9449aa84174");
            EXPECT_EQ(response.at(1)->asset->name, "USD Coin");
            EXPECT_EQ(response.at(1)->asset->logo, "usdc.png");
            EXPECT_TRUE(response.at(1)->asset->is_erc20);
            EXPECT_FALSE(response.at(1)->asset->is_erc721);
            EXPECT_FALSE(response.at(1)->asset->is_erc1155);
            EXPECT_FALSE(response.at(1)->asset->is_nft);
            EXPECT_FALSE(response.at(1)->asset->is_spam);
            EXPECT_EQ(response.at(1)->asset->symbol, "USDC");
            EXPECT_EQ(response.at(1)->asset->decimals, 6);
            EXPECT_TRUE(response.at(1)->asset->visible);
            EXPECT_EQ(response.at(1)->asset->token_id, "");
            EXPECT_EQ(response.at(1)->asset->coingecko_id, "usd-coin");
            EXPECT_EQ(response.at(1)->asset->chain_id,
                      mojom::kPolygonMainnetChainId);
            EXPECT_EQ(response.at(1)->asset->coin, mojom::CoinType::ETH);
            EXPECT_EQ(response.at(1)->balance, "8202765");
            EXPECT_EQ(response.at(1)->formatted_balance, "8.202765");
            EXPECT_EQ(response.at(1)->balance_usd, "8.202765");
            EXPECT_EQ(response.at(1)->price_usd, "1");

            EXPECT_EQ(error, mojom::ProviderError::kSuccess);
            EXPECT_EQ(error_string, "");
            run_loop_2.Quit();
          }));
  run_loop_2.Run();

  // Handle known provider errors
  SetInterceptor(R"(
    {
      "jsonrpc": "2.0",
      "id": 1,
      "error": {
        "code": -32602,
        "message": "invalid argument 0: invalid params"
      }
    }
  )");
  base::RunLoop run_loop_3;
  json_rpc_service_->AnkrGetAccountBalances(
      "0xa92d461a9a988a7f11ec285d39783a637fdd6ba4",
      {mojom::kPolygonMainnetChainId},
      base::BindLambdaForTesting(
          [&](std::vector<mojom::AnkrAssetBalancePtr> response,
              mojom::ProviderError error, const std::string& error_string) {
            EXPECT_EQ(response.size(), 0u);
            EXPECT_EQ(error, mojom::ProviderError::kInvalidParams);
            EXPECT_EQ(error_string, "invalid argument 0: invalid params");

            run_loop_3.Quit();
          }));
  run_loop_3.Run();

  // Invalid response yields parsing error
  SetInterceptor(R"(
    {
      "jsonrpc": "2.0",
      "id": 1,
      "foo": "bar"
    }
  )");
  base::RunLoop run_loop_4;
  json_rpc_service_->AnkrGetAccountBalances(
      "0xa92d461a9a988a7f11ec285d39783a637fdd6ba4",
      {mojom::kPolygonMainnetChainId},
      base::BindLambdaForTesting(
          [&](std::vector<mojom::AnkrAssetBalancePtr> response,
              mojom::ProviderError error, const std::string& error_string) {
            EXPECT_EQ(response.size(), 0u);
            EXPECT_EQ(error, mojom::ProviderError::kParsingError);
            EXPECT_EQ(error_string,
                      l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));

            run_loop_4.Quit();
          }));
  run_loop_4.Run();
}

TEST_F(JsonRpcServiceUnitTest, GetSPLTokenProgramByMint) {
  const std::string tsla_mint_addr =
      "2inRoG4DuMRRzZxAt913CCdNZCu2eGsDD9kZTrsj2DAZ";

  // Invalid mint or chain ID yields invalid params.
  TestGetSPLTokenProgramByMint(
      FROM_HERE, "", mojom::kSolanaMainnet, mojom::SPLTokenProgram::kUnknown,
      mojom::SolanaProviderError::kInvalidParams,
      l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
  TestGetSPLTokenProgramByMint(
      FROM_HERE, tsla_mint_addr, "", mojom::SPLTokenProgram::kUnknown,
      mojom::SolanaProviderError::kInvalidParams,
      l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));

  // Setup registry with two assets.
  const char token_list_json[] = R"(
    {
      "2inRoG4DuMRRzZxAt913CCdNZCu2eGsDD9kZTrsj2DAZ": {
        "name": "Tesla Inc.",
        "logo": "2inRoG4DuMRRzZxAt913CCdNZCu2eGsDD9kZTrsj2DAZ.png",
        "erc20": false,
        "symbol": "TSLA",
        "decimals": 8,
        "chainId": "0x65"
      },
      "2kMpEJCZL8vEDZe7YPLMCS9Y3WKSAMedXBn7xHPvsWvi": {
        "name": "SolarMoon",
        "logo": "2kMpEJCZL8vEDZe7YPLMCS9Y3WKSAMedXBn7xHPvsWvi.png",
        "erc20": false,
        "symbol": "MOON",
        "decimals": 5,
        "chainId": "0x65",
        "token2022": true
      }
    })";

  auto* registry = BlockchainRegistry::GetInstance();
  TokenListMap token_list_map;
  ASSERT_TRUE(
      ParseTokenList(token_list_json, &token_list_map, mojom::CoinType::SOL));
  registry->UpdateTokenList(std::move(token_list_map));

  // Setup two user assets.
  auto asset = mojom::BlockchainToken::New(
      tsla_mint_addr, "Tesla", "tsla.png", false, false, false, false,
      mojom::SPLTokenProgram::kToken2022, false, false, "TSLA", 8, true, "", "",
      mojom::kSolanaMainnet, mojom::CoinType::SOL, false);
  ASSERT_TRUE(AddUserAsset(prefs(), asset.Clone()));

  auto asset2 = mojom::BlockchainToken::New(
      "So11111111111111111111111111111111111111112", "Wrapped SOL", "sol.png",
      false, false, false, false, mojom::SPLTokenProgram::kUnknown, false,
      false, "WSOL", 8, true, "", "", mojom::kSolanaMainnet,
      mojom::CoinType::SOL, false);
  ASSERT_TRUE(AddUserAsset(prefs(), asset2.Clone()));

  // Test record in registry, the value should be used.
  TestGetSPLTokenProgramByMint(
      FROM_HERE, "2kMpEJCZL8vEDZe7YPLMCS9Y3WKSAMedXBn7xHPvsWvi",
      mojom::kSolanaMainnet, mojom::SPLTokenProgram::kToken2022,
      mojom::SolanaProviderError::kSuccess, "");

  // Test record in both registry and user assets. The value in user assets
  // should be used.
  TestGetSPLTokenProgramByMint(FROM_HERE, tsla_mint_addr, mojom::kSolanaMainnet,
                               mojom::SPLTokenProgram::kToken2022,
                               mojom::SolanaProviderError::kSuccess, "");

  std::string json = R"(
    {
      "jsonrpc":"2.0","id":1,
      "result": {
        "context":{"slot":123065869},
        "value":{
          "data":["SEVMTE8gV09STEQ=","base64"],
          "executable":false,
          "lamports":18446744073709551615,
          "owner":"$1",
          "rentEpoch":18446744073709551615
        }
      }
    }
  )";

  // Test record in user assets with unknown token program, result is from
  // network and the pref value should be updated based on the result.
  auto user_asset =
      GetUserAsset(prefs(), mojom::CoinType::SOL, mojom::kSolanaMainnet,
                   asset2->contract_address, "", false, false, false);
  ASSERT_TRUE(user_asset);
  EXPECT_EQ(user_asset->spl_token_program, mojom::SPLTokenProgram::kUnknown);

  auto expected_network_url =
      GetNetwork(mojom::kSolanaMainnet, mojom::CoinType::SOL);
  SetInterceptor(expected_network_url, "getAccountInfo", "",
                 base::ReplaceStringPlaceholders(
                     json, {mojom::kSolanaTokenProgramId}, nullptr));

  TestGetSPLTokenProgramByMint(
      FROM_HERE, asset2->contract_address, mojom::kSolanaMainnet,
      mojom::SPLTokenProgram::kToken, mojom::SolanaProviderError::kSuccess, "");

  user_asset =
      GetUserAsset(prefs(), mojom::CoinType::SOL, mojom::kSolanaMainnet,
                   asset2->contract_address, "", false, false, false);
  ASSERT_TRUE(user_asset);
  EXPECT_EQ(user_asset->spl_token_program, mojom::SPLTokenProgram::kToken);

  // Test record not in registry or user assets, result is from network.
  SetInterceptor(expected_network_url, "getAccountInfo", "",
                 base::ReplaceStringPlaceholders(
                     json, {mojom::kSolanaToken2022ProgramId}, nullptr));
  TestGetSPLTokenProgramByMint(
      FROM_HERE, "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v",
      mojom::kSolanaMainnet, mojom::SPLTokenProgram::kToken2022,
      mojom::SolanaProviderError::kSuccess, "");

  // Valid inputs but request times out yields internal error.
  SetHTTPRequestTimeoutInterceptor();
  TestGetSPLTokenProgramByMint(
      FROM_HERE, "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v",
      mojom::kSolanaMainnet, mojom::SPLTokenProgram::kUnknown,
      mojom::SolanaProviderError::kInternalError,
      l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
}

TEST_F(JsonRpcServiceUnitTest, SimulateSolanaTransaction) {
  // Empty transaction yields invalid params error
  TestSimulateSolanaTransaction(
      mojom::kSolanaMainnet, 0, mojom::SolanaProviderError::kInvalidParams,
      l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS), "");

  auto network_url = GetNetwork(mojom::kSolanaMainnet, mojom::CoinType::SOL);
  std::string response = R"({
    "jsonrpc": "2.0",
    "result": {
      "context": {
        "apiVersion": "1.17.25",
        "slot": 259225005
      },
      "value": {
        "accounts": null,
        "err": null,
        "logs": [
          "Program BGUMAp9Gq7iTEuizy4pqaxsTyUCBK68MDfK752saRPUY invoke [1]",
          "Program log: Instruction: Transfer",
          "Program BGUMAp9Gq7iTEuizy4pqaxsTyUCBK68MDfK752saRPUY success"
        ],
        "returnData": null,
        "unitsConsumed": 69017
      }
    },
    "id": 1
  })";
  SetInterceptor(network_url, "simulateTransaction", "", response);

  TestSimulateSolanaTransaction(mojom::kSolanaMainnet, 69017,
                                mojom::SolanaProviderError::kSuccess, "");

  // Response parsing error
  response = R"({"jsonrpc":"2.0","id":1,"result":0})";
  SetInterceptor(network_url, "simulateTransaction", "", response);
  TestSimulateSolanaTransaction(
      mojom::kSolanaMainnet, 0, mojom::SolanaProviderError::kParsingError,
      l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));

  // JSON RPC Error
  response = R"({
    "jsonrpc": "2.0",
    "id": 1,
    "error": {
      "code": -32601,
      "message": "method does not exist"
    }
  })";
  SetInterceptor(network_url, "simulateTransaction", "", response);
  TestSimulateSolanaTransaction(mojom::kSolanaMainnet, 0,
                                mojom::SolanaProviderError::kMethodNotFound,
                                "method does not exist");

  // HTTP error
  SetHTTPRequestTimeoutInterceptor();
  TestSimulateSolanaTransaction(
      mojom::kSolanaMainnet, 0, mojom::SolanaProviderError::kInternalError,
      l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));

  // Blockhash not found error
  response = R"({
    "jsonrpc": "2.0",
    "result": {
      "context": {
        "apiVersion": "1.18.11",
        "slot": 262367830
      },
      "value": {
        "accounts": null,
        "err": "BlockhashNotFound",
        "innerInstructions": null,
        "logs": [],
        "returnData": null,
        "unitsConsumed": 0
      }
    },
    "id": 1
  })";
  SetInterceptor(network_url, "simulateTransaction", "", response);
  TestSimulateSolanaTransaction(
      mojom::kSolanaMainnet, 0, mojom::SolanaProviderError::kParsingError,
      l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));
}

TEST_F(JsonRpcServiceUnitTest, GetRecentSolanaPrioritizationFees) {
  auto network_url = GetNetwork(mojom::kSolanaMainnet, mojom::CoinType::SOL);

  // Successful response
  std::string response = R"({
    "jsonrpc": "2.0",
    "result": [
      {
        "prioritizationFee": 100,
        "slot": 293251906
      },
      {
        "prioritizationFee": 200,
        "slot": 293251906
      },
      {
        "prioritizationFee": 0,
        "slot": 293251805
      }
    ],
    "id": 1
  })";
  SetInterceptor(network_url, "getRecentPrioritizationFees", "", response);
  TestGetRecentSolanaPrioritizationFees(
      mojom::kSolanaMainnet,
      {{293251906, 100}, {293251906, 200}, {293251805, 0}},
      mojom::SolanaProviderError::kSuccess, "");

  // Response parsing error
  response = R"({
    "jsonrpc": "2.0",
    "result": [
      {
      },
      {
        "prioritizationFee": 0,
        "slot": 293251805
      }
    ],
    "id": 1
  })";
  SetInterceptor(network_url, "getRecentPrioritizationFees", "", response);
  TestGetRecentSolanaPrioritizationFees(
      mojom::kSolanaMainnet, {}, mojom::SolanaProviderError::kParsingError,
      l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));

  // JSON RPC Error
  response = R"({
    "jsonrpc": "2.0",
    "id": 1,
    "error": {
      "code": -32601,
      "message": "method does not exist"
    }
  })";
  SetInterceptor(network_url, "getRecentPrioritizationFees", "", response);
  TestGetRecentSolanaPrioritizationFees(
      mojom::kSolanaMainnet, {}, mojom::SolanaProviderError::kMethodNotFound,
      "method does not exist");

  // HTTP error
  SetHTTPRequestTimeoutInterceptor();
  TestGetRecentSolanaPrioritizationFees(
      mojom::kSolanaMainnet, {}, mojom::SolanaProviderError::kInternalError,
      l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
}

TEST_F(JsonRpcServiceUnitTest, GetNftMetadatas) {
  // If there are no NFTs it returns invalid params.
  std::vector<mojom::NftIdentifierPtr> nft_identifiers;
  TestGetNftMetadatas(mojom::CoinType::SOL, std::move(nft_identifiers), {},
                      l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
  nft_identifiers = std::vector<mojom::NftIdentifierPtr>();

  // If there are duplicate NFTs it returns invalid params.
  auto duplicate_nft1 = mojom::NftIdentifier::New();
  duplicate_nft1->chain_id = mojom::kMainnetChainId;
  duplicate_nft1->contract_address =
      "0xed5af388653567af2f388e6224dc7c4b3241c544";
  duplicate_nft1->token_id = "0xacf";  // "2767"
  nft_identifiers.push_back(std::move(duplicate_nft1));

  auto duplicate_nft2 = mojom::NftIdentifier::New();
  duplicate_nft2->chain_id = mojom::kMainnetChainId;
  duplicate_nft2->contract_address =
      "0xed5af388653567af2f388e6224dc7c4b3241c544";
  duplicate_nft2->token_id = "0xacf";  // "2767"
  nft_identifiers.push_back(std::move(duplicate_nft2));

  TestGetNftMetadatas(mojom::CoinType::ETH, std::move(nft_identifiers), {},
                      l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
  nft_identifiers = std::vector<mojom::NftIdentifierPtr>();

  // If there are over 50 NFTs it returns invalid params.
  for (int i = 0; i < 51; i++) {
    auto nft_identifier = mojom::NftIdentifier::New();
    nft_identifier->chain_id = mojom::kSolanaMainnet;
    nft_identifier->contract_address =
        "BoSDWCAWmZEM7TQLg2gawt5wnurGyQu7c77tAcbtzfDG";
    nft_identifier->token_id = "";
    nft_identifiers.push_back(std::move(nft_identifier));
  }
  TestGetNftMetadatas(mojom::CoinType::SOL, std::move(nft_identifiers), {},
                      l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
  nft_identifiers = std::vector<mojom::NftIdentifierPtr>();

  // Add Ethereum NFT identifiers with non-checksum addresses
  auto eth_nft_identifier1 = mojom::NftIdentifier::New();
  eth_nft_identifier1->chain_id = mojom::kMainnetChainId;
  eth_nft_identifier1->contract_address =
      "0xed5af388653567af2f388e6224dc7c4b3241c544";
  eth_nft_identifier1->token_id = "0xacf";  // "2767";
  nft_identifiers.push_back(std::move(eth_nft_identifier1));

  auto eth_nft_identifier2 = mojom::NftIdentifier::New();
  eth_nft_identifier2->chain_id = mojom::kMainnetChainId;
  eth_nft_identifier2->contract_address =
      "0xabc1230000000000000000000000000000000000";
  eth_nft_identifier2->token_id = "0x4d2";  // "1234";
  nft_identifiers.push_back(std::move(eth_nft_identifier2));

  // Expected Ethereum metadata
  std::vector<mojom::NftMetadataPtr> expected_eth_metadata;
  mojom::NftMetadataPtr eth_metadata1 = mojom::NftMetadata::New();
  eth_metadata1->name = "Azuki #2767";
  eth_metadata1->description = "Azuki is a cute little bean";
  eth_metadata1->image = "https://simplehash.wallet-cdn.brave.com/assets/1.png";
  eth_metadata1->external_url = "";
  eth_metadata1->background_color = "";
  mojom::NftAttributePtr eth_attribute1 = mojom::NftAttribute::New();
  eth_attribute1->trait_type = "Color";
  eth_attribute1->value = "Red";
  eth_metadata1->attributes.push_back(std::move(eth_attribute1));
  mojom::NftAttributePtr eth_attribute2 = mojom::NftAttribute::New();
  eth_attribute2->trait_type = "Size";
  eth_attribute2->value = "Small";
  eth_metadata1->attributes.push_back(std::move(eth_attribute2));
  eth_metadata1->collection = "Azuki";
  expected_eth_metadata.push_back(std::move(eth_metadata1));

  mojom::NftMetadataPtr eth_metadata2 = mojom::NftMetadata::New();
  eth_metadata2->name = "NFT #1234";
  eth_metadata2->description = "Description of NFT #1234";
  eth_metadata2->image = "https://simplehash.wallet-cdn.brave.com/assets/2.png";
  eth_metadata2->external_url = "";
  eth_metadata2->background_color = "";
  mojom::NftAttributePtr eth_attribute3 = mojom::NftAttribute::New();
  eth_attribute3->trait_type = "Attribute";
  eth_attribute3->value = "Value";
  eth_metadata2->attributes.push_back(std::move(eth_attribute3));
  expected_eth_metadata.push_back(std::move(eth_metadata2));

  std::map<GURL, std::string> responses_eth;
  responses_eth[GURL(
      "https://simplehash.wallet.brave.com/api/v0/nfts/"
      "assets?nft_ids=ethereum.0xED5AF388653567Af2F388E6224dC7C4b3241C544.2767%"
      "2Cethereum.0xAbc1230000000000000000000000000000000000.1234")] = R"({
    "nfts": [
      {
        "chain": "ethereum",
        "contract_address": "0xED5AF388653567Af2F388E6224dC7C4b3241C544",
        "token_id": "2767",
        "name": "Azuki #2767",
        "description": "Azuki is a cute little bean",
        "image_url": "https://cdn.simplehash.com/assets/1.png",
        "external_url": null,
        "background_color": null,
        "extra_metadata": {
          "attributes": [
            {
              "trait_type": "Color",
              "value": "Red"
            },
            {
              "trait_type": "Size",
              "value": "Small"
            }
          ]
        },
        "collection": {
          "name": "Azuki"
        }
      },
      {
        "chain": "ethereum",
        "contract_address": "0xAbC1230000000000000000000000000000000000",
        "token_id": "1234",
        "name": "NFT #1234",
        "description": "Description of NFT #1234",
        "image_url": "https://cdn.simplehash.com/assets/2.png",
        "external_url": null,
        "background_color": null,
        "extra_metadata": {
          "attributes": [
            {
              "trait_type": "Attribute",
              "value": "Value"
            }
          ]
        }
      }
    ]
  })";

  SetInterceptors(responses_eth);
  TestGetNftMetadatas(mojom::CoinType::ETH, std::move(nft_identifiers),
                      std::move(expected_eth_metadata), "");

  // Add Solana NFT identifiers
  std::vector<mojom::NftIdentifierPtr> sol_nft_identifiers;
  auto sol_nft_identifier1 = mojom::NftIdentifier::New();
  sol_nft_identifier1->chain_id = mojom::kSolanaMainnet;
  sol_nft_identifier1->contract_address =
      "2iZBbRGnLVEEZH6JDsaNsTo66s2uxx7DTchVWKU8oisR";
  sol_nft_identifier1->token_id = "";
  sol_nft_identifiers.push_back(std::move(sol_nft_identifier1));

  auto sol_nft_identifier2 = mojom::NftIdentifier::New();
  sol_nft_identifier2->chain_id = mojom::kSolanaMainnet;
  sol_nft_identifier2->contract_address =
      "3knghmwnuaMxkiuqXrqzjL7gLDuRw6DkkZcW7F4mvkK8";
  sol_nft_identifier2->token_id = "";
  sol_nft_identifiers.push_back(std::move(sol_nft_identifier2));

  std::map<GURL, std::string> responses_sol;
  responses_sol[GURL(
      "https://simplehash.wallet.brave.com/api/v0/nfts/"
      "assets?nft_ids=solana.2iZBbRGnLVEEZH6JDsaNsTo66s2uxx7DTchVWKU8oisR%"
      "2Csolana.3knghmwnuaMxkiuqXrqzjL7gLDuRw6DkkZcW7F4mvkK8")] = R"({
    "nfts": [
      {
        "chain": "solana",
        "contract_address": "2iZBbRGnLVEEZH6JDsaNsTo66s2uxx7DTchVWKU8oisR",
        "token_id": null,
        "name": "Common Water Warrior #19",
        "description": "A true gladiator standing with his two back legs, big wings that make him move and attack quickly, and his tail like a big sword that can easily cut-off enemies into slices.",
        "image_url": "https://cdn.simplehash.com/assets/168e33bbf5276f717d8d190810ab93b4992ac8681054c1811f8248fe7636b54b.png",
        "external_url": null,
        "background_color": null,
        "extra_metadata": {
          "attributes": [
            {
              "trait_type": "rarity",
              "value": "Common"
            },
            {
              "trait_type": "dragonType",
              "value": "Water"
            },
            {
              "trait_type": "dragonClass",
              "value": "Warrior"
            }
          ]
        }
      },
      {
        "chain": "solana",
        "contract_address": "3knghmwnuaMxkiuqXrqzjL7gLDuRw6DkkZcW7F4mvkK8",
        "token_id": null,
        "name": "Sneaker #432819057",
        "description": "NFT Sneaker, use it in STEPN to move2earn",
        "image_url":
        "https://cdn.simplehash.com/assets/8ceccddf1868cf1d3860184fab3f084049efecdbaafb4eea43a1e33823c161a1.png",
        "external_url": "https://stepn.com",
        "background_color": null,
        "extra_metadata": {
          "attributes": [
            {
              "trait_type": "Sneaker type",
              "value": "Jogger"
            },
            {
              "trait_type": "Sneaker quality",
              "value": "Common"
            },
            {
              "trait_type": "Level",
              "value": "6"
            },
            {
              "trait_type": "Optimal Speed",
              "value": "4.0-10.0km/h"
            }
          ]
        }
      }
    ]
  })";

  // Add the expected Solana metadata
  std::vector<mojom::NftMetadataPtr> expected_sol_metadata;
  mojom::NftMetadataPtr sol_metadata1 = mojom::NftMetadata::New();
  sol_metadata1->name = "Common Water Warrior #19";
  sol_metadata1->description =
      "A true gladiator standing with his two back legs, big wings that make "
      "him move and attack quickly, and his tail like a big sword that can "
      "easily cut-off enemies into slices.";
  sol_metadata1->image =
      "https://simplehash.wallet-cdn.brave.com/assets/"
      "168e33bbf5276f717d8d190810ab93b4992ac8681054c1811f8248fe7636b54b.png";
  sol_metadata1->external_url = "";
  sol_metadata1->background_color = "";
  mojom::NftAttributePtr sol_attribute1 = mojom::NftAttribute::New();
  sol_attribute1->trait_type = "rarity";
  sol_attribute1->value = "Common";
  sol_metadata1->attributes.push_back(std::move(sol_attribute1));
  mojom::NftAttributePtr sol_attribute2 = mojom::NftAttribute::New();
  sol_attribute2->trait_type = "dragonType";
  sol_attribute2->value = "Water";
  sol_metadata1->attributes.push_back(std::move(sol_attribute2));
  mojom::NftAttributePtr sol_attribute3 = mojom::NftAttribute::New();
  sol_attribute3->trait_type = "dragonClass";
  sol_attribute3->value = "Warrior";
  sol_metadata1->attributes.push_back(std::move(sol_attribute3));
  sol_metadata1->background_color = "";
  sol_metadata1->animation_url = "";
  sol_metadata1->youtube_url = "";

  expected_sol_metadata.push_back(std::move(sol_metadata1));

  mojom::NftMetadataPtr sol_metadata2 = mojom::NftMetadata::New();
  sol_metadata2->name = "Sneaker #432819057";
  sol_metadata2->description = "NFT Sneaker, use it in STEPN to move2earn";
  sol_metadata2->image =
      "https://simplehash.wallet-cdn.brave.com/assets/"
      "8ceccddf1868cf1d3860184fab3f084049efecdbaafb4eea43a1e33823c161a1.png";
  sol_metadata2->external_url = "https://stepn.com";
  sol_metadata2->background_color = "";
  mojom::NftAttributePtr sol_attribute4 = mojom::NftAttribute::New();
  sol_attribute4->trait_type = "Sneaker type";
  sol_attribute4->value = "Jogger";
  sol_metadata2->attributes.push_back(std::move(sol_attribute4));
  mojom::NftAttributePtr sol_attribute5 = mojom::NftAttribute::New();
  sol_attribute5->trait_type = "Sneaker quality";
  sol_attribute5->value = "Common";
  sol_metadata2->attributes.push_back(std::move(sol_attribute5));
  mojom::NftAttributePtr sol_attribute6 = mojom::NftAttribute::New();
  sol_attribute6->trait_type = "Level";
  sol_attribute6->value = "6";
  sol_metadata2->attributes.push_back(std::move(sol_attribute6));
  mojom::NftAttributePtr sol_attribute7 = mojom::NftAttribute::New();
  sol_attribute7->trait_type = "Optimal Speed";
  sol_attribute7->value = "4.0-10.0km/h";
  sol_metadata2->attributes.push_back(std::move(sol_attribute7));
  sol_metadata2->background_color = "";
  sol_metadata2->animation_url = "";
  sol_metadata2->youtube_url = "";
  expected_sol_metadata.push_back(std::move(sol_metadata2));

  // First try with timeout response interceptor
  SetHTTPRequestTimeoutInterceptor();
  TestGetNftMetadatas(mojom::CoinType::SOL, std::move(sol_nft_identifiers), {},
                      l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));

  // Then try with the expected Solana metadata
  SetInterceptors(responses_sol);
  std::vector<mojom::NftIdentifierPtr> sol_nft_identifiers2;
  auto sol_nft_identifier3 = mojom::NftIdentifier::New();
  sol_nft_identifier3->chain_id = mojom::kSolanaMainnet;
  sol_nft_identifier3->contract_address =
      "2iZBbRGnLVEEZH6JDsaNsTo66s2uxx7DTchVWKU8oisR";
  sol_nft_identifier3->token_id = "";
  sol_nft_identifiers2.push_back(std::move(sol_nft_identifier3));

  auto sol_nft_identifier4 = mojom::NftIdentifier::New();
  sol_nft_identifier4->chain_id = mojom::kSolanaMainnet;
  sol_nft_identifier4->contract_address =
      "3knghmwnuaMxkiuqXrqzjL7gLDuRw6DkkZcW7F4mvkK8";
  sol_nft_identifier4->token_id = "";
  sol_nft_identifiers2.push_back(std::move(sol_nft_identifier4));

  TestGetNftMetadatas(mojom::CoinType::SOL, std::move(sol_nft_identifiers2),
                      std::move(expected_sol_metadata), "");
}

TEST_F(JsonRpcServiceUnitTest, GetNftBalances) {
  std::string wallet_address = "0x123";
  std::vector<mojom::NftIdentifierPtr> nft_identifiers;
  mojom::CoinType coin = mojom::CoinType::SOL;
  std::vector<uint64_t> expected_balances;

  // Empty parameters yields invalid params
  TestGetNftBalances(wallet_address, std::move(nft_identifiers), coin,
                     expected_balances,
                     l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
  nft_identifiers = std::vector<mojom::NftIdentifierPtr>();

  // More than 50 NFTs yields invalid params
  for (size_t i = 0; i < kSimpleHashMaxBatchSize + 1; i++) {
    auto nft_id = mojom::NftIdentifier::New();
    nft_id->chain_id = mojom::kMainnetChainId;
    nft_id->contract_address = "0x" + base::NumberToString(i);
    nft_id->token_id = "0x" + base::NumberToString(i);
    nft_identifiers.push_back(std::move(nft_id));
  }
  TestGetNftBalances(wallet_address, std::move(nft_identifiers), coin,
                     expected_balances,
                     l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
  nft_identifiers = std::vector<mojom::NftIdentifierPtr>();

  // Response includes two NFTs, wallet address is included in only one of them
  std::string json = R"({
    "nfts": [
      {
        "nft_id": "solana.3knghmwnuaMxkiuqXrqzjL7gLDuRw6DkkZcW7F4mvkK8",
        "chain": "solana",
        "contract_address": "3knghmwnuaMxkiuqXrqzjL7gLDuRw6DkkZcW7F4mvkK8",
        "token_id": null,
        "name": "Sneaker #432819057",
        "owners": [
          {
            "owner_address": "0x123",
            "quantity": 999
          },
          {
            "owner_address": "0x456",
            "quantity": 2
          }
        ]
      },
      {
        "nft_id": "solana.2iZBbRGnLVEEZH6JDsaNsTo66s2uxx7DTchVWKU8oisR",
        "chain": "solana",
        "contract_address": "2iZBbRGnLVEEZH6JDsaNsTo66s2uxx7DTchVWKU8oisR",
        "token_id": null,
        "name": "Common Water Warrior #19",
        "owners": [
          {
            "owner_address": "0x456",
            "quantity": 3
          }
        ]
      }
    ]
  })";

  // Add the chain_id, contract, and token_id from simple hash response
  auto nft_identifier1 = mojom::NftIdentifier::New();
  nft_identifier1->chain_id = mojom::kSolanaMainnet;
  nft_identifier1->contract_address =
      "3knghmwnuaMxkiuqXrqzjL7gLDuRw6DkkZcW7F4mvkK8";
  nft_identifier1->token_id = "";
  nft_identifiers.push_back(std::move(nft_identifier1));

  auto nft_identifier2 = mojom::NftIdentifier::New();
  nft_identifier2->chain_id = mojom::kSolanaMainnet;
  nft_identifier2->contract_address =
      "2izbbrgnlveezh6jdsansto66s2uxx7dtchvwku8oisr";
  nft_identifier2->token_id = "";
  nft_identifiers.push_back(std::move(nft_identifier2));

  std::map<GURL, std::string> responses;
  responses[GURL(
      "https://simplehash.wallet.brave.com/api/v0/nfts/"
      "assets?nft_ids=solana.3knghmwnuaMxkiuqXrqzjL7gLDuRw6DkkZcW7F4mvkK8%"
      "2Csolana.2izbbrgnlveezh6jdsansto66s2uxx7dtchvwku8oisr")] = json;

  // Add the expected balances
  expected_balances.push_back(999);
  expected_balances.push_back(0);
  SetInterceptors(responses);
  TestGetNftBalances(wallet_address, std::move(nft_identifiers), coin,
                     expected_balances, "");
}

}  // namespace brave_wallet
