/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_BROWSER_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_BROWSER_VIEW_H_

#include <memory>
#include <string>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/browser/ui/tabs/brave_tab_strip_model.h"
#include "brave/components/brave_vpn/buildflags/buildflags.h"
#include "chrome/browser/ui/views/frame/browser_view.h"

#if BUILDFLAG(ENABLE_BRAVE_VPN)
#include "brave/browser/ui/views/toolbar/brave_vpn_panel_controller.h"
#endif

#if BUILDFLAG(ENABLE_SPEEDREADER)
#include "brave/browser/ui/webui/speedreader/speedreader_panel_ui.h"
#include "chrome/browser/ui/views/bubble/webui_bubble_manager.h"
#endif

namespace speedreader {
class SpeedreaderBubbleView;
class SpeedreaderTabHelper;
}  // namespace speedreader

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

class BraveBrowserView : public BrowserView {
 public:
  explicit BraveBrowserView(std::unique_ptr<Browser> browser);
  BraveBrowserView(const BraveBrowserView&) = delete;
  BraveBrowserView& operator=(const BraveBrowserView&) = delete;
  ~BraveBrowserView() override;

  void SetStarredState(bool is_starred) override;
  void ShowUpdateChromeDialog() override;
  ShowTranslateBubbleResult ShowTranslateBubble(
      content::WebContents* web_contents,
      translate::TranslateStep step,
      const std::string& source_language,
      const std::string& target_language,
      translate::TranslateErrors error_type,
      bool is_user_gesture) override;
  speedreader::SpeedreaderBubbleView* ShowSpeedreaderBubble(
      speedreader::SpeedreaderTabHelper* tab_helper,
      bool is_enabled) override;
  void CreateWalletBubble();
  void CreateApproveWalletBubble();
  void CloseWalletBubble();
  WalletButton* GetWalletButton();
  views::View* GetWalletButtonAnchorView();
  void StartTabCycling() override;
  views::View* GetAnchorViewForBraveVPNPanel();
  gfx::Rect GetShieldsBubbleRect() override;
  void ShowSpeedreaderWebUIBubble(Browser* browser) override;
  void HideSpeedreaderWebUIBubble() override;
  bool GetTabStripVisible() const override;

  views::View* sidebar_host_view() { return sidebar_host_view_; }

 private:
  class TabCyclingEventHandler;
  friend class WindowClosingConfirmBrowserTest;
  friend class sidebar::SidebarBrowserTest;

  static void SetDownloadConfirmReturnForTesting(bool allow);

  // BrowserView overrides:
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

  void StopTabCycling();
  void UpdateSearchTabsButtonState();
  void OnPreferenceChanged(const std::string& pref_name);
  void OnWindowClosingConfirmResponse(bool allowed_to_close);
  BraveBrowser* GetBraveBrowser() const;

  sidebar::Sidebar* InitSidebar() override;
  void UpdateSideBarHorizontalAlignment();

  raw_ptr<SidebarContainerView> sidebar_container_view_ = nullptr;
  raw_ptr<views::View> sidebar_host_view_ = nullptr;
  raw_ptr<views::View> vertical_tabs_container_ = nullptr;

#if BUILDFLAG(ENABLE_BRAVE_VPN)
  BraveVPNPanelController vpn_panel_controller_{this};
#endif

#if BUILDFLAG(ENABLE_SPEEDREADER)
  std::unique_ptr<WebUIBubbleManagerT<SpeedreaderPanelUI>>
      speedreader_webui_bubble_manager_;
#endif

  std::unique_ptr<TabCyclingEventHandler> tab_cycling_event_handler_;
  PrefChangeRegistrar pref_change_registrar_;

  base::WeakPtrFactory<BraveBrowserView> weak_ptr_{this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_BROWSER_VIEW_H_
