// Copyright 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/notifications/ads_notification_handler.h"

#define BRAVE_ADD_BRAVE_ADS_NOTIFICATION_HANDLER \
  AddNotificationHandler(                        \
      NotificationHandler::Type::BRAVE_ADS,      \
      std::make_unique<brave_ads::AdsNotificationHandler>(profile));
#include "../../../../../chrome/browser/notifications/notification_display_service_impl.cc"
#undef BRAVE_ADD_BRAVE_ADS_NOTIFICATION_HANDLER
