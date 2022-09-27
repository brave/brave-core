/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINTS_POST_CONNECT_UPHOLD_POST_CONNECT_UPHOLD_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINTS_POST_CONNECT_UPHOLD_POST_CONNECT_UPHOLD_H_

#include <string>
#include <vector>

#include "bat/ledger/internal/endpoints/post_connect/post_connect.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

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
// Response body: -

namespace ledger {
class LedgerImpl;

namespace endpoints {

class PostConnectUphold final : public PostConnect {
 public:
  PostConnectUphold(LedgerImpl*, std::string&& address);
  ~PostConnectUphold() override;

 private:
  absl::optional<std::string> Content() const override;
  absl::optional<std::vector<std::string>> Headers(
      const std::string& content) const override;

  const char* Path() const override;

  std::string address_;
};

}  // namespace endpoints
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINTS_POST_CONNECT_UPHOLD_POST_CONNECT_UPHOLD_H_
