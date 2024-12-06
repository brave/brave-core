/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/url/request_builder/host/url_host_factory.h"

#include <ostream>

#include "base/notreached.h"
#include "base/types/cxx23_to_underlying.h"
#include "brave/components/brave_ads/core/internal/common/url/request_builder/host/hosts/anonymous_search_url_host.h"
#include "brave/components/brave_ads/core/internal/common/url/request_builder/host/hosts/anonymous_url_host.h"
#include "brave/components/brave_ads/core/internal/common/url/request_builder/host/hosts/geo_url_host.h"
#include "brave/components/brave_ads/core/internal/common/url/request_builder/host/hosts/non_anonymous_url_host.h"
#include "brave/components/brave_ads/core/internal/common/url/request_builder/host/hosts/static_url_host.h"

namespace brave_ads {

std::unique_ptr<UrlHostInterface> UrlHostFactory::Build(UrlHostType type) {
  switch (type) {
    case UrlHostType::kStatic: {
      return std::make_unique<StaticUrlHost>();
    }

    case UrlHostType::kGeo: {
      return std::make_unique<GeoUrlHost>();
    }

    case UrlHostType::kNonAnonymous: {
      return std::make_unique<NonAnonymousUrlHost>();
    }

    case UrlHostType::kAnonymous: {
      return std::make_unique<AnonymousUrlHost>();
    }

    case UrlHostType::kAnonymousSearch: {
      return std::make_unique<AnonymousSearchUrlHost>();
    }
  }

  NOTREACHED() << "Unexpected value for UrlHostType: "
               << base::to_underlying(type);
}

}  // namespace brave_ads
