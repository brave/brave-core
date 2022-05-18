/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_SERVING_TARGETING_GEOGRAPHIC_SUBDIVISION_GET_SUBDIVISION_URL_REQUEST_BUILDER_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_SERVING_TARGETING_GEOGRAPHIC_SUBDIVISION_GET_SUBDIVISION_URL_REQUEST_BUILDER_H_

#include "bat/ads/internal/server/url/url_request_builder_interface.h"
#include "bat/ads/public/interfaces/ads.mojom.h"
#include "url/gurl.h"

namespace ads {
namespace targeting {
namespace geographic {

class GetSubdivisionUrlRequestBuilder final : UrlRequestBuilderInterface {
 public:
  GetSubdivisionUrlRequestBuilder();
  ~GetSubdivisionUrlRequestBuilder() override;

  mojom::UrlRequestPtr Build() override;

 private:
  GURL BuildUrl() const;
};

}  // namespace geographic
}  // namespace targeting
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_SERVING_TARGETING_GEOGRAPHIC_SUBDIVISION_GET_SUBDIVISION_URL_REQUEST_BUILDER_H_
