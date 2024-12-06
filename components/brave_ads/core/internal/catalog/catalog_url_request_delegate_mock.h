/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CATALOG_CATALOG_URL_REQUEST_DELEGATE_MOCK_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CATALOG_CATALOG_URL_REQUEST_DELEGATE_MOCK_H_

#include "brave/components/brave_ads/core/internal/catalog/catalog_url_request_delegate.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace brave_ads {

class CatalogUrlRequestDelegateMock : public CatalogUrlRequestDelegate {
 public:
  CatalogUrlRequestDelegateMock();

  CatalogUrlRequestDelegateMock(const CatalogUrlRequestDelegateMock&) = delete;
  CatalogUrlRequestDelegateMock& operator=(
      const CatalogUrlRequestDelegateMock&) = delete;

  ~CatalogUrlRequestDelegateMock() override;

  MOCK_METHOD(void, OnWillFetchCatalog, (base::Time fetch_at));

  MOCK_METHOD(void, OnDidFetchCatalog, (const CatalogInfo& catalog));

  MOCK_METHOD(void, OnFailedToFetchCatalog, ());

  MOCK_METHOD(void, OnWillRetryFetchingCatalog, (base::Time retry_at));

  MOCK_METHOD(void, OnDidRetryFetchingCatalog, ());
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CATALOG_CATALOG_URL_REQUEST_DELEGATE_MOCK_H_
