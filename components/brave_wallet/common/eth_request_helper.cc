/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/common/eth_request_helper.h"

#include <memory>
#include <optional>
#include <tuple>
#include <utility>

#include "base/base64.h"
#include "base/containers/span.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/ranges/algorithm.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/values.h"
#include "brave/components/brave_wallet/common/eth_address.h"
#include "brave/components/brave_wallet/common/eth_requests.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "brave/components/brave_wallet/common/json_rpc_requests.h"
#include "brave/components/brave_wallet/common/web3_provider_constants.h"
#include "url/gurl.h"

namespace {

std::optional<base::Value::List> GetParamsList(const std::string& json) {
  auto json_value =
      base::JSONReader::Read(json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                                       base::JSON_ALLOW_TRAILING_COMMAS);
  if (!json_value || !json_value->is_dict()) {
    return std::nullopt;
  }

  auto& value = json_value->GetDict();
  auto* params = value.FindListByDottedPath(brave_wallet::kParams);
  if (!params) {
    return std::nullopt;
  }

  return std::move(*params);
}

std::optional<base::Value::Dict> GetObjectFromParamsList(
    const std::string& json) {
  auto list = GetParamsList(json);
  if (!list || list->size() != 1 || !(*list)[0].is_dict()) {
    return std::nullopt;
  }

  return std::move((*list)[0]).TakeDict();
}

std::optional<base::Value::Dict> GetParamsDict(const std::string& json) {
  auto json_value =
      base::JSONReader::Read(json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                                       base::JSON_ALLOW_TRAILING_COMMAS);
  if (!json_value || !json_value->is_dict()) {
    return std::nullopt;
  }
  auto* value = json_value->GetDict().FindDict(brave_wallet::kParams);
  if (!value) {
    return std::nullopt;
  }

  return std::move(*value);
}

// This is a best effort parsing of the data
brave_wallet::mojom::TxDataPtr ValueToTxData(
    const brave_wallet::json_rpc_requests::Transaction& tx,
    std::string* from_out) {
  CHECK(from_out);
  auto tx_data = brave_wallet::mojom::TxData::New();

  *from_out = tx.from;
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

mojom::TxDataPtr ParseEthTransactionParams(const std::string& json,
                                           std::string* from) {
  CHECK(from);
  from->clear();

  auto param_obj = GetObjectFromParamsList(json);
  if (!param_obj) {
    return nullptr;
  }
  auto tx = brave_wallet::json_rpc_requests::Transaction::FromValue(*param_obj);
  if (!tx) {
    return nullptr;
  }
  return ValueToTxData(*tx, from);
}

mojom::TxData1559Ptr ParseEthTransaction1559Params(const std::string& json,
                                                   std::string* from) {
  CHECK(from);
  from->clear();
  auto param_obj = GetObjectFromParamsList(json);
  if (!param_obj) {
    return nullptr;
  }

  auto tx = brave_wallet::json_rpc_requests::Transaction::FromValue(*param_obj);
  if (!tx) {
    return nullptr;
  }

  auto tx_data = mojom::TxData1559::New();
  auto base_data_ret = ValueToTxData(*tx, from);
  if (!base_data_ret) {
    return nullptr;
  }

  tx_data->base_data = std::move(base_data_ret);

  if (tx->max_priority_fee_per_gas) {
    tx_data->max_priority_fee_per_gas = *tx->max_priority_fee_per_gas;
  }
  if (tx->max_fee_per_gas) {
    tx_data->max_fee_per_gas = *tx->max_fee_per_gas;
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

bool GetEthJsonRequestInfo(const std::string& json,
                           base::Value* id,
                           std::string* method,
                           base::Value::List* params_list) {
  std::optional<base::Value> records_v =
      base::JSONReader::Read(json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                                       base::JSONParserOptions::JSON_PARSE_RFC);
  if (!records_v) {
    return false;
  }

  base::Value::Dict* response_dict = records_v->GetIfDict();
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
      } else if (!found_params->is_dict()) {
        // dict is a valid type of params but we don't support it.
        return false;
      }
    }
  }

  return true;
}

bool NormalizeEthRequest(const std::string& input_json,
                         std::string* output_json) {
  CHECK(output_json);
  std::optional<base::Value> records_v = base::JSONReader::Read(
      input_json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                      base::JSONParserOptions::JSON_PARSE_RFC);
  if (!records_v) {
    return false;
  }

  base::Value::Dict* out_dict = records_v->GetIfDict();
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

bool ParseEthSignParams(const std::string& json,
                        std::string* address,
                        std::string* message) {
  if (!address || !message) {
    return false;
  }

  auto list = GetParamsList(json);
  if (!list || list->size() != 2) {
    return false;
  }

  const std::string* address_str = (*list)[0].GetIfString();
  const std::string* message_str = (*list)[1].GetIfString();
  if (!address_str || !message_str) {
    return false;
  }

  *address = *address_str;
  *message = *message_str;

  return true;
}

bool ParsePersonalSignParams(const std::string& json,
                             std::string* address,
                             std::string* message) {
  if (!address || !message) {
    return false;
  }

  // personal_sign allows extra params
  auto list = GetParamsList(json);
  if (!list || list->size() < 2) {
    return false;
  }

  // personal_sign has the reversed order
  const std::string* message_str = (*list)[0].GetIfString();
  const std::string* address_str = (*list)[1].GetIfString();
  if (!address_str || !message_str) {
    return false;
  }

  // MetaMask accepts input in the wrong order, so we try for the right order
  // but if it's invalid then we allow it to be swapped if the other combination
  // is valid
  if (!EthAddress::IsValidAddress(*address_str) &&
      EthAddress::IsValidAddress(*message_str)) {
    const std::string* temp = address_str;
    address_str = message_str;
    message_str = temp;
  }

  *address = *address_str;
  // MM encodes 0x as a string and not an empty value
  if (IsValidHexString(*message_str) && *message_str != "0x") {
    *message = *message_str;
  } else if (IsValidHexString("0x" + *message_str)) {
    *message = "0x" + *message_str;
  } else {
    *message = ToHex(*message_str);
  }

  return true;
}

bool ParseEthGetEncryptionPublicKeyParams(const std::string& json,
                                          std::string* address) {
  if (!address) {
    return false;
  }

  // eth_getEncryptionPublicKey allows extra params
  auto list = GetParamsList(json);
  if (!list || list->size() < 1) {
    return false;
  }

  const std::string* address_str = (*list)[0].GetIfString();
  if (!address_str) {
    return false;
  }

  *address = *address_str;
  return true;
}

bool ParseEthDecryptParams(const std::string& json,
                           std::string* untrusted_encrypted_data_json,
                           std::string* address) {
  if (!address) {
    return false;
  }

  // eth_decrypt allows extra params
  auto list = GetParamsList(json);
  if (!list || list->size() < 2) {
    return false;
  }

  const std::string* untrusted_hex_json_str = (*list)[0].GetIfString();
  if (!untrusted_hex_json_str) {
    return false;
  }

  const std::string* address_str = (*list)[1].GetIfString();
  if (!address_str) {
    return false;
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
    return false;
  }

  // IsValidHexString guarantees at least 2 bytes and starts with 0x
  if (!base::HexStringToString(
          base::as_string_view(*untrusted_hex_json_str).substr(2),
          &untrusted_json)) {
    return false;
  }

  *untrusted_encrypted_data_json = untrusted_json;
  *address = *address_str;
  return true;
}

bool ParsePersonalEcRecoverParams(const std::string& json,
                                  std::string* message,
                                  std::string* signature) {
  if (!message || !signature) {
    return false;
  }

  // personal_ecRecover allows extra params
  auto list = GetParamsList(json);
  if (!list || list->size() < 2) {
    return false;
  }

  const std::string* message_str = (*list)[0].GetIfString();
  const std::string* signature_str = (*list)[1].GetIfString();
  if (!message_str || !signature_str) {
    return false;
  }

  if (IsValidHexString(*message_str)) {
    *message = *message_str;
  } else {
    *message = ToHex(*message_str);
  }

  if (!IsValidHexString(*signature_str)) {
    return false;
  }

  *signature = *signature_str;
  return true;
}

mojom::EthSignTypedDataPtr ParseEthSignTypedDataParams(
    const base::Value::List& params_list,
    EthSignTypedDataHelper::Version version) {
  if (params_list.size() != 2) {
    return nullptr;
  }

  const std::string* address_str = params_list[0].GetIfString();
  if (!address_str) {
    return nullptr;
  }

  const std::string* typed_data_str = params_list[1].GetIfString();
  if (!typed_data_str) {
    return nullptr;
  }

  // TODO(apaymyshev): support dict there per
  // https://github.com/MetaMask/metamask-extension/issues/18462
  auto typed_data = base::JSONReader::Read(
      *typed_data_str,
      base::JSON_PARSE_CHROMIUM_EXTENSIONS | base::JSON_ALLOW_TRAILING_COMMAS);
  if (!typed_data) {
    return nullptr;
  }
  auto* dict = typed_data->GetIfDict();
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

  result->domain_hash.assign(domain_hash->first.begin(),
                             domain_hash->first.end());
  if (!base::JSONWriter::Write(domain_hash->second, &result->domain_json)) {
    return nullptr;
  }

  result->primary_hash.assign(primary_hash->first.begin(),
                              primary_hash->first.end());
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

bool ParseEthDecryptData(const base::Value& obj,
                         std::string* version,
                         std::vector<uint8_t>* nonce,
                         std::vector<uint8_t>* ephemeral_public_key,
                         std::vector<uint8_t>* ciphertext) {
  // {
  //   "version": "x25519-xsalsa20-poly1305",
  //   "nonce": "base64-string",
  //   "ephemPublicKey": "base64-string",
  //   "ciphertext":"base64-string"
  // }
  auto* dict = obj.GetIfDict();
  if (!dict) {
    return false;
  }

  const std::string* version_str = dict->FindString("version");
  if (!version_str) {
    return false;
  }
  *version = *version_str;

  const std::string* nonce_str = dict->FindString("nonce");
  if (!nonce_str) {
    return false;
  }
  std::string decoded_nonce;
  if (!base::Base64Decode(*nonce_str, &decoded_nonce)) {
    return false;
  }
  *nonce = std::vector<uint8_t>(decoded_nonce.begin(), decoded_nonce.end());

  const std::string* ephemeral_public_key_str =
      dict->FindString("ephemPublicKey");
  if (!ephemeral_public_key_str) {
    return false;
  }
  std::string ephemeral_public_key_decoded;
  if (!base::Base64Decode(*ephemeral_public_key_str,
                          &ephemeral_public_key_decoded)) {
    return false;
  }
  *ephemeral_public_key = std::vector<uint8_t>(
      ephemeral_public_key_decoded.begin(), ephemeral_public_key_decoded.end());

  const std::string* ciphertext_str = dict->FindString("ciphertext");
  if (!ciphertext_str) {
    return false;
  }
  std::string decoded_ciphertext;
  if (!base::Base64Decode(*ciphertext_str, &decoded_ciphertext)) {
    return false;
  }
  *ciphertext = std::vector<uint8_t>(decoded_ciphertext.begin(),
                                     decoded_ciphertext.end());
  return true;
}

bool ParseSwitchEthereumChainParams(const std::string& json,
                                    std::string* chain_id) {
  if (!chain_id) {
    return false;
  }

  auto param_obj = GetObjectFromParamsList(json);
  if (!param_obj) {
    return false;
  }

  const std::string* chain_id_str = param_obj->FindString("chainId");
  if (!chain_id_str) {
    return false;
  }

  if (!IsValidHexString(*chain_id_str)) {
    return false;
  }

  *chain_id = base::ToLowerASCII(*chain_id_str);

  return true;
}

mojom::BlockchainTokenPtr ParseWalletWatchAssetParams(
    const std::string& json,
    const std::string& chain_id,
    std::string* error_message) {
  *error_message = "";

  // Might be a list from legacy send method.
  std::optional<base::Value::Dict> params = GetObjectFromParamsList(json);
  if (!params) {
    params = GetParamsDict(json);
  }

  if (!params) {
    *error_message = "params parameter is required";
    return nullptr;
  }

  const std::string* type = params->FindString("type");
  if (!type) {
    *error_message = "type parameter is required";
    return nullptr;
  }
  // Only ERC20 is supported currently.
  if (*type != "ERC20") {
    *error_message =
        base::StringPrintf("Asset of type '%s' not supported", type->c_str());
    return nullptr;
  }

  const auto* options_dict = params->FindDict("options");
  if (!options_dict) {
    *error_message = "options parameter is required";
    return nullptr;
  }

  const std::string* address = options_dict->FindString("address");
  if (!address) {
    *error_message = "address parameter is required";
    return nullptr;
  }

  const auto eth_addr = EthAddress::FromHex(*address);
  if (eth_addr.IsEmpty()) {
    *error_message =
        base::StringPrintf("Invalid address '%s'", address->c_str());
    return nullptr;
  }

  const std::string* symbol = options_dict->FindString("symbol");
  if (!symbol) {
    *error_message = "symbol parameter is required";
    return nullptr;
  }

  // EIP-747 limits the symbol length to 5, but metamask uses 11, so we use
  // the same limit here for compatibility.
  if (symbol->size() == 0 || symbol->size() > 11) {
    *error_message = base::StringPrintf(
        "Invalid symbol '%s': symbol length should be greater than 0 and less "
        "than 12",
        symbol->c_str());
    return nullptr;
  }

  // Allow decimals in both number and string for compatibility.
  // EIP747 specifies the type of decimals number, but websites like coingecko
  // uses string.
  const base::Value* decimals_value = options_dict->Find("decimals");
  if (!decimals_value ||
      (!decimals_value->is_int() && !decimals_value->is_string())) {
    *error_message = "decimals parameter is required.";
    return nullptr;
  }
  int decimals = decimals_value->is_int() ? decimals_value->GetInt() : 0;
  if (decimals_value->is_string() &&
      !base::StringToInt(decimals_value->GetString(), &decimals)) {
    *error_message = base::StringPrintf(
        "Invalid decimals '%s': decimals should be a number greater than 0 and "
        "less than 36",
        decimals_value->GetString().c_str());
    return nullptr;
  }

  if (decimals < 0 || decimals > 36) {
    *error_message = base::StringPrintf(
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
      base::ToLowerASCII(chain_id), mojom::CoinType::ETH, false);
}

// Parses param request objects from
// https://eips.ethereum.org/EIPS/eip-2255
bool ParseRequestPermissionsParams(
    const std::string& json,
    std::vector<std::string>* restricted_methods) {
  // [{
  //   "eth_accounts": {}
  // }]
  if (!restricted_methods) {
    return false;
  }
  restricted_methods->clear();
  auto param_obj = GetObjectFromParamsList(json);
  if (!param_obj) {
    return false;
  }
  for (auto prop : *param_obj) {
    restricted_methods->push_back(prop.first);
  }
  return true;
}

bool ParseEthSendRawTransactionParams(const std::string& json,
                                      std::string* signed_transaction) {
  if (!signed_transaction) {
    return false;
  }

  auto list = GetParamsList(json);
  if (!list || list->size() != 1) {
    return false;
  }

  const std::string* signed_tx_str = (*list)[0].GetIfString();
  if (!signed_tx_str) {
    return false;
  }

  *signed_transaction = *signed_tx_str;
  return true;
}

bool ParseEthSubscribeParams(const std::string& json,
                             std::string* event_type,
                             base::Value::Dict* filter) {
  if (filter == nullptr) {
    return false;
  }

  auto list = GetParamsList(json);
  if (!list || list->empty() || list->size() > 2) {
    return false;
  }

  if (!(*list)[0].is_string()) {
    return false;
  }

  *event_type = (*list)[0].GetString();

  if (list->size() == 2) {
    if (!(*list)[1].is_dict()) {
      return false;
    }

    *filter = std::move((*list)[1].GetDict());
  }

  return true;
}

bool ParseEthUnsubscribeParams(const std::string& json,
                               std::string* subscription_id) {
  if (!subscription_id) {
    return false;
  }

  auto list = GetParamsList(json);
  if (!list || list->size() != 1) {
    return false;
  }

  const std::string* subscription_id_str = (*list)[0].GetIfString();
  if (!subscription_id_str) {
    return false;
  }

  *subscription_id = *subscription_id_str;
  return true;
}

}  // namespace brave_wallet
