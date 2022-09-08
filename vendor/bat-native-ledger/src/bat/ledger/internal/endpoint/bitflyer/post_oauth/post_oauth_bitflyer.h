/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINT_BITFLYER_POST_OAUTH_POST_OAUTH_BITFLYER_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINT_BITFLYER_POST_OAUTH_POST_OAUTH_BITFLYER_H_

#include <string>

#include "bat/ledger/ledger.h"

// POST https://bitflyer.com/api/link/v1/token
//
// Request body:
// {
//   "client_id": "abcdedg",
//   "client_secret": "xxxxxxxxxxxxxxxxxx",
//   "code": "xxxxxxxxxxxxxxxxxxxxxxxxxx",
//   "grant_type": "code",
//   "code_verifier": "xxxxxxx",
//   "expires_in": 3600,
//   "external_account_id": "xxxxxxxxxx",
//   "request_id": "xxxxxxxx",
//   "request_deposit_id": true
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
//   "access_token": "xxxxbbbbccccddddeeeeqqqq",
//   "refresh_token": "yyyyyyyyyyyyyyyyyyyyyyyyyy",
//   "expires_in": 302010,
//   "account_hash": "xxxxxxxxxxxxxxxxxx",
//   "token_type": "Bearer",
//   "deposit_id": "xxxxxxxxx",
//   "linking_info": "xxxxx"
// }

namespace ledger {
class LedgerImpl;

namespace endpoint::bitflyer {

using PostOauthCallback = base::OnceCallback<void(mojom::Result,
                                                  std::string&& token,
                                                  std::string&& address,
                                                  std::string&& linking_info)>;

class PostOauth {
 public:
  explicit PostOauth(LedgerImpl*);
  ~PostOauth();

  void Request(const std::string& external_account_id,
               const std::string& code,
               const std::string& code_verifier,
               PostOauthCallback);

 private:
  std::string GetUrl();

  std::string GeneratePayload(const std::string& external_account_id,
                              const std::string& code,
                              const std::string& code_verifier);

  mojom::Result CheckStatusCode(int status_code);

  mojom::Result ParseBody(const std::string& body,
                          std::string* token,
                          std::string* address,
                          std::string* linking_info);

  void OnRequest(PostOauthCallback, const mojom::UrlResponse&);

  LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace endpoint::bitflyer
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINT_BITFLYER_POST_OAUTH_POST_OAUTH_BITFLYER_H_
