/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINTS_BRAVE_POST_CONNECT_UPHOLD_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINTS_BRAVE_POST_CONNECT_UPHOLD_H_

#include <optional>
#include <string>
#include <vector>

#include "brave/components/brave_rewards/core/endpoints/common/post_connect.h"

// POST /v3/wallet/uphold/{rewards_payment_id}/claim
//
// clang-format off
// Raw request body:
// {
//   "body": {
//     "denomination": {
//       "amount": "0",
//       "currency": "BAT"
//     },
//     "destination": "4a1efaf8-4c9c-4ab2-8978-8ac5ed106f64"
//   },
//   "headers": {
//     "digest": "SHA-256=BC+k/+7vBkuFFNsNojrZkUwvmeiIKhrglFCyz3fuYxO=",
//     "signature": "keyId=\"primary\",algorithm=\"ed25519\",headers=\"digest\",signature=\"tOWOpZLhZwXVQ57jfC+GfwjQEnhtd1GluanaqUn6VqRX7hA1cG8BxNhPZpMYgnfP7xrZUZ1tWmw5J3EsjPHxDA==\""  // NOLINT
//   },
//   "octets": "{\"denomination\":{\"amount\":\"0\",\"currency\":\"BAT\"},\"destination\":\"4a1efaf8-4c9c-4ab2-8978-8ac5ed106f64\"}"  // NOLINT
// }
//
// Base64-encoded request body:
// {
//   "signedLinkingRequest": "eyJib2R5Ijp7ImRlbm9taW5hdGlvbiI6eyJhbW91bnQiOiIwIiwiY3VycmVuY3kiOiJCQVQifSwiZGVzdGluYXRpb24iOiIyZDM2ODlmNC1jYjdiLTQxYjctOGYzMy05ZDcxNmYyZTcwMDYifSwiaGVhZGVycyI6eyJkaWdlc3QiOiJTSEEtMjU2PXA4MHpJVXZ5V01FUUgwT2w0a0dnYm1RV2xMN3VYdktFWnRYSXFtTjZPZ3M9Iiwic2lnbmF0dXJlIjoia2V5SWQ9XCJwcmltYXJ5XCIsYWxnb3JpdGhtPVwiZWQyNTUxOVwiLGhlYWRlcnM9XCJkaWdlc3RcIixzaWduYXR1cmU9XCJ6Snplb2Q3YXplUjRlZGN6VWxYblA5ejRqeDI3Zm01L05JbTBxdnQ5VGgwUlpYWi9XL0pIK0pvS05IMUt1V01vZ3FFVWVWRHdxdmlqbklzblMzOG5BZz09XCIifSwib2N0ZXRzIjoie1wiZGVub21pbmF0aW9uXCI6e1wiYW1vdW50XCI6XCIwXCIsXCJjdXJyZW5jeVwiOlwiQkFUXCJ9LFwiZGVzdGluYXRpb25cIjpcIjJkMzY4OWY0LWNiN2ItNDFiNy04ZjMzLTlkNzE2ZjJlNzAwNlwifSJ9"  // NOLINT
// }
//
// clang-format on
//
// Response body:
// {
//   "geoCountry": "US"
// }

namespace brave_rewards::internal {
class RewardsEngine;

namespace endpoints {

class PostConnectUphold final : public PostConnect {
 public:
  PostConnectUphold(RewardsEngine& engine, std::string&& address);
  ~PostConnectUphold() override;

 private:
  std::optional<std::string> Content() const override;
  std::optional<std::vector<std::string>> Headers(
      const std::string& content) const override;

  std::string Path(base::cstring_view payment_id) const override;

  std::string address_;
};

}  // namespace endpoints
}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINTS_BRAVE_POST_CONNECT_UPHOLD_H_
