/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/brave_browser_view.h"

#include <algorithm>
#include <iterator>
#include <map>
#include <memory>
#include <optional>
#include <vector>

#include "base/check.h"
#include "base/containers/contains.h"
#include "base/feature_list.h"
#include "base/functional/bind.h"
#include "base/task/sequenced_task_runner.h"
#include "brave/browser/brave_browser_features.h"
#include "brave/browser/sparkle_buildflags.h"
#include "brave/browser/translate/brave_translate_utils.h"
#include "brave/browser/ui/brave_browser.h"
#include "brave/browser/ui/color/brave_color_id.h"
#include "brave/browser/ui/commands/accelerator_service.h"
#include "brave/browser/ui/commands/accelerator_service_factory.h"
#include "brave/browser/ui/page_action/brave_page_action_icon_type.h"
#include "brave/browser/ui/sidebar/sidebar_utils.h"
#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "brave/browser/ui/tabs/features.h"
#include "brave/browser/ui/views/brave_actions/brave_actions_container.h"
#include "brave/browser/ui/views/brave_actions/brave_shields_action_view.h"
#include "brave/browser/ui/views/brave_help_bubble/brave_help_bubble_host_view.h"
#include "brave/browser/ui/views/frame/brave_contents_layout_manager.h"
#include "brave/browser/ui/views/frame/brave_contents_view_util.h"
#include "brave/browser/ui/views/frame/split_view/brave_contents_container_view.h"
#include "brave/browser/ui/views/frame/split_view/brave_multi_contents_view.h"
#include "brave/browser/ui/views/frame/vertical_tabs/vertical_tab_strip_region_view.h"
#include "brave/browser/ui/views/frame/vertical_tabs/vertical_tab_strip_widget_delegate_view.h"
#include "brave/browser/ui/views/location_bar/brave_location_bar_view.h"
#include "brave/browser/ui/views/omnibox/brave_omnibox_view_views.h"
#include "brave/browser/ui/views/sidebar/sidebar_container_view.h"
#include "brave/browser/ui/views/tabs/vertical_tab_utils.h"
#include "brave/browser/ui/views/toolbar/bookmark_button.h"
#include "brave/browser/ui/views/toolbar/brave_toolbar_view.h"
#include "brave/browser/ui/views/toolbar/wallet_button.h"
#include "brave/browser/ui/views/window_closing_confirm_dialog_view.h"
#include "brave/common/pref_names.h"
#include "brave/components/commands/common/features.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/sidebar/common/features.h"
#include "brave/components/speedreader/common/buildflags/buildflags.h"
#include "brave/ui/color/nala/nala_color_id.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/app_mode/app_mode_utils.h"
#include "chrome/browser/devtools/devtools_ui_controller.h"
#include "chrome/browser/devtools/devtools_window.h"
#include "chrome/browser/enterprise/watermark/watermark_view.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_element_identifiers.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/color/chrome_color_id.h"
#include "chrome/browser/ui/exclusive_access/exclusive_access_manager.h"
#include "chrome/browser/ui/exclusive_access/fullscreen_controller.h"
#include "chrome/browser/ui/frame/window_frame_util.h"
#include "chrome/browser/ui/ui_features.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/frame/browser_widget.h"
#include "chrome/browser/ui/views/frame/contents_layout_manager.h"
#include "chrome/browser/ui/views/frame/contents_web_view.h"
#include "chrome/browser/ui/views/frame/multi_contents_view.h"
#include "chrome/browser/ui/views/frame/tab_strip_region_view.h"
#include "chrome/browser/ui/views/frame/top_container_view.h"
#include "chrome/browser/ui/views/interaction/browser_elements_views.h"
#include "chrome/browser/ui/views/side_panel/side_panel_coordinator.h"
#include "chrome/browser/ui/views/tabs/tab_search_button.h"
#include "chrome/browser/ui/views/tabs/tab_strip.h"
#include "chrome/browser/ui/views/toolbar/browser_app_menu_button.h"
#include "chrome/common/pref_names.h"
#include "components/javascript_dialogs/tab_modal_dialog_manager.h"
#include "components/permissions/permission_request_manager.h"
#include "components/web_modal/web_contents_modal_dialog_manager.h"
#include "content/public/browser/page_navigator.h"
#include "content/public/browser/web_contents.h"
#include "extensions/buildflags/buildflags.h"
#include "third_party/blink/public/common/input/web_mouse_event.h"
#include "ui/base/accelerators/accelerator.h"
#include "ui/base/accelerators/accelerator_manager.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/compositor/layer.h"
#include "ui/events/event.h"
#include "ui/events/event_observer.h"
#include "ui/gfx/geometry/rounded_corners_f.h"
#include "ui/views/border.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"
#include "ui/views/controls/native/native_view_host.h"
#include "ui/views/event_monitor.h"
#include "ui/views/layout/fill_layout.h"

#if BUILDFLAG(ENABLE_BRAVE_VPN)
#include "brave/browser/ui/views/toolbar/brave_vpn_button.h"
#include "brave/components/brave_vpn/common/pref_names.h"
#endif

#if BUILDFLAG(ENABLE_SPARKLE)
#include "brave/browser/ui/views/update_recommended_message_box_mac.h"
#endif

