/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/common/eth_request_helper.h"

#include <algorithm>
#include <memory>
#include <optional>
#include <tuple>
#include <utility>

#include "base/base64.h"
#include "base/check.h"
#include "base/containers/to_vector.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/string_view_util.h"
#include "base/types/optional_util.h"
#include "base/values.h"
#include "brave/components/brave_wallet/common/eth_address.h"
#include "brave/components/brave_wallet/common/eth_requests.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "brave/components/brave_wallet/common/json_rpc_requests.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"
#include "url/gurl.h"

namespace {

constexpr char kId[] = "id";
constexpr char kMethod[] = "method";
constexpr char kParams[] = "params";

// This is a best effort parsing of the data
brave_wallet::mojom::TxDataPtr ValueToTxData(
    const brave_wallet::json_rpc_requests::Transaction& tx) {
  auto tx_data = brave_wallet::mojom::TxData::New();

  if (tx.to) {
    tx_data->to = *tx.to;
  }
  if (tx.gas) {
    tx_data->gas_limit = *tx.gas;
  }
  if (tx.gas_price) {
    tx_data->gas_price = *tx.gas_price;
  }
  if (tx.value) {
    tx_data->value = *tx.value;
  }

  // If data is specified it's best to make sure it's valid
  std::vector<uint8_t> bytes;
  if (tx.data && !tx.data->empty() &&
      !brave_wallet::PrefixedHexStringToBytes(*tx.data, &bytes)) {
    return nullptr;
  }
  tx_data->data = bytes;

  return tx_data;
}

// null request ID when unspecified is expected
constexpr char kDefaultRequestIdWhenUnspecified[] = "1";
constexpr char kRequestJsonRPC[] = "2.0";

}  // namespace

