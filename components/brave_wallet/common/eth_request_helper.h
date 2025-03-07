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
                           base::Value::List* params_list);

mojom::TxDataPtr ParseEthTransactionParams(std::string_view json,
                                           std::string* from);
mojom::TxData1559Ptr ParseEthTransaction1559Params(std::string_view json,
                                                   std::string* from);
bool ShouldCreate1559Tx(const mojom::TxData1559& tx_data_1559);

bool NormalizeEthRequest(std::string_view input_json, std::string* output_json);

bool ParseEthSignParams(std::string_view json,
                        std::string* address,
                        std::string* message);
bool ParsePersonalSignParams(std::string_view json,
                             std::string* address,
                             std::string* message);
bool ParsePersonalEcRecoverParams(std::string_view json,
                                  std::string* message,
                                  std::string* signature);
bool ParseEthGetEncryptionPublicKeyParams(std::string_view json,
                                          std::string* address);
bool ParseEthDecryptParams(std::string_view json,
                           std::string* untrusted_encrypted_data_json,
                           std::string* address);
bool ParseEthDecryptData(const base::Value& obj,
                         std::string* version,
                         std::vector<uint8_t>* nonce,
                         std::vector<uint8_t>* ephemeral_public_key,
                         std::vector<uint8_t>* ciphertext);

mojom::EthSignTypedDataPtr ParseEthSignTypedDataParams(
    const base::Value::List& params_list,
    EthSignTypedDataHelper::Version version);

bool ParseSwitchEthereumChainParams(std::string_view json,
                                    std::string* chain_id);

mojom::BlockchainTokenPtr ParseWalletWatchAssetParams(
    std::string_view json,
    std::string_view chain_id,
    std::string* error_message);
std::optional<base::flat_set<std::string>> ParseRequestPermissionsParams(
    std::string_view json);

bool ParseEthSendRawTransactionParams(std::string_view json,
                                      std::string* signed_transaction);
bool ParseEthSubscribeParams(std::string_view json,
                             std::string* event_type,
                             base::Value::Dict* filter);
bool ParseEthUnsubscribeParams(std::string_view json,
                               std::string* subscription_id);

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_ETH_REQUEST_HELPER_H_
