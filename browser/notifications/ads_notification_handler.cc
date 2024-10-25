/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/notifications/ads_notification_handler.h"

#include <optional>

#include "brave/browser/brave_ads/ads_service_factory.h"
#include "brave/components/brave_ads/browser/ads_service.h"
#include "build/build_config.h"
#include "url/gurl.h"

namespace brave_ads {

namespace {
const void* const kUserDataKey = &kUserDataKey;
}  // namespace

AdsNotificationHandler::AdsNotificationHandler(Profile& profile)
    : profile_(profile) {}

AdsNotificationHandler::~AdsNotificationHandler() = default;

void AdsNotificationHandler::OnShow(Profile* profile, const std::string& id) {
  AdsService* ads_service = AdsServiceFactory::GetForProfile(profile);
  if (!ads_service) {
    return;
  }

  did_click_notification_ad_ = false;

  ads_service->OnNotificationAdShown(id);
}

void AdsNotificationHandler::OnClose(Profile* profile,
                                     const GURL& origin,
                                     const std::string& id,
                                     const bool by_user,
                                     base::OnceClosure completed_closure) {
  AdsService* ads_service = AdsServiceFactory::GetForProfile(profile);
  if (!ads_service) {
    return;
  }

#if BUILDFLAG(IS_LINUX)
  if (did_click_notification_ad_) {
    // On Linux, clicking the notification triggers both 'clicked' and 'closed'
    // events. To avoid redundant event handling, we suppress the 'closed' event
    // if the notification ad was clicked.
    return;
  }
#endif  // BUILDFLAG(IS_LINUX)

  ads_service->OnNotificationAdClosed(id, by_user);
}

void AdsNotificationHandler::OnClick(Profile* profile,
                                     const GURL& origin,
                                     const std::string& id,
                                     const std::optional<int>& action_index,
                                     const std::optional<std::u16string>& reply,
                                     base::OnceClosure completed_closure) {
  AdsService* ads_service = AdsServiceFactory::GetForProfile(profile);
  if (!ads_service) {
    return;
  }

  did_click_notification_ad_ = true;

  ads_service->OnNotificationAdClicked(id);
}

void AdsNotificationHandler::OpenSettings(Profile* profile,
                                          const GURL& origin) {
  AdsService* ads_service = AdsServiceFactory::GetForProfile(profile);
  if (!ads_service) {
    return;
  }

  did_click_notification_ad_ = true;

  CHECK(origin.has_query());
  const std::string id = origin.query();

  ads_service->OnNotificationAdClicked(id);
}

}  // namespace brave_ads
