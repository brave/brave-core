/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_URL_REQUEST_BUILDER_URL_REQUEST_BUILDER_INTERFACE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_URL_REQUEST_BUILDER_URL_REQUEST_BUILDER_INTERFACE_H_

#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"

namespace brave_ads {

class UrlRequestBuilderInterface {
 public:
  virtual ~UrlRequestBuilderInterface() = default;

  virtual mojom::UrlRequestInfoPtr Build() = 0;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_URL_REQUEST_BUILDER_URL_REQUEST_BUILDER_INTERFACE_H_
