/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ui/brave_custom_notification/message_view_factory.h"
#include <vector>
#include "base/lazy_instance.h"
#include "brave/ui/brave_custom_notification/notification_view_md.h"

namespace brave_custom_notification {

// static
MessageView* MessageViewFactory::Create(const Notification& notification) {
  MessageView* notification_view = nullptr;
  if (!notification_view)
    notification_view = new NotificationViewMD(notification);
  return notification_view;
}

}  // namespace message_center
