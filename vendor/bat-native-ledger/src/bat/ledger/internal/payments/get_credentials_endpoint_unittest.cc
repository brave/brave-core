/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/payments/get_credentials_endpoint.h"

#include <utility>

#include "bat/ledger/internal/core/bat_ledger_test.h"
#include "net/http/http_status_code.h"

namespace ledger {

class GetCredentialsEndpointTest : public BATLedgerTest {
 protected:
  GetCredentialsEndpoint& GetEndpoint() {
    return context().Get<GetCredentialsEndpoint>();
  }
};

TEST_F(GetCredentialsEndpointTest, MapRequest) {
  URLRequest request = GetEndpoint().MapRequest("order_id_1", "item_id_2");
  auto& req = request.req();
  EXPECT_EQ(req.method, mojom::UrlMethod::GET);
  EXPECT_EQ(req.url,
            "https://payment.rewards.brave.com/v1/orders/order_id_1/"
            "credentials/item_id_2");
}

TEST_F(GetCredentialsEndpointTest, MapResponseSuccess) {
  auto resp = mojom::UrlResponse::New();
  resp->status_code = net::HTTP_OK;
  resp->body = R"json(
    {
      "signedCreds": [
        "ijSZoLLG+EnRN916RUQcjiV6c4W",
        "dj6glCJ2roHYcTFcXF21IrKx1uT",
        "nCF9a4KuASICVC0zrx2wGnllgIU"
      ],
      "batchProof": "zx0cdJhaB/OdYcUtnyXdi+lsoniN2vRTZ1w0U4D7M",
      "publicKey": "dvpysTSiJdZUPihius7pvGOfngRWfDiIbrowykgMi1I="
    }
  )json";

  absl::optional<PaymentCredentials> result =
      GetEndpoint().MapResponse(URLResponse(std::move(resp)));

  ASSERT_TRUE(result);
  EXPECT_EQ(result->batch_proof, "zx0cdJhaB/OdYcUtnyXdi+lsoniN2vRTZ1w0U4D7M");
  EXPECT_EQ(result->public_key, "dvpysTSiJdZUPihius7pvGOfngRWfDiIbrowykgMi1I=");
  ASSERT_EQ(result->signed_tokens.size(), size_t(3));
  EXPECT_EQ(result->signed_tokens[0], "ijSZoLLG+EnRN916RUQcjiV6c4W");
  EXPECT_EQ(result->signed_tokens[1], "dj6glCJ2roHYcTFcXF21IrKx1uT");
  EXPECT_EQ(result->signed_tokens[2], "nCF9a4KuASICVC0zrx2wGnllgIU");
}

}  // namespace ledger
