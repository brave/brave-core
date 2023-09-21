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
                           std::string* params);

mojom::TxDataPtr ParseEthTransactionParams(const std::string& json,
                                           std::string* from);
mojom::TxData1559Ptr ParseEthTransaction1559Params(const std::string& json,
                                                   std::string* from);
bool ShouldCreate1559Tx(mojom::TxData1559Ptr tx_data_1559,
                        bool network_supports_eip1559,
                        const std::vector<mojom::AccountInfoPtr>& account_infos,
                        const std::string& address);

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
bool ParseEthDecryptData(const std::string& json,
                         std::string* version,
                         std::vector<uint8_t>* nonce,
                         std::vector<uint8_t>* ephemeral_public_key,
                         std::vector<uint8_t>* ciphertext);

bool ParseEthSignTypedDataParams(const std::string& json,
                                 std::string* address,
                                 std::string* message,
                                 base::Value::Dict* domain,
                                 EthSignTypedDataHelper::Version version,
                                 std::vector<uint8_t>* domain_hash_out,
                                 std::vector<uint8_t>* primary_hash_out,
                                 mojom::EthSignTypedDataMetaPtr* meta_out);

bool ParseSwitchEthereumChainParams(const std::string& json,
                                    std::string* chain_id);

bool ParseWalletWatchAssetParams(const std::string& json,
                                 const std::string& chain_id,
                                 mojom::CoinType coin,
                                 mojom::BlockchainTokenPtr* token,
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
