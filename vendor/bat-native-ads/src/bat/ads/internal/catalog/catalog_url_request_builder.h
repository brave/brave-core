/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CATALOG_CATALOG_URL_REQUEST_BUILDER_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CATALOG_CATALOG_URL_REQUEST_BUILDER_H_

#include "bat/ads/internal/server/url/url_request_builder_interface.h"
#include "bat/ads/public/interfaces/ads.mojom.h"
#include "url/gurl.h"

namespace ads {

class CatalogUrlRequestBuilder final : public UrlRequestBuilderInterface {
 public:
  CatalogUrlRequestBuilder();
  ~CatalogUrlRequestBuilder() override;
  CatalogUrlRequestBuilder(const CatalogUrlRequestBuilder&) = delete;
  CatalogUrlRequestBuilder& operator=(const CatalogUrlRequestBuilder&) = delete;

  mojom::UrlRequestPtr Build() override;

 private:
  GURL BuildUrl() const;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CATALOG_CATALOG_URL_REQUEST_BUILDER_H_
