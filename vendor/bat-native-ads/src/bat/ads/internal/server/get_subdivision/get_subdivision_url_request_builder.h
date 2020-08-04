/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_SERVER_GET_SUBDIVISION_GET_SUBDIVISION_URL_REQUEST_BUILDER_H_  // NOLINT
#define BAT_ADS_INTERNAL_SERVER_GET_SUBDIVISION_GET_SUBDIVISION_URL_REQUEST_BUILDER_H_  // NOLINT

#include <string>

#include "bat/ads/internal/server/url_request_builder.h"

namespace ads {

class GetSubdivisionUrlRequestBuilder : UrlRequestBuilder {
 public:
  GetSubdivisionUrlRequestBuilder();

  ~GetSubdivisionUrlRequestBuilder() override;

  UrlRequestPtr Build() override;

 private:
  std::string BuildUrl() const;
};

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_SERVER_GET_SUBDIVISION_GET_SUBDIVISION_URL_REQUEST_BUILDER_H_  // NOLINT
