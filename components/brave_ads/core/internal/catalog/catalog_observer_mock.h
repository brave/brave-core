/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CATALOG_CATALOG_OBSERVER_MOCK_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CATALOG_CATALOG_OBSERVER_MOCK_H_

#include "brave/components/brave_ads/core/internal/catalog/catalog_observer.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace brave_ads {

class CatalogObserverMock : public CatalogObserver {
 public:
  CatalogObserverMock();

  CatalogObserverMock(const CatalogObserverMock&) = delete;
  CatalogObserverMock& operator=(const CatalogObserverMock&) = delete;

  CatalogObserverMock(CatalogObserverMock&&) noexcept = delete;
  CatalogObserverMock& operator=(CatalogObserverMock&&) noexcept = delete;

  ~CatalogObserverMock() override;

  MOCK_METHOD(void, OnDidFetchCatalog, (const CatalogInfo& catalog));

  MOCK_METHOD(void, OnFailedToFetchCatalog, ());
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CATALOG_CATALOG_OBSERVER_MOCK_H_
