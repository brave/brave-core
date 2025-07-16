// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "src/components/variations/service/variations_service_unittest.cc"

namespace variations {

TEST_F(VariationsServiceTest,
       SetVariationsCountryWithNotModifiedResponseOnFirstFetch) {
  VariationsService::EnableFetchForTesting();

  TestVariationsService service(
      std::make_unique<web_resource::TestRequestAllowedNotifier>(
          &prefs_, network_tracker_),
      &prefs_, GetMetricsStateManager(), /*use_secure_url=*/true);
  service.set_intercepts_fetch(/*value=*/false);

  std::string headers("HTTP/1.1 304 Not Modified\n\n");
  auto head = network::mojom::URLResponseHead::New();
  head->headers = base::MakeRefCounted<net::HttpResponseHeaders>(
      net::HttpUtil::AssembleRawHeaders(headers));
  head->headers->SetHeader("X-Country", "FOO");
  network::URLLoaderCompletionStatus status;
  service.test_url_loader_factory()->AddResponse(
      service.interception_url(), std::move(head), /*content=*/"", status);

  service.DoActualFetch();

  EXPECT_EQ(prefs_.GetString(prefs::kVariationsCountry), "FOO");
}

TEST_F(VariationsServiceTest,
       DoNotSetVariationsCountryWithNotModifiedResponseOnSubsequentFetch) {
  VariationsService::EnableFetchForTesting();

  TestVariationsService service(
      std::make_unique<web_resource::TestRequestAllowedNotifier>(
          &prefs_, network_tracker_),
      &prefs_, GetMetricsStateManager(), /*use_secure_url=*/true);
  service.set_intercepts_fetch(/*value=*/false);

  auto first_response_head = network::mojom::URLResponseHead::New();
  first_response_head->headers = base::MakeRefCounted<net::HttpResponseHeaders>(
      net::HttpUtil::AssembleRawHeaders("HTTP/1.1 200 OK\n\n"));
  first_response_head->headers->SetHeader("X-Country", "FOO");
  network::URLLoaderCompletionStatus status;
  service.test_url_loader_factory()->AddResponse(service.interception_url(),
                                                 std::move(first_response_head),
                                                 /*content=*/"", status);

  service.DoActualFetch();

  EXPECT_EQ("FOO", service.stored_country());
  prefs_.SetString(prefs::kVariationsCountry, "FOO");

  auto second_response_head = network::mojom::URLResponseHead::New();
  second_response_head->headers =
      base::MakeRefCounted<net::HttpResponseHeaders>(
          net::HttpUtil::AssembleRawHeaders("HTTP/1.1 304 Not Modified\n\n"));
  second_response_head->headers->SetHeader("X-Country", "BAR");
  service.test_url_loader_factory()->AddResponse(
      service.interception_url(), std::move(second_response_head),
      /*content=*/"", status);

  service.DoActualFetch();

  EXPECT_EQ(prefs_.GetString(prefs::kVariationsCountry), "FOO");
}

TEST_F(VariationsServiceTest,
       DoNotSetVariationsEmptyCountryWithNotModifiedResponseOnFirstFetch) {
  VariationsService::EnableFetchForTesting();
  prefs_.SetString(prefs::kVariationsCountry, "FOO");

  TestVariationsService service(
      std::make_unique<web_resource::TestRequestAllowedNotifier>(
          &prefs_, network_tracker_),
      &prefs_, GetMetricsStateManager(), /*use_secure_url=*/true);
  service.set_intercepts_fetch(/*value=*/false);

  std::string headers("HTTP/1.1 304 Not Modified\n\n");
  auto head = network::mojom::URLResponseHead::New();
  head->headers = base::MakeRefCounted<net::HttpResponseHeaders>(
      net::HttpUtil::AssembleRawHeaders(headers));
  head->headers->SetHeader("X-Country", "");
  network::URLLoaderCompletionStatus status;
  service.test_url_loader_factory()->AddResponse(
      service.interception_url(), std::move(head), /*content=*/"", status);

  service.DoActualFetch();

  EXPECT_EQ(prefs_.GetString(prefs::kVariationsCountry), "FOO");
}

}  // namespace variations
