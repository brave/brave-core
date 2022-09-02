/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/sidebar/sidebar_container_view.h"

#include <memory>
#include <utility>

#include "base/auto_reset.h"
#include "base/bind.h"
#include "brave/browser/ui/brave_browser.h"
#include "brave/browser/ui/color/brave_color_id.h"
#include "brave/browser/ui/sidebar/sidebar_controller.h"
#include "brave/browser/ui/sidebar/sidebar_model.h"
#include "brave/browser/ui/sidebar/sidebar_service_factory.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "brave/browser/ui/views/side_panel/brave_side_panel.h"
#include "brave/browser/ui/views/sidebar/sidebar_control_view.h"
#include "brave/browser/ui/views/sidebar/sidebar_side_panel_utils.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/sidebar/sidebar_item.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/color/chrome_color_id.h"
#include "chrome/browser/ui/exclusive_access/exclusive_access_manager.h"
#include "chrome/browser/ui/exclusive_access/fullscreen_controller.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/side_panel/side_panel_entry.h"
#include "chrome/browser/ui/views/toolbar/toolbar_view.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/native_web_keyboard_event.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/metadata/metadata_impl_macros.h"
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
        !host_->ShouldForceShowSidebar()) {
      host_->StopBrowserWindowEventMonitoring();
      host_->ShowSidebar(false, true);
    }
  }

 private:
  SidebarContainerView* host_ = nullptr;
};

SidebarContainerView::SidebarContainerView(
    BraveBrowser* browser,
    SidePanelCoordinator* side_panel_coordinator,
    std::unique_ptr<BraveSidePanel> side_panel)
    : browser_(browser),
      side_panel_coordinator_(side_panel_coordinator),
      browser_window_event_observer_(
          std::make_unique<BrowserWindowEventObserver>(this)) {
  SetNotifyEnterExitOnChild(true);
  side_panel_ = AddChildView(std::move(side_panel));
}

SidebarContainerView::~SidebarContainerView() = default;

void SidebarContainerView::Init() {
  initialized_ = true;

  sidebar_model_ = browser_->sidebar_controller()->model();
  sidebar_model_observation_.Observe(sidebar_model_);

  auto* browser_view = BrowserView::GetBrowserViewForBrowser(browser_);
  DCHECK(browser_view);

  auto* side_panel_registry =
      browser_view->side_panel_coordinator()->GetGlobalSidePanelRegistry();
  panel_registry_observation_.Observe(side_panel_registry);

  for (const auto& entry : side_panel_registry->entries()) {
    DVLOG(1) << "Observing panel entry in ctor: " << entry->name();
    panel_entry_observations_.AddObservation(entry.get());
  }

  show_side_panel_button_.Init(
      kShowSidePanelButton, browser_->profile()->GetPrefs(),
      base::BindRepeating(&SidebarContainerView::UpdateToolbarButtonVisibility,
                          base::Unretained(this)));

  AddChildViews();
  // Hide by default. Visibility will be controlled by show options later.
  DoHideSidebar(false);
  UpdateToolbarButtonVisibility();
}

void SidebarContainerView::SetSidebarOnLeft(bool sidebar_on_left) {
  sidebar_on_left_ = sidebar_on_left;
  if (sidebar_control_view_) {
    sidebar_control_view_->SetSidebarOnLeft(sidebar_on_left_);
  }

  DCHECK(side_panel_);
  side_panel_->SetHorizontalAlignment(sidebar_on_left
                                          ? BraveSidePanel::kAlignLeft
                                          : BraveSidePanel::kAlignRight);
}

void SidebarContainerView::SetSidebarShowOption(
    sidebar::SidebarService::ShowSidebarOption show_option) {
  UpdateSidebarVisibility(show_option);
}

void SidebarContainerView::UpdateSidebar() {
  sidebar_control_view_->Update();
}

void SidebarContainerView::MenuClosed() {
  UpdateSidebarVisibility();
}

void SidebarContainerView::UpdateBackground() {
  if (const ui::ColorProvider* color_provider = GetColorProvider()) {
    // Fill background because panel's color uses alpha value.
    SetBackground(
        views::CreateSolidBackground(color_provider->GetColor(kColorToolbar)));
  }
}

