// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/brave_shields/ad_block_subscription_download_client_ios.h"

#include "base/functional/bind.h"
#include "brave/components/brave_shields/core/browser/ad_block_service.h"
#include "brave/components/brave_shields/core/browser/ad_block_subscription_download_client.h"
#include "brave/ios/browser/brave_shields/ad_block_service_factory.h"

namespace brave_shields {

std::unique_ptr<download::Client> CreateAdBlockSubscriptionDownloadClientIOS(
    ProfileIOS* profile) {
  // The download service is owned by `profile`, so the client never outlives
  // it. The ad-block service is only looked up, never created, from here.
  return std::make_unique<AdBlockSubscriptionDownloadClient>(
      base::BindRepeating(
          [](ProfileIOS* profile) -> AdBlockSubscriptionServiceManager* {
            auto* service =
                AdBlockServiceFactory::GetForProfileIfExists(profile);
            return service ? service->subscription_service_manager() : nullptr;
          },
          base::Unretained(profile)));
}

}  // namespace brave_shields