#if BUILDFLAG(ENABLE_SPEEDREADER)
#include "brave/browser/ui/speedreader/speedreader_tab_helper.h"
#include "brave/browser/ui/views/speedreader/reader_mode_bubble.h"
#include "brave/browser/ui/views/speedreader/reader_mode_toolbar_view.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_WAYBACK_MACHINE)
#include "brave/browser/ui/views/wayback_machine_bubble_view.h"
#endif

namespace {

std::optional<bool> g_download_confirm_return_allow_for_testing;

bool IsUnsupportedCommand(int command_id, Browser* browser) {
  return IsRunningInForcedAppMode() &&
         !IsCommandAllowedInAppMode(command_id, browser->is_type_popup());
}

// A control separator that is displayed when the sidebar is displayed adjacent
// to the tabstrip in vertical tabs mode.
class SidebarSeparator : public views::View {
  METADATA_HEADER(SidebarSeparator, views::View)
 public:
  SidebarSeparator() {
    SetBackground(
        views::CreateSolidBackground(kColorBraveVerticalTabSeparator));
  }
};
BEGIN_METADATA(SidebarSeparator)
END_METADATA

// A view that paints a background under the content area of the browser view so
// that the web content area can be displayed with rounded corners and a shadow.
class ContentsBackground : public views::View {
  METADATA_HEADER(ContentsBackground, views::View)
 public:
  ContentsBackground() {
    SetBackground(views::CreateSolidBackground(kColorToolbar));
    SetEnabled(false);

    // Prevent to eat any events that goes to web contents because web contents
    // could be behind this background.
    SetCanProcessEventsWithinSubtree(false);
  }
};
BEGIN_METADATA(ContentsBackground)
END_METADATA

}  // namespace

// static
void BraveBrowserView::SetDownloadConfirmReturnForTesting(bool allow) {
  g_download_confirm_return_allow_for_testing = allow;
}

class BraveBrowserView::TabCyclingEventHandler : public ui::EventObserver,
                                                 public views::WidgetObserver {
 public:
  explicit TabCyclingEventHandler(BraveBrowserView* browser_view)
      : browser_view_(browser_view) {
    Start();
  }

  ~TabCyclingEventHandler() override { Stop(); }

  TabCyclingEventHandler(const TabCyclingEventHandler&) = delete;
  TabCyclingEventHandler& operator=(const TabCyclingEventHandler&) = delete;

 private:
  // ui::EventObserver overrides:
  void OnEvent(const ui::Event& event) override {
    if (event.type() == ui::EventType::kKeyReleased &&
        event.AsKeyEvent()->key_code() == ui::VKEY_CONTROL) {
      // Ctrl key was released, stop the tab cycling
      Stop();
      return;
    }

    if (event.type() == ui::EventType::kMousePressed) {
      Stop();
    }
  }

  // views::WidgetObserver overrides:
  void OnWidgetActivationChanged(views::Widget* widget, bool active) override {
    // We should stop cycling if other application gets active state.
    if (!active) {
      Stop();
    }
  }

  // Handle Browser widget closing while tab Cycling is in-progress.
  void OnWidgetClosing(views::Widget* widget) override { Stop(); }

  void Start() {
    // Add the event handler
    auto* widget = browser_view_->GetWidget();
    if (widget->GetNativeWindow()) {
      monitor_ = views::EventMonitor::CreateWindowMonitor(
          this, widget->GetNativeWindow(),
          {ui::EventType::kMousePressed, ui::EventType::kKeyReleased});
    }

    widget->AddObserver(this);
  }

  void Stop() {
    if (!monitor_.get()) {
      // We already stopped
      return;
    }

    // Remove event handler
    auto* widget = browser_view_->GetWidget();
    monitor_.reset();
    widget->RemoveObserver(this);
    browser_view_->StopTabCycling();
  }

  raw_ptr<BraveBrowserView> browser_view_ = nullptr;
  std::unique_ptr<views::EventMonitor> monitor_;
};

// static
BraveBrowserView* BraveBrowserView::From(BrowserView* view) {
  return static_cast<BraveBrowserView*>(view);
}

