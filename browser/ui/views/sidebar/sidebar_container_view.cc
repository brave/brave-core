/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/sidebar/sidebar_container_view.h"

#include <utility>

#include "base/bind.h"
#include "brave/browser/themes/theme_properties.h"
#include "brave/browser/ui/brave_browser.h"
#include "brave/browser/ui/sidebar/sidebar_controller.h"
#include "brave/browser/ui/sidebar/sidebar_model.h"
#include "brave/browser/ui/sidebar/sidebar_model_data.h"
#include "brave/browser/ui/sidebar/sidebar_service_factory.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "brave/browser/ui/views/sidebar/sidebar_control_view.h"
#include "brave/browser/ui/views/sidebar/sidebar_panel_webview.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/themes/theme_properties.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/native_web_keyboard_event.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/base/theme_provider.h"
#include "ui/events/event_observer.h"
#include "ui/events/types/event_type.h"
#include "ui/gfx/geometry/point.h"
#include "ui/views/border.h"
#include "ui/views/event_monitor.h"
#include "ui/views/widget/widget.h"
#include "url/gurl.h"

namespace {

using ShowSidebarOption = sidebar::SidebarService::ShowSidebarOption;

sidebar::SidebarService* GetSidebarService(BraveBrowser* browser) {
  return sidebar::SidebarServiceFactory::GetForProfile(browser->profile());
}

size_t GetPreferredPanelWidthForCurrentItem(BraveBrowser* browser) {
  auto* controller = browser->sidebar_controller();
  int active_index = controller->model()->active_index();
  const auto item = GetSidebarService(browser)->items()[active_index];
  // Shortcut type doesn't use panel.
  if (!item.open_in_panel)
    return 0;

  constexpr size_t kPanelWidthForBuiltIn = 260;
  constexpr size_t kPanelWidthForNonBuiltIn = 360;
  return IsBuiltInType(item) ? kPanelWidthForBuiltIn : kPanelWidthForNonBuiltIn;
}

}  // namespace

class SidebarContainerView::BrowserWindowEventObserver
    : public ui::EventObserver {
 public:
  explicit BrowserWindowEventObserver(SidebarContainerView* host)
      : host_(host) {}
  ~BrowserWindowEventObserver() override = default;
  BrowserWindowEventObserver(const BrowserWindowEventObserver&) = delete;
  BrowserWindowEventObserver& operator=(const BrowserWindowEventObserver&) =
      delete;

  void OnEvent(const ui::Event& event) override {
    DCHECK(event.IsMouseEvent());
    const auto* mouse_event = event.AsMouseEvent();

    gfx::Point window_event_position = mouse_event->location();
    // Convert window position to sidebar view's coordinate and check whether
    // it's included in sidebar ui or not.
    // If it's not included and sidebar could be hidden, stop monitoring and
    // hide UI.
    views::View::ConvertPointFromWidget(host_->sidebar_control_view_,
                                        &window_event_position);
    if (!host_->sidebar_control_view_->GetLocalBounds().Contains(
            window_event_position) &&
        !host_->ShouldShowSidebar()) {
      host_->StopBrowserWindowEventMonitoring();
      host_->ShowSidebar(false, true);
    }
  }

 private:
  SidebarContainerView* host_ = nullptr;
};

SidebarContainerView::SidebarContainerView(BraveBrowser* browser)
    : browser_(browser),
      browser_window_event_observer_(
          std::make_unique<BrowserWindowEventObserver>(this)) {
  SetNotifyEnterExitOnChild(true);
}

SidebarContainerView::~SidebarContainerView() = default;

void SidebarContainerView::Init() {
  initialized_ = true;

  sidebar_model_ = browser_->sidebar_controller()->model();
  observed_.Observe(sidebar_model_);

  AddChildViews();
  // Hide by default. Visibility will be controlled by show options later.
  DoHideSidebar(false);
}