void SidebarContainerView::UpdateSidebarVisibility() {
  const auto show_option = GetSidebarService(browser_)->GetSidebarShowOption();
  UpdateSidebarVisibility(show_option);
}

void SidebarContainerView::UpdateSidebarVisibility(
    sidebar::SidebarService::ShowSidebarOption show_option) {
  // Always show, don't need to use mouse event detection.
  if (show_option == ShowSidebarOption::kShowAlways) {
    ShowSidebar(true, false);
    return;
  }
  // Never show, except if there's a UI operation in progress. Still
  // don't need to use mouse event detection.
  if (show_option == ShowSidebarOption::kShowNever) {
    ShowSidebar(ShouldForceShowSidebar(), false);
    return;
  }
  // Only show if mouse is hovered or there's a UI operation in progress.
  ShowSidebar(IsMouseHovered() || ShouldForceShowSidebar(), true);
}

void SidebarContainerView::AddChildViews() {
  // Insert to index 0 because |side_panel_| will already be at 0 but
  // we want the controls first.
  sidebar_control_view_ =
      AddChildViewAt(std::make_unique<SidebarControlView>(this, browser_), 0);
  sidebar_control_view_->SetSidebarOnLeft(sidebar_on_left_);
}

void SidebarContainerView::Layout() {
  if (!initialized_)
    return View::Layout();

  const int control_view_preferred_width =
      sidebar_control_view_->GetPreferredSize().width();

  int control_view_x = 0;
  int side_panel_x = control_view_x + control_view_preferred_width;
  if (!sidebar_on_left_) {
    control_view_x = width() - control_view_preferred_width;
    side_panel_x = 0;
  }

  sidebar_control_view_->SetBounds(control_view_x, 0,
                                   control_view_preferred_width, height());
  if (side_panel_->GetVisible()) {
    side_panel_->SetBounds(side_panel_x, 0,
                           side_panel_->GetPreferredSize().width(), height());
  }
}

gfx::Size SidebarContainerView::CalculatePreferredSize() const {
  if (!initialized_ || !sidebar_control_view_->GetVisible() ||
      IsFullscreenByTab())
    return View::CalculatePreferredSize();

  int preferred_width =
      sidebar_control_view_->GetPreferredSize().width() + GetInsets().width();
  if (side_panel_->GetVisible())
    preferred_width += side_panel_->GetPreferredSize().width();
  // height is determined by parent.
  return {preferred_width, 0};
}

void SidebarContainerView::OnThemeChanged() {
  View::OnThemeChanged();

  UpdateBackground();
}

bool SidebarContainerView::IsFullscreenByTab() const {
  DCHECK(browser_->exclusive_access_manager() &&
         browser_->exclusive_access_manager()->fullscreen_controller());
  return browser_->exclusive_access_manager()
      ->fullscreen_controller()
      ->IsWindowFullscreenForTabOrPending();
}

bool SidebarContainerView::ShouldForceShowSidebar() const {
  // Always show if panel should be visible. It is more reliable to check
  // whether the active index is a panel item rather than checking if
  // side_panel_ is visible.
  bool panel_is_active = false;
  if (auto active_index = sidebar_model_->active_index()) {
    const auto& items = sidebar_model_->GetAllSidebarItems();
    const auto& active_item = items[*active_index];
    panel_is_active = active_item.open_in_panel;
  }
  // Always show if user is reordering or a menu is visible
  return panel_is_active ||
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

  if (ShouldForceShowSidebar()) {
    StartBrowserWindowEventMonitoring();
    return;
  }

  ShowSidebar(false, true);
}

