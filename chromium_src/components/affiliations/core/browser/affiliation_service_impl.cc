/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/affiliations/core/browser/affiliation_service_impl.h"

#define AffiliationServiceImpl AffiliationServiceImpl_ChromiumImpl
#include <components/affiliations/core/browser/affiliation_service_impl.cc>
#undef AffiliationServiceImpl

namespace affiliations {

void AffiliationServiceImpl::FetchChangePasswordURL(
    const GURL& url,
    base::OnceCallback<void(GURL)> callback) {
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(std::move(callback), GURL()));
}

void AffiliationServiceImpl::Prefetch(const FacetURI& facet_uri,
                                      const base::Time& keep_fresh_until) {}

void AffiliationServiceImpl::RegisterSource(
    std::unique_ptr<AffiliationSource> source) {}

}  // namespace affiliations
