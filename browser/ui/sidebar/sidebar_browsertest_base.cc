/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/sidebar/sidebar_browsertest_base.h"

#include <algorithm>
#include <optional>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/location.h"
#include "base/run_loop.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/test/bind.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "brave/browser/ui/sidebar/sidebar_model.h"
#include "brave/browser/ui/sidebar/sidebar_service_factory.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "brave/browser/ui/views/sidebar/sidebar_container_view.h"
#include "brave/browser/ui/views/sidebar/sidebar_control_view.h"
#include "brave/browser/ui/views/sidebar/sidebar_items_contents_view.h"
#include "brave/browser/ui/views/sidebar/sidebar_items_scroll_view.h"
#include "brave/browser/ui/views/toolbar/brave_toolbar_view.h"
#include "brave/browser/ui/views/toolbar/side_panel_button.h"
#include "brave/components/brave_origin/buildflags/buildflags.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/sidebar/browser/sidebar_item.h"
#include "brave/components/sidebar/browser/sidebar_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/side_panel/side_panel_ui.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/side_panel/side_panel.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/events/base_event_utils.h"
#include "ui/events/event.h"
#include "ui/gfx/animation/animation.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/point_f.h"
#include "ui/views/view.h"
#include "url/gurl.h"

namespace sidebar {

SidebarBrowserTest::SidebarBrowserTest() = default;
SidebarBrowserTest::~SidebarBrowserTest() = default;

void SidebarBrowserTest::SetUpOnMainThread() {
  InProcessBrowserTest::SetUpOnMainThread();
  animation_resetter_ = gfx::AnimationTestApi::SetRichAnimationRenderMode(
      gfx::Animation::RichAnimationRenderMode::FORCE_DISABLED);
}

void SidebarBrowserTest::PreRunTestOnMainThread() {
  InProcessBrowserTest::PreRunTestOnMainThread();

  auto* service = SidebarServiceFactory::GetForProfile(browser()->profile());
  // Enable sidebar explicitely because sidebar option is different based on
  // channel.
  service->SetSidebarShowOption(SidebarService::ShowSidebarOption::kShowAlways);

  // Start test with visible toolbar button.
  // It's hidden by default in origin build.
#if BUILDFLAG(IS_BRAVE_ORIGIN_BRANDED)
  auto* prefs = browser()->profile()->GetPrefs();
  prefs->SetBoolean(kShowSidePanelButton, true);
#endif
}

SidePanelButton* SidebarBrowserTest::GetSidePanelToolbarButton() const {
  return static_cast<BraveToolbarView*>(
             BrowserView::GetBrowserViewForBrowser(browser())->toolbar())
      ->side_panel_button();
}

views::View* SidebarBrowserTest::GetVerticalTabsContainer() const {
  auto* view = BrowserView::GetBrowserViewForBrowser(browser());
  return static_cast<BraveBrowserView*>(view)->vertical_tab_strip_host_view_;
}

raw_ptr<SidebarItemsContentsView>
SidebarBrowserTest::GetSidebarItemsContentsView(
    SidebarController* controller) const {
  auto* sidebar_container_view =
      static_cast<SidebarContainerView*>(controller->sidebar());
  auto sidebar_control_view = sidebar_container_view->sidebar_control_view_;
  auto sidebar_scroll_view = sidebar_control_view->sidebar_items_view_;
  auto sidebar_items_contents_view = sidebar_scroll_view->contents_view_;
  DCHECK(sidebar_items_contents_view);

  return sidebar_items_contents_view;
}

SidebarItemsScrollView* SidebarBrowserTest::GetSidebarItemsScrollView(
    SidebarController* controller) const {
  auto* sidebar_container_view =
      static_cast<SidebarContainerView*>(controller->sidebar());
  auto sidebar_control_view = sidebar_container_view->sidebar_control_view_;
  return sidebar_control_view->sidebar_items_view_;
}

void SidebarBrowserTest::SimulateSidebarItemClickAt(size_t index) {
  auto sidebar_items_contents_view = GetSidebarItemsContentsView(controller());

  auto* item = sidebar_items_contents_view->children()[index].get();
  DCHECK(item);

  const gfx::Point origin(0, 0);
  ui::MouseEvent event(ui::EventType::kMousePressed, origin, origin,
                       ui::EventTimeForNow(), ui::EF_LEFT_MOUSE_BUTTON, 0);
  sidebar_items_contents_view->OnItemPressed(item, event);

  if (model()->GetAllSidebarItems()[index].open_in_panel) {
    auto* panel_ui = browser()->GetFeatures().side_panel_ui();
    WaitUntil(base::BindLambdaForTesting([&]() {
      return (model()->active_index() == index &&
              panel_ui->IsSidePanelShowing());
    }));
  }
}

void SidebarBrowserTest::HandleBrowserWindowMouseEvent() {
  browser_view()->HandleBrowserWindowMouseEvent(GetDummyEvent());
}

SidebarControlView* SidebarBrowserTest::GetSidebarControlView() const {
  return GetSidebarContainerView()->sidebar_control_view_;
}

SidebarContainerView* SidebarBrowserTest::GetSidebarContainerView() const {
  return static_cast<SidebarContainerView*>(controller()->sidebar());
}

SidePanel* SidebarBrowserTest::GetSidePanel() {
  return browser_view()->side_panel();
}

bool SidebarBrowserTest::IsSidebarUIOnLeft() const {
  return GetSidebarContainerView()->sidebar_on_left_ &&
         GetSidebarControlView()->sidebar_on_left_;
}

void SidebarBrowserTest::ShowSidebar() {
  GetSidebarContainerView()->ShowSidebar();
}

void SidebarBrowserTest::HideSidebar() {
  GetSidebarContainerView()->HideSidebar();
}

void SidebarBrowserTest::WaitUntil(base::RepeatingCallback<bool()> condition) {
  if (condition.Run()) {
    return;
  }

  base::RepeatingTimer scheduler;
  scheduler.Start(FROM_HERE, base::Milliseconds(100),
                  base::BindLambdaForTesting([this, &condition] {
                    if (condition.Run()) {
                      run_loop_->Quit();
                    }
                  }));
  Run();
}

void SidebarBrowserTest::Run() {
  run_loop_ = std::make_unique<base::RunLoop>();
  run_loop()->Run();
}

void SidebarBrowserTest::SetItemAddedBubbleLaunchedCallback(
    SidebarItemsContentsView* items_contents_view) {
  items_contents_view->item_added_bubble_launched_for_test_ =
      base::BindRepeating(&SidebarBrowserTest::ItemAddedBubbleLaunchedCallback,
                          weak_factory_.GetWeakPtr());
}

void SidebarBrowserTest::ItemAddedBubbleLaunchedCallback(views::View* anchor) {
  item_added_bubble_anchor_ = anchor;
}

void SidebarBrowserTest::AddItemsTillScrollable(
    SidebarItemsScrollView* scroll_view,
    SidebarService* sidebar_service) {
  int url_prefix = 0;
  while (true) {
    sidebar_service->AddItem(sidebar::SidebarItem::Create(
        GURL(base::StrCat(
            {"https://foo/bar_", base::NumberToString(url_prefix)})),
        u"title", SidebarItem::Type::kTypeWeb,
        SidebarItem::BuiltInItemType::kNone, false));
    url_prefix++;
    base::RunLoop().RunUntilIdle();
    // Add items till first item becomes invisible.
    if (scroll_view->NeedScrollForItemAt(0)) {
      break;
    }
  }
}

bool SidebarBrowserTest::NeedScrollForItemAt(
    size_t index,
    SidebarItemsScrollView* scroll_view) {
  return scroll_view->NeedScrollForItemAt(index);
}

void SidebarBrowserTest::VerifyTargetDragIndicatorIndexCalc(
    const gfx::Point& screen_position) {
  auto sidebar_items_contents_view = GetSidebarItemsContentsView(controller());
  EXPECT_NE(std::nullopt,
            sidebar_items_contents_view->CalculateTargetDragIndicatorIndex(
                screen_position));
}

size_t SidebarBrowserTest::GetDefaultItemCount() const {
  return SidebarServiceFactory::GetForProfile(browser()->profile())
      ->GetDefaultSidebarItemsCountForTesting();
}

int SidebarBrowserTest::GetFirstPanelItemIndex() {
  auto const items = model()->GetAllSidebarItems();
  auto const iter = std::ranges::find(items, true, &SidebarItem::open_in_panel);
  return std::distance(items.cbegin(), iter);
}

int SidebarBrowserTest::GetFirstWebItemIndex() {
  const auto items = model()->GetAllSidebarItems();
  auto const iter =
      std::ranges::find(items, false, &SidebarItem::open_in_panel);
  return std::distance(items.cbegin(), iter);
}

BraveBrowserView* SidebarBrowserTest::browser_view() {
  return BraveBrowserView::From(
      BrowserView::GetBrowserViewForBrowser(browser()));
}

// static
ui::MouseEvent SidebarBrowserTest::GetDummyEvent() {
  return ui::MouseEvent(ui::EventType::kMouseMoved, gfx::PointF(),
                        gfx::PointF(), base::TimeTicks::Now(), 0, 0);
}

}  // namespace sidebar
