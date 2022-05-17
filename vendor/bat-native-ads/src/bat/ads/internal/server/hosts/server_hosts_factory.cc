/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/server/hosts/server_hosts_factory.h"

#include "bat/ads/internal/server/hosts/anonymous_server_host.h"
#include "bat/ads/internal/server/hosts/geo_server_host.h"
#include "bat/ads/internal/server/hosts/non_anonymous_server_host.h"
#include "bat/ads/internal/server/hosts/static_server_host.h"

namespace ads {

std::unique_ptr<ServerHostInterface> ServerHostsFactory::Build(
    const ServerHostType type) {
  switch (type) {
    case ServerHostType::kStatic: {
      return std::make_unique<StaticServerHost>();
    }

    case ServerHostType::kGeo: {
      return std::make_unique<GeoServerHost>();
    }

    case ServerHostType::kNonAnonymous: {
      return std::make_unique<NonAnonymousServerHost>();
    }

    case ServerHostType::kAnonymous: {
      return std::make_unique<AnonymousServerHost>();
    }
  }
}

}  // namespace ads