void SidebarContainerView::OnActiveIndexChanged(
    absl::optional<size_t> old_index,
    absl::optional<size_t> new_index) {
  DVLOG(1) << "OnActiveIndexChanged: "
           << (old_index ? std::to_string(*old_index) : "none") << " to "
           << (new_index ? std::to_string(*new_index) : "none");
  if (!new_index) {
    // `is_side_panel_event_` is used because `SidePanelCoordinator::Close`
    // unfortunately calls both the event handler for `OnEntryHidden` as well
    // as removing the View. Without it, we end up calling
    // `SidePanelCoordinator::Close` recursively when the event originates from
    // the SidePanelCoordinator itself (as opposed to out Sidebar buttons). This
    // would then attempt to remove the entry's panel View twice.
    // TODO(petemill): Consider reorganising the control between sidebar and
    // sidepanel so that this is clearer.
    if (!is_side_panel_event_)
      side_panel_coordinator_->Close();
    GetFocusManager()->ClearFocus();
  } else if (!is_side_panel_event_) {
    const auto& item = sidebar_model_->GetAllSidebarItems()[*new_index];
    if (item.open_in_panel) {
      // Get side panel entry information
      side_panel_coordinator_->Show(SidePanelIdFromSideBarItem(item));
    } else {
      side_panel_coordinator_->Close();
    }
  }
  InvalidateLayout();
}

void SidebarContainerView::OnItemAdded(const sidebar::SidebarItem& item,
                                       size_t index,
                                       bool user_gesture) {
  UpdateToolbarButtonVisibility();
}

void SidebarContainerView::OnItemRemoved(size_t index) {
  UpdateToolbarButtonVisibility();
}

SidebarShowOptionsEventDetectWidget*
SidebarContainerView::GetEventDetectWidget() {
  if (!show_options_widget_) {
    show_options_widget_ =
        std::make_unique<SidebarShowOptionsEventDetectWidget>(
            static_cast<BraveBrowserView*>(
                BrowserView::GetBrowserViewForBrowser(browser_)),
            this);
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

void SidebarContainerView::UpdateToolbarButtonVisibility() {
  // Coordinate sidebar toolbar button visibility based on
  // whether there are any sibar items with a sidepanel.
  // This is similar to how chromium's side_panel_coordinator View
  // also has some control on the toolbar button.
  auto has_panel_item =
      GetSidebarService(browser_)->GetDefaultPanelItem().has_value();
  auto* browser_view = BrowserView::GetBrowserViewForBrowser(browser_);
  browser_view->toolbar()->side_panel_button()->SetVisible(
      has_panel_item && show_side_panel_button_.GetValue());
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

void SidebarContainerView::OnEntryShown(SidePanelEntry* entry) {
  base::AutoReset auto_reset(&is_side_panel_event_, true);

  // Make sure item is selected. We need to observe the SidePanel system
  // as well as Sidebar as there are other ways than Sidebar for SidePanel
  // items to be shown and hidden, e.g. toolbar button.
  DVLOG(1) << "Panel shown: " << entry->name();
  for (const auto& item : sidebar_model_->GetAllSidebarItems()) {
    if (!item.open_in_panel) {
      continue;
    }
    if (entry->key().id() == sidebar::SidePanelIdFromSideBarItem(item)) {
      auto side_bar_index = sidebar_model_->GetIndexOf(item);
      auto* controller = browser_->sidebar_controller();
      controller->ActivateItemAt(side_bar_index);
      break;
    }
  }
}

void SidebarContainerView::OnEntryHidden(SidePanelEntry* entry) {
  base::AutoReset auto_reset(&is_side_panel_event_, true);
  // Make sure item is deselected
  DVLOG(1) << "Panel hidden: " << entry->name();
  for (const auto& item : sidebar_model_->GetAllSidebarItems()) {
    if (!item.open_in_panel) {
      continue;
    }
    if (entry->key().id() == sidebar::SidePanelIdFromSideBarItem(item)) {
      auto side_bar_index = sidebar_model_->GetIndexOf(item);
      auto* controller = browser_->sidebar_controller();
      if (controller->IsActiveIndex(side_bar_index)) {
        controller->ActivateItemAt(absl::nullopt);
      }
      break;
    }
  }
}

void SidebarContainerView::OnEntryRegistered(SidePanelEntry* entry) {
  // Observe when it's shown or hidden
  DVLOG(1) << "Observing panel entry in registry observer: " << entry->name();
  panel_entry_observations_.AddObservation(entry);
}

void SidebarContainerView::OnEntryWillDeregister(SidePanelEntry* entry) {
  // Stop observing
  DVLOG(1) << "Unobserving panel entry in registry observer: " << entry->name();
  panel_entry_observations_.RemoveObservation(entry);
}

BEGIN_METADATA(SidebarContainerView, views::View)
END_METADATA
