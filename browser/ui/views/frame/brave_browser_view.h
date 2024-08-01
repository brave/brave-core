/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_BROWSER_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_BROWSER_VIEW_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/gtest_prod_util.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/scoped_observation.h"
#include "brave/browser/ui/commands/accelerator_service.h"
#include "brave/browser/ui/tabs/brave_tab_strip_model.h"
#include "brave/browser/ui/tabs/split_view_browser_data.h"
#include "brave/browser/ui/tabs/split_view_browser_data_observer.h"
#include "brave/browser/ui/views/split_view/split_view_separator.h"
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "brave/components/brave_wayback_machine/buildflags/buildflags.h"
#include "brave/components/commands/browser/accelerator_pref_manager.h"
#include "build/build_config.h"
#include "chrome/browser/ui/tabs/tab_model.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/frame/contents_web_view.h"
#include "ui/base/accelerators/accelerator.h"
#include "ui/base/metadata/metadata_header_macros.h"

#if BUILDFLAG(ENABLE_BRAVE_VPN)
#include "brave/browser/ui/views/toolbar/brave_vpn_panel_controller.h"
#endif

#if BUILDFLAG(ENABLE_SPEEDREADER)
#include "brave/browser/ui/views/speedreader/reader_mode_toolbar_view.h"
#endif

#if BUILDFLAG(ENABLE_SPEEDREADER)
namespace speedreader {
class SpeedreaderBubbleView;
class SpeedreaderTabHelper;
enum class SpeedreaderBubbleLocation : int;
}  // namespace speedreader
#endif

namespace content {
class WebContents;
}  // namespace content

namespace sidebar {
class SidebarBrowserTest;
}  // namespace sidebar

class BraveBrowser;
class ContentsLayoutManager;
class SidebarContainerView;
class WalletButton;
class ViewShadow;
class VerticalTabStripWidgetDelegateView;
class BraveHelpBubbleHostView;
class SplitViewSeparator;

