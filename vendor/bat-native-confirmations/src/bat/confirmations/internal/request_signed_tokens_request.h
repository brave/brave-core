/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_CONFIRMATIONS_INTERNAL_REQUEST_SIGNED_TOKENS_REQUEST_H_
#define BAT_CONFIRMATIONS_INTERNAL_REQUEST_SIGNED_TOKENS_REQUEST_H_

#include <string>
#include <vector>

#include "bat/confirmations/confirmations_client.h"
#include "bat/confirmations/wallet_info.h"

#include "wrapper.hpp"  // NOLINT

namespace confirmations {

using challenge_bypass_ristretto::BlindedToken;

class RequestSignedTokensRequest {
 public:
  RequestSignedTokensRequest();
  ~RequestSignedTokensRequest();

  std::string BuildUrl(
      const WalletInfo& wallet_info) const;

  URLRequestMethod GetMethod() const;

  std::string BuildBody(
      const std::vector<BlindedToken>& tokens) const;

  std::vector<std::string> BuildHeaders(
      const std::string& body,
      const WalletInfo& wallet_info) const;
  std::string BuildDigestHeaderValue(
      const std::string& body) const;
  std::string BuildSignatureHeaderValue(
      const std::string& body,
      const WalletInfo& wallet_info) const;
  std::string GetAcceptHeaderValue() const;

  std::string GetContentType() const;
};

}  // namespace confirmations

#endif  // BAT_CONFIRMATIONS_INTERNAL_REQUEST_SIGNED_TOKENS_REQUEST_H_
