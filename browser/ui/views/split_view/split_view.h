/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_SPLIT_VIEW_SPLIT_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_SPLIT_VIEW_SPLIT_VIEW_H_

#include <memory>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/memory/raw_ref.h"
#include "base/types/pass_key.h"
#include "brave/browser/ui/tabs/split_view_browser_data.h"
#include "brave/browser/ui/tabs/split_view_browser_data_observer.h"
#include "brave/browser/ui/views/split_view/split_view_layout_manager.h"
#include "brave/components/speedreader/common/buildflags/buildflags.h"
#include "chrome/browser/ui/exclusive_access/fullscreen_observer.h"
#include "chrome/browser/ui/views/frame/scrim_view.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/gfx/geometry/rounded_corners_f.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget_observer.h"

#if BUILDFLAG(ENABLE_SPEEDREADER)
#include "brave/browser/ui/views/speedreader/reader_mode_toolbar_view.h"
#endif

namespace content {
class WebContents;
}  // namespace content

namespace views {
class WebView;
}  // namespace views

class BraveBrowserView;
class Browser;
class ContentsWebView;
class DevToolsContentsResizingStrategy;
class FullscreenController;
class SplitViewLayoutManager;
class SplitViewLocationBar;
class SplitViewSeparator;

// Contains a pair of contents container view.
class SplitView : public views::View,
#if BUILDFLAG(ENABLE_SPEEDREADER)
                  public ReaderModeToolbarView::Delegate,
#endif
                  public views::WidgetObserver,
                  public FullscreenObserver,
                  public SplitViewBrowserDataObserver {
  METADATA_HEADER(SplitView, views::View)
 public:
  using BrowserViewKey = base::PassKey<BraveBrowserView>;

  static constexpr int kInactiveBorderThickness = 1;
  static constexpr int kBorderThickness = 2;

  SplitView(Browser& browser,
            views::View* contents_container,
            ContentsWebView* contents_web_view);

  ~SplitView() override;

  // true when active tab is in tile.
  bool IsSplitViewActive() const;

  void ListenFullscreenChanges();

  // Called before/after BrowserView::OnActiveTabChanged() as we have some
  // jobs such as reducing flickering while active tab changing. See the
  // comments of each methods for more details.
  void WillChangeActiveWebContents(BrowserViewKey,
                                   content::WebContents* old_contents,
                                   content::WebContents* new_contents);
  void DidChangeActiveWebContents(BrowserViewKey,
                                  content::WebContents* old_contents,
                                  content::WebContents* new_contents);

  // Called before/after BrowseView::UpdateDevToolsForContents() to avoid
  // holiding same WebContents from primary and secondary devtools webview.
  void WillUpdateDevToolsForActiveContents(BrowserViewKey);
  void DidUpdateDevToolsForActiveContents(BrowserViewKey);

  // Fills secondary web views if accessible.
  void GetAccessiblePanes(BrowserViewKey, std::vector<views::View*>* panes);

  // Update dev tools
  void UpdateSecondaryDevtoolsLayoutAndVisibility();

  views::View* secondary_contents_container() {
    return secondary_contents_container_;
  }

  ContentsWebView* secondary_contents_web_view() {
    return secondary_contents_web_view_;
  }

#if BUILDFLAG(ENABLE_SPEEDREADER)
  ReaderModeToolbarView* secondary_reader_mode_toolbar() {
    return secondary_reader_mode_toolbar_;
  }
  void OnReaderModeToolbarActivate(ReaderModeToolbarView* toolbar) override;
  void UpdateSecondaryReaderModeToolbarVisibility();
  void UpdateSecondaryReaderModeToolbar();
#endif

  void UpdateCornerRadius(const gfx::RoundedCornersF& corners);

  // Sets the contents resizing strategy.
  void SetSecondaryContentsResizingStrategy(
      const DevToolsContentsResizingStrategy& strategy);

  // views::View:
  void Layout(PassKey) override;
  void AddedToWidget() override;

  // SplitViewBrowserDataObserver:
  void OnTileTabs(const TabTile& tile) override;
  void OnDidBreakTile(const TabTile& tile) override;
  void OnSwapTabsInTile(const TabTile& tile) override;

  // views::WidgetObserver:
  void OnWidgetDestroying(views::Widget* widget) override;
  void OnWidgetWindowModalVisibilityChanged(views::Widget* widget,
                                            bool visible) override;

  // FullscreenObserver:
  void OnFullscreenStateChanged() override;

 private:
  friend class SplitViewBrowserTest;
  friend class SplitViewLocationBarBrowserTest;
  FRIEND_TEST_ALL_PREFIXES(SpeedReaderBrowserTest, SplitView);

  tabs::TabHandle GetActiveTabHandle() const;
  bool IsActiveWebContentsTiled(const TabTile& tile) const;
  bool IsWebContentsTiled(content::WebContents* contents) const;
  void UpdateSplitViewSizeDelta(content::WebContents* old_contents,
                                content::WebContents* new_contents);
  void UpdateContentsWebViewVisual();
  void UpdateContentsWebViewBorder();
  void UpdateSecondaryContentsWebViewVisibility();

  SplitViewLayoutManager* GetSplitViewLayoutManager();
  const SplitViewLayoutManager* GetSplitViewLayoutManager() const;
  bool ShouldHideSecondaryContentsByTabFullscreen() const;

  raw_ref<Browser> browser_;

  raw_ptr<views::View> contents_container_ = nullptr;
  raw_ptr<views::WebView> contents_web_view_ = nullptr;

  raw_ptr<views::View> secondary_contents_container_ = nullptr;
  raw_ptr<views::WebView> secondary_devtools_web_view_ = nullptr;
  raw_ptr<ContentsWebView> secondary_contents_web_view_ = nullptr;
  raw_ptr<ScrimView> secondary_contents_scrim_view_ = nullptr;
  raw_ptr<views::View> secondary_lens_overlay_view_ = nullptr;

#if BUILDFLAG(ENABLE_SPEEDREADER)
  raw_ptr<ReaderModeToolbarView> secondary_reader_mode_toolbar_ = nullptr;
#endif

  raw_ptr<SplitViewSeparator> split_view_separator_ = nullptr;

  std::unique_ptr<SplitViewLocationBar> secondary_location_bar_;
  std::unique_ptr<views::Widget> secondary_location_bar_widget_;

  base::ScopedObservation<SplitViewBrowserData, SplitViewBrowserDataObserver>
      split_view_observation_{this};
  base::ScopedObservation<views::Widget, views::WidgetObserver>
      widget_observation_{this};
  base::ScopedObservation<FullscreenController, FullscreenObserver>
      fullscreen_observation_{this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_SPLIT_VIEW_SPLIT_VIEW_H_
