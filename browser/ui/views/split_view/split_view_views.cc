/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/split_view/split_view_views.h"

#include <utility>

#include "base/types/to_address.h"
#include "brave/browser/ui/brave_browser.h"
#include "brave/browser/ui/color/brave_color_id.h"
#include "brave/browser/ui/split_view/split_view_controller.h"
#include "brave/browser/ui/tabs/features.h"
#include "brave/browser/ui/tabs/split_view_browser_data.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "brave/browser/ui/views/frame/brave_contents_layout_manager.h"
#include "brave/browser/ui/views/frame/brave_contents_view_util.h"
#include "brave/browser/ui/views/split_view/split_view_layout_manager.h"
#include "brave/browser/ui/views/split_view/split_view_location_bar.h"
#include "brave/browser/ui/views/split_view/split_view_separator.h"
#include "brave/ui/color/nala/nala_color_id.h"
#include "chrome/browser/devtools/devtools_window.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/exclusive_access/exclusive_access_manager.h"
#include "chrome/browser/ui/exclusive_access/fullscreen_controller.h"
#include "chrome/browser/ui/ui_features.h"
#include "chrome/browser/ui/views/frame/contents_layout_manager.h"
#include "chrome/browser/ui/views/frame/contents_web_view.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/compositor/layer.h"
#include "ui/views/background.h"
#include "ui/views/border.h"
#include "ui/views/controls/webview/webview.h"
#include "ui/views/painter.h"

#if BUILDFLAG(ENABLE_SPEEDREADER)
#include "brave/browser/speedreader/speedreader_tab_helper.h"
#endif

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

#if BUILDFLAG(ENABLE_SPEEDREADER)

bool IsTabDistilled(content::WebContents* web_contents) {
  if (!web_contents) {
    return false;
  }
  if (auto* th =
          speedreader::SpeedreaderTabHelper::FromWebContents(web_contents)) {
    return speedreader::DistillStates::IsDistilled(th->PageDistillState());
  }
  return false;
}

#endif

}  // namespace

SplitViewViews::SplitViewViews(Browser& browser,
                               views::View* contents_container,
                               ContentsWebView* contents_web_view)
    : browser_(browser),
      contents_container_(contents_container),
      contents_web_view_(contents_web_view) {
  CHECK(base::FeatureList::IsEnabled(tabs::features::kBraveSplitView));

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
  secondary_contents_scrim_view_ = secondary_contents_container_->AddChildView(
      std::make_unique<ScrimView>());

  secondary_lens_overlay_view_ = secondary_contents_container_->AddChildView(
      std::make_unique<views::View>());
  secondary_lens_overlay_view_->SetVisible(false);

  split_view_separator_ = AddChildView(
      std::make_unique<SplitViewSeparator>(base::to_address(browser_)));

#if BUILDFLAG(ENABLE_SPEEDREADER)
  secondary_reader_mode_toolbar_ = secondary_contents_container_->AddChildView(
      std::make_unique<ReaderModeToolbarView>(
          browser_->profile(),
          BraveBrowser::ShouldUseBraveWebViewRoundedCorners(
              base::to_address(browser_))));

  secondary_reader_mode_toolbar_->SetDelegate(this);

  secondary_contents_container_->SetLayoutManager(
      std::make_unique<BraveContentsLayoutManager>(
          secondary_devtools_web_view_, secondary_contents_web_view_,
          secondary_lens_overlay_view_, secondary_contents_scrim_view_, nullptr,
          nullptr, secondary_reader_mode_toolbar_));
#endif

  SetLayoutManager(std::make_unique<SplitViewLayoutManager>(
      contents_container_, secondary_contents_container_,
      split_view_separator_));

  CHECK(browser_->GetFeatures().split_view_controller());
  browser_->GetFeatures().split_view_controller()->set_split_view(this);
}

SplitViewViews::~SplitViewViews() {
  browser_->GetFeatures().split_view_controller()->set_split_view(nullptr);
}

void SplitViewViews::ListenFullscreenChanges() {
  fullscreen_observation_.Observe(
      browser_->exclusive_access_manager()->fullscreen_controller());
}

