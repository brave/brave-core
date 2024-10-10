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
#include <utility>
#include <vector>

#include "base/containers/contains.h"
#include "base/feature_list.h"
#include "base/functional/bind.h"
#include "base/task/sequenced_task_runner.h"
#include "brave/browser/brave_rewards/rewards_service_factory.h"
#include "brave/browser/sparkle_buildflags.h"
#include "brave/browser/translate/brave_translate_utils.h"
#include "brave/browser/ui/brave_browser.h"
#include "brave/browser/ui/brave_rewards/rewards_panel_coordinator.h"
#include "brave/browser/ui/brave_rewards/tip_panel_coordinator.h"
#include "brave/browser/ui/color/brave_color_id.h"
#include "brave/browser/ui/commands/accelerator_service.h"
#include "brave/browser/ui/commands/accelerator_service_factory.h"
#include "brave/browser/ui/page_action/brave_page_action_icon_type.h"
#include "brave/browser/ui/sidebar/sidebar_utils.h"
#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "brave/browser/ui/tabs/features.h"
#include "brave/browser/ui/tabs/split_view_browser_data.h"
#include "brave/browser/ui/views/brave_actions/brave_actions_container.h"
#include "brave/browser/ui/views/brave_actions/brave_shields_action_view.h"
#include "brave/browser/ui/views/brave_help_bubble/brave_help_bubble_host_view.h"
#include "brave/browser/ui/views/brave_rewards/tip_panel_bubble_host.h"
#include "brave/browser/ui/views/brave_shields/cookie_list_opt_in_bubble_host.h"
#include "brave/browser/ui/views/frame/brave_contents_layout_manager.h"
#include "brave/browser/ui/views/frame/brave_contents_view_util.h"
#include "brave/browser/ui/views/frame/vertical_tab_strip_region_view.h"
#include "brave/browser/ui/views/frame/vertical_tab_strip_widget_delegate_view.h"
#include "brave/browser/ui/views/location_bar/brave_location_bar_view.h"
#include "brave/browser/ui/views/omnibox/brave_omnibox_view_views.h"
#include "brave/browser/ui/views/sidebar/sidebar_container_view.h"
#include "brave/browser/ui/views/speedreader/reader_mode_toolbar_view.h"
#include "brave/browser/ui/views/split_view/split_view_location_bar.h"
#include "brave/browser/ui/views/split_view/split_view_separator.h"
#include "brave/browser/ui/views/tabs/vertical_tab_utils.h"
#include "brave/browser/ui/views/toolbar/bookmark_button.h"
#include "brave/browser/ui/views/toolbar/brave_toolbar_view.h"
#include "brave/browser/ui/views/toolbar/wallet_button.h"
#include "brave/browser/ui/views/window_closing_confirm_dialog_view.h"
#include "brave/components/commands/common/features.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/speedreader/common/buildflags/buildflags.h"
#include "brave/ui/color/nala/nala_color_id.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/app_mode/app_mode_utils.h"
#include "chrome/browser/devtools/devtools_window.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/color/chrome_color_id.h"
#include "chrome/browser/ui/exclusive_access/exclusive_access_manager.h"
#include "chrome/browser/ui/exclusive_access/fullscreen_controller.h"
#include "chrome/browser/ui/frame/window_frame_util.h"
#include "chrome/browser/ui/views/frame/browser_frame.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/frame/contents_layout_manager.h"
#include "chrome/browser/ui/views/frame/contents_web_view.h"
#include "chrome/browser/ui/views/frame/tab_strip_region_view.h"
#include "chrome/browser/ui/views/frame/top_container_view.h"
#include "chrome/browser/ui/views/side_panel/side_panel_coordinator.h"
#include "chrome/browser/ui/views/tabs/tab_search_button.h"
#include "chrome/browser/ui/views/tabs/tab_strip.h"
#include "chrome/browser/ui/views/toolbar/browser_app_menu_button.h"
#include "chrome/common/pref_names.h"
#include "content/public/browser/page_navigator.h"
#include "content/public/browser/web_contents.h"
#include "extensions/buildflags/buildflags.h"
#include "ui/base/accelerators/accelerator.h"
#include "ui/base/accelerators/accelerator_manager.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/compositor/layer.h"
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
#include "brave/browser/speedreader/speedreader_tab_helper.h"
#include "brave/browser/ui/views/speedreader/reader_mode_bubble.h"
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
        views::CreateThemedSolidBackground(kColorBraveVerticalTabSeparator));
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
    SetBackground(views::CreateThemedSolidBackground(kColorToolbar));
    SetEnabled(false);
  }
};
BEGIN_METADATA(ContentsBackground)
END_METADATA

