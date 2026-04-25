/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_ETH_REQUEST_HELPER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_ETH_REQUEST_HELPER_H_

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "base/containers/flat_set.h"
#include "base/values.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/eth_sign_typed_data_helper.h"

namespace brave_wallet {

bool GetEthJsonRequestInfo(std::string_view json,
                           base::Value* id,
                           std::string* method,
                           base::ListValue* params_list);

struct JsonRpcRequest {
  base::Value id;
  std::string method;
  base::ListValue params;
};

std::optional<JsonRpcRequest> ParseJsonRpcRequest(base::Value input_value);

mojom::TxData1559Ptr ParseEthTransaction1559Params(
    const base::ListValue& params,
    std::string& from_out);
bool ShouldCreate1559Tx(const mojom::TxData1559& tx_data_1559);

bool NormalizeEthRequest(std::string_view input_json, std::string* output_json);

struct EthSignParams {
  std::string address;
  std::string message;
};

std::optional<EthSignParams> ParseEthSignParams(const base::ListValue& params);

std::optional<EthSignParams> ParsePersonalSignParams(
    const base::ListValue& params);

struct PersonalEcRecoverParams {
  std::string message;
  std::string signature;
};

std::optional<PersonalEcRecoverParams> ParsePersonalEcRecoverParams(
    const base::ListValue& params);

std::optional<std::string> ParseEthGetEncryptionPublicKeyParams(
    const base::ListValue& params);

struct EthDecryptParams {
  std::string untrusted_encrypted_data_json;
  std::string address;
};

std::optional<EthDecryptParams> ParseEthDecryptParams(
    const base::ListValue& params);

struct EthDecryptData {
  EthDecryptData();
  ~EthDecryptData();
  EthDecryptData(EthDecryptData&&);
  std::string version;
  std::vector<uint8_t> nonce;
  std::vector<uint8_t> ephemeral_public_key;
  std::vector<uint8_t> ciphertext;
};
std::optional<EthDecryptData> ParseEthDecryptData(const base::DictValue& dict);

mojom::EthSignTypedDataPtr ParseEthSignTypedDataParams(
    const base::ListValue& params,
    EthSignTypedDataHelper::Version version);

std::optional<std::string> ParseSwitchEthereumChainParams(
    const base::ListValue& params);

mojom::BlockchainTokenPtr ParseWalletWatchAssetParams(
    const base::ListValue& params,
    std::string& error_message);
std::optional<base::flat_set<std::string>> ParseRequestPermissionsParams(
    const base::ListValue& params);

std::optional<std::string> ParseEthSendRawTransactionParams(
    const base::ListValue& params);

struct EthSubscribeParams {
  std::string event_type;
  base::DictValue filter;
};
std::optional<EthSubscribeParams> ParseEthSubscribeParams(
    const base::ListValue& params);
std::optional<std::string> ParseEthUnsubscribeParams(
    const base::ListValue& params);

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_ETH_REQUEST_HELPER_H_
