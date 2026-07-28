/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/affiliations/core/browser/hash_affiliation_fetcher.h"

#include <components/affiliations/core/browser/hash_affiliation_fetcher_unittest.cc>

namespace affiliations {

class DisabledHashAffiliationFetcherTest : public HashAffiliationFetcherTest {};

TEST_F(DisabledHashAffiliationFetcherTest, BraveBuildQueryURL) {
  HashAffiliationFetcher fetcher(test_shared_loader_factory());

  GURL query_url = fetcher.BuildQueryURL();

  EXPECT_EQ("https", query_url.GetScheme());
  EXPECT_EQ("www.googleapis.com", query_url.GetHost());
  EXPECT_EQ("/affiliation/v1/affiliation:lookupByHashPrefix",
            query_url.GetPath());
}

TEST_F(DisabledHashAffiliationFetcherTest, BraveStartRequestIsStubbed) {
  HashAffiliationFetcher fetcher(test_shared_loader_factory());

  std::vector<FacetURI> requested_uris = {
      FacetURI::FromCanonicalSpec(kExampleWebFacet1URI)};

  bool callback_ran = false;
  fetcher.StartRequest(
      requested_uris, {},
      base::BindLambdaForTesting(
          [&](AffiliationFetcherInterface::FetchResult result) {
            callback_ran = true;
            EXPECT_FALSE(result.data.has_value());
            EXPECT_EQ(net::OK, result.network_status);
          }));

  // The result must not be delivered synchronously: callers of StartRequest
  // do not expect completion to happen before it returns.
  EXPECT_FALSE(callback_ran);

  WaitForResponse();

  EXPECT_TRUE(callback_ran);
  EXPECT_TRUE(fetcher.GetRequestedFacetURIs().empty());
}

TEST_F(DisabledHashAffiliationFetcherTest,
       BraveStartRequestCallbackFiresOnDestruction) {
  auto fetcher =
      std::make_unique<HashAffiliationFetcher>(test_shared_loader_factory());

  std::vector<FacetURI> requested_uris = {
      FacetURI::FromCanonicalSpec(kExampleWebFacet1URI)};

  bool callback_ran = false;
  fetcher->StartRequest(
      requested_uris, {},
      base::BindLambdaForTesting(
          [&](AffiliationFetcherInterface::FetchResult result) {
            callback_ran = true;
            EXPECT_FALSE(result.IsSuccessful());
          }));

  // Destroying the fetcher before its posted completion task ever runs must
  // still invoke the callback synchronously, exactly once: callers (see
  // AffiliationFetcherManager::Fetch()) rely on this to never leak state
  // waiting on a callback that will now never come.
  fetcher.reset();

  EXPECT_TRUE(callback_ran);
}

}  // namespace affiliations
