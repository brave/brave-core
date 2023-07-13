/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINT_GEMINI_POST_ACCOUNT_POST_ACCOUNT_GEMINI_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINT_GEMINI_POST_ACCOUNT_POST_ACCOUNT_GEMINI_H_

#include <string>

#include "base/memory/raw_ref.h"
#include "brave/components/brave_rewards/core/rewards_callbacks.h"

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

namespace brave_rewards::internal {
class RewardsEngineImpl;

namespace endpoint::gemini {

using PostAccountCallback = base::OnceCallback<void(mojom::Result,
                                                    std::string&& linking_info,
                                                    std::string&& user_name,
                                                    std::string&& country_id)>;

class PostAccount {
 public:
  explicit PostAccount(RewardsEngineImpl& engine);
  ~PostAccount();

  void Request(const std::string& token, PostAccountCallback);

 private:
  std::string GetUrl();

  mojom::Result ParseBody(const std::string& body,
                          std::string* linking_info,
                          std::string* user_name,
                          std::string* country_id);

  void OnRequest(PostAccountCallback, mojom::UrlResponsePtr);

  const raw_ref<RewardsEngineImpl> engine_;
};

}  // namespace endpoint::gemini
}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINT_GEMINI_POST_ACCOUNT_POST_ACCOUNT_GEMINI_H_
