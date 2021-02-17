/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_UI_BRAVE_ADS_NOTIFICATION_CONTROL_BUTTONS_VIEW_H_
#define BRAVE_UI_BRAVE_ADS_NOTIFICATION_CONTROL_BUTTONS_VIEW_H_

#include <memory>

#include "base/macros.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/view.h"

namespace ui {
class Event;
}

namespace views {
class Button;
}

namespace brave_ads {

class NotificationView;
class PaddedImage;
class PaddedButton;

class NotificationControlButtonsView : public views::View {
 public:
  // String to be returned by GetClassName() method.
  static const char kViewClassName[];

  explicit NotificationControlButtonsView(NotificationView* message_view);
  ~NotificationControlButtonsView() override;

  // Change the visibility of the close button. True to show, false to hide.
  void ShowCloseButton(bool show);

  void ShowInfoButton(bool show);
  // Change the visibility of all buttons. True to show, false to hide.
  void ShowButtons(bool show);

  // Return the focus status of any button. True if the focus is on any button,
  // false otherwise.
  bool IsAnyButtonFocused() const;

  // Methods for retrieving the control buttons directly.
  views::Button* close_button() const;
  views::ImageView* info_button() const;

  // views::View
  const char* GetClassName() const override;

 private:
  NotificationView* message_view_;

  std::unique_ptr<PaddedButton> close_button_;
  std::unique_ptr<PaddedImage> info_button_;

  DISALLOW_COPY_AND_ASSIGN(NotificationControlButtonsView);
};

}  // namespace brave_ads

#endif  // BRAVE_UI_BRAVE_ADS_NOTIFICATION_CONTROL_BUTTONS_VIEW_H_