BraveBrowserView::BraveBrowserView(Browser* browser) : BrowserView(browser) {
#if BUILDFLAG(ENABLE_SPEEDREADER)
  // When SideBySide is enabled, each ContentsContainerView in MultiContentsView
  // own ReaderModeToolbarView.
  if (!base::FeatureList::IsEnabled(features::kSideBySide)) {
    reader_mode_toolbar_ = contents_container_->AddChildView(
        std::make_unique<ReaderModeToolbarView>(browser_->profile()));
    contents_container_->SetLayoutManager(
        std::make_unique<BraveContentsLayoutManager>(
            contents_container_view_, lens_overlay_view_, reader_mode_toolbar_,
            /*scrim_view=*/nullptr));
  }
#endif

  // Need this background view always as we have contents margin/rounded corners
  // when split view is active regardless of rounded corners feature.
  contents_background_view_ =
      AddChildViewAt(std::make_unique<ContentsBackground>(), 0);

  pref_change_registrar_.Init(GetProfile()->GetPrefs());
  pref_change_registrar_.Add(
      kTabsSearchShow,
      base::BindRepeating(&BraveBrowserView::OnPreferenceChanged,
                          base::Unretained(this)));
  // Show the correct value in settings on initial start
  UpdateSearchTabsButtonState();

  pref_change_registrar_.Add(
      kWebViewRoundedCorners,
      base::BindRepeating(&BraveBrowserView::OnPreferenceChanged,
                          base::Unretained(this)));

#if BUILDFLAG(ENABLE_BRAVE_VPN)
  pref_change_registrar_.Add(
      brave_vpn::prefs::kBraveVPNShowButton,
      base::BindRepeating(&BraveBrowserView::OnPreferenceChanged,
                          base::Unretained(this)));
#endif

  // Only normal window (tabbed) should have sidebar.
  const bool can_have_sidebar = sidebar::CanUseSidebar(browser_);
  if (can_have_sidebar) {
    // Wrap chromium side panel with our sidebar container
    auto original_side_panel =
        main_container_->RemoveChildViewT(contents_height_side_panel_.get());
    sidebar_container_view_ =
        main_container_->AddChildView(std::make_unique<SidebarContainerView>(
            browser_, browser_->GetFeatures().side_panel_coordinator(),
            std::move(original_side_panel)));
    contents_height_side_panel_ = sidebar_container_view_->side_panel();

    if (BraveBrowser::IsBraveWebViewRoundedCornersEnabled(browser_)) {
      sidebar_separator_view_ =
          main_container_->AddChildView(std::make_unique<SidebarSeparator>());
    }

#if defined(USE_AURA)
    sidebar_host_view_ = AddChildView(std::make_unique<views::View>());
#endif

    pref_change_registrar_.Add(
        prefs::kSidePanelHorizontalAlignment,
        base::BindRepeating(&BraveBrowserView::OnPreferenceChanged,
                            base::Unretained(this)));
  }

  const bool supports_vertical_tabs =
      tabs::utils::SupportsVerticalTabs(browser_);
  if (supports_vertical_tabs) {
    vertical_tab_strip_host_view_ =
        AddChildView(std::make_unique<views::View>());
  }

  if (!supports_vertical_tabs && !can_have_sidebar) {
    return;
  }

  // Make sure |find_bar_host_view_| is the last child of BrowserView by
  // re-ordering. FindBarHost widgets uses this view as a  kHostViewKey.
  // See the comments of BrowserView::find_bar_host_view().
  ReorderChildView(find_bar_host_view_, -1);
}

void BraveBrowserView::OnPreferenceChanged(const std::string& pref_name) {
  if (pref_name == kTabsSearchShow) {
    UpdateSearchTabsButtonState();
    return;
  }

  if (pref_name == kWebViewRoundedCorners) {
    UpdateRoundedCornersUI();
    return;
  }

  if (pref_name == prefs::kSidePanelHorizontalAlignment) {
    UpdateSideBarHorizontalAlignment();
    return;
  }

#if BUILDFLAG(ENABLE_BRAVE_VPN)
  if (pref_name == brave_vpn::prefs::kBraveVPNShowButton) {
    vpn_panel_controller_.ResetBubbleManager();
    return;
  }
#endif
}

void BraveBrowserView::UpdateSideBarHorizontalAlignment() {
  DCHECK(sidebar_container_view_);

  const bool on_left = !GetProfile()->GetPrefs()->GetBoolean(
      prefs::kSidePanelHorizontalAlignment);

  sidebar_container_view_->SetSidebarOnLeft(on_left);

  if (multi_contents_view_ &&
      base::FeatureList::IsEnabled(sidebar::features::kSidebarWebPanel)) {
    GetBraveMultiContentsView()->SetWebPanelOnLeft(on_left);
  }

  DeprecatedLayoutImmediately();
}

void BraveBrowserView::UpdateSearchTabsButtonState() {
  const bool is_vertical_tabs = tabs::utils::ShouldShowVerticalTabs(browser());
  const bool use_search_button =
      browser()->profile()->GetPrefs()->GetBoolean(kTabsSearchShow);
  if (features::HasTabSearchToolbarButton()) {
    if (auto* tab_search_button = toolbar()->tab_search_button()) {
      tab_search_button->SetVisible(!is_vertical_tabs && use_search_button);
    }
  } else if (auto* tab_search_button =
                 BrowserElementsViews::From(browser())
                     ->GetViewAs<TabSearchButton>(kTabSearchButtonElementId)) {
    tab_search_button->SetVisible(!is_vertical_tabs && use_search_button);
  }
}

BraveBrowserView::~BraveBrowserView() {
  tab_cycling_event_handler_.reset();
}

sidebar::Sidebar* BraveBrowserView::InitSidebar() {
  // Start Sidebar UI initialization.
  DCHECK(sidebar_container_view_);
  sidebar_container_view_->Init();

  // Ask BraveMultiContentsView for preparing web panel feature.
  if (multi_contents_view_ &&
      base::FeatureList::IsEnabled(sidebar::features::kSidebarWebPanel)) {
    GetBraveMultiContentsView()->SetWebPanelWidth(
        sidebar_container_view_->side_panel()->GetPreferredSize().width());
    GetBraveMultiContentsView()->UseContentsContainerViewForWebPanel();
  }

  UpdateSideBarHorizontalAlignment();

  return sidebar_container_view_;
}

void BraveBrowserView::ToggleSidebar() {
  browser_->GetFeatures().side_panel_ui()->Toggle();
}

void BraveBrowserView::ShowBraveVPNBubble(bool show_select) {
#if BUILDFLAG(ENABLE_BRAVE_VPN)
  vpn_panel_controller_.ShowBraveVPNPanel(show_select);
#endif
}

views::View* BraveBrowserView::GetAnchorViewForBraveVPNPanel() {
#if BUILDFLAG(ENABLE_BRAVE_VPN)
  auto* vpn_button =
      static_cast<BraveToolbarView*>(toolbar())->brave_vpn_button();
  if (vpn_button->GetVisible()) {
    return vpn_button;
  }
  return toolbar()->app_menu_button();
#else
  return nullptr;
#endif
}