// A `ContentsWebView` that activates its contents when it gets focus.
class ActivatableContentsWebView : public ContentsWebView {
  METADATA_HEADER(ActivatableContentsWebView, ContentsWebView)
 public:
  using ContentsWebView::ContentsWebView;
  ~ActivatableContentsWebView() override = default;

  // ContentsWebView:
  void OnFocus() override {
    ContentsWebView::OnFocus();
    if (web_contents() && web_contents()->GetDelegate()) {
      web_contents()->GetDelegate()->ActivateContents(web_contents());
    }
  }
};
BEGIN_METADATA(ActivatableContentsWebView)
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

BraveBrowserView::BraveBrowserView(std::unique_ptr<Browser> browser)
    : BrowserView(std::move(browser)) {
  if (BraveBrowser::ShouldUseBraveWebViewRoundedCorners(browser_.get())) {
    // Collapse the separator line between the toolbar or bookmark bar and the
    // views below.
    contents_separator_->SetPreferredSize(gfx::Size());
    contents_shadow_ = BraveContentsViewUtil::CreateShadow(contents_container_);
    contents_background_view_ =
        AddChildView(std::make_unique<ContentsBackground>());
  }

  pref_change_registrar_.Init(GetProfile()->GetPrefs());
  pref_change_registrar_.Add(
      kTabsSearchShow,
      base::BindRepeating(&BraveBrowserView::OnPreferenceChanged,
                          base::Unretained(this)));
  // Show the correct value in settings on initial start
  UpdateSearchTabsButtonState();

  auto* rewards_service =
      brave_rewards::RewardsServiceFactory::GetForProfile(browser_->profile());
  if (rewards_service) {
    brave_rewards::RewardsPanelCoordinator::CreateForBrowser(browser_.get());
    brave_rewards::TipPanelCoordinator::CreateForBrowser(browser_.get(),
                                                         rewards_service);
  }

  brave_rewards::TipPanelBubbleHost::MaybeCreateForBrowser(browser_.get());

  brave_shields::CookieListOptInBubbleHost::MaybeCreateForBrowser(
      browser_.get());

#if BUILDFLAG(ENABLE_BRAVE_VPN)
  pref_change_registrar_.Add(
      brave_vpn::prefs::kBraveVPNShowButton,
      base::BindRepeating(&BraveBrowserView::OnPreferenceChanged,
                          base::Unretained(this)));
#endif

  // Only normal window (tabbed) should have sidebar.
  const bool can_have_sidebar = sidebar::CanUseSidebar(browser_.get());
  if (can_have_sidebar) {
    // Wrap chromium side panel with our sidebar container
    auto original_side_panel = RemoveChildViewT(unified_side_panel_.get());
    sidebar_container_view_ =
        AddChildView(std::make_unique<SidebarContainerView>(
            browser_.get(), browser_->GetFeatures().side_panel_coordinator(),
            std::move(original_side_panel)));
    unified_side_panel_ = sidebar_container_view_->side_panel();

    if (BraveBrowser::ShouldUseBraveWebViewRoundedCorners(browser_.get())) {
      sidebar_separator_view_ =
          AddChildView(std::make_unique<SidebarSeparator>());
    }

#if defined(USE_AURA)
    sidebar_host_view_ = AddChildView(std::make_unique<views::View>());
#endif

    pref_change_registrar_.Add(
        prefs::kSidePanelHorizontalAlignment,
        base::BindRepeating(&BraveBrowserView::OnPreferenceChanged,
                            base::Unretained(this)));
  }

  if (base::FeatureList::IsEnabled(tabs::features::kBraveSplitView) &&
      browser_->is_type_normal()) {
    SplitViewBrowserData::CreateForBrowser(browser_.get());

    auto devtools_web_view =
        std::make_unique<views::WebView>(browser_->profile());
    devtools_web_view->SetVisible(false);
    auto contents_web_view =
        std::make_unique<ActivatableContentsWebView>(browser_->profile());
    contents_web_view->SetVisible(false);

    secondary_devtools_web_view_ =
        contents_container_->AddChildView(std::move(devtools_web_view));
    secondary_contents_web_view_ =
        contents_container_->AddChildView(std::move(contents_web_view));
    split_view_separator_ = contents_container_->AddChildView(
        std::make_unique<SplitViewSeparator>(browser_.get()));
    secondary_location_bar_ = std::make_unique<SplitViewLocationBar>(
        browser_->profile()->GetPrefs(), secondary_contents_web_view_);
    secondary_location_bar_widget_ = std::make_unique<views::Widget>();

    auto* contents_layout_manager = static_cast<BraveContentsLayoutManager*>(
        contents_container()->GetLayoutManager());
    contents_layout_manager->set_browser_view(this);
    contents_layout_manager->set_secondary_contents_view(
        secondary_contents_web_view_);
    contents_layout_manager->set_secondary_devtools_view(
        secondary_devtools_web_view_);
    contents_layout_manager->SetSplitViewSeparator(split_view_separator_);

    auto* split_view_browser_data =
        SplitViewBrowserData::FromBrowser(browser_.get());
    contents_layout_manager->set_split_view_browser_data(
        split_view_browser_data);
    split_view_observation_.Observe(split_view_browser_data);
  }

  const bool supports_vertical_tabs =
      tabs::utils::SupportsVerticalTabs(browser_.get());
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

  DeprecatedLayoutImmediately();
}

