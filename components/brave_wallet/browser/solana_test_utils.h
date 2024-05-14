/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_TEST_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_TEST_UTILS_H_

#include <cstdint>

namespace brave_wallet {

class SolanaMessage;
class SolanaInstruction;

inline constexpr char kFromAccount[] =
    "3Lu176FQzbQJCc8iL9PnmALbpMPhZeknoturApnXRDJw";
inline constexpr char kToAccount[] =
    "3QpJ3j1vq1PfqJdvCcHKWuePykqoUYSvxyRb3Cnh79BD";
inline constexpr char kTestAccount[] =
    "BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8";
inline constexpr char kRecentBlockhash[] =
    "9sHcv6xwn9YkB8nxTUGKDwPwNnmqVp5oAXxU8Fdkm4J6";
inline constexpr uint64_t kLastValidBlockHeight = 3090;

SolanaMessage GetTestLegacyMessage();
SolanaMessage GetTestV0Message();

SolanaInstruction GetAdvanceNonceAccountInstruction();

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_TEST_UTILS_H_
