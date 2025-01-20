/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/split_view/split_view.h"

#include <utility>

#include "base/types/to_address.h"
#include "brave/browser/ui/brave_browser.h"
#include "brave/browser/ui/color/brave_color_id.h"
#include "brave/browser/ui/tabs/features.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "brave/browser/ui/views/frame/brave_contents_view_util.h"
#include "brave/browser/ui/views/split_view/split_view_layout_manager.h"
#include "brave/browser/ui/views/split_view/split_view_location_bar.h"
#include "brave/browser/ui/views/split_view/split_view_separator.h"
#include "brave/ui/color/nala/nala_color_id.h"
#include "chrome/browser/devtools/devtools_window.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/views/frame/contents_layout_manager.h"
#include "chrome/browser/ui/views/frame/contents_web_view.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/views/background.h"
#include "ui/views/border.h"
#include "ui/views/controls/webview/webview.h"
#include "ui/views/layout/fill_layout.h"

namespace {

// A `ContentsWebView` that activates its contents when it gets focus.
class ActivatableContentsWebView : public ContentsWebView {
  METADATA_HEADER(ActivatableContentsWebView, ContentsWebView)
 public:
  using ContentsWebView::ContentsWebView;
  ~ActivatableContentsWebView() override = default;

  // ContentsWebView:
  void OnFocus() override {
    ContentsWebView::OnFocus();

    // Only activate if this focus comes from direct request such as clicking
    // over web contents. Except that case, we should not make this focus change
    // affect active tab state. TabStripModel will do it.
    if (web_contents() && web_contents()->GetDelegate() && GetFocusManager() &&
        GetFocusManager()->focus_change_reason() ==
            views::FocusManager::FocusChangeReason::kDirectFocusChange) {
      web_contents()->GetDelegate()->ActivateContents(web_contents());
    }
  }
};
BEGIN_METADATA(ActivatableContentsWebView)
END_METADATA

}  // namespace

SplitView::SplitView(Browser& browser,
                     views::View* contents_container,
                     ContentsWebView* contents_web_view)
    : browser_(browser),
      contents_container_(contents_container),
      contents_web_view_(contents_web_view) {
  CHECK(base::FeatureList::IsEnabled(tabs::features::kBraveSplitView));
  SplitViewBrowserData::CreateForBrowser(&browser_.get());

  // Re-parent the |contents_container| to this view.
  AddChildView(
      contents_container->parent()->RemoveChildViewT(contents_container));

  // Make secondary contents view and related views to support split view mode
  secondary_contents_container_ = AddChildView(std::make_unique<views::View>());
  secondary_contents_container_->SetVisible(false);

  secondary_devtools_web_view_ = secondary_contents_container_->AddChildView(
      std::make_unique<views::WebView>(browser_->profile()));
  secondary_contents_web_view_ = secondary_contents_container_->AddChildView(
      std::make_unique<ActivatableContentsWebView>(browser_->profile()));

  split_view_separator_ = AddChildView(
      std::make_unique<SplitViewSeparator>(base::to_address(browser_)));

  secondary_contents_container_->SetLayoutManager(
      std::make_unique<ContentsLayoutManager>(secondary_devtools_web_view_,
                                              secondary_contents_web_view_));

  SetLayoutManager(std::make_unique<SplitViewLayoutManager>(
      contents_container_, secondary_contents_container_,
      split_view_separator_));

  auto* split_view_browser_data =
      SplitViewBrowserData::FromBrowser(base::to_address(browser_));
  split_view_observation_.Observe(split_view_browser_data);
}

SplitView::~SplitView() = default;

void SplitView::WillChangeActiveWebContents(
    BrowserViewKey,
    content::WebContents* old_contents,
    content::WebContents* new_contents) {
  // Early return if this active state changes is not related with split view.
  // |secondary_contents_container_| is not visible if previous active contents
  // is not in tile.
  if (!secondary_contents_container_->GetVisible() &&
      !IsWebContentsTiled(new_contents)) {
    // In this state, we don't need to call DidChangeActiveWebContents() after
    // changing primary WebContents but it's ok as it's no-op.
    // Otherwise, we need to use another flag to avoid calling it.
    return;
  }

  // This helps reduce flickering when switching between tiled tabs.
  contents_web_view_->SetFastResize(true);
  secondary_contents_web_view_->SetFastResize(true);

  if (!IsWebContentsTiled(new_contents)) {
    // This will help reduce flickering when switching to non tiled tab by
    // hiding secondary web view before detaching web contents.
    UpdateSecondaryContentsWebViewVisibility();
  }

  // WebContents in secondary webview could be used by primary when active tab
  // changes. As same webcontents could not be hold by multiple webview, it
  // should be cleared WebContents from secondary webview in advance before
  // active tab changes. Secondary WebContents will be set again via
  // DidChangeActiveWebContents() after BrowserView::OnActiveTabChanged()
  // called.
  secondary_contents_web_view_->SetWebContents(nullptr);
}

