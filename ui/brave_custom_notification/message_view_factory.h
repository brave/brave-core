// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BRAVE_CUSTOM_NOTIFICATION_MESSAGE_VIEW_FACTORY_H_
#define UI_BRAVE_CUSTOM_NOTIFICATION_MESSAGE_VIEW_FACTORY_H_

#include <memory>
#include <string>

#include "base/callback_forward.h"

namespace brave_custom_notification {

class MessageView;
class Notification;

// Creates appropriate MessageViews for notifications depending on the
// notification type. A notification is top level if it needs to be rendered
// outside the browser window. No custom shadows are created for top level
// notifications on Linux with Aura.
class MessageViewFactory {
 public:
  static MessageView* Create(const Notification& notification);
};

}  // namespace message_center

#endif  // UI_BRAVE_CUSTOM_NOTIFICATION_MESSAGE_VIEW_FACTORY_H_
