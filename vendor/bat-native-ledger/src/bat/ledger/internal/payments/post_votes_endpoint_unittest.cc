/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/payments/post_votes_endpoint.h"

#include <utility>

#include "bat/ledger/internal/core/bat_ledger_test.h"
#include "net/http/http_status_code.h"

namespace ledger {

class PostVotesEndpointTest : public BATLedgerTest {
 protected:
  PostVotesEndpoint& GetEndpoint() {
    return context().Get<PostVotesEndpoint>();
  }
};

TEST_F(PostVotesEndpointTest, MapRequest) {
  URLRequest request = GetEndpoint().MapRequest(
      "publisher1", PaymentVoteType::kOneOffTip,
      {{.unblinded_token =
            "rZqo/CuR9uPQUAbN/vVk+Oq+4BQ/"
            "bnwSg2kfUjVtXkgaAJd5yK8VLiSJexiZvFnLIftmXgAzJJ/"
            "kZX0eY352YJpVMrmh6ZxofuwN9ResCzmwHIa9j8idbVXkF9TxWxt6",
        .public_key = "6AphTvx13IgxVRG1nljV2ql1Y7yGUol6yrVMhEP85wI="},
       {.unblinded_token =
            "Iqu/"
            "C1HIAV7ihHTY5DvBbhjhgPV6r1zvaxrUH3NELJWfEBNk7IFvRxg5N8kZBmvql5YkSZ"
            "l6vzzvN9iY/gTmfRgk36Rkgwu4+BVIo/1/hxkAMpN13EdE8hE5fPUSpGR4",
        .public_key = "6AphTvx13IgxVRG1nljV2ql1Y7yGUol6yrVMhEP85wI="}});

  auto& req = request.req();
  EXPECT_EQ(req.method, mojom::UrlMethod::POST);
  EXPECT_EQ(req.url, "https://payment.rewards.brave.com/v1/votes");
  EXPECT_EQ(
      req.content,
      R"({"credentials":[{"publicKey":"6AphTvx13IgxVRG1nljV2ql1Y7yGUol6yrVMhEP)"
      R"(85wI=","signature":"CYRf96+52uWyLli5AcyTAYKdgCb2tadXEnB/sNvrtrmCLOoWr)"
      R"(9F0Getaa81SwTJKNprYd9+ZGlPxcQkuQvXQhw==","t":"rZqo/CuR9uPQUAbN/vVk+Oq)"
      R"(+4BQ/bnwSg2kfUjVtXkgaAJd5yK8VLiSJexiZvFnLIftmXgAzJJ/kZX0eY352YA=="},{)"
      R"("publicKey":"6AphTvx13IgxVRG1nljV2ql1Y7yGUol6yrVMhEP85wI=","signature)"
      R"(":"1cItka5ffL0oWK22QOXPOf64ePsTtPrA7He+ZXKmkhihW1pFvwGTVNx5t92PaeIfFN)"
      R"(AT+t9lZAIoqAzBLb1p0Q==","t":"Iqu/C1HIAV7ihHTY5DvBbhjhgPV6r1zvaxrUH3NE)"
      R"(LJWfEBNk7IFvRxg5N8kZBmvql5YkSZl6vzzvN9iY/gTmfQ=="}],"vote":"eyJjaGFub)"
      R"(mVsIjoicHVibGlzaGVyMSIsInR5cGUiOiJvbmVvZmYtdGlwIn0="})");
}

TEST_F(PostVotesEndpointTest, MapResponseSuccess) {
  auto resp = mojom::UrlResponse::New();
  resp->status_code = net::HTTP_OK;
  bool result = GetEndpoint().MapResponse(URLResponse(std::move(resp)));
  EXPECT_TRUE(result);
}

}  // namespace ledger
