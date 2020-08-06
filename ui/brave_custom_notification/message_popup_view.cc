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
constexpr gfx::Size kSmallContainerSize(328, 50);

namespace {
static MessagePopupView* g_message_popup_view = nullptr;
static Notification* g_notification = nullptr;
}

// static
void MessagePopupView::Show(Notification& notification) {
  if (g_message_popup_view == nullptr) {
    g_message_popup_view = new MessagePopupView(notification);
  }
//  g_notification = &notification;
  
  NotificationDelegate* delegate = g_notification->delegate();
  if (delegate){
    LOG(INFO) << "albert delegate found for MPV::Show";
  }
}

void MessagePopupView::Clicked(const std::string& notification_id) {
  LOG(INFO) << "albert MPV::Clicked";
  NotificationDelegate* delegate = g_notification->delegate();
  if (delegate){
    LOG(INFO) << "albert delegate found for MPV::Clicked";
    delegate->Click(base::nullopt, base::nullopt);
  }
}

// static
void MessagePopupView::ClosePopup() {
  if (g_message_popup_view) {
    g_message_popup_view->Close();
    g_message_popup_view = nullptr;
  }
}

// Todo: Albert need to fix this error before we can compile
//
//In file included from ../../brave/ui/brave_custom_notification/message_popup_view.cc:5:
// In file included from ../../brave/ui/brave_custom_notification/message_popup_view.h:11:
// In file included from ../../chrome/browser/profiles/profile.h:19:
// ../../content/public/browser/browser_context.h:29:10: fatal error: 'third_party/blink/public/mojom/push_messaging/push_messaging_status.mojom-forward.h' file not found
//
// MessagePopupView::MessagePopupView(Profile* profile) {
MessagePopupView::MessagePopupView() {
  views::Widget* window = new views::Widget();
  views::Widget::InitParams window_params;
  window_params.ownership = views::Widget::InitParams::WIDGET_OWNS_NATIVE_WIDGET;
  window_params.bounds = { 0, 0, 1500, 1000 };
  views::View* container = new views::View();
 // views::View* wv_container = new views::View();
  container->SetLayoutManager(std::make_unique<views::BoxLayout>(views::BoxLayout::Orientation::kVertical, gfx::Insets(), 0));
  container->SetSize(kSmallContainerSize);
  views::Label* tv = new views::Label(base::ASCIIToUTF16("toplevel"));
  tv->SetBackgroundColor(SkColorSetRGB(0xf5, 0xf5, 0xf5));
  container->AddChildView(tv);

  /*
   wv_container->SetLayoutManager(std::make_unique<views::FillLayout>());
  wv = views::ViewsDelegate::GetInstance()->GetWebViewForWindow();
  wv_container->SetSize(kContainerSize);
  wv_container->SetPreferredSize(kContainerSize);
  wv_container->SizeToPreferredSize();
  wv_container->AddChildView(wv);
  */
  // views::Label* tv2 = new views::Label(base::ASCIIToUTF16("bottomlevel"));
//  tv2->SetBackgroundColor(kBackground);
  window_params.type = views::Widget::InitParams::TYPE_WINDOW_FRAMELESS;
  window_params.opacity = views::Widget::InitParams::WindowOpacity::kOpaque;
  window_params.shadow_type = views::Widget::InitParams::ShadowType::kDrop;
  window->Init(std::move(window_params));
  window->CenterWindow(window_params.bounds.size());
  window->Show();
  window->SetContentsView(container);
  views::Widget* child = new views::Widget;
  views::Widget::InitParams child_params(views::Widget::InitParams::TYPE_POPUP);
  child_params.ownership = views::Widget::InitParams::WIDGET_OWNS_NATIVE_WIDGET;
  child_params.opacity = views::Widget::InitParams::WindowOpacity::kOpaque;
  child_params.bounds = { 1000, 500, 200, 200 };
  child_params.parent = window->GetNativeWindow();
  child->Init(std::move(child_params));
  child->Show();
  // child->SetContentsView(wv_container);
  g_message_popup_view = this;
}

// Worries about bounds. 
MessagePopupView::MessagePopupView(const Notification& notification)
    : message_view_(MessageViewFactory::Create(notification)) {
  SetLayoutManager(std::make_unique<views::FillLayout>());
  views::Widget::InitParams params(views::Widget::InitParams::TYPE_POPUP);
  params.z_order = ui::ZOrderLevel::kFloatingWindow;
  params.bounds = { 30, 30, 400, 200 };
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

  AddChildView(message_view_);
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

}  // namespace message_center
