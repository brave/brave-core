/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ui/brave_ads/message_popup_view.h"

#include <map>
#include <memory>
#include <string>
#include <utility>

#include "brave/ui/brave_ads/notification_view.h"
#include "brave/ui/brave_ads/notification_view_factory.h"
#include "brave/ui/brave_ads/public/cpp/constants.h"
#include "build/build_config.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/textfield/textfield.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/widget/widget.h"

#if defined(OS_WIN)
#include "ui/views/widget/desktop_aura/desktop_native_widget_aura.h"
#endif

#if defined(OS_CHROMEOS)
#include "ui/aura/window.h"
#include "ui/aura/window_targeter.h"
#endif

namespace brave_ads {
namespace {
static std::map<std::string, MessagePopupView*> g_notifications_;
static const int kPopupPadding = 10;
static const int kPopupBaseWidth = 344;
static const int kPopupBaseHeight = 88;
static const int kBodyPixelLineHeight = 10;
static const int kBodyCharactersPerLine = 40;
}  // namespace

// static
void MessagePopupView::Show(const Notification& notification) {
  // Close previous showing notification
  MessagePopupView::ClosePopup(false);
  g_notifications_[notification.id()] = new MessagePopupView(notification);
}

// static
void MessagePopupView::Clicked(const std::string& notification_id) {
  MessagePopupView* message_popup_view = g_notifications_[notification_id];
  NotificationDelegate* notification_delegate =
      message_popup_view->notification_.delegate();
  if (notification_delegate) {
    notification_delegate->Click(base::nullopt, base::nullopt);
  }
  message_popup_view->Close();
  g_notifications_.erase(notification_id);
}

// static
void MessagePopupView::ClosePopup(const bool by_user) {
  for (const auto& notification : g_notifications_) {
    MessagePopupView* message_popup_view = notification.second;

    NotificationDelegate* notification_delegate =
        message_popup_view->notification_.delegate();
    if (notification_delegate) {
      notification_delegate->Close(by_user);
    }

    message_popup_view->Close();
  }

  g_notifications_.clear();
}

MessagePopupView::MessagePopupView(const Notification& notification)
    : notification_(notification) {
  SetLayoutManager(std::make_unique<views::FillLayout>());
  views::Widget::InitParams params(views::Widget::InitParams::TYPE_POPUP);
  params.type = views::Widget::InitParams::TYPE_WINDOW_FRAMELESS;
  params.z_order = ui::ZOrderLevel::kFloatingWindow;

  const gfx::Rect visible_frame = GetVisibleFrameForPrimaryDisplay();

  const int x = visible_frame.right() - (kPopupBaseWidth + kPopupPadding);

  const int width = kPopupBaseWidth;
  const int height = kPopupBaseHeight + GetBodyHeight(notification.message());

#if defined(OS_WIN)
  const int y = visible_frame.bottom() - (height + kPopupPadding);
#elif defined(OS_LINUX)
  static const int kLinuxPopupOffsetY = 20;
  const int y = visible_frame.y() + kLinuxPopupOffsetY + kPopupPadding;
#else
  const int y = visible_frame.y() + kPopupPadding;
#endif

  params.bounds = {
    x,
    y,
    width,
    height
  };

#if defined(OS_LINUX) && !defined(OS_CHROMEOS)
  // Make the widget explicitly activatable as TYPE_POPUP is not activatable by
  // default but we need focus for the inline reply textarea.
  params.activatable = views::Widget::InitParams::ACTIVATABLE_YES;
  params.opacity = views::Widget::InitParams::WindowOpacity::kOpaque;
#else
  params.opacity = views::Widget::InitParams::WindowOpacity::kTranslucent;
#endif
  params.delegate = this;
  popup_window_ = new views::Widget();
  popup_window_->set_focus_on_creation(true);
  observer_.Add(popup_window_);

#if defined(OS_WIN)
  // We want to ensure that this toast always goes to the native desktop,
  // not the Ash desktop (since there is already another toast contents view
  // there.
  if (!params.parent)
    params.native_widget = new views::DesktopNativeWidgetAura(popup_window_);
#endif

  popup_window_->Init(std::move(params));
  popup_window_->ShowInactive();

  NotificationView* message_view_ =
      NotificationViewFactory::Create(notification);
  popup_window_->SetContentsView(message_view_);
  SetNotifyEnterExitOnChild(true);
}

MessagePopupView::~MessagePopupView() {}

#if !defined(OS_WIN) && !defined(OS_MAC)
gfx::Rect MessagePopupView::GetVisibleFrameForPrimaryDisplay() const {
  return gfx::Rect(display::Screen::GetScreen()->GetPrimaryDisplay().size());
}
#endif

#if !defined(OS_MAC)
float MessagePopupView::GetOpacity() const {
  if (!IsWidgetValid())
    return 0.f;
  return GetWidget()->GetLayer()->opacity();
}
#endif

void MessagePopupView::SetPopupBounds(const gfx::Rect& bounds) {
  if (!IsWidgetValid())
    return;
  GetWidget()->SetBounds(bounds);
}

void MessagePopupView::SetOpacity(float opacity) {
  if (!IsWidgetValid())
    return;
  GetWidget()->SetOpacity(opacity);
}

void MessagePopupView::Show() {}

void MessagePopupView::Close() {
  if (popup_window_ && !popup_window_->IsClosed()) {
    popup_window_->CloseNow();
  }
}

const char* MessagePopupView::GetClassName() const {
  return "MessagePopupView";
}

void MessagePopupView::OnFocus() {
  // This view is just a container, so advance focus to the underlying
  // NotificationView.
  GetFocusManager()->SetFocusedView(message_view_);
}

void MessagePopupView::OnWidgetActivationChanged(views::Widget* widget,
                                                 bool active) {
  is_active_ = active;
}

void MessagePopupView::OnWidgetDestroyed(views::Widget* widget) {
  observer_.Remove(widget);
}

bool MessagePopupView::IsWidgetValid() const {
  return GetWidget() && !GetWidget()->IsClosed();
}

int MessagePopupView::GetBodyHeight(const std::u16string& message) {
  return (kBodyPixelLineHeight * (message.size() / kBodyCharactersPerLine));
}

}  // namespace brave_ads
