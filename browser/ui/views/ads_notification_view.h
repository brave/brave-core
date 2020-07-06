/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_ADS_NOTIFICATION_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_ADS_NOTIFICATION_VIEW_H_

#include "content/public/browser/web_contents_observer.h"
#include "ui/views/widget/widget_delegate.h"
#include "chrome/browser/profiles/profile.h"

#include <string>
#include "base/strings/string16.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
class Browser;

class AdsNotificationView : public views::WidgetDelegateView {
 public:
  static views::Widget* Show(Profile* profile,
                   const GURL& url,
                   const gfx::Rect& bounds = gfx::Rect());

  AdsNotificationView(Profile* profile);
  ~AdsNotificationView() override;

 private:
  void Close();

  DISALLOW_COPY_AND_ASSIGN(AdsNotificationView);
};

#endif  // BRAVE_BROWSER_UI_VIEWS_ADS_NOTIFICATION_VIEW_H_