void SidebarContainerView::SetSidebarShowOption(
    sidebar::SidebarService::ShowSidebarOption show_option) {
  if (show_option == ShowSidebarOption::kShowAlways) {
    ShowSidebar(true, false);
    return;
  }

  if (show_option == ShowSidebarOption::kShowNever) {
    ShowSidebar(false, false);
    return;
  }

  ShowSidebar(false, true);
}

void SidebarContainerView::UpdateSidebar() {
  sidebar_control_view_->Update();
}

void SidebarContainerView::ShowCustomContextMenu(
    const gfx::Point& point,
    std::unique_ptr<ui::MenuModel> menu_model) {
  if (!sidebar_panel_webview_->GetVisible()) {
    LOG(ERROR) << __func__
               << " sidebar panel UI is loaded at non sidebar panel!";
    return;
  }

  sidebar_panel_webview_->ShowCustomContextMenu(point, std::move(menu_model));
}

void SidebarContainerView::HideCustomContextMenu() {
  if (!sidebar_panel_webview_->GetVisible()) {
    LOG(ERROR) << __func__
               << " sidebar panel UI is loaded at non sidebar panel!";
    return;
  }

  sidebar_panel_webview_->HideCustomContextMenu();
}

bool SidebarContainerView::HandleKeyboardEvent(
    content::WebContents* source,
    const content::NativeWebKeyboardEvent& event) {
  DCHECK(sidebar_panel_webview_->GetVisible());
  return sidebar_panel_webview_->TreatUnHandledKeyboardEvent(source, event);
}

void SidebarContainerView::UpdateBackgroundAndBorder() {
  if (const ui::ThemeProvider* theme_provider = GetThemeProvider()) {
    constexpr int kBorderThickness = 1;
    // Fill background because panel's color uses alpha value.
    SetBackground(views::CreateSolidBackground(
        theme_provider->GetColor(ThemeProperties::COLOR_TOOLBAR)));
    if (sidebar_panel_webview_ && sidebar_panel_webview_->GetVisible()) {
      SetBorder(views::CreateSolidSidedBorder(
          gfx::Insets::TLBR(0, 0, 0, kBorderThickness),
          theme_provider->GetColor(
              ThemeProperties::COLOR_TOOLBAR_CONTENT_AREA_SEPARATOR)));
    } else {
      // Don't need right side border when panel is closed.
      SetBorder(nullptr);
    }
  }
}

void SidebarContainerView::AddChildViews() {
  sidebar_control_view_ =
      AddChildView(std::make_unique<SidebarControlView>(browser_));
  sidebar_panel_webview_ =
      AddChildView(std::make_unique<SidebarPanelWebView>(browser_->profile()));
}

void SidebarContainerView::Layout() {
  if (!initialized_)
    return View::Layout();

  const int control_view_preferred_width =
      sidebar_control_view_->GetPreferredSize().width();
  sidebar_control_view_->SetBounds(0, 0, control_view_preferred_width,
                                   height());
  if (sidebar_panel_webview_->GetVisible()) {
    sidebar_panel_webview_->SetBounds(
        control_view_preferred_width, 0,
        GetPreferredPanelWidthForCurrentItem(browser_), height());
  }
}

gfx::Size SidebarContainerView::CalculatePreferredSize() const {
  if (!initialized_ || !sidebar_control_view_->GetVisible() ||
      browser_->window()->IsFullscreen())
    return View::CalculatePreferredSize();

  int preferred_width =
      sidebar_control_view_->GetPreferredSize().width() + GetInsets().width();
  if (sidebar_panel_webview_->GetVisible())
    preferred_width += GetPreferredPanelWidthForCurrentItem(browser_);
  // height is determined by parent.
  return {preferred_width, 0};
}

void SidebarContainerView::OnThemeChanged() {
  View::OnThemeChanged();

  UpdateBackgroundAndBorder();
}

bool SidebarContainerView::ShouldShowSidebar() const {
  return sidebar_panel_webview_->GetVisible() ||
         sidebar_control_view_->IsItemReorderingInProgress() ||
         sidebar_control_view_->IsBubbleWidgetVisible();
}

