/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CATALOG_CATALOG_REQUEST_DELEGATE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CATALOG_CATALOG_REQUEST_DELEGATE_H_

namespace brave_ads {

struct CatalogInfo;

class CatalogRequestDelegate {
 public:
  // Invoked when the catalog was fetched.
  virtual void OnDidFetchCatalog(const CatalogInfo& catalog) {}

  // Invoked when failing to fetch the catalog.
  virtual void OnFailedToFetchCatalog() {}

 protected:
  virtual ~CatalogRequestDelegate() = default;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CATALOG_CATALOG_REQUEST_DELEGATE_H_