tabs::TabHandle BraveBrowserView::GetActiveTabHandle() {
  CHECK(base::FeatureList::IsEnabled(tabs::features::kBraveSplitView));

  auto* model = browser()->tab_strip_model();
  return model->GetTabHandleAt(
      model->GetIndexOfWebContents(GetActiveWebContents()));
}

bool BraveBrowserView::IsActiveWebContentsTiled(
    const SplitViewBrowserData::Tile& tile) {
  CHECK(base::FeatureList::IsEnabled(tabs::features::kBraveSplitView));

  auto active_tab_handle = GetActiveTabHandle();
  return tile.first == active_tab_handle || tile.second == active_tab_handle;
}

void BraveBrowserView::UpdateSplitViewSizeDelta(
    content::WebContents* old_contents,
    content::WebContents* new_contents) {
  auto get_index_of = [this](content::WebContents* contents) {
    return browser()->tab_strip_model()->GetIndexOfWebContents(contents);
  };
  if (get_index_of(old_contents) == TabStripModel::kNoTab ||
      get_index_of(new_contents) == TabStripModel::kNoTab) {
    // This can happen on start-up or closing a tab.
    return;
  }

  auto* split_view_browser_data = SplitViewBrowserData::FromBrowser(browser());
  auto get_tab_handle = [this, &get_index_of](content::WebContents* contents) {
    return browser()->tab_strip_model()->GetTabHandleAt(get_index_of(contents));
  };
  auto old_tab_handle = get_tab_handle(old_contents);
  auto new_tab_handle = get_tab_handle(new_contents);

  auto old_tab_tile = split_view_browser_data->GetTile(old_tab_handle);
  auto new_tab_tile = split_view_browser_data->GetTile(new_tab_handle);
  if ((!old_tab_tile && !new_tab_tile) || old_tab_tile == new_tab_tile) {
    // Both tabs are not tiled, or in a same tile. So we don't need to update
    // size delta
    return;
  }

  auto* contents_layout_manager = static_cast<BraveContentsLayoutManager*>(
      contents_container()->GetLayoutManager());
  if (old_tab_tile) {
    split_view_browser_data->SetSizeDelta(
        old_tab_handle, contents_layout_manager->split_view_size_delta());
  }

  if (new_tab_tile) {
    contents_layout_manager->set_split_view_size_delta(
        split_view_browser_data->GetSizeDelta(new_tab_handle));
  }
}

void BraveBrowserView::UpdateContentsWebViewVisual() {
  auto* split_view_browser_data =
      SplitViewBrowserData::FromBrowser(browser_.get());
  if (!split_view_browser_data) {
    return;
  }

  UpdateContentsWebViewBorder();
  UpdateSecondaryContentsWebViewVisibility();
}

