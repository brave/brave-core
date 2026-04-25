/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SIWE_MESSAGE_PARSER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SIWE_MESSAGE_PARSER_H_

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"

class GURL;

namespace url {
class Origin;
}  // namespace url

namespace brave_wallet {

// Parser for parsing and validating
// https://eips.ethereum.org/EIPS/eip-4361#message-format
class SIWEMessageParser {
 public:
  SIWEMessageParser();
  ~SIWEMessageParser();
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

  bool ParseSchemeAndDomain(std::string_view& msg_view, url::Origin& origin);
  bool ParseAddress(std::string_view& msg_view, std::string& address);
  bool ParseStatement(std::string_view& msg_view,
                      std::optional<std::string>& statement);
  bool ParseURI(std::string_view& msg_view, GURL& uri);
  bool ParseVersion(std::string_view& msg_view, uint32_t& version);
  bool ParseChainId(std::string_view& msg_view, uint64_t& chain_id);
  bool ParseNonce(std::string_view& msg_view, std::string& nonce);
  bool ParseIssuedAt(std::string_view& msg_view, std::string& issued_at);
  bool ParseOptionalStringField(std::string_view& msg_view,
                                const std::string& name,
                                std::optional<std::string>& value);
  bool ParseOptionalResources(std::string_view& msg_view,
                              std::optional<std::vector<GURL>>& resources);

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