gfx::Rect BraveBrowserView::GetShieldsBubbleRect() {
  auto* brave_location_bar_view =
      static_cast<BraveLocationBarView*>(GetLocationBarView());
  if (!brave_location_bar_view) {
    return gfx::Rect();
  }

  auto* shields_action_view =
      brave_location_bar_view->brave_actions_contatiner_view()
          ->GetShieldsActionView();
  if (!shields_action_view) {
    return gfx::Rect();
  }

  auto* bubble_widget = shields_action_view->GetBubbleWidget();
  if (!bubble_widget) {
    return gfx::Rect();
  }

  return bubble_widget->GetClientAreaBoundsInScreen();
}

bool BraveBrowserView::GetTabStripVisible() const {
  if (tabs::utils::ShouldShowVerticalTabs(browser())) {
    return false;
  }

  return BrowserView::GetTabStripVisible();
}

void BraveBrowserView::SetStarredState(bool is_starred) {
  BraveBookmarkButton* button =
      static_cast<BraveToolbarView*>(toolbar())->bookmark_button();
  if (button) {
    button->SetToggled(is_starred);
  }
}

#if BUILDFLAG(ENABLE_SPEEDREADER)
ReaderModeToolbarView* BraveBrowserView::reader_mode_toolbar() {
  if (base::FeatureList::IsEnabled(features::kSideBySide)) {
    return GetBraveMultiContentsView()
        ->GetActiveContentsContainerView()
        ->reader_mode_toolbar();
  }

  return reader_mode_toolbar_;
}

speedreader::SpeedreaderBubbleView* BraveBrowserView::ShowSpeedreaderBubble(
    speedreader::SpeedreaderTabHelper* tab_helper,
    speedreader::SpeedreaderBubbleLocation location) {
  views::View* anchor = nullptr;
  views::BubbleBorder::Arrow arrow = views::BubbleBorder::NONE;
  switch (location) {
    case speedreader::SpeedreaderBubbleLocation::kLocationBar:
      anchor = GetLocationBarView();
      arrow = views::BubbleBorder::TOP_RIGHT;
      break;
    case speedreader::SpeedreaderBubbleLocation::kToolbar:
      anchor = reader_mode_toolbar()->toolbar();
      arrow = views::BubbleBorder::TOP_LEFT;
      break;
  }

  auto* reader_mode_bubble =
      new speedreader::ReaderModeBubble(anchor, tab_helper);
  views::BubbleDialogDelegateView::CreateBubble(reader_mode_bubble);
  reader_mode_bubble->SetArrow(arrow);
  reader_mode_bubble->Show();
  return reader_mode_bubble;
}

void BraveBrowserView::UpdateReaderModeToolbar() {
  auto is_distilled = [](content::WebContents* web_contents) {
    if (!web_contents) {
      return false;
    }
    if (auto* th =
            speedreader::SpeedreaderTabHelper::FromWebContents(web_contents)) {
      return speedreader::DistillStates::IsDistilled(th->PageDistillState());
    }
    return false;
  };
  reader_mode_toolbar()->SetVisible(
      is_distilled(browser()->tab_strip_model()->GetActiveWebContents()));

  if (base::FeatureList::IsEnabled(features::kSideBySide)) {
    // Need to update inactive split tabs' reader mode toolbar because
    // it's also visible.
    auto* contents_container =
        GetBraveMultiContentsView()->GetInactiveContentsContainerView();
    auto* reader_mode_toolbar = contents_container->reader_mode_toolbar();
    reader_mode_toolbar->SetVisible(
        is_distilled(contents_container->contents_view()->web_contents()));
  }
}
#endif  // BUILDFLAG(ENABLE_SPEEDREADER)

void BraveBrowserView::ShowUpdateChromeDialog() {
#if BUILDFLAG(ENABLE_SPARKLE)
  // On mac, sparkle frameworks's relaunch api is used.
  UpdateRecommendedMessageBoxMac::Show(GetNativeWindow());
#else
  BrowserView::ShowUpdateChromeDialog();
#endif
}

bool BraveBrowserView::HasSelectedURL() const {
  if (!GetLocationBarView() || !GetLocationBarView()->HasFocus()) {
    return false;
  }
  auto* brave_omnibox_view =
      static_cast<BraveOmniboxViewViews*>(GetLocationBarView()->omnibox_view());
  return brave_omnibox_view && brave_omnibox_view->SelectedTextIsURL();
}

void BraveBrowserView::CleanAndCopySelectedURL() {
  if (!GetLocationBarView()) {
    return;
  }
  auto* brave_omnibox_view =
      static_cast<BraveOmniboxViewViews*>(GetLocationBarView()->omnibox_view());
  if (!brave_omnibox_view) {
    return;
  }
  brave_omnibox_view->CleanAndCopySelectedURL();
}

#if BUILDFLAG(ENABLE_PLAYLIST_WEBUI)
void BraveBrowserView::ShowPlaylistBubble() {
  static_cast<BraveLocationBarView*>(GetLocationBarView())
      ->ShowPlaylistBubble();
}
#endif