void BraveBrowserView::UpdateContentsWebViewBorder() {
  auto* split_view_browser_data =
      SplitViewBrowserData::FromBrowser(browser_.get());
  if (!split_view_browser_data) {
    return;
  }

  if (browser()->tab_strip_model()->empty()) {
    // Happens on startup
    return;
  }

  if (browser()->IsBrowserClosing()) {
    return;
  }

  DCHECK(split_view_browser_data);

  if (split_view_browser_data->GetTile(GetActiveTabHandle())) {
    auto create_border = [this](SkColor color, int border_thickness) {
      return BraveBrowser::ShouldUseBraveWebViewRoundedCorners(browser_.get())
                 ? views::CreateRoundedRectBorder(
                       border_thickness,
                       BraveContentsViewUtil::kBorderRadius +
                           border_thickness / 2,
                       color)
                 : views::CreateSolidBorder(border_thickness, color);
    };

    if (auto* cp = GetColorProvider()) {
      contents_web_view_->SetBorder(
          create_border(cp->GetColor(nala::kColorPrimitivePrimary70), 2));

      secondary_contents_web_view_->SetBorder(create_border(
          cp->GetColor(kColorBraveSplitViewInactiveWebViewBorder), 1));
    }
  } else {
    contents_web_view_->SetBorder(nullptr);
    secondary_contents_web_view_->SetBorder(nullptr);
  }
}

void BraveBrowserView::UpdateSecondaryContentsWebViewVisibility() {
  if (browser()->IsBrowserClosing()) {
    secondary_contents_web_view_->SetWebContents(nullptr);
    return;
  }

  auto* split_view_browser_data =
      SplitViewBrowserData::FromBrowser(browser_.get());
  DCHECK(split_view_browser_data);

  auto active_tab_handle = GetActiveTabHandle();
  if (auto tile = split_view_browser_data->GetTile(active_tab_handle)) {
    const bool second_tile_is_active_web_contents =
        active_tab_handle == tile->second;

    // Active tab should be put in the original |contents_web_view_| as many
    // other UI components are dependent on it. So, in case |tile.second| is
    // the active tab, we let it be held by |contents_web_view_| and
    // |tile.first| by |secondary_contents_web_view_|. But we should rotate
    // the layout order. The layout rotation is done by
    // BraveContentsLayoutManager.
    //
    // ex1) When tile.first is the active tab
    //  Tiled tabs | tile.first(active) |         tile.second          |
    //                        ||                        ||
    //  Contents   | contents_web_view_ | secondary_contents_web_view_ |
    //
    // ex2) When tile.second is the active tab
    //  Tiled tabs |           tile.first         | tile.second(active) |
    //                             ||                        ||
    //  Contents   | secondary_contents_web_view_ | contents_web_view_  |
    auto* model = browser()->tab_strip_model();
    auto* contents = model->GetWebContentsAt(model->GetIndexOfTab(
        second_tile_is_active_web_contents ? tile->first : tile->second));
    CHECK_NE(contents, contents_web_view_->web_contents());
    if (secondary_contents_web_view_->web_contents() != contents) {
      secondary_contents_web_view_->SetWebContents(nullptr);
      secondary_contents_web_view_->SetWebContents(contents);
      secondary_location_bar_->SetWebContents(contents);
    }

    secondary_contents_web_view_->SetVisible(true);
    UpdateSecondaryDevtoolsLayoutAndVisibility(contents);

    auto* contents_layout_manager = static_cast<BraveContentsLayoutManager*>(
        contents_container()->GetLayoutManager());
    contents_layout_manager->show_main_web_contents_at_tail(
        second_tile_is_active_web_contents);
  } else {
    secondary_contents_web_view_->SetWebContents(nullptr);
    secondary_location_bar_->SetWebContents(nullptr);
    secondary_contents_web_view_->SetVisible(false);
    secondary_devtools_web_view_->SetWebContents(nullptr);
    secondary_devtools_web_view_->SetVisible(false);
  }

  split_view_separator_->SetVisible(secondary_contents_web_view_->GetVisible());

  contents_container()->DeprecatedLayoutImmediately();
}

