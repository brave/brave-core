/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/password_manager/core/browser/affiliation/hash_affiliation_fetcher.h"

namespace {

class BraveHashAffiliationFetcher
    : public password_manager::HashAffiliationFetcher {
 public:
  using HashAffiliationFetcher::HashAffiliationFetcher;

  void StartRequest(const std::vector<password_manager::FacetURI>& facet_uris,
                    RequestInfo request_info) override {}
};

}  // namespace

#define HashAffiliationFetcher BraveHashAffiliationFetcher
#include "src/components/password_manager/core/browser/affiliation/affiliation_fetcher_factory_impl.cc"
#undef HashAffiliationFetcher
