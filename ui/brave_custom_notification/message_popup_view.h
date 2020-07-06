// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BRAVE_UI_BRAVE_CUSTOM_NOTIFICATION_MESSAGE_POPUP_VIEW_H_
#define BRAVE_UI_BRAVE_CUSTOM_NOTIFICATION_MESSAGE_POPUP_VIEW_H_

#include "base/scoped_observer.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_delegate.h"
// #include "chrome/browser/profiles/profile.h"
#include "ui/views/widget/widget_observer.h"

namespace brave_custom_notification {

class MessageView;
class Notification;

class MessagePopupView : public views::WidgetDelegateView,
                         public views::WidgetObserver {
 public:
  static void Show(const Notification& notification); // Returns static reference
  static void ClosePopup(); // Destroys the widget
  MessagePopupView();
   MessagePopupView(const Notification& notification);
  ~MessagePopupView() override;

  // Return opacity of the widget.
  float GetOpacity() const;

  // Sets widget bounds.
  void SetPopupBounds(const gfx::Rect& bounds);

  // Set widget opacity.
  void SetOpacity(float opacity);

  // Collapses the notification unless the user is interacting with it. The
  // request can be ignored. Virtual for unit testing.
// virtual void AutoCollapse();

  // Shows popup. After this call, MessagePopupView should be owned by the
  // widget.
  void Show();

  // Closes popup. It should be callable even if Show() is not called, and
  // in such case MessagePopupView should be deleted. Virtual for unit testing.
  virtual void Close();

  // views::WidgetDelegateView:
//  void OnMouseEntered(const ui::MouseEvent& event) override;
//  void OnMouseExited(const ui::MouseEvent& event) override;
//  void ChildPreferredSizeChanged(views::View* child) override;
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override;
  const char* GetClassName() const override;
//  void OnDisplayChanged() override;
//  void OnWorkAreaChanged() override;
  void OnFocus() override;

  // views::WidgetObserver:
  void OnWidgetActivationChanged(views::Widget* widget, bool active) override;
  void OnWidgetDestroyed(views::Widget* widget) override;

  bool is_hovered() const { return is_hovered_; }
  bool is_active() const { return is_active_; }

  MessageView* message_view() { return message_view_; }

 private:
  // True if the view has a widget and the widget is not closed.
  bool IsWidgetValid() const;

  // Owned by views hierarchy.
  MessageView* message_view_;

  bool is_hovered_ = false;
  bool is_active_ = false;
  views::Widget* popup_window_ = nullptr;

  ScopedObserver<views::Widget, views::WidgetObserver> observer_{this};

  DISALLOW_COPY_AND_ASSIGN(MessagePopupView);
};

}  // namespace message_center

#endif  // UI_MESSAGE_CENTER_VIEWS_MESSAGE_POPUP_VIEW_H_