void BraveBrowserView::UpdateSecondaryDevtoolsLayoutAndVisibility(
    content::WebContents* inspected_contents) {
  DevToolsContentsResizingStrategy strategy;
  content::WebContents* devtools =
      DevToolsWindow::GetInTabWebContents(inspected_contents, &strategy);
  if (secondary_devtools_web_view_->web_contents() != devtools) {
    secondary_devtools_web_view_->SetWebContents(devtools);
  }

  if (devtools) {
    secondary_devtools_web_view_->SetVisible(true);
    GetContentsLayoutManager()->SetSecondaryContentsResizingStrategy(strategy);
  } else {
    secondary_devtools_web_view_->SetVisible(false);
    GetContentsLayoutManager()->SetSecondaryContentsResizingStrategy(
        DevToolsContentsResizingStrategy());
  }
}

void BraveBrowserView::UpdateSearchTabsButtonState() {
  if (auto* tab_search_container =
          tab_strip_region_view()->tab_search_container()) {
    if (auto* button = tab_search_container->tab_search_button()) {
      auto is_tab_search_visible =
          GetProfile()->GetPrefs()->GetBoolean(kTabsSearchShow);
      button->SetVisible(is_tab_search_visible);
    }
  }
}

BraveBrowserView::~BraveBrowserView() {
  tab_cycling_event_handler_.reset();
  DCHECK(!tab_cycling_event_handler_);
}

sidebar::Sidebar* BraveBrowserView::InitSidebar() {
  // Start Sidebar UI initialization.
  DCHECK(sidebar_container_view_);
  sidebar_container_view_->Init();
  UpdateSideBarHorizontalAlignment();
  return sidebar_container_view_;
}

void BraveBrowserView::ToggleSidebar() {
  browser_->GetFeatures().side_panel_ui()->Toggle();
}