#if BUILDFLAG(ENABLE_BRAVE_WAYBACK_MACHINE)
void BraveBrowserView::ShowWaybackMachineBubble() {
  if (auto* anchor = toolbar_button_provider_->GetPageActionIconView(
          brave::kWaybackMachineActionIconType)) {
    DCHECK(anchor->GetVisible());
    // Launch bubble with this anchor.
    WaybackMachineBubbleView::Show(browser(), anchor);
  }
}
#endif

WalletButton* BraveBrowserView::GetWalletButton() {
  return static_cast<BraveToolbarView*>(toolbar())->wallet_button();
}

void BraveBrowserView::NotifyDialogPositionRequiresUpdate() {
  GetBrowserViewLayout()->NotifyDialogPositionRequiresUpdate();
}

views::View* BraveBrowserView::GetWalletButtonAnchorView() {
  return static_cast<BraveToolbarView*>(toolbar())
      ->wallet_button()
      ->GetAsAnchorView();
}

void BraveBrowserView::OnAcceleratorsChanged(
    const commands::Accelerators& changed) {
  DCHECK(base::FeatureList::IsEnabled(commands::features::kBraveCommands));

  auto* focus_manager = GetFocusManager();
  DCHECK(focus_manager);

  for (const auto& [command_id, accelerators] : changed) {
    if (IsUnsupportedCommand(command_id, browser())) {
      continue;
    }

    std::vector<ui::Accelerator> old_accelerators;
    for (const auto& [accelerator, accelerator_command] : accelerator_table_) {
      if (accelerator_command != command_id) {
        continue;
      }
      old_accelerators.push_back(accelerator);
    }

    // Register current accelerators
    for (const auto& accelerator : accelerators) {
      if (focus_manager->IsAcceleratorRegistered(accelerator)) {
        focus_manager->UnregisterAccelerator(accelerator, this);
      }

      focus_manager->RegisterAccelerator(
          accelerator, ui::AcceleratorManager::kNormalPriority, this);
      accelerator_table_[accelerator] = command_id;
    }

    // Unregister removed accelerators
    for (const auto& old_accelerator : old_accelerators) {
      if (base::Contains(accelerators, old_accelerator)) {
        continue;
      }
      focus_manager->UnregisterAccelerator(old_accelerator, this);
      accelerator_table_.erase(old_accelerator);
    }
  }
}

void BraveBrowserView::CreateWalletBubble() {
  DCHECK(GetWalletButton());
  GetWalletButton()->ShowWalletBubble();
}

void BraveBrowserView::CreateApproveWalletBubble() {
  DCHECK(GetWalletButton());
  GetWalletButton()->ShowApproveWalletBubble();
}

void BraveBrowserView::CloseWalletBubble() {
  if (GetWalletButton()) {
    GetWalletButton()->CloseWalletBubble();
  }
}

void BraveBrowserView::AddedToWidget() {
  BrowserView::AddedToWidget();
  // we must call all new views once BraveBrowserView is added to widget

  GetBrowserViewLayout()->set_contents_background(contents_background_view_);
  GetBrowserViewLayout()->set_sidebar_container(sidebar_container_view_);
  GetBrowserViewLayout()->set_sidebar_separator(sidebar_separator_view_);

  UpdateWebViewRoundedCorners();

  if (vertical_tab_strip_host_view_) {
    vertical_tab_strip_widget_ = VerticalTabStripWidgetDelegateView::Create(
        this, vertical_tab_strip_host_view_);
    vertical_tab_strip_widget_delegate_view_ =
        static_cast<VerticalTabStripWidgetDelegateView*>(
            vertical_tab_strip_widget_->widget_delegate());

    // By setting this property to the widget for vertical tabs,
    // BrowserView::GetBrowserViewForNativeWindow() will return browser view
    // properly even when we pass the native window for vertical tab strip.
    // As a result, we don't have to call GetTopLevelWidget() in order to
    // get browser view from the vertical tab strip's widget.
    SetNativeWindowPropertyForWidget(vertical_tab_strip_widget_.get());

    GetBrowserViewLayout()->set_vertical_tab_strip_host(
        vertical_tab_strip_host_view_.get());
  }
}

bool BraveBrowserView::ShowBraveHelpBubbleView(const std::string& text) {
  auto* shields_action_view =
      static_cast<BraveLocationBarView*>(GetLocationBarView())
          ->brave_actions_contatiner_view()
          ->GetShieldsActionView();
  if (!shields_action_view || !shields_action_view->GetVisible()) {
    return false;
  }

  // When help bubble is closed, this host view gets hidden.
  // For now, this help bubble host view is only used for shield icon, but it
  // could be re-used for other icons or views in the future.
  if (!brave_help_bubble_host_view_) {
    brave_help_bubble_host_view_ =
        AddChildView(std::make_unique<BraveHelpBubbleHostView>());
  }
  brave_help_bubble_host_view_->set_text(text);
  brave_help_bubble_host_view_->set_tracked_element(shields_action_view);
  return brave_help_bubble_host_view_->Show();
}

void BraveBrowserView::LoadAccelerators() {
  if (base::FeatureList::IsEnabled(commands::features::kBraveCommands)) {
    auto* accelerator_service =
        commands::AcceleratorServiceFactory::GetForContext(
            browser()->profile());
    if (accelerator_service) {
      accelerators_observation_.Observe(accelerator_service);
      return;
    }
  }
  BrowserView::LoadAccelerators();
}