class BraveBrowserView : public BrowserView,
                         public commands::AcceleratorService::Observer,
                         public SplitViewBrowserDataObserver {
  METADATA_HEADER(BraveBrowserView, BrowserView)
 public:
  explicit BraveBrowserView(std::unique_ptr<Browser> browser);
  BraveBrowserView(const BraveBrowserView&) = delete;
  BraveBrowserView& operator=(const BraveBrowserView&) = delete;
  ~BraveBrowserView() override;

  void SetStarredState(bool is_starred) override;
  void ShowUpdateChromeDialog() override;
  void CreateWalletBubble();
  void CreateApproveWalletBubble();
  void CloseWalletBubble();
  WalletButton* GetWalletButton();
  views::View* GetWalletButtonAnchorView();

  // Triggers layout of web modal dialogs
  void NotifyDialogPositionRequiresUpdate();

  // BrowserView overrides:
  void Layout(PassKey) override;
  void StartTabCycling() override;
  views::View* GetAnchorViewForBraveVPNPanel();
  gfx::Rect GetShieldsBubbleRect() override;
#if BUILDFLAG(ENABLE_SPEEDREADER)
  speedreader::SpeedreaderBubbleView* ShowSpeedreaderBubble(
      speedreader::SpeedreaderTabHelper* tab_helper,
      speedreader::SpeedreaderBubbleLocation location) override;
  void ShowReaderModeToolbar() override;
  void HideReaderModeToolbar() override;
#endif
  bool GetTabStripVisible() const override;
#if BUILDFLAG(IS_WIN)
  bool GetSupportsTitle() const override;
#endif
  bool ShouldShowWindowTitle() const override;
  void OnThemeChanged() override;
  TabSearchBubbleHost* GetTabSearchBubbleHost() override;
  void OnActiveTabChanged(content::WebContents* old_contents,
                          content::WebContents* new_contents,
                          int index,
                          int reason) override;
  bool AcceleratorPressed(const ui::Accelerator& accelerator) override;
  bool IsInTabDragging() const override;

#if defined(USE_AURA)
  views::View* sidebar_host_view() { return sidebar_host_view_; }
#endif

  bool IsSidebarVisible() const;
  void SetSidePanelOperationByActiveTabChange(bool tab_change);

  VerticalTabStripWidgetDelegateView*
  vertical_tab_strip_widget_delegate_view() {
    return vertical_tab_strip_widget_delegate_view_;
  }
  bool ShowBraveHelpBubbleView(const std::string& text) override;

  // commands::AcceleratorService:
  void OnAcceleratorsChanged(const commands::Accelerators& changed) override;

  // SplitViewBrowserDataObserver:
  void OnTileTabs(const SplitViewBrowserData::Tile& tile) override;
  void OnWillBreakTile(const SplitViewBrowserData::Tile& tile) override;
  void OnSwapTabsInTile(const SplitViewBrowserData::Tile& tile) override;

  views::WebView* secondary_contents_web_view() {
    return secondary_contents_web_view_.get();
  }

 private:
  class TabCyclingEventHandler;
  friend class WindowClosingConfirmBrowserTest;
  friend class sidebar::SidebarBrowserTest;
  friend class VerticalTabStripDragAndDropBrowserTest;
  friend class SplitViewBrowserTest;

  FRIEND_TEST_ALL_PREFIXES(VerticalTabStripBrowserTest, VisualState);
  FRIEND_TEST_ALL_PREFIXES(VerticalTabStripBrowserTest, Fullscreen);
  FRIEND_TEST_ALL_PREFIXES(VerticalTabStripDragAndDropBrowserTest,
                           DragTabToReorder);
  FRIEND_TEST_ALL_PREFIXES(SpeedReaderBrowserTest, Toolbar);
  FRIEND_TEST_ALL_PREFIXES(SpeedReaderBrowserTest, ToolbarLangs);
  FRIEND_TEST_ALL_PREFIXES(VerticalTabStripBrowserTest, ExpandedState);
  FRIEND_TEST_ALL_PREFIXES(VerticalTabStripBrowserTest, ExpandedWidth);

  static void SetDownloadConfirmReturnForTesting(bool allow);

  // BrowserView overrides:
  void AddedToWidget() override;
  void LoadAccelerators() override;
  void OnTabStripModelChanged(
      TabStripModel* tab_strip_model,
      const TabStripModelChange& change,
      const TabStripSelectionChange& selection) override;
  void ShowBraveVPNBubble() override;
  views::CloseRequestResult OnWindowCloseRequested() override;
  void ConfirmBrowserCloseWithPendingDownloads(
      int download_count,
      Browser::DownloadCloseType dialog_type,
      base::OnceCallback<void(bool)> callback) override;
  void MaybeShowReadingListInSidePanelIPH() override;
  void UpdateDevToolsForContents(content::WebContents* web_contents,
                                 bool update_devtools_web_contents) override;
  void OnWidgetActivationChanged(views::Widget* widget, bool active) override;
  void GetAccessiblePanes(std::vector<views::View*>* panes) override;

  void StopTabCycling();
  void UpdateSearchTabsButtonState();
  void OnPreferenceChanged(const std::string& pref_name);
  void OnWindowClosingConfirmResponse(bool allowed_to_close);
  BraveBrowser* GetBraveBrowser() const;
  void UpdateWebViewRoundedCorners();

  sidebar::Sidebar* InitSidebar() override;
  void ToggleSidebar() override;
  bool HasSelectedURL() const override;
  void CleanAndCopySelectedURL() override;

#if BUILDFLAG(ENABLE_PLAYLIST_WEBUI)
  void ShowPlaylistBubble() override;
#endif

#if BUILDFLAG(ENABLE_BRAVE_WAYBACK_MACHINE)
  void ShowWaybackMachineBubble() override;
#endif

  void UpdateSideBarHorizontalAlignment();

  tabs::TabHandle GetActiveTabHandle() const;
  bool IsActiveWebContentsTiled(const SplitViewBrowserData::Tile& tile) const;
  void UpdateSplitViewSizeDelta(content::WebContents* old_contents,
                                content::WebContents* new_contents);
  void UpdateContentsWebViewVisual();
  void UpdateContentsWebViewBorder();
  void UpdateSecondaryContentsWebViewVisibility();
  void UpdateSecondaryDevtoolsLayoutAndVisibility(
      content::WebContents* inspected_contents);

  bool closing_confirm_dialog_activated_ = false;
  raw_ptr<BraveHelpBubbleHostView> brave_help_bubble_host_view_ = nullptr;
  raw_ptr<SidebarContainerView> sidebar_container_view_ = nullptr;
  raw_ptr<views::View> sidebar_separator_view_ = nullptr;
  raw_ptr<views::View> contents_background_view_ = nullptr;
  raw_ptr<views::View> vertical_tab_strip_host_view_ = nullptr;
  raw_ptr<VerticalTabStripWidgetDelegateView>
      vertical_tab_strip_widget_delegate_view_ = nullptr;

#if defined(USE_AURA)
  raw_ptr<views::View> sidebar_host_view_ = nullptr;
#endif

#if BUILDFLAG(ENABLE_BRAVE_VPN)
  BraveVPNPanelController vpn_panel_controller_{this};
#endif

#if BUILDFLAG(ENABLE_SPEEDREADER)
  std::unique_ptr<ReaderModeToolbarView> reader_mode_toolbar_view_;
#endif

  std::unique_ptr<TabCyclingEventHandler> tab_cycling_event_handler_;
  std::unique_ptr<ViewShadow> contents_shadow_;

  raw_ptr<views::WebView> secondary_devtools_web_view_ = nullptr;
  raw_ptr<ContentsWebView> secondary_contents_web_view_ = nullptr;
  raw_ptr<SplitViewSeparator> split_view_separator_ = nullptr;

  PrefChangeRegistrar pref_change_registrar_;
  base::ScopedObservation<commands::AcceleratorService,
                          commands::AcceleratorService::Observer>
      accelerators_observation_{this};

  base::ScopedObservation<SplitViewBrowserData, SplitViewBrowserDataObserver>
      split_view_observation_{this};

  base::WeakPtrFactory<BraveBrowserView> weak_ptr_{this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_BROWSER_VIEW_H_
