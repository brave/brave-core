/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINT_GEMINI_POST_OAUTH_POST_OAUTH_GEMINI_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINT_GEMINI_POST_OAUTH_POST_OAUTH_GEMINI_H_

#include <string>

#include "bat/ledger/ledger.h"

// POST https://exchange.sandbox.gemini.com/auth/token
//
// Request body:
// {
//   "client_id": "xxxxx",
//   "client_secret": "yyyyy",
//   "code": "aaaaa",
//   "grant_type": "authorization_code",
//   "redirect_uri": "rewards://gemini/authorization"
// }
//
// Success code:
// HTTP_OK (200)
//
// Error codes:
// HTTP_UNAUTHORIZED (401)
//
// Response body:
// {
//   "access_token": "aaaaa",
//   "expires_in": 83370,
//   "scope":
//   "account:read,addresses:create,balances:read,orders:create,orders:read,payments:create,payments:read,payments:send",
//   "refresh_token":"bbbbb",
//   "token_type": "Bearer"
// }

namespace ledger {
class LedgerImpl;

namespace endpoint {
namespace gemini {

using PostOauthCallback =
    std::function<void(const type::Result result, const std::string& token)>;

class PostOauth {
 public:
  explicit PostOauth(LedgerImpl* ledger);
  ~PostOauth();

  void Request(const std::string& external_account_id,
               const std::string& code,
               PostOauthCallback callback);

 private:
  std::string GetUrl();

  std::string GeneratePayload(const std::string& external_account_id,
                              const std::string& code);

  type::Result ParseBody(const std::string& body, std::string* token);

  void OnRequest(const type::UrlResponse& response, PostOauthCallback callback);

  LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace gemini
}  // namespace endpoint
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINT_GEMINI_POST_OAUTH_POST_OAUTH_GEMINI_H_