namespace brave_wallet {

namespace {
// EIP-712 type hash for Order struct used in CoW swap
//
// keccak256("Order(address sellToken,address buyToken,address receiver,
//                  uint256 sellAmount,uint256 buyAmount,uint32 validTo,
//                  bytes32 appData,uint256 feeAmount,string kind,
//                  bool partiallyFillable,string sellTokenBalance,
//                  string buyTokenBalance)")
constexpr char kCowSwapTypeHash[] =
    "D5A25BA2E97094AD7D83DC28A6572DA797D6B3E7FC6663BD93EFB789FC17E489";

mojom::EthSignTypedDataMetaPtr ParseCowSwapOrder(
    const base::Value::Dict& value) {
  const auto cow_swap_order_value =
      eth_requests::CowSwapOrder::FromValue(value);
  if (!cow_swap_order_value) {
    return nullptr;
  }

  auto cow_swap_order = mojom::CowSwapOrder::New();
  cow_swap_order->buy_token = cow_swap_order_value->buy_token;
  cow_swap_order->buy_amount = cow_swap_order_value->buy_amount;
  cow_swap_order->sell_token = cow_swap_order_value->sell_token;
  cow_swap_order->sell_amount = cow_swap_order_value->sell_amount;
  cow_swap_order->deadline = cow_swap_order_value->valid_to;
  cow_swap_order->receiver = cow_swap_order_value->receiver;

  return mojom::EthSignTypedDataMeta::NewCowSwapOrder(
      std::move(cow_swap_order));
}

}  // namespace

mojom::TxData1559Ptr ParseEthTransaction1559Params(
    const base::Value::List& params,
    std::string& from_out) {
  if (params.size() != 1) {
    return nullptr;
  }

  const auto* param_obj = params.front().GetIfDict();
  if (!param_obj) {
    return nullptr;
  }

  auto tx = brave_wallet::json_rpc_requests::Transaction::FromValue(*param_obj);
  if (!tx) {
    return nullptr;
  }
  from_out = tx->from;

  auto tx_data = mojom::TxData1559::New();
  auto base_data_ret = ValueToTxData(*tx);
  if (!base_data_ret) {
    return nullptr;
  }

  tx_data->base_data = std::move(base_data_ret);

  // MM accepts `null` in these fields as no value:
  // https://github.com/MetaMask/core/blob/5d9dec2085607b9dbf7667e328a16a5652da07fa/packages/transaction-controller/src/utils/validation.ts#L374-L395
  if (tx->max_priority_fee_per_gas) {
    if (tx->max_priority_fee_per_gas->is_string()) {
      tx_data->max_priority_fee_per_gas =
          tx->max_priority_fee_per_gas->GetString();
    } else if (!tx->max_priority_fee_per_gas->is_none()) {
      return nullptr;
    }
  }
  if (tx->max_fee_per_gas) {
    if (tx->max_fee_per_gas->is_string()) {
      tx_data->max_fee_per_gas = tx->max_fee_per_gas->GetString();
    } else if (!tx->max_fee_per_gas->is_none()) {
      return nullptr;
    }
  }

  return tx_data;
}

bool ShouldCreate1559Tx(const mojom::TxData1559& tx_data_1559) {
  // Network with EIP1559 support and EIP1559 gas fields are specified.
  if (!tx_data_1559.max_priority_fee_per_gas.empty() &&
      !tx_data_1559.max_fee_per_gas.empty()) {
    return true;
  }

  // Network with EIP1559 support and legacy gas fields are specified.
  if (!tx_data_1559.base_data->gas_price.empty()) {
    return false;
  }

  // Network with EIP1559 support and no gas fields are specified.
  return true;
}

bool GetEthJsonRequestInfo(std::string_view json,
                           base::Value* id,
                           std::string* method,
                           base::Value::List* params_list) {
  std::optional<base::Value::Dict> response_dict = base::JSONReader::ReadDict(
      json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                base::JSONParserOptions::JSON_PARSE_RFC);
  if (!response_dict) {
    return false;
  }

  if (id) {
    const base::Value* found_id = response_dict->FindByDottedPath(kId);
    if (found_id) {
      *id = found_id->Clone();
    } else {
      *id = base::Value();
    }
  }

  if (method) {
    const std::string* found_method =
        response_dict->FindStringByDottedPath(kMethod);
    if (!found_method) {
      return false;
    }
    *method = *found_method;
  }

  if (params_list) {
    params_list->clear();
    if (auto* found_params = response_dict->FindByDottedPath(kParams)) {
      if (found_params->is_list()) {
        *params_list = std::move(found_params->GetList());
      } else if (found_params->is_dict()) {
        params_list->Append(std::move(found_params->GetDict()));
      } else {
        return false;
      }
    }
  }

  return true;
}

std::optional<JsonRpcRequest> ParseJsonRpcRequest(base::Value input_value) {
  auto* dict = input_value.GetIfDict();
  if (!dict) {
    return std::nullopt;
  }

  JsonRpcRequest result;
  auto* found_id = dict->Find(kId);
  if (found_id) {
    result.id = std::move(*found_id);
  } else {
    result.id = base::Value(kDefaultRequestIdWhenUnspecified);
  }

  auto* found_method = dict->FindString(kMethod);
  if (!found_method) {
    return std::nullopt;
  }
  result.method = std::move(*found_method);

  // Only list and dict types for `params` are supported. Otherwise parsing
  // fails.
  // If it is a list just use it. It is a dict use it as a 1-item list to
  // simplify further methods handling.
  if (auto* found_params = dict->Find(kParams)) {
    if (found_params->is_list()) {
      result.params = std::move(found_params->GetList());
    } else if (found_params->is_dict()) {
      result.params.Append(std::move(found_params->GetDict()));
    } else {
      return std::nullopt;
    }
  }

  return result;
}

bool NormalizeEthRequest(std::string_view input_json,
                         std::string* output_json) {
  CHECK(output_json);
  std::optional<base::Value::Dict> out_dict = base::JSONReader::ReadDict(
      input_json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                      base::JSONParserOptions::JSON_PARSE_RFC);
  if (!out_dict) {
    return false;
  }

  const base::Value* found_id = out_dict->FindByDottedPath(kId);
  if (!found_id) {
    std::ignore = out_dict->Set("id", kDefaultRequestIdWhenUnspecified);
  }

  std::ignore = out_dict->Set("jsonrpc", kRequestJsonRPC);
  base::JSONWriter::Write(*out_dict, output_json);

  return true;
}

std::optional<EthSignParams> ParseEthSignParams(
    const base::Value::List& params) {
  if (params.size() != 2) {
    return std::nullopt;
  }

  const std::string* address_str = params[0].GetIfString();
  const std::string* message_str = params[1].GetIfString();
  if (!address_str || !message_str) {
    return std::nullopt;
  }

  EthSignParams result;
  result.address = *address_str;
  result.message = *message_str;

  return result;
}

std::optional<EthSignParams> ParsePersonalSignParams(
    const base::Value::List& params) {
  // personal_sign allows extra params
  if (params.size() < 2) {
    return std::nullopt;
  }

  // personal_sign has the reversed order
  const std::string* message_str = params[0].GetIfString();
  const std::string* address_str = params[1].GetIfString();
  if (!address_str || !message_str) {
    return std::nullopt;
  }

  // MetaMask accepts input in the wrong order, so we try for the right order
  // but if it's invalid then we allow it to be swapped if the other combination
  // is valid
  if (!EthAddress::IsValidAddress(*address_str) &&
      EthAddress::IsValidAddress(*message_str)) {
    std::swap(address_str, message_str);
  }

  EthSignParams result;
  result.address = *address_str;
  // MM encodes 0x as a string and not an empty value
  if (IsValidHexString(*message_str) && *message_str != "0x") {
    result.message = *message_str;
  } else if (IsValidHexString("0x" + *message_str)) {
    result.message = "0x" + *message_str;
  } else {
    result.message = ToHex(*message_str);
  }

  return result;
}

std::optional<std::string> ParseEthGetEncryptionPublicKeyParams(
    const base::Value::List& params) {
  // eth_getEncryptionPublicKey allows extra params
  if (params.empty()) {
    return std::nullopt;
  }

  return base::OptionalFromPtr(params.front().GetIfString());
}

std::optional<EthDecryptParams> ParseEthDecryptParams(
    const base::Value::List& params) {
  // eth_decrypt allows extra params
  if (params.size() < 2) {
    return std::nullopt;
  }

  const std::string* untrusted_hex_json_str = params[0].GetIfString();
  if (!untrusted_hex_json_str) {
    return std::nullopt;
  }

  const std::string* address_str = params[1].GetIfString();
  if (!address_str) {
    return std::nullopt;
  }

  std::string untrusted_json;

  // untrusted_hex_json should hex decode to this schema example:
  // {
  //   "version": "x25519-xsalsa20-poly1305",
  //   "nonce": "base64-string",
  //   "ephemPublicKey": "base64-string",
  //   "ciphertext":"base64-string"
  // }

  if (!IsValidHexString(*untrusted_hex_json_str)) {
    return std::nullopt;
  }

  // IsValidHexString guarantees at least 2 bytes and starts with 0x
  if (!base::HexStringToString(
          base::as_string_view(*untrusted_hex_json_str).substr(2),
          &untrusted_json)) {
    return std::nullopt;
  }

  EthDecryptParams result;
  result.untrusted_encrypted_data_json = untrusted_json;
  result.address = *address_str;
  return result;
}

std::optional<PersonalEcRecoverParams> ParsePersonalEcRecoverParams(
    const base::Value::List& params) {
  // personal_ecRecover allows extra params
  if (params.size() < 2) {
    return std::nullopt;
  }

  const std::string* message_str = params[0].GetIfString();
  const std::string* signature_str = params[1].GetIfString();
  if (!message_str || !signature_str) {
    return std::nullopt;
  }

  PersonalEcRecoverParams result;
  if (IsValidHexString(*message_str)) {
    result.message = *message_str;
  } else {
    result.message = ToHex(*message_str);
  }

  if (!IsValidHexString(*signature_str)) {
    return std::nullopt;
  }

  result.signature = *signature_str;
  return result;
}

mojom::EthSignTypedDataPtr ParseEthSignTypedDataParams(
    const base::Value::List& params,
    EthSignTypedDataHelper::Version version) {
  if (params.size() != 2) {
    return nullptr;
  }

  const std::string* address_str = params[0].GetIfString();
  if (!address_str) {
    return nullptr;
  }

  const base::Value::Dict* dict = nullptr;
  std::optional<base::Value::Dict> dict_from_str;

  if (const auto* typed_data_str = params[1].GetIfString()) {
    dict_from_str = base::JSONReader::ReadDict(
        *typed_data_str, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                             base::JSON_ALLOW_TRAILING_COMMAS);
    if (dict_from_str) {
      dict = &*dict_from_str;
    }
  } else {
    dict = params[1].GetIfDict();
  }

  if (!dict) {
    return nullptr;
  }

  const std::string* primary_type = dict->FindString("primaryType");
  if (!primary_type) {
    return nullptr;
  }

  auto* domain = dict->FindDict("domain");
  if (!domain) {
    return nullptr;
  }

  const auto* message = dict->FindDict("message");
  if (!message) {
    return nullptr;
  }

  auto* types = dict->FindDict("types");
  if (!types) {
    return nullptr;
  }
  std::unique_ptr<EthSignTypedDataHelper> helper =
      EthSignTypedDataHelper::Create(types->Clone(), version);
  if (!helper) {
    return nullptr;
  }
  auto domain_hash = helper->GetTypedDataDomainHash(*domain);
  if (!domain_hash) {
    return nullptr;
  }
  // TODO(apaymyshev): there might be no message hash
  // https://github.com/trezor/trezor-firmware/blob/a1ab50017d55c9986fc4a11ddcaff86158804604/legacy/firmware/ethereum.c#L984-L986
  // https://github.com/MetaMask/eth-sig-util/blob/66a8c0935c14d6ef80b583148d0c758c198a9c4a/src/index.ts#L345
  // https://github.com/LedgerHQ/app-ethereum/blob/f0f20d1db69d82263f67ad3e2172fc4cea524d3a/src_features/signMessageEIP712/path.c#L414-L416
  auto primary_hash = helper->GetTypedDataPrimaryHash(*primary_type, *message);
  if (!primary_hash) {
    return nullptr;
  }

  mojom::EthSignTypedDataPtr result = mojom::EthSignTypedData::New();
  result->address_param = *address_str;

  auto type_hash = base::HexEncode(helper->GetTypeHash(*primary_type));
  if (type_hash == kCowSwapTypeHash) {
    result->meta = ParseCowSwapOrder(*message);
  } else {
    result->meta = nullptr;
  }

  result->domain_hash = base::ToVector(domain_hash->first);
  if (!base::JSONWriter::Write(domain_hash->second, &result->domain_json)) {
    return nullptr;
  }

  result->primary_hash = base::ToVector(primary_hash->first);
  if (!base::JSONWriter::Write(primary_hash->second, &result->message_json)) {
    return nullptr;
  }

  if (!base::JSONWriter::Write(*types, &result->types_json)) {
    return nullptr;
  }

  result->primary_type = *primary_type;

  auto chain_id = domain->FindDouble("chainId");
  if (chain_id) {
    result->chain_id = Uint256ValueToHex(uint256_t(uint64_t(*chain_id)));
  }

  DCHECK(!result->domain_hash.empty());
  DCHECK(!result->primary_hash.empty());
  return result;
}

EthDecryptData::EthDecryptData() = default;
EthDecryptData::~EthDecryptData() = default;
EthDecryptData::EthDecryptData(EthDecryptData&&) = default;

std::optional<EthDecryptData> ParseEthDecryptData(
    const base::Value::Dict& dict) {
  // {
  //   "version": "x25519-xsalsa20-poly1305",
  //   "nonce": "base64-string",
  //   "ephemPublicKey": "base64-string",
  //   "ciphertext":"base64-string"
  // }

  EthDecryptData result;
  const std::string* version_str = dict.FindString("version");
  if (!version_str) {
    return std::nullopt;
  }
  result.version = *version_str;

  const std::string* nonce_str = dict.FindString("nonce");
  if (!nonce_str) {
    return std::nullopt;
  }
  auto nonce = base::Base64Decode(*nonce_str);
  if (!nonce) {
    return std::nullopt;
  }
  result.nonce = std::move(*nonce);

  const std::string* ephemeral_public_key_str =
      dict.FindString("ephemPublicKey");
  if (!ephemeral_public_key_str) {
    return std::nullopt;
  }
  auto ephemeral_public_key = base::Base64Decode(*ephemeral_public_key_str);
  if (!ephemeral_public_key) {
    return std::nullopt;
  }
  result.ephemeral_public_key = std::move(*ephemeral_public_key);

  const std::string* ciphertext_str = dict.FindString("ciphertext");
  if (!ciphertext_str) {
    return std::nullopt;
  }
  auto ciphertext = base::Base64Decode(*ciphertext_str);
  if (!ciphertext) {
    return std::nullopt;
  }
  result.ciphertext = std::move(*ciphertext);

  return result;
}

std::optional<std::string> ParseSwitchEthereumChainParams(
    const base::Value::List& params) {
  if (params.size() != 1 || !params.front().is_dict()) {
    return std::nullopt;
  }

  auto& param_obj = params.front().GetDict();

  const std::string* chain_id_str = param_obj.FindString("chainId");
  if (!chain_id_str) {
    return std::nullopt;
  }

  if (!IsValidHexString(*chain_id_str)) {
    return std::nullopt;
  }

  return base::ToLowerASCII(*chain_id_str);
}

mojom::BlockchainTokenPtr ParseWalletWatchAssetParams(
    const base::Value::List& params,
    std::string& error_message) {
  error_message = "";

  if (params.size() != 1) {
    return nullptr;
  }

  auto* dict = params.front().GetIfDict();

  if (!dict) {
    error_message = "params parameter is required";
    return nullptr;
  }

  const std::string* type = dict->FindString("type");
  if (!type) {
    error_message = "type parameter is required";
    return nullptr;
  }
  // Only ERC20 is supported currently.
  if (*type != "ERC20") {
    error_message = absl::StrFormat("Asset of type '%s' not supported", *type);
    return nullptr;
  }

  const auto* options_dict = dict->FindDict("options");
  if (!options_dict) {
    error_message = "options parameter is required";
    return nullptr;
  }

  const std::string* address = options_dict->FindString("address");
  if (!address) {
    error_message = "address parameter is required";
    return nullptr;
  }

  const auto eth_addr = EthAddress::FromHex(*address);
  if (eth_addr.IsEmpty()) {
    error_message = absl::StrFormat("Invalid address '%s'", *address);
    return nullptr;
  }

  const std::string* symbol = options_dict->FindString("symbol");
  if (!symbol) {
    error_message = "symbol parameter is required";
    return nullptr;
  }

  // EIP-747 limits the symbol length to 5, but metamask uses 11, so we use
  // the same limit here for compatibility.
  if (symbol->size() == 0 || symbol->size() > 11) {
    error_message = absl::StrFormat(
        "Invalid symbol '%s': symbol length should be greater than 0 and less "
        "than 12",
        *symbol);
    return nullptr;
  }

  // Allow decimals in both number and string for compatibility.
  // EIP747 specifies the type of decimals number, but websites like coingecko
  // uses string.
  const base::Value* decimals_value = options_dict->Find("decimals");
  if (!decimals_value ||
      (!decimals_value->is_int() && !decimals_value->is_string())) {
    error_message = "decimals parameter is required.";
    return nullptr;
  }
  int decimals = decimals_value->is_int() ? decimals_value->GetInt() : 0;
  if (decimals_value->is_string() &&
      !base::StringToInt(decimals_value->GetString(), &decimals)) {
    error_message = absl::StrFormat(
        "Invalid decimals '%s': decimals should be a number greater than 0 and "
        "less than 36",
        decimals_value->GetString());
    return nullptr;
  }

  if (decimals < 0 || decimals > 36) {
    error_message = absl::StrFormat(
        "Invalid decimals '%d': decimals should be greater than 0 and less "
        "than 36",
        decimals);
    return nullptr;
  }

  std::string logo;
  const std::string* image = options_dict->FindString("image");
  if (image) {
    GURL url = GURL(*image);
    if (url.is_valid() &&
        (url.SchemeIsHTTPOrHTTPS() || image->starts_with("data:image/"))) {
      logo = url.spec();
    }
  }

  return mojom::BlockchainToken::New(
      eth_addr.ToChecksumAddress(), *symbol /* name */, logo,
      false /* is_compressed */, true /* is_erc20 */, false /* is_erc721 */,
      false /* is_erc1155 */, mojom::SPLTokenProgram::kUnsupported,
      false /* is_nft */, false /* is_spam */, *symbol, decimals, true, "", "",
      "" /* chain_id */, mojom::CoinType::ETH, false);
}

// Parses param request objects from
// https://eips.ethereum.org/EIPS/eip-2255
std::optional<base::flat_set<std::string>> ParseRequestPermissionsParams(
    const base::Value::List& params) {
  // [{
  //   "eth_accounts": {}
  // }]

  if (params.size() != 1) {
    return std::nullopt;
  }

  const auto* param_obj = params.front().GetIfDict();
  if (!param_obj) {
    return std::nullopt;
  }
  std::vector<std::string> restricted_methods;
  for (auto prop : *param_obj) {
    restricted_methods.push_back(prop.first);
  }
  return restricted_methods;
}

std::optional<std::string> ParseEthSendRawTransactionParams(
    const base::Value::List& params) {
  if (params.size() != 1) {
    return std::nullopt;
  }

  return base::OptionalFromPtr(params.front().GetIfString());
}

std::optional<EthSubscribeParams> ParseEthSubscribeParams(
    const base::Value::List& params) {
  if (params.size() != 1 && params.size() != 2) {
    return std::nullopt;
  }

  if (!params[0].is_string()) {
    return std::nullopt;
  }

  if (params.size() == 2 && !params[1].is_dict()) {
    return std::nullopt;
  }

  EthSubscribeParams result;
  result.event_type = params[0].GetString();
  if (params.size() == 2) {
    result.filter = params[1].GetDict().Clone();
  }

  return result;
}

std::optional<std::string> ParseEthUnsubscribeParams(
    const base::Value::List& params) {
  if (params.size() != 1) {
    return std::nullopt;
  }

  return base::OptionalFromPtr(params.front().GetIfString());
}

}  // namespace brave_wallet