void SplitView::DidChangeActiveWebContents(BrowserViewKey,
                                           content::WebContents* old_contents,
                                           content::WebContents* new_contents) {
  // Update secondary webview & UI after changing active WebContents.
  UpdateSplitViewSizeDelta(old_contents, new_contents);
  UpdateContentsWebViewVisual();

  // Revert back to default state.
  contents_web_view_->SetFastResize(false);
  secondary_contents_web_view_->SetFastResize(false);
  InvalidateLayout();
}

void SplitView::WillUpdateDevToolsForActiveContents(BrowserViewKey) {
  // WebContents in secondary devtools webview could be used by primary's when
  // active tab changes. As same WebContents could not be hold by multiple
  // webview, it should be cleared WebContents from secondary devtools webview
  // in advance before active tab changes. Secondary devtools's WebContents will
  // be set again via DidUpdateDevToolsForActiveContents() after
  // BrowserView::UpdateDevToolsForContents() called.
  secondary_devtools_web_view_->SetWebContents(nullptr);
}

void SplitView::DidUpdateDevToolsForActiveContents(BrowserViewKey) {
  if (secondary_contents_container_->GetVisible()) {
    UpdateSecondaryDevtoolsLayoutAndVisibility();
  }
}

void SplitView::GetAccessiblePanes(BrowserViewKey,
                                   std::vector<views::View*>* panes) {
  if (!secondary_contents_container_->GetVisible()) {
    return;
  }

  if (secondary_contents_web_view_ &&
      secondary_contents_web_view_->GetVisible()) {
    panes->push_back(secondary_contents_web_view_);
  }

  if (secondary_devtools_web_view_ &&
      secondary_devtools_web_view_->GetVisible()) {
    panes->push_back(secondary_devtools_web_view_);
  }
}

void SplitView::SetSecondaryContentsResizingStrategy(
    const DevToolsContentsResizingStrategy& strategy) {
  static_cast<ContentsLayoutManager*>(
      secondary_contents_container_->GetLayoutManager())
      ->SetContentsResizingStrategy(strategy);
}

void SplitView::OnThemeChanged() {
  View::OnThemeChanged();
  UpdateContentsWebViewBorder();
}

void SplitView::Layout(PassKey key) {
  LayoutSuperclass<views::View>(this);

  auto* browser_view = static_cast<BraveBrowserView*>(browser_->window());
  if (!browser_view) {
    // This can happen on start up
    return;
  }

  browser_view->NotifyDialogPositionRequiresUpdate();
}

void SplitView::AddedToWidget() {
  secondary_location_bar_ = std::make_unique<SplitViewLocationBar>(
      browser_->profile()->GetPrefs(), secondary_contents_container_);
  secondary_location_bar_widget_ = std::make_unique<views::Widget>();

  secondary_location_bar_widget_->Init(
      SplitViewLocationBar::GetWidgetInitParams(GetWidget()->GetNativeView(),
                                                secondary_location_bar_.get()));

  // Initialize secondary view state.
  UpdateSecondaryContentsWebViewVisibility();
}

void SplitView::OnTileTabs(const TabTile& tile) {
  if (!IsActiveWebContentsTiled(tile)) {
    return;
  }

  UpdateContentsWebViewVisual();
}

void SplitView::OnDidBreakTile(const TabTile& tile) {
  if (!IsActiveWebContentsTiled(tile)) {
    return;
  }

  UpdateContentsWebViewVisual();
}

void SplitView::OnSwapTabsInTile(const TabTile& tile) {
  if (!IsActiveWebContentsTiled(tile)) {
    return;
  }

  UpdateSecondaryContentsWebViewVisibility();
}

tabs::TabHandle SplitView::GetActiveTabHandle() const {
  auto* model = browser_->tab_strip_model();
  if (model->empty()) {
    return {};
  }
  return model->GetTabHandleAt(model->active_index());
}

bool SplitView::IsActiveWebContentsTiled(const TabTile& tile) const {
  auto active_tab_handle = GetActiveTabHandle();
  return tile.first == active_tab_handle || tile.second == active_tab_handle;
}

bool SplitView::IsWebContentsTiled(content::WebContents* contents) const {
  const int tab_index =
      browser_->tab_strip_model()->GetIndexOfWebContents(contents);
  if (tab_index == TabStripModel::kNoTab) {
    return false;
  }
  const auto tab_handle =
      browser_->tab_strip_model()->GetTabHandleAt(tab_index);
  return SplitViewBrowserData::FromBrowser(base::to_address(browser_))
      ->IsTabTiled(tab_handle);
}

