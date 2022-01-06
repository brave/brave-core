/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/payments/post_credentials_endpoint.h"

#include <utility>

#include "bat/ledger/internal/core/bat_ledger_test.h"
#include "net/http/http_status_code.h"

namespace ledger {

class PostCredentialsEndpointTest : public BATLedgerTest {
 protected:
  PostCredentialsEndpoint& GetEndpoint() {
    return context().Get<PostCredentialsEndpoint>();
  }
};

TEST_F(PostCredentialsEndpointTest, MapRequest) {
  URLRequest request = GetEndpoint().MapRequest(
      "order_id_1", "item_id_2", PaymentCredentialType::kSingleUse,
      {"wqto9FnferrKUM0lcp2B0lecMQwArvUq3hWGCYlXiQo=",
       "ZiSXpF61aZ/tL2MxkKzI5Vnw2aLJE2ln2FMHAtKc9Co="});

  auto& req = request.req();
  EXPECT_EQ(req.method, mojom::UrlMethod::POST);
  EXPECT_EQ(
      req.url,
      "https://payment.rewards.brave.com/v1/orders/order_id_1/credentials");
  EXPECT_EQ(
      req.content,
      R"({"blindedCreds":["wqto9FnferrKUM0lcp2B0lecMQwArvUq3hWGCYlXiQo=",)"
      R"("ZiSXpF61aZ/tL2MxkKzI5Vnw2aLJE2ln2FMHAtKc9Co="],"itemId":"item_id_2")"
      R"(,"type":"single-use"})");
}

TEST_F(PostCredentialsEndpointTest, MapResponseSuccess) {
  auto resp = mojom::UrlResponse::New();
  resp->status_code = net::HTTP_OK;
  bool result = GetEndpoint().MapResponse(URLResponse(std::move(resp)));
  EXPECT_TRUE(result);
}

}  // namespace ledger
