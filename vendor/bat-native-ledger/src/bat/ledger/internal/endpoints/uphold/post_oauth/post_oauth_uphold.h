/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINTS_UPHOLD_POST_OAUTH_POST_OAUTH_UPHOLD_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINTS_UPHOLD_POST_OAUTH_POST_OAUTH_UPHOLD_H_

#include <string>
#include <vector>

#include "bat/ledger/internal/endpoints/request_builder.h"
#include "bat/ledger/internal/endpoints/response_handler.h"
#include "bat/ledger/internal/endpoints/result_for.h"
#include "brave/vendor/bat-native-ledger/include/bat/ledger/public/interfaces/ledger_endpoints.mojom.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

// POST /oauth2/token
//
// Request body:
// code=bb50f9d4782fb86a4302ef18179033abb17c257f&grant_type=authorization_code
//
// clang-format off
// Response body:
// {
//   "access_token": "9fd84e43c803622cc65a63c6d380a47612d7d718",
//   "expires_in": 7775999,
//   "scope": "cards:read cards:write user:read transactions:transfer:application transactions:transfer:others",  // NOLINT
//   "token_type": "bearer"
// }
// clang-format on

namespace ledger {
class LedgerImpl;

namespace endpoints {

class PostOAuthUphold;

template <>
struct ResultFor<PostOAuthUphold> {
  using Value = std::string;  // access token
  using Error = mojom::PostOAuthUpholdError;
};

class PostOAuthUphold final : public RequestBuilder,
                              public ResponseHandler<PostOAuthUphold> {
 public:
  static Result ProcessResponse(const mojom::UrlResponse&);

  PostOAuthUphold(LedgerImpl*, std::string&& code);
  ~PostOAuthUphold() override;

 private:
  absl::optional<std::string> Url() const override;
  absl::optional<std::vector<std::string>> Headers(
      const std::string& content) const override;
  absl::optional<std::string> Content() const override;
  std::string ContentType() const override;
  bool SkipLog() const override;

  std::string code_;
};

}  // namespace endpoints
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINTS_UPHOLD_POST_OAUTH_POST_OAUTH_UPHOLD_H_
