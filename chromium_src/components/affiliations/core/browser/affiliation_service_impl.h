/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_AFFILIATIONS_CORE_BROWSER_AFFILIATION_SERVICE_IMPL_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_AFFILIATIONS_CORE_BROWSER_AFFILIATION_SERVICE_IMPL_H_

// Subclass AffiliationServiceImpl so that we can disable prefetch. This
// functionality requires Google API key.
#define AffiliationServiceImpl AffiliationServiceImpl_ChromiumImpl
#include <components/affiliations/core/browser/affiliation_service_impl.h>  // IWYU pragma: export
#undef AffiliationServiceImpl

namespace affiliations {

class AffiliationServiceImpl : public AffiliationServiceImpl_ChromiumImpl {
 public:
  using AffiliationServiceImpl_ChromiumImpl::
      AffiliationServiceImpl_ChromiumImpl;

  // AffiliationService overrides:
  void PrefetchChangePasswordURL(const GURL& urls,
                                 base::OnceClosure callback) override;
  void Prefetch(const FacetURI& facet_uri,
                const base::Time& keep_fresh_until) override;
  void RegisterSource(std::unique_ptr<AffiliationSource> source) override;
};

}  // namespace affiliations

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_AFFILIATIONS_CORE_BROWSER_AFFILIATION_SERVICE_IMPL_H_
