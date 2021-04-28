/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/notifications/ads_notification_handler.h"

#include <memory>

#include "brave/browser/brave_ads/ads_service_factory.h"
#include "url/gurl.h"

namespace brave_ads {

const void* const kUserDataKey = &kUserDataKey;

AdsNotificationHandler::AdsNotificationHandler(Profile* profile)
    : profile_(profile) {
  DCHECK(profile_);
}

AdsNotificationHandler::~AdsNotificationHandler() = default;

void AdsNotificationHandler::OnShow(Profile* profile, const std::string& id) {
  AdsService* ads_service = AdsServiceFactory::GetForProfile(profile);
  if (!ads_service) {
    return;
  }

  ads_service->OnShowAdNotification(id);
}

void AdsNotificationHandler::OnClose(Profile* profile,
                                     const GURL& origin,
                                     const std::string& id,
                                     bool by_user,
                                     base::OnceClosure completed_closure) {
  AdsService* ads_service = AdsServiceFactory::GetForProfile(profile);
  if (!ads_service) {
    return;
  }

  ads_service->OnCloseAdNotification(id, by_user);
}

void AdsNotificationHandler::OnClick(
    Profile* profile,
    const GURL& origin,
    const std::string& id,
    const base::Optional<int>& action_index,
    const base::Optional<base::string16>& reply,
    base::OnceClosure completed_closure) {
  AdsService* ads_service = AdsServiceFactory::GetForProfile(profile);
  if (!ads_service) {
    return;
  }

  ads_service->OnClickAdNotification(id);
}

void AdsNotificationHandler::OpenSettings(Profile* profile,
                                          const GURL& origin) {
  AdsService* ads_service = AdsServiceFactory::GetForProfile(profile);
  if (!ads_service) {
    return;
  }

  DCHECK(origin.has_query());
  const std::string id = origin.query();

  ads_service->OnClickAdNotification(id);
}

}  // namespace brave_ads
