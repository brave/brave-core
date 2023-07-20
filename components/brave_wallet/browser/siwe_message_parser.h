/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SIWE_MESSAGE_PARSER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SIWE_MESSAGE_PARSER_H_

#include <string>

#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"

namespace brave_wallet {

// Parser for parsing and validating
// https://eips.ethereum.org/EIPS/eip-4361#message-format
class SIWEMessageParser {
 public:
  SIWEMessageParser() = default;
  ~SIWEMessageParser() = default;
  SIWEMessageParser(const SIWEMessageParser&) = delete;
  SIWEMessageParser& operator=(const SIWEMessageParser&) = delete;

  enum class State {
    kStart = 0,
    kAddress,
    kStatement,
    kURI,
    kVersion,
    kChainId,
    kNonce,
    kIssuedAt,
    kOptionalFields,
    kEnd
  };

  State state() const { return state_; }

  // If there is any error, nullptr will be returned and state() will indicate
  // which state it fails at. Only successful parsing with kEnd state contains
  // valid result.
  mojom::SIWEMessagePtr Parse(const std::string& message);

 private:
  friend class SIWEMessageParserTest;

  static std::string GetStartingTokenForTesting();
  static std::string GetURITokenForTesting();
  static std::string GetVersionTokenForTesting();
  static std::string GetChainIdTokenForTesting();
  static std::string GetNonceTokenForTesting();
  static std::string GetIssuedAtTokenForTesting();
  static std::string GetExpirationTimeTokenForTesting();
  static std::string GetNotBeforeTokenForTesting();
  static std::string GetRequestIdTokenForTesting();
  static std::string GetResourcesTokenForTesting();
  static std::string GetResourcesSeperatorForTesting();

  State state_ = State::kStart;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SIWE_MESSAGE_PARSER_H_