void BraveBrowserView::OnTabStripModelChanged(
    TabStripModel* tab_strip_model,
    const TabStripModelChange& change,
    const TabStripSelectionChange& selection) {
  BrowserView::OnTabStripModelChanged(tab_strip_model, change, selection);

  if (change.type() != TabStripModelChange::kSelectionOnly) {
    // Stop tab cycling if tab is closed dusing the cycle.
    // This can happen when tab is closed by shortcut (ex, ctrl + F4).
    // After stopping, current tab cycling, new tab cycling will be started.
    StopTabCycling();
  }

  if (selection.active_tab_changed() && brave_help_bubble_host_view_ &&
      brave_help_bubble_host_view_->GetVisible()) {
    brave_help_bubble_host_view_->Hide();
  }
}

views::CloseRequestResult BraveBrowserView::OnWindowCloseRequested() {
  if (GetBraveBrowser()->ShouldAskForBrowserClosingBeforeHandlers()) {
    if (!closing_confirm_dialog_activated_) {
      WindowClosingConfirmDialogView::Show(
          browser(),
          base::BindOnce(&BraveBrowserView::OnWindowClosingConfirmResponse,
                         weak_ptr_.GetWeakPtr()));
      closing_confirm_dialog_activated_ = true;
    }
    return views::CloseRequestResult::kCannotClose;
  }

  return BrowserView::OnWindowCloseRequested();
}

void BraveBrowserView::OnWindowClosingConfirmResponse(bool allowed_to_close) {
  DCHECK(closing_confirm_dialog_activated_);
  closing_confirm_dialog_activated_ = false;

  auto* browser = GetBraveBrowser();
  // Set to Browser instance because Browser instance knows about the result
  // of any warning handlers or beforeunload handlers.
  browser->set_confirmed_to_close(allowed_to_close);
  if (allowed_to_close) {
    // Start close window again as user allowed to close it.
    // Confirm dialog will not be launched for this closing request
    // as we set BraveBrowser::confirmed_to_closed_window_ to true.
    // If user cancels this window closing via additional warnings
    // or beforeunload handler, this dialog will be shown again.
    chrome::CloseWindow(browser);
  }
}

void BraveBrowserView::ConfirmBrowserCloseWithPendingDownloads(
    int download_count,
    Browser::DownloadCloseType dialog_type,
    base::OnceCallback<void(bool)> callback) {
  // Simulate user response.
  if (g_download_confirm_return_allow_for_testing) {
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE,
        base::BindOnce(std::move(callback),
                       *g_download_confirm_return_allow_for_testing));
    return;
  }
  BrowserView::ConfirmBrowserCloseWithPendingDownloads(
      download_count, dialog_type, std::move(callback));
}

void BraveBrowserView::MaybeShowReadingListInSidePanelIPH() {
  // Do nothing.
}

bool BraveBrowserView::MaybeUpdateDevtools(content::WebContents* web_contents) {
  CHECK(!web_contents || web_contents == GetActiveWebContents())
      << "This method is supposed to be called only for the active web "
         "contents";

  bool result = BrowserView::MaybeUpdateDevtools(web_contents);

  UpdateWebViewRoundedCorners();
  return result;
}

void BraveBrowserView::OnWidgetActivationChanged(views::Widget* widget,
                                                 bool active) {
  BrowserView::OnWidgetActivationChanged(widget, active);

  // For updating sidebar's item state.
  // As we can activate other window's Talk tab with current window's sidebar
  // Talk item, sidebar Talk item should have activated state if other windows
  // have Talk tab. It would be complex to get updated when Talk tab is opened
  // from other windows. So, simply trying to update when window activation
  // state is changed. With this, active window could have correct sidebar
  // item state.
  if (sidebar_container_view_) {
    sidebar_container_view_->UpdateSidebarItemsState();
  }
}

void BraveBrowserView::OnWidgetWindowModalVisibilityChanged(
    views::Widget* widget,
    bool visible) {
  // We explicitly override this and don't call the parent class, because we
  // currently don't support scrim views for tab modals and thus don't want the
  // parent class to make the scrim view visible
}

void BraveBrowserView::UpdateContentsShadowVisibility() {
  bool show_contents_shadow =
      BraveBrowser::IsBraveWebViewRoundedCornersEnabled(browser());

  // With SideBySide, we use chromium's mini toolbar.
  // Unfortunately, it's not rendered well with contents shadow.
  // Fixed by hiding contents shadow when split view is active.
  // As split view has another border around contents, we don't need
  // contents shadow.
  if (browser()->tab_strip_model()->IsActiveTabSplit()) {
    show_contents_shadow = false;
  }

  // Toggle shadow.
  if (show_contents_shadow && !contents_shadow_) {
    contents_shadow_ = BraveContentsViewUtil::CreateShadow(contents_container_);
    return;
  }

  if (!show_contents_shadow && contents_shadow_) {
    contents_shadow_.reset();
    contents_container_->DestroyLayer();
  }
}

void BraveBrowserView::ShowSplitView(bool focus_active_view) {
  BrowserView::ShowSplitView(focus_active_view);

  UpdateRoundedCornersUI();
}

void BraveBrowserView::HideSplitView() {
  BrowserView::HideSplitView();

  UpdateRoundedCornersUI();
}

BraveMultiContentsView* BraveBrowserView::GetBraveMultiContentsView() const {
  return BraveMultiContentsView::From(multi_contents_view_);
}

bool BraveBrowserView::ShouldShowWindowTitle() const {
  if (BrowserView::ShouldShowWindowTitle()) {
    return true;
  }

  if (tabs::utils::ShouldShowWindowTitleForVerticalTabs(browser())) {
    return true;
  }

  return false;
}

