/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/brave_browser_dialogs.h"

#include "ui/gfx/image/image_skia_operations.h"
#include "brave/browser/ui/views/ads_notification_view.h"
#include "brave/ui/brave_custom_notification/message_popup_view.h"
#include "brave/ui/brave_custom_notification/public/cpp/notification.h"
#include "brave/grit/brave_theme_resources.h"
#include "brave/grit/brave_unscaled_resources.h"

namespace brave {

void ShowAdsNotification(Profile* profile) {
//  brave_custom_notification::MessagePopupView* mpv = new brave_custom_notification::MessagePopupView(profile);
  brave_custom_notification::Notification* notification = new brave_custom_notification::Notification(
    brave_custom_notification::NOTIFICATION_TYPE_SIMPLE,
     "id1",
     base::UTF8ToUTF16("title"),
     base::UTF8ToUTF16("message"),
     base::string16() /* display_source */,
     GURL("Brave Ad"), // origin url
     brave_custom_notification::RichNotificationData(), // option fields
     nullptr); // delegate
  brave_custom_notification::MessagePopupView::Show(*notification);
//  brave_custom_notification::MessagePopupView* mpv = new brave_custom_notification::MessagePopupView(*notification);
  // brave_custom_notification::MessagePopupView* mpv = new brave_custom_notification::MessagePopupView();
  // mpv->Show();
  LOG(ERROR) << __FUNCTION__;
  static int show_count = 0;
  show_count++;
  /*
  AdsNotificationView::Show(
      profile,
    GURL("https://m.media-amazon.com/images/I/418oH6YjpFL.jpg"));
    */
      /*
      show_count % 2 == 0 ? GURL("https://simonhong.github.io/")
                          : GURL("brave://rewards/"));
                          */
}

}
