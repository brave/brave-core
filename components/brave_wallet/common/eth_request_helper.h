/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_ETH_REQUEST_HELPER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_ETH_REQUEST_HELPER_H_

#include <string>
#include <vector>

#include "base/values.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/eth_sign_typed_data_helper.h"

namespace brave_wallet {

bool GetEthJsonRequestInfo(const std::string& json,
                           base::Value* id,
                           std::string* method,
                           base::Value::List* params_list);

mojom::TxDataPtr ParseEthTransactionParams(const std::string& json,
                                           std::string* from);
mojom::TxData1559Ptr ParseEthTransaction1559Params(const std::string& json,
                                                   std::string* from);
bool ShouldCreate1559Tx(const mojom::TxData1559& tx_data_1559);

bool NormalizeEthRequest(const std::string& input_json,
                         std::string* output_json);

bool ParseEthSignParams(const std::string& json,
                        std::string* address,
                        std::string* message);
bool ParsePersonalSignParams(const std::string& json,
                             std::string* address,
                             std::string* message);
bool ParsePersonalEcRecoverParams(const std::string& json,
                                  std::string* message,
                                  std::string* signature);
bool ParseEthGetEncryptionPublicKeyParams(const std::string& json,
                                          std::string* address);
bool ParseEthDecryptParams(const std::string& json,
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

bool ParseSwitchEthereumChainParams(const std::string& json,
                                    std::string* chain_id);

mojom::BlockchainTokenPtr ParseWalletWatchAssetParams(
    const std::string& json,
    const std::string& chain_id,
    std::string* error_message);
bool ParseRequestPermissionsParams(
    const std::string& json,
    std::vector<std::string>* restricted_methods);

bool ParseEthSendRawTransactionParams(const std::string& json,
                                      std::string* signed_transaction);
bool ParseEthSubscribeParams(const std::string& json,
                             std::string* event_type,
                             base::Value::Dict* filter);
bool ParseEthUnsubscribeParams(const std::string& json,
                               std::string* subscription_id);

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_ETH_REQUEST_HELPER_H_