void BraveBrowserView::OnThemeChanged() {
  BrowserView::OnThemeChanged();
  if (vertical_tab_strip_host_view_) {
    const auto background_color = GetColorProvider()->GetColor(kColorToolbar);
    vertical_tab_strip_host_view_->SetBackground(
        views::CreateSolidBackground(background_color));
  }
}

void BraveBrowserView::UpdateRoundedCornersUI() {
  // Update various UI that can be affected by rounded corners.
  UpdateContentsSeparatorVisibility();
  UpdateContentsShadowVisibility();
  UpdateWebViewRoundedCorners();
  UpdateVerticalTabStripBorder();
  UpdateSidebarBorder();
  InvalidateLayout();
}

void BraveBrowserView::UpdateVerticalTabStripBorder() {
  // Vertical tab strip's border could be toggled based on split view state.
  if (vertical_tab_strip_widget_delegate_view_) {
    vertical_tab_strip_widget_delegate_view_->vertical_tab_strip_region_view()
        ->UpdateBorder();
  }
}

void BraveBrowserView::UpdateSidebarBorder() {
  if (contents_height_side_panel_) {
    contents_height_side_panel_->UpdateBorder();
  }

  if (sidebar_container_view_) {
    sidebar_container_view_->UpdateBorder();
  }
}

void BraveBrowserView::OnActiveTabChanged(content::WebContents* old_contents,
                                          content::WebContents* new_contents,
                                          int index,
                                          int reason) {
  UpdateRoundedCornersUI();

  BrowserView::OnActiveTabChanged(old_contents, new_contents, index, reason);

#if BUILDFLAG(ENABLE_SPEEDREADER)
  UpdateReaderModeToolbar();
#endif

  // Some managers need to consider tab's active state with web content's
  // visibility.
  if (old_contents) {
    auto* permission_manager =
        permissions::PermissionRequestManager::FromWebContents(old_contents);
    CHECK(permission_manager);
    permission_manager->OnTabActiveStateChanged(false);

    // web/tab modal dialog manger can get tab activation state fromm their
    // delegates.
    auto* web_modal_dialog_manager =
        web_modal::WebContentsModalDialogManager::FromWebContents(old_contents);
    CHECK(web_modal_dialog_manager);
    web_modal_dialog_manager->OnTabActiveStateChanged();

    auto* tab_modal_dialog_manager =
        javascript_dialogs::TabModalDialogManager::FromWebContents(
            old_contents);
    CHECK(tab_modal_dialog_manager);
    tab_modal_dialog_manager->OnTabActiveStateChanged();
  }

  if (new_contents) {
    auto* permission_manager =
        permissions::PermissionRequestManager::FromWebContents(new_contents);
    CHECK(permission_manager);
    permission_manager->OnTabActiveStateChanged(true);

    auto* web_modal_dialog_manager =
        web_modal::WebContentsModalDialogManager::FromWebContents(new_contents);
    CHECK(web_modal_dialog_manager);
    web_modal_dialog_manager->OnTabActiveStateChanged();

    auto* tab_modal_dialog_manager =
        javascript_dialogs::TabModalDialogManager::FromWebContents(
            new_contents);
    CHECK(tab_modal_dialog_manager);
    tab_modal_dialog_manager->OnTabActiveStateChanged();
  }
}

void BraveBrowserView::UpdateContentsSeparatorVisibility() {
  // It's not shown with rounded corners mode always.
  if (BraveBrowser::ShouldUseBraveWebViewRoundedCornersForContents(
          browser_.get())) {
    top_container_separator_->SetPreferredSize({});
    return;
  }

  top_container_separator_->SetPreferredSize(
      gfx::Size(views::Separator::kThickness, views::Separator::kThickness));
}

bool BraveBrowserView::AcceleratorPressed(const ui::Accelerator& accelerator) {
  if (base::FeatureList::IsEnabled(tabs::features::kBraveSharedPinnedTabs) &&
      browser()->profile()->GetPrefs()->GetBoolean(
          brave_tabs::kSharedPinnedTab)) {
    if (int command_id; FindCommandIdForAccelerator(accelerator, &command_id) &&
                        command_id == IDC_CLOSE_TAB) {
      auto* tab_strip_model = browser()->tab_strip_model();
      if (tab_strip_model->IsTabPinned(tab_strip_model->active_index())) {
        // Ignore CLOSE TAB command via accelerator if the tab is shared/dummy
        // pinned tab.
        return true;
      }
    }
  }
  return BrowserView::AcceleratorPressed(accelerator);
}

bool BraveBrowserView::IsInTabDragging() const {
  return browser_widget()->tab_drag_kind() == TabDragKind::kAllTabs;
}

void BraveBrowserView::ReadyToListenFullscreenChanges() {
  CHECK(browser_->GetFeatures().exclusive_access_manager());

  if (vertical_tab_strip_widget_delegate_view_) {
    vertical_tab_strip_widget_delegate_view_->vertical_tab_strip_region_view()
        ->ListenFullscreenChanges();
  }
}

void BraveBrowserView::StopListeningFullscreenChanges() {
  CHECK(browser_->GetFeatures().exclusive_access_manager());

  if (vertical_tab_strip_widget_delegate_view_) {
    vertical_tab_strip_widget_delegate_view_->vertical_tab_strip_region_view()
        ->StopListeningFullscreenChanges();
  }
}

