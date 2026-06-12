/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/workspaces/workspaces_bubble_view.h"

#include <string>

#include "base/functional/bind.h"
#include "base/location.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/bind.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/test_future.h"
#include "base/time/time.h"
#include "brave/browser/ui/views/frame/brave_tab_strip_region_view.h"
#include "brave/browser/ui/views/workspaces/workspace_row_view.h"
#include "brave/browser/workspaces/features.h"
#include "brave/browser/workspaces/workspace_metadata.h"
#include "brave/browser/workspaces/workspace_service.h"
#include "brave/browser/workspaces/workspace_service_factory.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/tabs/tab_strip_control_button.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/test/browser_test.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/events/base_event_utils.h"
#include "ui/events/event.h"
#include "ui/views/controls/label.h"
#include "ui/views/test/button_test_api.h"
#include "ui/views/view_utils.h"
#include "ui/views/widget/any_widget_observer.h"
#include "ui/views/widget/widget.h"

namespace {

BraveHorizontalTabStripRegionView* GetHorizontalTabStripRegion(
    Browser* browser) {
  auto* browser_view = BrowserView::GetBrowserViewForBrowser(browser);
  return views::AsViewClass<BraveHorizontalTabStripRegionView>(
      browser_view->tab_strip_view());
}

WorkspaceMetadata MakeMeta(const std::string& name, int windows, int tabs) {
  return {.name = name,
          .modified_at = base::Time::Now(),
          .number_of_windows = windows,
          .number_of_tabs = tabs};
}

template <typename Pred>
bool AnyDescendantMatches(views::View* view, Pred pred) {
  if (pred(view)) {
    return true;
  }
  for (views::View* child : view->children()) {
    if (AnyDescendantMatches(child, pred)) {
      return true;
    }
  }
  return false;
}

template <typename ViewType>
int CountDescendants(views::View* view) {
  int count = views::AsViewClass<ViewType>(view) ? 1 : 0;
  for (views::View* child : view->children()) {
    count += CountDescendants<ViewType>(child);
  }
  return count;
}

void ClickButton(views::Button* button) {
  const ui::MouseEvent event(ui::EventType::kMousePressed, gfx::Point(),
                             gfx::Point(), ui::EventTimeForNow(),
                             ui::EF_LEFT_MOUSE_BUTTON,
                             ui::EF_LEFT_MOUSE_BUTTON);
  views::test::ButtonTestApi(button).NotifyClick(event);
}

}  // namespace

class WorkspacesBubbleBrowserTest : public InProcessBrowserTest {
 public:
  WorkspacesBubbleBrowserTest() {
    feature_list_.InitAndEnableFeature(features::kWorkspaces);
  }

  TabStripControlButton* GetWorkspacesButton() {
    auto* region = GetHorizontalTabStripRegion(browser());
    return region ? region->workspaces_button_for_testing() : nullptr;
  }

  WorkspaceService* GetWorkspaceService() {
    return WorkspaceServiceFactory::GetForProfile(browser()->profile());
  }

  // Clicks the workspaces button and returns the resulting bubble view, or
  // nullptr if no bubble was shown. |location| is the caller's source location
  // so failures in this helper point at the test that called it.
  WorkspacesBubbleView* OpenBubble(
      const base::Location& location = base::Location::Current()) {
    SCOPED_TRACE(location.ToString());

    base::test::TestFuture<WorkspacesBubbleView*> bubble_future;
    views::AnyWidgetObserver observer(views::test::AnyWidgetTestPasskey{});
    observer.set_shown_callback(
        base::BindLambdaForTesting([&](views::Widget* widget) {
          // widget->GetContentsView() returns the NonClientView wrapper, so go
          // through the delegate to reach the actual bubble view.
          auto* delegate = widget->widget_delegate();
          if (!delegate) {
            return;
          }
          if (auto* b = views::AsViewClass<WorkspacesBubbleView>(
                  delegate->GetContentsView())) {
            bubble_future.SetValue(b);
          }
        }));

    auto* button = GetWorkspacesButton();
    EXPECT_TRUE(button) << "Workspaces button not found in the tab strip";
    if (!button) {
      return nullptr;
    }
    ClickButton(button);
    return bubble_future.Take();
  }

 private:
  base::test::ScopedFeatureList feature_list_;
};

class WorkspacesFeatureDisabledBrowserTest : public InProcessBrowserTest {
 public:
  WorkspacesFeatureDisabledBrowserTest() {
    feature_list_.InitAndDisableFeature(features::kWorkspaces);
  }

 private:
  base::test::ScopedFeatureList feature_list_;
};

// No workspaces button is created when the feature is disabled.
IN_PROC_BROWSER_TEST_F(WorkspacesFeatureDisabledBrowserTest,
                       ButtonAbsentWhenDisabled) {
  auto* region = GetHorizontalTabStripRegion(browser());
  ASSERT_TRUE(region);
  EXPECT_EQ(region->workspaces_button_for_testing(), nullptr);
}

// Clicking the workspaces button shows a WorkspacesBubbleView.
IN_PROC_BROWSER_TEST_F(WorkspacesBubbleBrowserTest, BubbleOpensOnClick) {
  EXPECT_TRUE(OpenBubble());
}

// With no saved workspaces, the bubble shows the empty-state title label.
IN_PROC_BROWSER_TEST_F(WorkspacesBubbleBrowserTest, EmptyStateLabelShown) {
  ASSERT_TRUE(GetWorkspaceService());
  ASSERT_TRUE(GetWorkspaceService()->ListWorkspaces().empty());

  auto* bubble = OpenBubble();
  ASSERT_TRUE(bubble);

  const std::u16string expected =
      l10n_util::GetStringUTF16(IDS_WORKSPACES_BUBBLE_EMPTY_TITLE);
  EXPECT_TRUE(
      AnyDescendantMatches(bubble,
                           [&](views::View* v) {
                             auto* label = views::AsViewClass<views::Label>(v);
                             return label && label->GetText() == expected;
                           }))
      << "Expected empty-state label '" << base::UTF16ToUTF8(expected)
      << "' not found among bubble descendants";
}

// The bubble renders one WorkspaceRowView per saved workspace.
IN_PROC_BROWSER_TEST_F(WorkspacesBubbleBrowserTest, OneRowPerWorkspace) {
  auto* service = GetWorkspaceService();
  ASSERT_TRUE(service);
  service->SaveWorkspaceMetadata(MakeMeta("Alpha", 1, 3));
  service->SaveWorkspaceMetadata(MakeMeta("Beta", 2, 5));

  auto* bubble = OpenBubble();
  ASSERT_TRUE(bubble);

  EXPECT_EQ(CountDescendants<WorkspaceRowView>(bubble), 2)
      << "Bubble should render one WorkspaceRowView per saved workspace";
}
