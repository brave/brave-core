// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <memory>

#include "components/download/public/background_service/client.h"
#include "components/download/public/background_service/clients.h"

class ProfileIOS;

namespace brave_shields {
std::unique_ptr<download::Client> CreateAdBlockSubscriptionDownloadClientIOS(
    ProfileIOS* profile);
}  // namespace brave_shields

#define BRAVE_BACKGROUND_DOWNLOAD_SERVICE_FACTORY_BUILD_SERVICE_INSTANCE_FOR \
  clients->insert(std::make_pair(                                            \
      download::DownloadClient::CUSTOM_LIST_SUBSCRIPTIONS,                   \
      brave_shields::CreateAdBlockSubscriptionDownloadClientIOS(profile)));

#include <ios/chrome/browser/download/model/background_service/background_download_service_factory.cc>

#undef BRAVE_BACKGROUND_DOWNLOAD_SERVICE_FACTORY_BUILD_SERVICE_INSTANCE_FOR
