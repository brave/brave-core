/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINT_POST_CONNECT_BITFLYER_POST_CONNECT_BITFLYER_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINT_POST_CONNECT_BITFLYER_POST_CONNECT_BITFLYER_H_

#include <string>

#include "base/location.h"
#include "base/types/expected.h"
#include "bat/ledger/internal/endpoint/post_connect/post_connect.h"

// POST /v3/wallet/bitflyer/{rewards_payment_id}/claim
//
// clang-format off
// Request body:
// {
//   "linkingInfo": "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJleHRlcm5hbF9hY2NvdW50X2lkMjoiMzU5Qzg1NUJCRTdBRUFENjc3QUQxMjQ5ODAzQkQ5NURBNTI3OEQ4MTU3QjU4REJCNDU0MTVEOUZBUEVBMzU4MyIsInJlcXVlc3RfaWQiOiJhM2RjHGRhYi0xZDc0LTQ0YzYtOGE5Zi34YTVhMTNhYWE0MjgiLCJ0aW1lc3RhbXAiOiIyNDIyLTA4LTE4VDIwOjM0OjA5LjE4MDIxMTFaIiwiYWNjb3VudF9oYXNoIjoiZjUwYjAxOGI1ZjJiNzVhMDBjMzBlYjI4NmEyMmJhZjExYzg4Y2VjMSIsImRlcG9zaXRfaWQiOiI4ZjgxMmU0MS0yODUyLTRmNGItOTgxNy0wNDdiZjA5NDYzZmMifQ.P9_JMU5QRwmaaDjjldXvax5WlbjxksZi7ljiKEJ5kMk"  // NOLINT
// }
// clang-format on
//
// Success code:
// HTTP_OK (200)
//
// Error codes:
// HTTP_BAD_REQUEST (400)
// HTTP_FORBIDDEN (403)
// HTTP_NOT_FOUND (404)
// HTTP_CONFLICT (409)
// HTTP_INTERNAL_SERVER_ERROR (500)

namespace ledger {
class LedgerImpl;

namespace endpoint::connect {

class PostConnectBitflyer final : public PostConnect {
 public:
  PostConnectBitflyer(LedgerImpl*, const std::string& linking_info);
  ~PostConnectBitflyer() override;

 private:
  base::expected<std::string, base::Location> Content() const override;

  const char* Path() const override;

  std::string linking_info_;
};

}  // namespace endpoint::connect
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINT_POST_CONNECT_BITFLYER_POST_CONNECT_BITFLYER_H_
