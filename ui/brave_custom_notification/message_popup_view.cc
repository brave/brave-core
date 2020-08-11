// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "brave/ui/brave_custom_notification/message_popup_view.h"

#include "build/build_config.h"
#include "ui/accessibility/ax_enums.mojom.h"
#include "ui/accessibility/ax_node_data.h"
#include "ui/display/display.h"
// #include "chrome/browser/profiles/profile.h"
#include "ui/display/screen.h"
#include "brave/ui/brave_custom_notification/public/cpp/constants.h"
#include "brave/ui/brave_custom_notification/message_view.h"
#include "brave/ui/brave_custom_notification/message_view_factory.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/widget/widget.h"
#include "ui/views/layout/box_layout.h"
// #include "ui/views/views_delegate.h"
#include "ui/views/controls/textfield/textfield.h"
#include "ui/views/controls/label.h"

#if defined(OS_WIN)
#include "ui/views/widget/desktop_aura/desktop_native_widget_aura.h"
#endif

#if defined(OS_CHROMEOS)
#include "ui/aura/window.h"
#include "ui/aura/window_targeter.h"
#endif

namespace brave_custom_notification {
namespace {
static MessagePopupView* g_message_popup_view = nullptr;
static Notification* g_notification = nullptr;
static scoped_refptr<NotificationDelegate> delegate_ = nullptr;
}

// static
void MessagePopupView::Show(Notification& notification) {
  if (g_message_popup_view == nullptr) {
    g_message_popup_view = new MessagePopupView(notification);
  }
  g_notification = &notification;
  delegate_ = g_notification->delegate();
}

void MessagePopupView::Clicked(const std::string& notification_id) {
  if (delegate_){
    delegate_->Click(base::nullopt, base::nullopt);
    delegate_ = nullptr;
  }
  g_message_popup_view->Close();
  g_message_popup_view = nullptr;
}

// static
void MessagePopupView::ClosePopup() {
  if (delegate_) {
    delegate_->Close(true);
    delegate_ = nullptr;
  }
  if (g_message_popup_view) {
    g_message_popup_view->Close();
    g_message_popup_view = nullptr;
  }
}

MessagePopupView::MessagePopupView(const Notification& notification) {
  SetLayoutManager(std::make_unique<views::FillLayout>());
  views::Widget::InitParams params(views::Widget::InitParams::TYPE_POPUP);
  params.type = views::Widget::InitParams::TYPE_WINDOW_FRAMELESS;
  params.shadow_type = views::Widget::InitParams::ShadowType::kDrop;
  params.z_order = ui::ZOrderLevel::kFloatingWindow;
  params.bounds = { 30, 30, 300, 100 + GetBodyHeight(notification.message())};
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

#if defined(OS_CHROMEOS)
  // On Chrome OS, this widget is shown in the shelf container. It means this
  // widget would inherit the parent's window targeter (ShelfWindowTarget) by
  // default. But it is not good for popup. So we override it with the normal
  // WindowTargeter.
  gfx::NativeWindow native_window = widget->GetNativeWindow();
  native_window->SetEventTargeter(std::make_unique<aura::WindowTargeter>());
#endif

  popup_window_->ShowInactive();

  MessageView* message_view_ = MessageViewFactory::Create(notification);
  popup_window_->SetContentsView(message_view_);
//   AddChildView(message_view_);
  set_notify_enter_exit_on_child(true);
  g_message_popup_view = this;
}

MessagePopupView::~MessagePopupView() {
}

#if !defined(OS_MACOSX)
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

void MessagePopupView::Show() {
}

void MessagePopupView::Close() {
  if (!GetWidget()) {
    DeleteDelegate();
    return;
  }

  if (!GetWidget()->IsClosed())
    GetWidget()->CloseNow();
}

void MessagePopupView::GetAccessibleNodeData(ui::AXNodeData* node_data) {
  message_view_->GetAccessibleNodeData(node_data);
  node_data->role = ax::mojom::Role::kAlertDialog;
}

const char* MessagePopupView::GetClassName() const {
  return "MessagePopupView";
}

void MessagePopupView::OnFocus() {
  // This view is just a container, so advance focus to the underlying
  // MessageView.
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

int MessagePopupView::GetBodyHeight(const base::string16& message) {
  return (10 * (message.size() / 40)) + 10;
}

}  // namespace message_center
