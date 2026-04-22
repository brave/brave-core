/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/buildflags/buildflags.h"
#include "brave/components/brave_wallet/common/buildflags/buildflags.h"
#include "build/build_config.h"

#if BUILDFLAG(ENABLE_BRAVE_ADS)
#include "brave/browser/notifications/ads_notification_handler.h"  // IWYU pragma: keep

#define BRAVE_ADD_BRAVE_ADS_NOTIFICATION_HANDLER \
  AddNotificationHandler(                        \
      NotificationHandler::Type::BRAVE_ADS,      \
      std::make_unique<brave_ads::AdsNotificationHandler>(*profile));
#else
#define BRAVE_ADD_BRAVE_ADS_NOTIFICATION_HANDLER
#endif  // BUILDFLAG(ENABLE_BRAVE_ADS)

#if BUILDFLAG(ENABLE_BRAVE_WALLET) && !BUILDFLAG(IS_ANDROID)
#include "brave/browser/brave_wallet/notifications/brave_wallet_notification_handler.h"  // IWYU pragma: keep

#define BRAVE_ADD_BRAVE_WALLET_NOTIFICATION_HANDLER                   \
  AddNotificationHandler(                                             \
      NotificationHandler::Type::BRAVE_WALLET,                        \
      std::make_unique<brave_wallet::BraveWalletNotificationHandler>( \
          *profile));
#else
#define BRAVE_ADD_BRAVE_WALLET_NOTIFICATION_HANDLER
#endif  // BUILDFLAG(ENABLE_BRAVE_WALLET) && !BUILDFLAG(IS_ANDROID)

#include <chrome/browser/notifications/notification_display_service_impl.cc>
#undef BRAVE_ADD_BRAVE_ADS_NOTIFICATION_HANDLER
#undef BRAVE_ADD_BRAVE_WALLET_NOTIFICATION_HANDLER