void SidebarContainerView::OnMouseEntered(const ui::MouseEvent& event) {
  const auto show_option = GetSidebarService(browser_)->GetSidebarShowOption();
  const bool autohide_sidebar =
      show_option == ShowSidebarOption::kShowOnMouseOver;

  // When user select to non-autohide option like "Never" option,
  // hide timer is scheduled but this view can get mouse event when context
  // menu is hidden. In this case, this should not be cancelled.
  if (!autohide_sidebar)
    return;

  // Cancel hide schedule when mouse entered again quickly.
  sidebar_hide_timer_.Stop();
}

void SidebarContainerView::OnMouseExited(const ui::MouseEvent& event) {
  // When context menu is shown, this view can get this exited callback.
  // In that case, ignore this callback because mouse is still in this view.
  if (IsMouseHovered())
    return;

  const auto show_option = GetSidebarService(browser_)->GetSidebarShowOption();
  const bool autohide_sidebar =
      show_option == ShowSidebarOption::kShowOnMouseOver;

  if (!autohide_sidebar)
    return;

  if (ShouldShowSidebar()) {
    StartBrowserWindowEventMonitoring();
    return;
  }

  ShowSidebar(false, true);
}

void SidebarContainerView::OnActiveIndexChanged(int old_index, int new_index) {
  if (new_index == -1) {
    sidebar_panel_webview_->SetVisible(false);
    GetFocusManager()->ClearFocus();
  } else {
    const auto item = sidebar_model_->GetAllSidebarItems()[new_index];
    if (item.open_in_panel) {
      sidebar_panel_webview_->SetWebContents(
          sidebar_model_->GetWebContentsAt(new_index));
      sidebar_panel_webview_->SetVisible(true);
      // When panel is opened, it will get focused.
      sidebar_panel_webview_->RequestFocus();
    } else {
      sidebar_panel_webview_->SetVisible(false);
      GetFocusManager()->ClearFocus();
    }
  }
  UpdateBackgroundAndBorder();
  InvalidateLayout();
}

SidebarShowOptionsEventDetectWidget*
SidebarContainerView::GetEventDetectWidget() {
  if (!show_options_widget_) {
    show_options_widget_.reset(new SidebarShowOptionsEventDetectWidget(
        static_cast<BraveBrowserView*>(
            BrowserView::GetBrowserViewForBrowser(browser_)),
        this));
  }

  return show_options_widget_.get();
}

void SidebarContainerView::ShowOptionsEventDetectWidget(bool show) {
  show ? GetEventDetectWidget()->Show() : GetEventDetectWidget()->Hide();
}

void SidebarContainerView::ShowSidebar() {
  ShowSidebar(true, false);
}

void SidebarContainerView::ShowSidebar(bool show_sidebar,
                                       bool show_event_detect_widget) {
  sidebar_hide_timer_.Stop();

  if (show_sidebar) {
    // Show immediately.
    sidebar_control_view_->SetVisible(true);
    ShowOptionsEventDetectWidget(show_event_detect_widget);
    InvalidateLayout();
  } else {
    constexpr int kHideDelayInMS = 400;
    sidebar_hide_timer_.Start(
        FROM_HERE, base::Milliseconds(kHideDelayInMS),
        base::BindOnce(&SidebarContainerView::DoHideSidebar,
                       base::Unretained(this), show_event_detect_widget));
  }
}

void SidebarContainerView::DoHideSidebar(bool show_event_detect_widget) {
  sidebar_control_view_->SetVisible(false);
  ShowOptionsEventDetectWidget(show_event_detect_widget);
  InvalidateLayout();
}

void SidebarContainerView::StartBrowserWindowEventMonitoring() {
  if (browser_window_event_monitor_)
    return;

  browser_window_event_monitor_ = views::EventMonitor::CreateWindowMonitor(
      browser_window_event_observer_.get(), GetWidget()->GetNativeWindow(),
      {ui::ET_MOUSE_MOVED});
}

void SidebarContainerView::StopBrowserWindowEventMonitoring() {
  browser_window_event_monitor_.reset();
}

BEGIN_METADATA(SidebarContainerView, views::View)
END_METADATA
