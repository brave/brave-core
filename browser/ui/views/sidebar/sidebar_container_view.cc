/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/sidebar/sidebar_container_view.h"

#include "base/bind.h"
#include "brave/browser/themes/theme_properties.h"
#include "brave/browser/ui/brave_browser.h"
#include "brave/browser/ui/sidebar/sidebar_controller.h"
#include "brave/browser/ui/sidebar/sidebar_model.h"
#include "brave/browser/ui/sidebar/sidebar_model_data.h"
#include "brave/browser/ui/sidebar/sidebar_service_factory.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "brave/browser/ui/views/sidebar/sidebar_control_view.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "content/public/browser/browser_context.h"
#include "ui/base/theme_provider.h"
#include "ui/events/event_observer.h"
#include "ui/events/types/event_type.h"
#include "ui/gfx/geometry/point.h"
#include "ui/views/border.h"
#include "ui/views/controls/webview/webview.h"
#include "ui/views/event_monitor.h"
#include "ui/views/widget/widget.h"
#include "url/gurl.h"

namespace {

using ShowSidebarOption = sidebar::SidebarService::ShowSidebarOption;

sidebar::SidebarService* GetSidebarService(BraveBrowser* browser) {
  return sidebar::SidebarServiceFactory::GetForProfile(browser->profile());
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
  UpdateChildViewVisibility();
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

void SidebarContainerView::UpdateBackgroundAndBorder() {
  if (const ui::ThemeProvider* theme_provider = GetThemeProvider()) {
    constexpr int kBorderThickness = 1;
    SetBorder(views::CreateSolidSidedBorder(
        0, 0, 0, kBorderThickness,
        theme_provider->GetColor(
            BraveThemeProperties::COLOR_SIDEBAR_PANEL_BORDER)));
  }
}

void SidebarContainerView::AddChildViews() {
  sidebar_control_view_ =
      AddChildView(std::make_unique<SidebarControlView>(browser_));
  sidebar_panel_view_ =
      AddChildView(std::make_unique<views::WebView>(browser_->profile()));
  // |sidebar_panel_view_| will be visible when user opens sidebar panel.
  sidebar_panel_view_->SetVisible(false);
}

void SidebarContainerView::UpdateChildViewVisibility() {}

void SidebarContainerView::Layout() {
  if (!initialized_)
    return View::Layout();

  const int control_view_preferred_width =
      sidebar_control_view_->GetPreferredSize().width();
  sidebar_control_view_->SetBounds(0, 0, control_view_preferred_width,
                                   height());
  if (sidebar_panel_view_->GetVisible()) {
    sidebar_panel_view_->SetBounds(
        control_view_preferred_width, 0,
        width() - control_view_preferred_width - GetInsets().width(), height());
  }
}

gfx::Size SidebarContainerView::CalculatePreferredSize() const {
  if (!initialized_ || !sidebar_control_view_->GetVisible() ||
      browser_->window()->IsFullscreen())
    return View::CalculatePreferredSize();

  const int control_view_preferred_width =
      sidebar_control_view_->GetPreferredSize().width();

  constexpr int kSidebarContainerViewFullWidth = 380;
  return {sidebar_panel_view_->GetVisible() ? kSidebarContainerViewFullWidth
                                            : control_view_preferred_width,
          0};
}

void SidebarContainerView::OnThemeChanged() {
  View::OnThemeChanged();

  UpdateBackgroundAndBorder();
}

bool SidebarContainerView::ShouldShowSidebar() const {
  return sidebar_panel_view_->GetVisible() ||
         sidebar_control_view_->IsItemReorderingInProgress() ||
         sidebar_control_view_->IsBubbleWidgetVisible();
}

void SidebarContainerView::OnMouseEntered(const ui::MouseEvent& event) {
  const auto show_option = GetSidebarService(browser_)->GetSidebarShowOption();
  const bool autohide_sidebar =
      show_option == ShowSidebarOption::kShowOnMouseOver ||
      show_option == ShowSidebarOption::kShowOnClick;

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
      show_option == ShowSidebarOption::kShowOnMouseOver ||
      show_option == ShowSidebarOption::kShowOnClick;

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
    sidebar_panel_view_->SetVisible(false);
  } else {
    const auto item = sidebar_model_->GetAllSidebarItems()[new_index];
    if (item.open_in_panel) {
      sidebar_panel_view_->SetWebContents(
          sidebar_model_->GetWebContentsAt(new_index));
      sidebar_panel_view_->SetVisible(true);
    } else {
      sidebar_panel_view_->SetVisible(false);
    }
  }

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

bool SidebarContainerView::ShouldShowOnHover() {
  const auto show_option = GetSidebarService(browser_)->GetSidebarShowOption();
  return show_option == ShowSidebarOption::kShowOnMouseOver;
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
        FROM_HERE, base::TimeDelta::FromMilliseconds(kHideDelayInMS),
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