void SplitView::UpdateSplitViewSizeDelta(content::WebContents* old_contents,
                                         content::WebContents* new_contents) {
  auto get_index_of = [this](content::WebContents* contents) {
    return browser_->tab_strip_model()->GetIndexOfWebContents(contents);
  };
  if (get_index_of(old_contents) == TabStripModel::kNoTab ||
      get_index_of(new_contents) == TabStripModel::kNoTab) {
    // This can happen on start-up or closing a tab.
    return;
  }

  auto* split_view_browser_data =
      SplitViewBrowserData::FromBrowser(base::to_address(browser_));
  auto get_tab_handle = [this, &get_index_of](content::WebContents* contents) {
    return browser_->tab_strip_model()->GetTabHandleAt(get_index_of(contents));
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

  auto* split_view_layout_manager =
      static_cast<SplitViewLayoutManager*>(GetLayoutManager());
  if (old_tab_tile) {
    split_view_browser_data->SetSizeDelta(
        old_tab_handle, split_view_layout_manager->split_view_size_delta());
  }

  if (new_tab_tile) {
    split_view_layout_manager->set_split_view_size_delta(
        split_view_browser_data->GetSizeDelta(new_tab_handle));
  }
}

void SplitView::UpdateContentsWebViewVisual() {
  auto* split_view_browser_data =
      SplitViewBrowserData::FromBrowser(base::to_address(browser_));
  if (!split_view_browser_data) {
    return;
  }

  UpdateContentsWebViewBorder();
  UpdateSecondaryContentsWebViewVisibility();
}

void SplitView::UpdateContentsWebViewBorder() {
  auto* split_view_browser_data =
      SplitViewBrowserData::FromBrowser(base::to_address(browser_));
  if (!split_view_browser_data) {
    return;
  }

  if (browser_->tab_strip_model()->empty()) {
    // Happens on startup
    return;
  }

  if (browser_->IsBrowserClosing()) {
    return;
  }

  DCHECK(split_view_browser_data);

  if (split_view_browser_data->GetTile(GetActiveTabHandle())) {
    auto create_border = [this](SkColor color) {
      constexpr int kBorderThickness = 2;
      return BraveBrowser::ShouldUseBraveWebViewRoundedCorners(
                 base::to_address(browser_))
                 ? views::CreateRoundedRectBorder(
                       kBorderThickness,
                       BraveContentsViewUtil::kBorderRadius +
                           kBorderThickness / 2,
                       color)
                 : views::CreateSolidBorder(kBorderThickness, color);
    };

    if (auto* cp = GetColorProvider()) {
      contents_web_view_->SetBorder(
          create_border(cp->GetColor(nala::kColorPrimitivePrimary70)));

      secondary_contents_web_view_->SetBorder(create_border(
          cp->GetColor(kColorBraveSplitViewInactiveWebViewBorder)));
    }
  } else {
    contents_web_view_->SetBorder(nullptr);
    secondary_contents_web_view_->SetBorder(nullptr);
  }
}

void SplitView::UpdateSecondaryContentsWebViewVisibility() {
  if (browser_->IsBrowserClosing()) {
    secondary_contents_web_view_->SetWebContents(nullptr);
    return;
  }

  auto* split_view_browser_data =
      SplitViewBrowserData::FromBrowser(base::to_address(browser_));
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
    // SplitViewLayoutManager.
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
    auto* model = browser_->tab_strip_model();
    auto* contents = model->GetWebContentsAt(model->GetIndexOfTab(
        second_tile_is_active_web_contents ? tile->first : tile->second));
    CHECK_NE(contents, contents_web_view_->web_contents());
    if (secondary_contents_web_view_->web_contents() != contents) {
      secondary_contents_web_view_->SetWebContents(contents);
      secondary_location_bar_->SetWebContents(contents);
    }

    secondary_contents_container_->SetVisible(true);
    UpdateSecondaryDevtoolsLayoutAndVisibility();

    GetSplitViewLayoutManager()->show_main_web_contents_at_tail(
        second_tile_is_active_web_contents);
  } else {
    secondary_location_bar_->SetWebContents(nullptr);
    secondary_contents_web_view_->SetWebContents(nullptr);
    secondary_devtools_web_view_->SetWebContents(nullptr);
    secondary_contents_container_->SetVisible(false);
  }

  split_view_separator_->SetVisible(
      secondary_contents_container_->GetVisible());

  InvalidateLayout();
}

void SplitView::UpdateCornerRadius(const gfx::RoundedCornersF& corners) {
  secondary_contents_web_view_->holder()->SetCornerRadii(corners);
  secondary_devtools_web_view_->holder()->SetCornerRadii(corners);
}

void SplitView::UpdateSecondaryDevtoolsLayoutAndVisibility() {
  DevToolsContentsResizingStrategy strategy;
  content::WebContents* devtools = DevToolsWindow::GetInTabWebContents(
      secondary_contents_web_view_->web_contents(), &strategy);

  if (secondary_devtools_web_view_->web_contents() != devtools) {
    secondary_devtools_web_view_->SetWebContents(devtools);
  }

  if (devtools) {
    secondary_devtools_web_view_->SetVisible(true);
    SetSecondaryContentsResizingStrategy(strategy);
  } else {
    secondary_devtools_web_view_->SetVisible(false);
    SetSecondaryContentsResizingStrategy(DevToolsContentsResizingStrategy());
  }
}

SplitViewLayoutManager* SplitView::GetSplitViewLayoutManager() {
  return const_cast<SplitViewLayoutManager*>(
      const_cast<const SplitView*>(this)->GetSplitViewLayoutManager());
}

const SplitViewLayoutManager* SplitView::GetSplitViewLayoutManager() const {
  return static_cast<SplitViewLayoutManager*>(GetLayoutManager());
}

BEGIN_METADATA(SplitView)
END_METADATA
