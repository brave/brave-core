/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SERVICES_BRAVE_WALLET_ZCASH_ZCASH_DECODER_H_
#define BRAVE_COMPONENTS_SERVICES_BRAVE_WALLET_ZCASH_ZCASH_DECODER_H_

#include "brave/components/services/brave_wallet/public/mojom/zcash_decoder.mojom.h"

#include <string>
#include <vector>

#include "mojo/public/cpp/bindings/receiver_set.h"

namespace brave_wallet {

// Parses Zcash protobuf objects and translates them to mojo.
class ZCashDecoder : public zcash::mojom::ZCashDecoder {
 public:
  ZCashDecoder();
  ~ZCashDecoder() override;

  ZCashDecoder(const ZCashDecoder&) = delete;
  ZCashDecoder& operator=(const ZCashDecoder&) = delete;

  void ParseRawTransaction(const std::string& data,
                           ParseRawTransactionCallback callback) override;
  void ParseBlockID(const std::string& data,
                    ParseBlockIDCallback callback) override;
  void ParseGetAddressUtxos(const std::string& data,
                            ParseGetAddressUtxosCallback callback) override;
  void ParseSendResponse(const std::string& data,
                         ParseSendResponseCallback) override;
  void ParseTreeState(const std::string& data,
                      ParseTreeStateCallback callback) override;
  void ParseCompactBlocks(const std::vector<std::string>& data,
                          ParseCompactBlocksCallback callback) override;
  void ParseSubtreeRoots(const std::vector<std::string>& data,
                         ParseSubtreeRootsCallback callback) override;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_SERVICES_BRAVE_WALLET_ZCASH_ZCASH_DECODER_H_