void BraveBrowserView::ShowBraveVPNBubble() {
#if BUILDFLAG(ENABLE_BRAVE_VPN)
  vpn_panel_controller_.ShowBraveVPNPanel();
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

#if BUILDFLAG(IS_WIN)
bool BraveBrowserView::GetSupportsTitle() const {
  if (tabs::utils::SupportsVerticalTabs(browser())) {
    return true;
  }

  return BrowserView::GetSupportsTitle();
}
#endif

void BraveBrowserView::SetStarredState(bool is_starred) {
  BraveBookmarkButton* button =
      static_cast<BraveToolbarView*>(toolbar())->bookmark_button();
  if (button) {
    button->SetToggled(is_starred);
  }
}

#if BUILDFLAG(ENABLE_SPEEDREADER)

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
      anchor = reader_mode_toolbar_view_->toolbar();
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

void BraveBrowserView::ShowReaderModeToolbar() {
  if (!reader_mode_toolbar_view_) {
    reader_mode_toolbar_view_ =
        std::make_unique<ReaderModeToolbarView>(GetProfile());
    if (!BraveBrowser::ShouldUseBraveWebViewRoundedCorners(browser_.get())) {
      SetBorder(views::CreateThemedSolidSidedBorder(
          gfx::Insets::TLBR(0, 0, 1, 0), kColorToolbarContentAreaSeparator));
    }
    AddChildView(reader_mode_toolbar_view_.get());

    // See the comment of same code in ctor.
    // TODO(simonhong): Find more better way instead of calling multiple
    // times.
    ReorderChildView(find_bar_host_view_, -1);
    GetBrowserViewLayout()->set_reader_mode_toolbar(
        reader_mode_toolbar_view_.get());
  } else {
    reader_mode_toolbar_view_->SetVisible(true);
  }

  DeprecatedLayoutImmediately();
}

void BraveBrowserView::HideReaderModeToolbar() {
  if (reader_mode_toolbar_view_ && reader_mode_toolbar_view_->GetVisible()) {
    reader_mode_toolbar_view_->SetVisible(false);
    DeprecatedLayoutImmediately();
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

void BraveBrowserView::OnTileTabs(const SplitViewBrowserData::Tile& tile) {
  if (!IsActiveWebContentsTiled(tile)) {
    return;
  }

  UpdateContentsWebViewVisual();
}

void BraveBrowserView::OnWillBreakTile(const SplitViewBrowserData::Tile& tile) {
  if (!IsActiveWebContentsTiled(tile)) {
    return;
  }

  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(&BraveBrowserView::UpdateContentsWebViewVisual,
                                weak_ptr_.GetWeakPtr()));
}

void BraveBrowserView::OnSwapTabsInTile(
    const SplitViewBrowserData::Tile& tile) {
  if (!IsActiveWebContentsTiled(tile)) {
    return;
  }

  UpdateSecondaryContentsWebViewVisibility();
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
    vertical_tab_strip_widget_delegate_view_ =
        VerticalTabStripWidgetDelegateView::Create(
            this, vertical_tab_strip_host_view_);

    // By setting this property to the widget for vertical tabs,
    // BrowserView::GetBrowserViewForNativeWindow() will return browser view
    // properly even when we pass the native window for vertical tab strip.
    // As a result, we don't have to call GetTopLevelWidget() in order to
    // get browser view from the vertical tab strip's widget.
    SetNativeWindowPropertyForWidget(
        vertical_tab_strip_widget_delegate_view_->GetWidget());

    GetBrowserViewLayout()->set_vertical_tab_strip_host(
        vertical_tab_strip_host_view_.get());
  }

  if (secondary_location_bar_widget_) {
    CHECK(secondary_location_bar_);
    secondary_location_bar_widget_->Init(
        SplitViewLocationBar::GetWidgetInitParams(
            GetWidget()->GetNativeView(), secondary_location_bar_.get()));
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

void BraveBrowserView::UpdateDevToolsForContents(
    content::WebContents* web_contents,
    bool update_devtools_web_contents) {
  if (base::FeatureList::IsEnabled(tabs::features::kBraveSplitView) &&
      browser()->is_type_normal()) {
    secondary_devtools_web_view_->SetWebContents(nullptr);
  }

  BrowserView::UpdateDevToolsForContents(web_contents,
                                         update_devtools_web_contents);
  if (base::FeatureList::IsEnabled(tabs::features::kBraveSplitView) &&
      browser()->is_type_normal() &&
      secondary_contents_web_view_->GetVisible()) {
    UpdateSecondaryDevtoolsLayoutAndVisibility(
        secondary_contents_web_view_->web_contents());

    contents_container_->DeprecatedLayoutImmediately();
  }

  UpdateWebViewRoundedCorners();
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

void BraveBrowserView::GetAccessiblePanes(std::vector<views::View*>* panes) {
  BrowserView::GetAccessiblePanes(panes);

  if (secondary_contents_web_view_ &&
      secondary_contents_web_view_->GetVisible()) {
    panes->push_back(secondary_contents_web_view_);
  }

  if (secondary_devtools_web_view_ &&
      secondary_devtools_web_view_->GetVisible()) {
    panes->push_back(secondary_devtools_web_view_);
  }
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

  UpdateContentsWebViewBorder();
}

TabSearchBubbleHost* BraveBrowserView::GetTabSearchBubbleHost() {
  if (!tabs::utils::ShouldShowVerticalTabs(browser())) {
    return BrowserView::GetTabSearchBubbleHost();
  }

  return vertical_tab_strip_widget_delegate_view_
      ->vertical_tab_strip_region_view()
      ->GetTabSearchBubbleHost();
}

void BraveBrowserView::OnActiveTabChanged(content::WebContents* old_contents,
                                          content::WebContents* new_contents,
                                          int index,
                                          int reason) {
  const bool supports_split_view =
      base::FeatureList::IsEnabled(tabs::features::kBraveSplitView) &&
      browser()->is_type_normal();
  bool need_to_update_secondary_web_view = false;
  if (supports_split_view) {
    // In order to minimize flickering during tab activation, we should update
    // split view only when it's needed.
    auto* browser_data = SplitViewBrowserData::FromBrowser(browser_.get());
    auto* tab_strip_model = browser_->tab_strip_model();
    if (auto tile =
            browser_data->GetTile(tab_strip_model->GetTabHandleAt(index))) {
      auto* main_web_contents = tab_strip_model->GetWebContentsAt(
          tab_strip_model->GetIndexOfTab(tile->first));
      auto* secondary_web_contents = tab_strip_model->GetWebContentsAt(
          tab_strip_model->GetIndexOfTab(tile->second));
      if (main_web_contents != new_contents) {
        std::swap(main_web_contents, secondary_web_contents);
      }

      need_to_update_secondary_web_view =
          contents_web_view_->web_contents() != main_web_contents ||
          secondary_contents_web_view_->web_contents() !=
              secondary_web_contents;
    } else {
      // Old contents was in a split view. We should hide split view.
      need_to_update_secondary_web_view =
          secondary_contents_web_view_->web_contents();
    }
  }

  if (need_to_update_secondary_web_view) {
    // This helps reduce flickering when switching between tiled tabs.
    contents_web_view_->SetFastResize(true);
    secondary_contents_web_view_->SetFastResize(true);

    if (!SplitViewBrowserData::FromBrowser(browser_.get())
             ->GetTile(browser_->tab_strip_model()->GetTabHandleAt(index))) {
      // This will help reduce flickering when switching to non tiled tab.
      UpdateSecondaryContentsWebViewVisibility();
    }

    secondary_contents_web_view_->SetWebContents(nullptr);
  }

  BrowserView::OnActiveTabChanged(old_contents, new_contents, index, reason);

  if (supports_split_view) {
    UpdateSplitViewSizeDelta(old_contents, new_contents);

    // Setting nullptr doesn't detach the previous contents.
    UpdateContentsWebViewVisual();

    if (need_to_update_secondary_web_view) {
      // Revert back to default state.
      contents_web_view_->SetFastResize(false);
      secondary_contents_web_view_->SetFastResize(false);
      contents_web_view_->DeprecatedLayoutImmediately();
      secondary_contents_web_view_->DeprecatedLayoutImmediately();
    }
  }
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
  return frame()->tab_drag_kind() == TabDragKind::kAllTabs;
}

bool BraveBrowserView::IsSidebarVisible() const {
  return sidebar_container_view_ && sidebar_container_view_->IsSidebarVisible();
}

BraveBrowser* BraveBrowserView::GetBraveBrowser() const {
  return static_cast<BraveBrowser*>(browser_.get());
}

void BraveBrowserView::UpdateWebViewRoundedCorners() {
  if (!BraveBrowser::ShouldUseBraveWebViewRoundedCorners(browser_.get())) {
    return;
  }

  gfx::RoundedCornersF corners(BraveContentsViewUtil::kBorderRadius);

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
  contents_container_->layer()->SetRoundedCornerRadius(corners);

  const auto in_split_view_mode =
      !!SplitViewBrowserData::FromBrowser(browser_.get());

  auto update_corner_radius = [in_split_view_mode](
                                  views::NativeViewHost* contents_holder,
                                  views::NativeViewHost* devtools_holder,
                                  DevToolsDockedPlacement devtools_placement,
                                  gfx::RoundedCornersF corners) {
    // In addition to giving the contents container rounded corners, we also
    // need to round the corners of the native view holder that displays the web
    // contents.

    // Devtools lies underneath the contents webview. Round all four corners.
    if (devtools_holder) {
      devtools_holder->SetCornerRadii(corners);
    }

    if (!in_split_view_mode) {
      // In order to make the contents web view and devtools appear to be
      // contained within a single rounded-corner view, square the contents
      // webview corners that are adjacent to devtools.
      // TODO(sko) We need to override
      // BrowserView::GetDevToolsDockedPlacement(). It depends on coordinate of
      // it but in split view mode, the calculation is not correct.
      switch (devtools_placement) {
        case DevToolsDockedPlacement::kLeft:
          corners.set_upper_left(0);
          corners.set_lower_left(0);
          break;
        case DevToolsDockedPlacement::kRight:
          corners.set_upper_right(0);
          corners.set_lower_right(0);
          break;
        case DevToolsDockedPlacement::kBottom:
          corners.set_lower_left(0);
          corners.set_lower_right(0);
          break;
        case DevToolsDockedPlacement::kNone:
          break;
        case DevToolsDockedPlacement::kUnknown:
          break;
      }
    }

    if (contents_holder) {
      contents_holder->SetCornerRadii(corners);
    }
  };

  update_corner_radius(contents_web_view_->holder(),
                       devtools_web_view_->holder(),
                       devtools_docked_placement(), corners);

  if (in_split_view_mode) {
    update_corner_radius(secondary_contents_web_view_->holder(),
                         secondary_devtools_web_view_->holder(),
                         DevToolsDockedPlacement::kNone, corners);
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
