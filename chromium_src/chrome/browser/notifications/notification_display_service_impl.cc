/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/notifications/notification_display_service_impl.h"

#include <memory>

#include "brave/components/brave_ads/buildflags/buildflags.h"
#include "brave/components/brave_wallet/common/buildflags/buildflags.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/profile.h"

#if BUILDFLAG(ENABLE_BRAVE_ADS)
#include "brave/browser/notifications/ads_notification_handler.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_WALLET) && !BUILDFLAG(IS_ANDROID)
#include "brave/browser/brave_wallet/notifications/brave_wallet_notification_handler.h"
#endif

namespace {

void AddBraveNotificationHandlers(NotificationDisplayServiceImpl& service,
                                  Profile& profile) {
#if BUILDFLAG(ENABLE_BRAVE_ADS)
  service.AddNotificationHandler(
      NotificationHandler::Type::BRAVE_ADS,
      std::make_unique<brave_ads::AdsNotificationHandler>(profile));
#endif
#if BUILDFLAG(ENABLE_BRAVE_WALLET) && !BUILDFLAG(IS_ANDROID)
  service.AddNotificationHandler(
      NotificationHandler::Type::BRAVE_WALLET,
      std::make_unique<brave_wallet::BraveWalletNotificationHandler>(profile));
#endif
}

}  // namespace

#include <chrome/browser/notifications/notification_display_service_impl.cc>
