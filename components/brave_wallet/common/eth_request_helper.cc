/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/common/eth_request_helper.h"

#include <memory>
#include <tuple>
#include <utility>

#include "base/base64.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "brave/components/brave_wallet/common/eth_address.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "brave/components/brave_wallet/common/web3_provider_constants.h"
#include "url/gurl.h"

namespace {

absl::optional<base::Value::List> GetParamsList(const std::string& json) {
  auto json_value =
      base::JSONReader::Read(json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                                       base::JSON_ALLOW_TRAILING_COMMAS);
  if (!json_value || !json_value->is_dict())
    return absl::nullopt;

  auto& value = json_value->GetDict();
  auto* params = value.FindListByDottedPath(brave_wallet::kParams);
  if (!params)
    return absl::nullopt;

  return std::move(*params);
}

absl::optional<base::Value> GetObjectFromParamsList(const std::string& json) {
  auto list = GetParamsList(json);
  if (!list || list->size() != 1 || !(*list)[0].is_dict())
    return absl::nullopt;

  return std::move((*list)[0]);
}

absl::optional<base::Value> GetParamsDict(const std::string& json) {
  auto json_value =
      base::JSONReader::Read(json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                                       base::JSON_ALLOW_TRAILING_COMMAS);
  if (!json_value || !json_value->is_dict()) {
    return absl::nullopt;
  }
  auto* value = json_value->FindDictKey(brave_wallet::kParams);
  if (!value)
    return absl::nullopt;

  return value->Clone();
}

// This is a best effort parsing of the data
brave_wallet::mojom::TxDataPtr ValueToTxData(const base::Value& tx_value,
                                             std::string* from_out) {
  CHECK(from_out);
  auto tx_data = brave_wallet::mojom::TxData::New();
  const base::Value::Dict* params_dict = tx_value.GetIfDict();
  if (!params_dict)
    return nullptr;

  const std::string* from = params_dict->FindString("from");
  if (from)
    *from_out = *from;

  const std::string* to = params_dict->FindString("to");
  if (to)
    tx_data->to = *to;

  const std::string* gas = params_dict->FindString("gas");
  if (gas)
    tx_data->gas_limit = *gas;

  const std::string* gas_price = params_dict->FindString("gasPrice");
  if (gas_price)
    tx_data->gas_price = *gas_price;

  const std::string* value = params_dict->FindString("value");
  if (value)
    tx_data->value = *value;

  const std::string* data = params_dict->FindString("data");
  if (data) {
    // If data is specified it's best to make sure it's valid
    std::vector<uint8_t> bytes;
    if (!data->empty() &&
        !brave_wallet::PrefixedHexStringToBytes(*data, &bytes))
      return nullptr;
    tx_data->data = bytes;
  }

  return tx_data;
}

// null request ID when unspecified is expected
const char kDefaultRequestIdWhenUnspecified[] = "1";
const char kRequestJsonRPC[] = "2.0";

}  // namespace

