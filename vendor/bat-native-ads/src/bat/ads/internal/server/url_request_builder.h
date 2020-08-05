/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_SERVER_URL_REQUEST_BUILDER_H_
#define BAT_ADS_INTERNAL_SERVER_URL_REQUEST_BUILDER_H_

#include "bat/ads/mojom.h"

namespace ads {

class UrlRequestBuilder {
 public:
  virtual ~UrlRequestBuilder() = default;

  virtual UrlRequestPtr Build() = 0;
};

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_SERVER_URL_REQUEST_BUILDER_H_
