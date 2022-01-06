/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/payments/post_external_transaction_endpoint.h"

#include <utility>

#include "bat/ledger/internal/core/bat_ledger_test.h"
#include "net/http/http_status_code.h"

namespace ledger {

class PostExternalTransactionEndpointTest : public BATLedgerTest {
 protected:
  PostExternalTransactionEndpoint& GetEndpoint() {
    return context().Get<PostExternalTransactionEndpoint>();
  }
};

TEST_F(PostExternalTransactionEndpointTest, MapRequest) {
  URLRequest request = GetEndpoint().MapRequest(
      "order_id_1", "transaction_id_2", ExternalWalletProvider::kUphold);

  auto& req = request.req();
  EXPECT_EQ(req.method, mojom::UrlMethod::POST);
  EXPECT_EQ(req.url,
            "https://payment.rewards.brave.com/v1/orders/order_id_1/"
            "transactions/uphold");
  EXPECT_EQ(req.content,
            R"({"externalTransactionId":"transaction_id_2","kind":"uphold"})");
}

TEST_F(PostExternalTransactionEndpointTest, MapResponseSuccess) {
  auto resp = mojom::UrlResponse::New();
  resp->status_code = net::HTTP_OK;
  bool result = GetEndpoint().MapResponse(URLResponse(std::move(resp)));
  EXPECT_TRUE(result);
}

}  // namespace ledger
