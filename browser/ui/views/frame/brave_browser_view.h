/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_BROWSER_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_BROWSER_VIEW_H_

#include <memory>
#include <string>

#include "brave/browser/ui/tabs/brave_tab_strip_model.h"
#include "brave/components/sidebar/buildflags/buildflags.h"
#include "chrome/browser/ui/views/frame/browser_view.h"

#if BUILDFLAG(ENABLE_SIDEBAR)
class ContentsLayoutManager;
class SidebarContainerView;
#endif

class WalletButton;

class BraveBrowserView : public BrowserView {
 public:
  explicit BraveBrowserView(std::unique_ptr<Browser> browser);
  ~BraveBrowserView() override;

  void SetStarredState(bool is_starred) override;
  void ShowUpdateChromeDialog() override;
  ShowTranslateBubbleResult ShowTranslateBubble(
      content::WebContents* web_contents,
      translate::TranslateStep step,
      const std::string& source_language,
      const std::string& target_language,
      translate::TranslateErrors::Type error_type,
      bool is_user_gesture) override;
  void CreateWalletBubble();
  void CloseWalletBubble();
  WalletButton* GetWalletButton();
  void StartTabCycling() override;

 private:
  class TabCyclingEventHandler;

  // BrowserView overrides:
  void OnTabStripModelChanged(
      TabStripModel* tab_strip_model,
      const TabStripModelChange& change,
      const TabStripSelectionChange& selection) override;

  void StopTabCycling();

#if BUILDFLAG(ENABLE_SIDEBAR)
  sidebar::Sidebar* InitSidebar() override;
  ContentsLayoutManager* GetContentsLayoutManager() const override;

  // If sidebar is enabled, |BrowserView::contents_container_| points to
  // |brave_contents_container_| that includes sidebar and contents container.
  // |original_contents_container_| points to original contents container that
  // includes contents & devtools webview. It's used by
  // GetContentsLayoutManager().
  views::View* original_contents_container_ = nullptr;
  SidebarContainerView* sidebar_container_view_ = nullptr;
#endif

  std::unique_ptr<TabCyclingEventHandler> tab_cycling_event_handler_;

  DISALLOW_COPY_AND_ASSIGN(BraveBrowserView);
};

#endif  // BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_BROWSER_VIEW_H_
