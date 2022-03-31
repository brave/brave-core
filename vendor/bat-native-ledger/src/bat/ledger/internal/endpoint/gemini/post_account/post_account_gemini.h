/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINT_GEMINI_POST_ACCOUNT_POST_ACCOUNT_GEMINI_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINT_GEMINI_POST_ACCOUNT_POST_ACCOUNT_GEMINI_H_

#include <string>

#include "bat/ledger/ledger.h"

// POST https://api.sandbox.gemini.com/v1/account
//
// Success code:
// HTTP_OK (200)
//
// Error codes:
// HTTP_UNAUTHORIZED (401)
//
// Response body:
// {
//   "account": {
//     "accountName": "Primary",
//     "shortName": "primary",
//     "type": "exchange",
//     "created": "1619040615242",
//     "verificationToken": "token"
//   },
//   "users": [{
//     "name": "Test",
//     "lastSignIn": "2021-04-30T18:46:03.017Z",
//     "status": "Active",
//     "countryCode": "US",
//     "isVerified": true
//   }],
//   "memo_reference_code": "GEMAPLLV"
// }

namespace ledger {
class LedgerImpl;

namespace endpoint {
namespace gemini {

using PostAccountCallback = std::function<void(const type::Result result,
                                               const std::string& linking_info,
                                               const std::string& user_name)>;

class PostAccount {
 public:
  explicit PostAccount(LedgerImpl* ledger);
  ~PostAccount();

  void Request(const std::string& token, PostAccountCallback callback);

 private:
  std::string GetUrl();

  type::Result ParseBody(const std::string& body,
                         std::string* linking_info,
                         std::string* user_name);

  void OnRequest(const type::UrlResponse& response,
                 PostAccountCallback callback);

  LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace gemini
}  // namespace endpoint
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINT_GEMINI_POST_ACCOUNT_POST_ACCOUNT_GEMINI_H_