namespace brave_wallet {

mojom::TxDataPtr ParseEthSendTransactionParams(const std::string& json,
                                               std::string* from) {
  CHECK(from);
  from->clear();

  auto param_obj = GetObjectFromParamsList(json);
  if (!param_obj)
    return nullptr;
  return ValueToTxData(*param_obj, from);
}

mojom::TxData1559Ptr ParseEthSendTransaction1559Params(const std::string& json,
                                                       std::string* from) {
  CHECK(from);
  from->clear();
  auto param_obj = GetObjectFromParamsList(json);
  if (!param_obj)
    return nullptr;

  auto tx_data = mojom::TxData1559::New();
  auto base_data_ret = ValueToTxData(*param_obj, from);
  if (!base_data_ret)
    return nullptr;

  tx_data->base_data = std::move(base_data_ret);
  const base::Value::Dict* params_dict = param_obj->GetIfDict();
  if (!params_dict)
    return nullptr;

  const std::string* max_priority_fee_per_gas =
      params_dict->FindString("maxPriorityFeePerGas");
  if (max_priority_fee_per_gas)
    tx_data->max_priority_fee_per_gas = *max_priority_fee_per_gas;

  const std::string* max_fee_per_gas = params_dict->FindString("maxFeePerGas");
  if (max_fee_per_gas)
    tx_data->max_fee_per_gas = *max_fee_per_gas;

  return tx_data;
}

bool ShouldCreate1559Tx(brave_wallet::mojom::TxData1559Ptr tx_data_1559,
                        bool network_supports_eip1559,
                        const std::vector<mojom::AccountInfoPtr>& account_infos,
                        const std::string& address) {
  bool keyring_supports_eip1559 = true;
  auto account_it = std::find_if(account_infos.begin(), account_infos.end(),
                                 [&](const mojom::AccountInfoPtr& account) {
                                   return base::EqualsCaseInsensitiveASCII(
                                       account->address, address);
                                 });

  // Only ledger and trezor hardware keyrings support EIP-1559 at the moment.
  if (account_it != account_infos.end() && (*account_it)->hardware &&
      ((*account_it)->hardware->vendor != mojom::kLedgerHardwareVendor &&
       (*account_it)->hardware->vendor != mojom::kTrezorHardwareVendor)) {
    keyring_supports_eip1559 = false;
  }

  // Network or keyring without EIP1559 support.
  if (!network_supports_eip1559 || !keyring_supports_eip1559)
    return false;

  // Network with EIP1559 support and EIP1559 gas fields are specified.
  if (tx_data_1559 && !tx_data_1559->max_priority_fee_per_gas.empty() &&
      !tx_data_1559->max_fee_per_gas.empty())
    return true;

  // Network with EIP1559 support and legacy gas fields are specified.
  if (tx_data_1559 && !tx_data_1559->base_data->gas_price.empty())
    return false;

  // Network with EIP1559 support and no gas fields are specified.
  return true;
}

bool GetEthJsonRequestInfo(const std::string& json,
                           base::Value* id,
                           std::string* method,
                           std::string* params) {
  base::JSONReader::ValueWithError value_with_error =
      base::JSONReader::ReadAndReturnValueWithError(
          json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                    base::JSONParserOptions::JSON_PARSE_RFC);
  absl::optional<base::Value>& records_v = value_with_error.value;
  if (!records_v) {
    return false;
  }

  const base::Value::Dict* response_dict = records_v->GetIfDict();
  if (!response_dict) {
    return false;
  }

  if (id) {
    const base::Value* found_id = response_dict->FindByDottedPath(kId);
    if (found_id)
      *id = found_id->Clone();
    else
      *id = base::Value();
  }

  if (method) {
    const std::string* found_method =
        response_dict->FindStringByDottedPath(kMethod);
    if (!found_method)
      return false;
    *method = *found_method;
  }

  if (params) {
    const auto* found_params = response_dict->FindListByDottedPath(kParams);
    if (!found_params)
      return false;
    base::JSONWriter::Write(*found_params, params);
  }

  return true;
}

bool NormalizeEthRequest(const std::string& input_json,
                         std::string* output_json) {
  CHECK(output_json);
  base::JSONReader::ValueWithError value_with_error =
      base::JSONReader::ReadAndReturnValueWithError(
          input_json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                          base::JSONParserOptions::JSON_PARSE_RFC);
  absl::optional<base::Value>& records_v = value_with_error.value;
  if (!records_v)
    return false;

  base::Value::Dict* out_dict = records_v->GetIfDict();
  if (!out_dict)
    return false;

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
  if (!address || !message)
    return false;

  auto list = GetParamsList(json);
  if (!list || list->size() != 2)
    return false;

  const std::string* address_str = (*list)[0].GetIfString();
  const std::string* message_str = (*list)[1].GetIfString();
  if (!address_str || !message_str)
    return false;

  *address = *address_str;
  *message = *message_str;

  return true;
}

bool ParsePersonalSignParams(const std::string& json,
                             std::string* address,
                             std::string* message) {
  if (!address || !message)
    return false;

  // personal_sign allows extra params
  auto list = GetParamsList(json);
  if (!list || list->size() < 2)
    return false;

  // personal_sign has the reversed order
  const std::string* message_str = (*list)[0].GetIfString();
  const std::string* address_str = (*list)[1].GetIfString();
  if (!address_str || !message_str)
    return false;

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
  if (!address)
    return false;

  // eth_getEncryptionPublicKey allows extra params
  auto list = GetParamsList(json);
  if (!list || list->size() < 1)
    return false;

  const std::string* address_str = (*list)[0].GetIfString();
  if (!address_str)
    return false;

  *address = *address_str;
  return true;
}

bool ParseEthDecryptParams(const std::string& json,
                           std::string* untrusted_encrypted_data_json,
                           std::string* address) {
  if (!address)
    return false;

  // eth_decrypt allows extra params
  auto list = GetParamsList(json);
  if (!list || list->size() < 2)
    return false;

  const std::string* untrusted_hex_json_str = (*list)[0].GetIfString();
  if (!untrusted_hex_json_str)
    return false;

  const std::string* address_str = (*list)[1].GetIfString();
  if (!address_str)
    return false;

  std::string untrusted_json;

  // untrusted_hex_json should hex decode to this schema example:
  // {
  //   "version": "x25519-xsalsa20-poly1305",
  //   "nonce": "base64-string",
  //   "ephemPublicKey": "base64-string",
  //   "ciphertext":"base64-string"
  // }

  if (!IsValidHexString(*untrusted_hex_json_str))
    return false;

  // IsValidHexString guarantees at least 2 bytes and starts with 0x
  if (!base::HexStringToString(untrusted_hex_json_str->data() + 2,
                               &untrusted_json))
    return false;

  *untrusted_encrypted_data_json = untrusted_json;
  *address = *address_str;
  return true;
}

bool ParsePersonalEcRecoverParams(const std::string& json,
                                  std::string* message,
                                  std::string* signature) {
  if (!message || !signature)
    return false;

  // personal_ecRecover allows extra params
  auto list = GetParamsList(json);
  if (!list || list->size() < 2)
    return false;

  const std::string* message_str = (*list)[0].GetIfString();
  const std::string* signature_str = (*list)[1].GetIfString();
  if (!message_str || !signature_str)
    return false;

  if (IsValidHexString(*message_str)) {
    *message = *message_str;
  } else {
    *message = ToHex(*message_str);
  }

  if (!IsValidHexString(*signature_str))
    return false;

  *signature = *signature_str;
  return true;
}

bool ParseEthSignTypedDataParams(const std::string& json,
                                 std::string* address,
                                 std::string* message_out,
                                 base::Value* domain_out,
                                 EthSignTypedDataHelper::Version version,
                                 std::vector<uint8_t>* domain_hash_out,
                                 std::vector<uint8_t>* primary_hash_out) {
  if (!address || !message_out || !domain_out || !domain_hash_out ||
      !primary_hash_out)
    return false;

  auto list = GetParamsList(json);
  if (!list || list->size() != 2)
    return false;

  const std::string* address_str = (*list)[0].GetIfString();
  const std::string* typed_data_str = (*list)[1].GetIfString();
  if (!address_str || !typed_data_str)
    return false;

  auto typed_data = base::JSONReader::Read(
      *typed_data_str,
      base::JSON_PARSE_CHROMIUM_EXTENSIONS | base::JSON_ALLOW_TRAILING_COMMAS);
  if (!typed_data)
    return false;
  auto* dict = typed_data->GetIfDict();
  if (!dict)
    return false;

  const std::string* primary_type = dict->FindString("primaryType");
  if (!primary_type)
    return false;

  base::Value* domain = dict->Find("domain");
  if (!domain)
    return false;

  const base::Value* message = dict->Find("message");
  if (!message)
    return false;

  *address = *address_str;
  if (!base::JSONWriter::Write(*message, message_out))
    return false;

  const base::Value* types = dict->Find("types");
  if (!types)
    return false;
  std::unique_ptr<EthSignTypedDataHelper> helper =
      EthSignTypedDataHelper::Create(*types, version);
  if (!helper)
    return false;
  auto domain_hash = helper->GetTypedDataDomainHash(*domain);
  if (!domain_hash)
    return false;
  auto primary_hash = helper->GetTypedDataPrimaryHash(*primary_type, *message);
  if (!primary_hash)
    return false;
  *domain_hash_out = *domain_hash;
  *primary_hash_out = *primary_hash;

  *domain_out = std::move(*domain);

  return true;
}

bool ParseEthDecryptData(const std::string& json,
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
  auto obj = base::JSONReader::Read(json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                                              base::JSON_ALLOW_TRAILING_COMMAS);
  if (!obj)
    return false;
  auto* dict = obj->GetIfDict();
  if (!dict)
    return false;

  const std::string* version_str = dict->FindString("version");
  if (!version_str)
    return false;
  *version = *version_str;

  const std::string* nonce_str = dict->FindString("nonce");
  if (!nonce_str)
    return false;
  std::string decoded_nonce;
  if (!base::Base64Decode(*nonce_str, &decoded_nonce))
    return false;
  *nonce = std::vector<uint8_t>(decoded_nonce.begin(), decoded_nonce.end());

  const std::string* ephemeral_public_key_str =
      dict->FindString("ephemPublicKey");
  if (!ephemeral_public_key_str)
    return false;
  std::string ephemeral_public_key_decoded;
  if (!base::Base64Decode(*ephemeral_public_key_str,
                          &ephemeral_public_key_decoded))
    return false;
  *ephemeral_public_key = std::vector<uint8_t>(
      ephemeral_public_key_decoded.begin(), ephemeral_public_key_decoded.end());

  const std::string* ciphertext_str = dict->FindString("ciphertext");
  if (!ciphertext_str)
    return false;
  std::string decoded_ciphertext;
  if (!base::Base64Decode(*ciphertext_str, &decoded_ciphertext))
    return false;
  *ciphertext = std::vector<uint8_t>(decoded_ciphertext.begin(),
                                     decoded_ciphertext.end());
  return true;
}

bool ParseSwitchEthereumChainParams(const std::string& json,
                                    std::string* chain_id) {
  if (!chain_id)
    return false;

  auto param_obj = GetObjectFromParamsList(json);
  if (!param_obj || !param_obj->is_dict())
    return false;

  const std::string* chain_id_str = param_obj->GetDict().FindString("chainId");
  if (!chain_id_str)
    return false;

  if (!IsValidHexString(*chain_id_str))
    return false;

  *chain_id = *chain_id_str;

  return true;
}

bool ParseWalletWatchAssetParams(const std::string& json,
                                 const std::string& chain_id,
                                 mojom::CoinType coin,
                                 mojom::BlockchainTokenPtr* token,
                                 std::string* error_message) {
  if (!token || !error_message) {
    return false;
  }
  *error_message = "";

  // Might be a list from legacy send method.
  absl::optional<base::Value> params = GetObjectFromParamsList(json);
  if (!params)
    params = GetParamsDict(json);

  if (!params || !params.value().is_dict()) {
    *error_message = "params parameter is required";
    return false;
  }

  const auto& dict = params->GetDict();
  const std::string* type = dict.FindString("type");
  if (!type) {
    *error_message = "type parameter is required";
    return false;
  }
  // Only ERC20 is supported currently.
  if (*type != "ERC20") {
    *error_message =
        base::StringPrintf("Asset of type '%s' not supported", type->c_str());
    return false;
  }

  const auto* options_dict = dict.FindDict("options");
  if (!options_dict) {
    *error_message = "options parameter is required";
    return false;
  }

  const std::string* address = options_dict->FindString("address");
  if (!address) {
    *error_message = "address parameter is required";
    return false;
  }

  const auto eth_addr = EthAddress::FromHex(*address);
  if (eth_addr.IsEmpty()) {
    *error_message =
        base::StringPrintf("Invalid address '%s'", address->c_str());
    return false;
  }

  const std::string* symbol = options_dict->FindString("symbol");
  if (!symbol) {
    *error_message = "symbol parameter is required";
    return false;
  }

  // EIP-747 limits the symbol length to 5, but metamask uses 11, so we use
  // the same limit here for compatibility.
  if (symbol->size() == 0 || symbol->size() > 11) {
    *error_message = base::StringPrintf(
        "Invalid symbol '%s': symbol length should be greater than 0 and less "
        "than 12",
        symbol->c_str());
    return false;
  }

  // Allow decimals in both number and string for compability.
  // EIP747 specifies the type of decimals number, but websites like coingecko
  // uses string.
  const base::Value* decimals_value = options_dict->Find("decimals");
  if (!decimals_value ||
      (!decimals_value->is_int() && !decimals_value->is_string())) {
    *error_message = "decimals parameter is required.";
    return false;
  }
  int decimals = decimals_value->is_int() ? decimals_value->GetInt() : 0;
  if (decimals_value->is_string() &&
      !base::StringToInt(decimals_value->GetString(), &decimals)) {
    *error_message = base::StringPrintf(
        "Invalid decimals '%s': decimals should be a number greater than 0 and "
        "less than 36",
        decimals_value->GetString().c_str());
    return false;
  }

  if (decimals < 0 || decimals > 36) {
    *error_message = base::StringPrintf(
        "Invalid decimals '%d': decimals should be greater than 0 and less "
        "than 36",
        decimals);
    return false;
  }

  std::string logo;
  const std::string* image = options_dict->FindString("image");
  if (image) {
    GURL url = GURL(*image);
    if (url.is_valid() && (url.SchemeIsHTTPOrHTTPS() ||
                           base::StartsWith(*image, "data:image/"))) {
      logo = url.spec();
    }
  }

  *token = mojom::BlockchainToken::New(
      eth_addr.ToChecksumAddress(), *symbol /* name */, logo, true, false,
      *symbol, decimals, true, "", "", chain_id, coin);
  return true;
}

// Parses param request objects from
// https://eips.ethereum.org/EIPS/eip-2255
bool ParseRequestPermissionsParams(
    const std::string& json,
    std::vector<std::string>* restricted_methods) {
  // [{
  //   "eth_accounts": {}
  // }]
  if (!restricted_methods)
    return false;
  restricted_methods->clear();
  auto param_obj = GetObjectFromParamsList(json);
  if (!param_obj)
    return false;
  for (auto prop : param_obj->DictItems()) {
    restricted_methods->push_back(prop.first);
  }
  return true;
}

}  // namespace brave_wallet
