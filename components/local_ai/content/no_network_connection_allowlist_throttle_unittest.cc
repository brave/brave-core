/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/local_ai/content/no_network_connection_allowlist_throttle.h"

#include "base/memory/scoped_refptr.h"
#include "brave/components/local_ai/core/url_constants.h"
#include "net/http/http_response_headers.h"
#include "services/network/public/cpp/connection_allowlist.h"
#include "services/network/public/cpp/parsed_headers.h"
#include "services/network/public/mojom/url_response_head.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace local_ai {

namespace {

GURL WorkerUrl() {
  return GURL(kOnDeviceSpeechRecognitionWorkerURL);
}

// A response as the WebUI loader builds it for the worker page.
network::mojom::URLResponseHeadPtr MakeWebUIResponse() {
  auto head = network::mojom::URLResponseHead::New();
  head->headers =
      base::MakeRefCounted<net::HttpResponseHeaders>("HTTP/1.1 200 OK");
  head->headers->SetHeader("Cross-Origin-Embedder-Policy", "require-corp");
  head->parsed_headers =
      network::PopulateParsedHeaders(head->headers.get(), WorkerUrl());
  return head;
}

}  // namespace

TEST(NoNetworkConnectionAllowlistThrottleTest, CreatedForSpeechWorkerOrigin) {
  EXPECT_TRUE(NoNetworkConnectionAllowlistThrottle::MaybeCreateThrottleFor(
      WorkerUrl()));
  EXPECT_TRUE(NoNetworkConnectionAllowlistThrottle::MaybeCreateThrottleFor(
      WorkerUrl().Resolve("ort.worker.js")));
}

TEST(NoNetworkConnectionAllowlistThrottleTest, NotCreatedForOtherOrigins) {
  EXPECT_FALSE(NoNetworkConnectionAllowlistThrottle::MaybeCreateThrottleFor(
      GURL("chrome-untrusted://other-page/")));
  GURL::Replacements https;
  https.SetSchemeStr("https");
  EXPECT_FALSE(NoNetworkConnectionAllowlistThrottle::MaybeCreateThrottleFor(
      WorkerUrl().ReplaceComponents(https)));
}

TEST(NoNetworkConnectionAllowlistThrottleTest, AddsEmptyEnforcedAllowlist) {
  NoNetworkConnectionAllowlistThrottle throttle;
  auto head = MakeWebUIResponse();
  bool defer = false;

  throttle.WillProcessResponse(WorkerUrl(), head.get(), &defer);

  EXPECT_EQ(head->headers->GetNormalizedHeader("Connection-Allowlist"), "()");
  // Debug builds check that the parsed copy matches the headers.
  EXPECT_TRUE(head->parsed_headers.Equals(
      network::PopulateParsedHeaders(head->headers.get(), WorkerUrl())));

  const auto& enforced = head->parsed_headers->connection_allowlists.enforced;
  ASSERT_TRUE(enforced.has_value());
  EXPECT_TRUE(enforced->allowlist.empty());
  EXPECT_EQ(enforced->webrtc_behavior,
            network::ConnectionAllowlist::WebRtcBehavior::kBlock);
}

}  // namespace local_ai
