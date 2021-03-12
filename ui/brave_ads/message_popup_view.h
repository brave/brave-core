/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_UI_BRAVE_ADS_MESSAGE_POPUP_VIEW_H_
#define BRAVE_UI_BRAVE_ADS_MESSAGE_POPUP_VIEW_H_

#include <string>

#include "base/scoped_observer.h"
#include "brave/ui/brave_ads/public/cpp/notification.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_delegate.h"
#include "ui/views/widget/widget_observer.h"

namespace brave_ads {

class NotificationView;

class MessagePopupView : public views::WidgetDelegateView,
                         public views::WidgetObserver {
 public:
  // Returns static reference
  static void Show(const Notification& notification);

  // Destroys the widget
  static void ClosePopup(const bool by_user);

  // Tells AdsNotificationHandler that this was clicked
  static void Clicked(const std::string& notification_id);

  explicit MessagePopupView(const Notification& notification);
  ~MessagePopupView() override;

  Notification notification_;

  // Return visible frame for primary display.
  gfx::Rect GetVisibleFrameForPrimaryDisplay() const;

  // Return opacity of the widget.
  float GetOpacity() const;

  // Sets widget bounds.
  void SetPopupBounds(const gfx::Rect& bounds);

  // Set widget opacity.
  void SetOpacity(float opacity);

  // Shows popup. After this call, MessagePopupView should be owned by the
  // widget.
  void Show();

  // Closes popup. It should be callable even if Show() is not called, and
  // in such case MessagePopupView should be deleted. Virtual for unit testing.
  virtual void Close();

  const char* GetClassName() const override;
  void OnFocus() override;

  // views::WidgetObserver:
  void OnWidgetActivationChanged(views::Widget* widget, bool active) override;
  void OnWidgetDestroyed(views::Widget* widget) override;

  bool is_hovered() const { return is_hovered_; }
  bool is_active() const { return is_active_; }

  NotificationView* message_view() { return message_view_; }

 private:
  // True if the view has a widget and the widget is not closed.
  bool IsWidgetValid() const;
  int GetBodyHeight(const base::string16& message);

  // Owned by views hierarchy.
  NotificationView* message_view_;

  bool is_hovered_ = false;
  bool is_active_ = false;
  views::Widget* popup_window_ = nullptr;

  ScopedObserver<views::Widget, views::WidgetObserver> observer_{this};

  DISALLOW_COPY_AND_ASSIGN(MessagePopupView);
};

}  // namespace brave_ads

#endif  // BRAVE_UI_BRAVE_ADS_MESSAGE_POPUP_VIEW_H_
