/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINTS_POST_CONNECT_GEMINI_POST_CONNECT_GEMINI_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINTS_POST_CONNECT_GEMINI_POST_CONNECT_GEMINI_H_

#include <string>

#include "bat/ledger/internal/endpoints/post_connect/post_connect.h"

// POST /v3/wallet/gemini/{rewards_payment_id}/claim
//
// clang-format off
// Request body:
// {
//   "linking_info": "eyJ0eXAiOiJKV1QiLCJhbGciOiJIUzI8NiJ9.eyJpc3MiOiJHZW1pbmkiLCJleHAiOjE2NjA5NDA5ODUsImlhdCI1MTY2MDg1NDU4NTA4OSwiYWNjb3VudEhhc7hJZCI6IjNXUlc0RFExIiwiY0JlYXRlZEF0IjoxNjQ1MTE5NDcwMjAyfQ.cOt5NLeafF0OigHke7UFSrRnUdFXWRXzNYC344rSZ9M",  // NOLINT
//   "recipient_id": "62fea7848-ec12-42de-99c8-cf62da16c90f"
// }
// clang-format on
//
// Response body: -

namespace ledger {
class LedgerImpl;

namespace endpoints {

class PostConnectGemini final : public PostConnect {
 public:
  PostConnectGemini(LedgerImpl*,
                    std::string&& linking_info,
                    std::string&& recipient_id);
  ~PostConnectGemini() override;

 private:
  absl::optional<std::string> Content() const override;

  const char* Path() const override;

  std::string linking_info_;
  std::string recipient_id_;
};

}  // namespace endpoints
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINTS_POST_CONNECT_GEMINI_POST_CONNECT_GEMINI_H_
