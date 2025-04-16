/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/affiliations/core/browser/hash_affiliation_fetcher.h"

namespace {

class BraveHashAffiliationFetcher
    : public affiliations::HashAffiliationFetcher {
 public:
  using HashAffiliationFetcher::HashAffiliationFetcher;

  void StartRequest(
      const std::vector<affiliations::FacetURI>& facet_uris,
      RequestInfo request_info,
      base::OnceCallback<void(FetchResult)> result_callback) override {}
};

}  // namespace

#define HashAffiliationFetcher BraveHashAffiliationFetcher
#include "src/components/affiliations/core/browser/affiliation_fetcher_factory_impl.cc"
#undef HashAffiliationFetcher
