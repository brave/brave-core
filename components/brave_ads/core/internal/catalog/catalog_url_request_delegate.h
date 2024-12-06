/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CATALOG_CATALOG_URL_REQUEST_DELEGATE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CATALOG_CATALOG_URL_REQUEST_DELEGATE_H_

#include "base/time/time.h"

namespace brave_ads {

struct CatalogInfo;

class CatalogUrlRequestDelegate {
 public:
  // Invoked to tell the delegate we will fetch the catalog at `fetch_at`.
  virtual void OnWillFetchCatalog(base::Time fetch_at) {}

  // Invoked to tell the delegate we successfully fetched the `catalog`.
  virtual void OnDidFetchCatalog(const CatalogInfo& catalog) {}

  // Invoked to tell the delegate we failed to fetch the catalog.
  virtual void OnFailedToFetchCatalog() {}

  // Invoked to tell the delegate we will retry fetching the catalog at
  // `retry_at`.
  virtual void OnWillRetryFetchingCatalog(base::Time retry_at) {}

  // Invoked to tell the delegate we retried fetching the catalog.
  virtual void OnDidRetryFetchingCatalog() {}

 protected:
  virtual ~CatalogUrlRequestDelegate() = default;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CATALOG_CATALOG_URL_REQUEST_DELEGATE_H_
