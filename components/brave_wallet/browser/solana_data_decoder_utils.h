/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_DATA_DECODER_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_DATA_DECODER_UTILS_H_

#include <string>
#include <vector>

#include "brave/components/brave_wallet/browser/solana_instruction_decoded_data.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_wallet {

absl::optional<uint8_t> DecodeUint8(const std::vector<uint8_t>& input,
                                    size_t& offset);
absl::optional<std::string> DecodeUint8String(const std::vector<uint8_t>& input,
                                              size_t& offset);
absl::optional<std::string> DecodeAuthorityTypeString(
    const std::vector<uint8_t>& input,
    size_t& offset);

absl::optional<uint32_t> DecodeUint32(const std::vector<uint8_t>& input,
                                      size_t& offset);

absl::optional<std::string> DecodeUint32String(
    const std::vector<uint8_t>& input,
    size_t& offset);

absl::optional<GURL> DecodeMetadataUri(const std::vector<uint8_t> data);

absl::optional<uint64_t> DecodeUint64(const std::vector<uint8_t>& input,
                                      size_t& offset);

absl::optional<std::string> DecodeUint64String(
    const std::vector<uint8_t>& input,
    size_t& offset);

absl::optional<std::string> DecodePublicKey(const std::vector<uint8_t>& input,
                                            size_t& offset);

absl::optional<std::string> DecodeOptionalPublicKey(
    const std::vector<uint8_t>& input,
    size_t& offset);

absl::optional<std::string> DecodeString(const std::vector<uint8_t>& input,
                                         size_t& offset);

absl::optional<GURL> DecodeMetadataUri(const std::vector<uint8_t> data);

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_DATA_DECODER_UTILS_H_
