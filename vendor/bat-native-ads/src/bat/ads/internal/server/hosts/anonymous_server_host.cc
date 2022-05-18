/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/server/hosts/anonymous_server_host.h"

#include "bat/ads/ads.h"
#include "bat/ads/public/interfaces/ads.mojom.h"

namespace ads {

namespace {

constexpr char kProductionHost[] = "https://anonymous.ads.brave.com";
constexpr char kStagingHost[] = "https://anonymous.ads.bravesoftware.com";

}  // namespace

AnonymousServerHost::AnonymousServerHost() = default;

AnonymousServerHost::~AnonymousServerHost() = default;

std::string AnonymousServerHost::Get() const {
  switch (g_environment) {
    case mojom::Environment::kProduction: {
      return kProductionHost;
    }

    case mojom::Environment::kStaging: {
      return kStagingHost;
    }
  }
}

}  // namespace ads
