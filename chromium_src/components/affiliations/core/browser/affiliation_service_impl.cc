/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/affiliations/core/browser/affiliation_service_impl.h"

#define AffiliationServiceImpl AffiliationServiceImpl_ChromiumImpl
#include <components/affiliations/core/browser/affiliation_service_impl.cc>
#undef AffiliationServiceImpl

namespace affiliations {

void AffiliationServiceImpl::PrefetchChangePasswordURL(
    const GURL& urls,
    base::OnceClosure callback) {
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(FROM_HERE,
                                                           std::move(callback));
}

void AffiliationServiceImpl::Prefetch(const FacetURI& facet_uri,
                                      const base::Time& keep_fresh_until) {}

void AffiliationServiceImpl::RegisterSource(
    std::unique_ptr<AffiliationSource> source) {}

}  // namespace affiliations
