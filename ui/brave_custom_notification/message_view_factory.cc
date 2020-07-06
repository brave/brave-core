// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

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