bool BraveBrowserView::PreHandleMouseEvent(const blink::WebMouseEvent& event) {
  if (event.GetTypeAsUiEventType() == ui::EventType::kMouseMoved &&
      sidebar_container_view_) {
    return sidebar_container_view_->PreHandleMouseEvent(
        event.PositionInScreen());
  }

  return false;
}

void BraveBrowserView::OnMouseMoved(const ui::MouseEvent& event) {
  BrowserView::OnMouseMoved(event);

  // To make sidebar UI visible when mouse moved to space between window border
  // & contents. This space exists when rounded corners feature is enabled. As
  // BraveBrowserView::PreHandleMouseEvent() is only called from web contents,
  // we need to handle move event from browser view.
  // This handling is useful when it's in fullscreen. If move the mouse point to
  // window edge quickly BraveBrowserView::PreHandleMouseEvent() is not called.
  if (sidebar_container_view_ && event.type() == ui::EventType::kMouseMoved &&
      BraveBrowser::ShouldUseBraveWebViewRoundedCornersForContents(
          browser_.get())) {
    gfx::Point position_in_screen = event.location();
    views::View::ConvertPointToScreen(this, &position_in_screen);
    sidebar_container_view_->PreHandleMouseEvent(
        gfx::PointF(position_in_screen));
  }
}

bool BraveBrowserView::IsSidebarVisible() const {
  return sidebar_container_view_ && sidebar_container_view_->IsSidebarVisible();
}

BraveBrowser* BraveBrowserView::GetBraveBrowser() const {
  return static_cast<BraveBrowser*>(browser_.get());
}

void BraveBrowserView::UpdateWebViewRoundedCorners() {
  gfx::RoundedCornersF corners;

  if (BraveBrowser::ShouldUseBraveWebViewRoundedCornersForContents(
          browser_.get())) {
    corners = gfx::RoundedCornersF(BraveContentsViewUtil::kBorderRadius);
  }

  // In fullscreen-for-tab mode (e.g. full-screen video), no corners should be
  // rounded.
  if (auto* exclusive_access_manager = GetExclusiveAccessManager()) {
    if (auto* controller = exclusive_access_manager->fullscreen_controller()) {
      if (controller->IsWindowFullscreenForTabOrPending()) {
        corners = gfx::RoundedCornersF(0);
      }
    }
  }

  // Set the appropriate corner radius for the view that contains both the web
  // contents and devtools.
  if (contents_container_->layer()) {
    contents_container_->layer()->SetRoundedCornerRadius(corners);
  }

  if (multi_contents_view_) {
    GetBraveMultiContentsView()->UpdateCornerRadius();
    return;
  }

  auto update_corner_radius =
      [](views::WebView* contents, views::WebView* devtools,
         ContentsContainerView::DevToolsDockedPlacement devtools_placement,
         gfx::RoundedCornersF corners) {
        // In addition to giving the contents container rounded corners, we also
        // need to round the corners of the native view holder that displays the
        // web contents.

        // Devtools lies underneath the contents webview. Round all four
        // corners.
        if (devtools && devtools->holder()) {
          devtools->holder()->SetCornerRadii(corners);
        }

        // In order to make the contents web view and devtools appear to be
        // contained within a single rounded-corner view, square the contents
        // webview corners that are adjacent to devtools.
        // TODO(sko) We need to override
        // BrowserView::GetDevToolsDockedPlacement(). It depends on coordinate
        // of it but in split view mode, the calculation is not correct.
        switch (devtools_placement) {
          case ContentsContainerView::DevToolsDockedPlacement::kLeft:
            corners.set_upper_left(0);
            corners.set_lower_left(0);
            break;
          case ContentsContainerView::DevToolsDockedPlacement::kRight:
            corners.set_upper_right(0);
            corners.set_lower_right(0);
            break;
          case ContentsContainerView::DevToolsDockedPlacement::kBottom:
            corners.set_lower_left(0);
            corners.set_lower_right(0);
            break;
          case ContentsContainerView::DevToolsDockedPlacement::kNone:
            break;
          case ContentsContainerView::DevToolsDockedPlacement::kUnknown:
            break;
        }

        if (contents && contents->holder()) {
          // Upstream uses layer for its background.
          CHECK(contents->layer());
          contents->layer()->SetRoundedCornerRadius(corners);
          contents->holder()->SetCornerRadii(corners);
        }
      };

  if (contents_container_view_) {
    update_corner_radius(contents_container_view_->contents_view(),
                         contents_container_view_->devtools_web_view(),
                         contents_container_view_->devtools_docked_placement(),
                         corners);
  }
}

void BraveBrowserView::Layout(PassKey) {
  LayoutSuperclass<BrowserView>(this);
  UpdateWebViewRoundedCorners();
}

void BraveBrowserView::StartTabCycling() {
  tab_cycling_event_handler_ = std::make_unique<TabCyclingEventHandler>(this);
}

void BraveBrowserView::StopTabCycling() {
  tab_cycling_event_handler_.reset();
  static_cast<BraveTabStripModel*>(browser()->tab_strip_model())
      ->StopMRUCycling();
}

void BraveBrowserView::SetSidePanelOperationByActiveTabChange(bool tab_change) {
  if (!sidebar_container_view_) {
    return;
  }

  sidebar_container_view_->set_operation_from_active_tab_change(tab_change);
}

BEGIN_METADATA(BraveBrowserView)
END_METADATA