void SplitViewViews::WillChangeActiveWebContents(
    BrowserViewKey,
    content::WebContents* old_contents,
    content::WebContents* new_contents) {
  auto* controller = browser_->GetFeatures().split_view_controller();
  // Early return if this active state changes is not related with split view.
  // |secondary_contents_container_| is not visible if previous active contents
  // is not shown in split view.
  if (!secondary_contents_container_->GetVisible() &&
      !controller->IsOpenedFor(new_contents)) {
    // In this state, we don't need to call DidChangeActiveWebContents() after
    // changing primary WebContents but it's ok as it's no-op.
    // Otherwise, we need to use another flag to avoid calling it.
    return;
  }

  // This helps reduce flickering when switching between tabs in split view.
  contents_web_view_->SetFastResize(true);
  secondary_contents_web_view_->SetFastResize(true);

  if (!controller->IsOpenedFor(new_contents)) {
    // This will help reduce flickering when switching to non split view tab by
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

void SplitViewViews::DidChangeActiveWebContents(
    BrowserViewKey,
    content::WebContents* old_contents,
    content::WebContents* new_contents) {
  // Update secondary webview & UI after changing active WebContents.
  UpdateSplitViewSizeDelta(old_contents, new_contents);
  UpdateContentsWebViewVisual();

#if BUILDFLAG(ENABLE_SPEEDREADER)
  UpdateSecondaryReaderModeToolbar();
#endif

  // Revert back to default state.
  contents_web_view_->SetFastResize(false);
  secondary_contents_web_view_->SetFastResize(false);
  InvalidateLayout();
}

void SplitViewViews::WillUpdateDevToolsForActiveContents(BrowserViewKey) {
  // WebContents in secondary devtools webview could be used by primary's when
  // active tab changes. As same WebContents could not be hold by multiple
  // webview, it should be cleared WebContents from secondary devtools webview
  // in advance before active tab changes. Secondary devtools's WebContents will
  // be set again via DidUpdateDevToolsForActiveContents() after
  // BrowserView::UpdateDevToolsForContents() called.
  secondary_devtools_web_view_->SetWebContents(nullptr);
}

void SplitViewViews::DidUpdateDevToolsForActiveContents(BrowserViewKey) {
  if (secondary_contents_container_->GetVisible()) {
    UpdateSecondaryDevtoolsLayoutAndVisibility();
  }
}

void SplitViewViews::GetAccessiblePanes(BrowserViewKey,
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

void SplitViewViews::SetSecondaryContentsResizingStrategy(
    const DevToolsContentsResizingStrategy& strategy) {
  static_cast<ContentsLayoutManager*>(
      secondary_contents_container_->GetLayoutManager())
      ->SetContentsResizingStrategy(strategy);
}

void SplitViewViews::Layout(PassKey key) {
  LayoutSuperclass<views::View>(this);

  auto* browser_view = static_cast<BraveBrowserView*>(browser_->window());
  if (!browser_view) {
    // This can happen on start up
    return;
  }

  browser_view->NotifyDialogPositionRequiresUpdate();
}

void SplitViewViews::AddedToWidget() {
  widget_observation_.Observe(GetWidget());

  secondary_location_bar_ = std::make_unique<SplitViewLocationBar>(
      browser_->profile()->GetPrefs(), this);
  secondary_location_bar_widget_ = std::make_unique<views::Widget>();

  secondary_location_bar_widget_->Init(
      SplitViewLocationBar::GetWidgetInitParams(GetWidget()->GetNativeView(),
                                                secondary_location_bar_.get()));

  // Initialize secondary view state.
  UpdateSecondaryContentsWebViewVisibility();
}

void SplitViewViews::Update() {
  // Update separator visibility first before starting split view layout
  // to give their final position.
  static_cast<BraveBrowserView*>(browser_->window())
      ->UpdateContentsSeparatorVisibility();

  UpdateContentsWebViewVisual();
}

void SplitViewViews::UpdateSplitViewSizeDelta(
    content::WebContents* old_contents,
    content::WebContents* new_contents) {
  auto* model = browser_->tab_strip_model();
  if (model->GetIndexOfWebContents(old_contents) == TabStripModel::kNoTab ||
      model->GetIndexOfWebContents(new_contents) == TabStripModel::kNoTab) {
    // This can happen on start-up or closing a tab.
    return;
  }

  auto* controller = browser_->GetFeatures().split_view_controller();
  // Both tabs are not related with split view, or shown together.
  // So we don't need to update size delta.
  if (!controller->IsOpenedFor(old_contents) &&
      !controller->IsOpenedFor(new_contents)) {
    return;
  }
  if (controller->AreShowingTogether({old_contents, new_contents})) {
    return;
  }

  auto* split_view_layout_manager =
      static_cast<SplitViewLayoutManager*>(GetLayoutManager());
  if (controller->IsOpenedFor(old_contents)) {
    controller->SetSizeDeltaFor(
        old_contents, split_view_layout_manager->split_view_size_delta());
  }

  if (controller->IsOpenedFor(new_contents)) {
    split_view_layout_manager->set_split_view_size_delta(
        controller->GetSizeDeltaFor(new_contents));
  }
}

void SplitViewViews::UpdateContentsWebViewVisual() {
  UpdateContentsWebViewBorder();
  UpdateSecondaryContentsWebViewVisibility();
}

void SplitViewViews::UpdateContentsWebViewBorder() {
  auto* controller = browser_->GetFeatures().split_view_controller();
  if (!controller) {
    return;
  }

  if (browser_->tab_strip_model()->empty()) {
    // Happens on startup
    return;
  }

  if (browser_->IsBrowserClosing()) {
    return;
  }

  auto* cp = GetColorProvider();
  if (!cp) {
    return;
  }

  // In tab-fullscreen mode, don't need any border if secondary contents is not
  // visible as user only can see primary contents.
  if (controller->IsSplitViewActive() &&
      !ShouldHideSecondaryContentsByTabFullscreen()) {
    const auto kRadius =
        BraveBrowser::ShouldUseBraveWebViewRoundedCorners(
            base::to_address(browser_))
            ? BraveContentsViewUtil::kBorderRadius + kBorderThickness
            : 0;
    // Use same color for active focus border.
    contents_container_->SetBorder(views::CreateRoundedRectBorder(
        kBorderThickness, kRadius, kColorBraveSplitViewActiveWebViewBorder));

    BraveContentsLayoutManager::GetLayoutManagerForView(contents_container_)
        ->SetWebContentsBorderInsets(gfx::Insets(kBorderThickness));

    secondary_contents_container_->SetBorder(views::CreateBorderPainter(
        views::Painter::CreateRoundRectWith1PxBorderPainter(
            cp->GetColor(kColorBraveSplitViewInactiveWebViewBorder),
            cp->GetColor(kColorToolbar), kRadius, SkBlendMode::kSrc,
            /*anti_alias*/ true,
            /*should_border_scale*/ true),
        gfx::Insets(kBorderThickness)));
    BraveContentsLayoutManager::GetLayoutManagerForView(
        secondary_contents_container_)
        ->SetWebContentsBorderInsets(gfx::Insets(kBorderThickness));
  } else {
    contents_container_->SetBorder(nullptr);
    BraveContentsLayoutManager::GetLayoutManagerForView(contents_container_)
        ->SetWebContentsBorderInsets({});

    secondary_contents_container_->SetBorder(nullptr);
    BraveContentsLayoutManager::GetLayoutManagerForView(
        secondary_contents_container_)
        ->SetWebContentsBorderInsets({});
  }
  SchedulePaint();
}

void SplitViewViews::UpdateSecondaryContentsWebViewVisibility() {
  if (browser_->IsBrowserClosing()) {
    secondary_contents_web_view_->SetWebContents(nullptr);
    return;
  }

#if BUILDFLAG(ENABLE_SPEEDREADER)
  // Update before |secondary_contents_container_| visibility is
  // changed because SplitViewLocationBar updates its bounds by
  // monitoring |secondary_contents_container_|.
  UpdateSecondaryReaderModeToolbarVisibility();
#endif

  auto* controller = browser_->GetFeatures().split_view_controller();
  CHECK(controller);
  if (controller->IsSplitViewActive()) {
    auto* secondary_contents = controller->GetNonActiveWebContents();
    CHECK_NE(secondary_contents, contents_web_view_->web_contents());
    if (secondary_contents_web_view_->web_contents() != secondary_contents) {
      secondary_contents_web_view_->SetWebContents(secondary_contents);
      secondary_location_bar_->SetWebContents(secondary_contents);
    }

    secondary_contents_container_->SetVisible(true);
    UpdateSecondaryDevtoolsLayoutAndVisibility();

    // Active tab should be put in the original |contents_web_view_| as many
    // other UI components are dependent on it. The layout rotation is done by
    // SplitViewLayoutManager.
    //
    // ex1) When left is the active tab
    //  Contents   | contents_web_view_ | secondary_contents_web_view_ |
    //
    // ex2) When right is the active tab
    //  Contents   | secondary_contents_web_view_ | contents_web_view_  |
    GetSplitViewLayoutManager()->show_main_web_contents_at_tail(
        controller->ShouldShowActiveWebContentsAtRight());
  } else {
    secondary_location_bar_->SetWebContents(nullptr);
    secondary_contents_web_view_->SetWebContents(nullptr);
    secondary_devtools_web_view_->SetWebContents(nullptr);
    secondary_contents_container_->SetVisible(false);
  }

  // Hide secondary contents if primary contents initiates its tab-fullscreen.
  if (secondary_contents_container_->GetVisible() &&
      ShouldHideSecondaryContentsByTabFullscreen()) {
    secondary_contents_container_->SetVisible(false);
  }

  split_view_separator_->SetVisible(
      secondary_contents_container_->GetVisible());

  InvalidateLayout();
}

void SplitViewViews::UpdateCornerRadius(const gfx::RoundedCornersF& corners) {
  secondary_contents_web_view_->layer()->SetRoundedCornerRadius(corners);
  secondary_contents_web_view_->holder()->SetCornerRadii(corners);
  secondary_devtools_web_view_->holder()->SetCornerRadii(corners);
}

#if BUILDFLAG(ENABLE_SPEEDREADER)
void SplitViewViews::OnReaderModeToolbarActivate(
    ReaderModeToolbarView* toolbar) {
  CHECK_EQ(secondary_reader_mode_toolbar_, toolbar);
  CHECK(secondary_contents_web_view_->web_contents());
  if (secondary_contents_web_view_->web_contents()->GetDelegate()) {
    secondary_contents_web_view_->web_contents()
        ->GetDelegate()
        ->ActivateContents(secondary_contents_web_view_->web_contents());
  }
}

void SplitViewViews::UpdateSecondaryReaderModeToolbarVisibility() {
  auto* controller = browser_->GetFeatures().split_view_controller();
  if (controller->IsSplitViewActive()) {
    secondary_reader_mode_toolbar_->SetVisible(
        IsTabDistilled(controller->GetNonActiveWebContents()));
  } else if (secondary_reader_mode_toolbar_) {
    secondary_reader_mode_toolbar_->SetVisible(false);
  }
}

void SplitViewViews::UpdateSecondaryReaderModeToolbar() {
  auto* browser_view = static_cast<BraveBrowserView*>(browser_->window());
  if (!browser_view) {
    return;
  }

  UpdateSecondaryReaderModeToolbarVisibility();

  ReaderModeToolbarView* primary_toolbar = browser_view->reader_mode_toolbar();

  auto* controller = browser_->GetFeatures().split_view_controller();
  if (controller && controller->IsSplitViewActive()) {
    // We need to swap the WebContents of the toolbars because, when the active
    // browser tab is switched, the split view swaps both the views displaying
    // the pages and the WebContents within those views. The toolbar does the
    // same thing to ensure that the toolbar state follows the correct tab.
    // DevTools views do the same.
    primary_toolbar->SwapToolbarContents(secondary_reader_mode_toolbar_.get());
  } else {
    // In case we activate the non split view tab then restore straight
    // toolbars' contents. It means in split view disabled we always see the
    // primary toolbar.
    primary_toolbar->RestoreToolbarContents(
        secondary_reader_mode_toolbar_.get());
  }
}
#endif

void SplitViewViews::UpdateSecondaryDevtoolsLayoutAndVisibility() {
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

void SplitViewViews::OnWidgetDestroying(views::Widget* widget) {
  DCHECK(widget_observation_.IsObservingSource(widget));
  widget_observation_.Reset();
}

void SplitViewViews::OnWidgetWindowModalVisibilityChanged(views::Widget* widget,
                                                          bool visible) {
  if (!base::FeatureList::IsEnabled(features::kScrimForBrowserWindowModal)) {
    return;
  }

#if !BUILDFLAG(IS_MAC)
  // MacOS does not need views window scrim. We use sheets to show window modals
  // (-[NSWindow beginSheet:]), which natively draw a scrim since macOS 11.
  if (secondary_contents_container_->GetVisible()) {
    secondary_contents_scrim_view_->SetVisible(visible);
  }
#endif
}

void SplitViewViews::OnFullscreenStateChanged() {
  // Hide secondary contents when tab fullscreen is initiated by
  // primary contents.
  if (!browser_->GetFeatures().split_view_controller()->IsSplitViewActive()) {
    return;
  }

  UpdateContentsWebViewVisual();
}

bool SplitViewViews::ShouldHideSecondaryContentsByTabFullscreen() const {
  auto* exclusive_access_manager = browser_->exclusive_access_manager();
  if (!exclusive_access_manager) {
    return false;
  }

  return exclusive_access_manager->fullscreen_controller()->IsTabFullscreen();
}

SplitViewLayoutManager* SplitViewViews::GetSplitViewLayoutManager() {
  return const_cast<SplitViewLayoutManager*>(
      const_cast<const SplitViewViews*>(this)->GetSplitViewLayoutManager());
}

const SplitViewLayoutManager* SplitViewViews::GetSplitViewLayoutManager()
    const {
  return static_cast<SplitViewLayoutManager*>(GetLayoutManager());
}

BEGIN_METADATA(SplitViewViews)
END_METADATA
