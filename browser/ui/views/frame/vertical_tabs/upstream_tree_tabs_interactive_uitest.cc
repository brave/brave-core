/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>
#include <string_view>
#include <vector>

#include "base/command_line.h"
#include "base/functional/bind.h"
#include "base/memory/raw_ptr.h"
#include "base/test/bind.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "brave/browser/ui/tabs/brave_tab_strip_model.h"
#include "brave/browser/ui/tabs/public/switches.h"
#include "brave/browser/ui/tabs/tree_tab_model.h"
#include "brave/components/tabs/public/tree_tab_node.h"
#include "brave/components/tabs/public/tree_tab_node_tab_collection.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/browser_window/public/global_browser_collection.h"
#include "chrome/browser/ui/tabs/features.h"
#include "chrome/browser/ui/tabs/split_tab_metrics.h"
#include "chrome/browser/ui/tabs/tab_enums.h"
#include "chrome/browser/ui/tabs/tab_model.h"
#include "chrome/browser/ui/tabs/vertical_tab_strip_state_controller.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/frame/vertical_tab_strip_region_view.h"
#include "chrome/browser/ui/views/tabs/common/root_tab_collection_node.h"
#include "chrome/browser/ui/views/tabs/common/tab_collection_node.h"
#include "chrome/browser/ui/views/tabs/dragging/tab_drag_controller.h"
#include "chrome/test/interaction/interactive_browser_test.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "ui/base/page_transition_types.h"
#include "ui/base/test/ui_controls.h"
#include "ui/gfx/animation/animation.h"
#include "ui/gfx/animation/animation_test_api.h"
#include "ui/gfx/geometry/vector2d.h"
#include "ui/views/test/views_test_utils.h"

namespace {

DEFINE_LOCAL_STATE_IDENTIFIER_VALUE(ui::test::PollingStateObserver<bool>,
                                    kDragStatePoller);
DEFINE_LOCAL_STATE_IDENTIFIER_VALUE(ui::test::PollingStateObserver<size_t>,
                                    kBrowserCountPoller);
DEFINE_LOCAL_STATE_IDENTIFIER_VALUE(
    ui::test::PollingStateObserver<std::vector<tabs::TabInterface*>>,
    kTabOrderPoller);

}  // namespace

// Tests for dragging tree tabs on Chromium's vertical tab strip.
class UpstreamTreeTabsDragTest : public InteractiveBrowserTest {
 public:
  void SetUpCommandLine(base::CommandLine* command_line) override {
    InteractiveBrowserTest::SetUpCommandLine(command_line);
    command_line->AppendSwitchASCII(
        tabs::switches::kVerticalTabMigrationSwitch,
        tabs::switches::kVerticalTabMigrationForceUpstreamValue);
  }

  void SetUpOnMainThread() override {
    InteractiveBrowserTest::SetUpOnMainThread();
    auto* state_controller =
        tabs::VerticalTabStripStateController::From(browser());
    state_controller->SetVerticalTabsEnabled(true);
    ASSERT_TRUE(state_controller->ShouldDisplayVerticalTabs());
    state_controller->RequestCollapse(false);

    browser()->GetProfile()->GetPrefs()->SetBoolean(
        brave_tabs::kTreeTabsEnabled, true);
    ASSERT_TRUE(model().tree_model());

    // Creates tabs as below.
    // - Tab 0
    //   - Tab 1
    //     - Tab 2
    // - Tab 3
    // - Tab 4
    model().ActivateTabAt(0);
    AddTab(/*child_of_active=*/true);
    AddTab(/*child_of_active=*/true);
    AddTab(/*child_of_active=*/false);
    AddTab(/*child_of_active=*/false);
    ASSERT_EQ(5, model().count());
    for (int i = 0; i < model().count(); ++i) {
      ASSERT_EQ(i < 3 ? i : 0, GetLevel(model().GetTabAtIndex(i)))
          << "tab index: " << i;
    }
    model().ActivateTabAt(4);
    for (int i = 0; i < model().count(); ++i) {
      tabs_.push_back(model().GetTabAtIndex(i));
    }
    views::test::RunScheduledLayout(browser_view());
  }

  void TearDownOnMainThread() override {
    tabs_.clear();
    InteractiveBrowserTest::TearDownOnMainThread();
  }

  BraveTabStripModel& model() {
    return *static_cast<BraveTabStripModel*>(browser()->tab_strip_model());
  }

  BrowserView* browser_view() {
    return BrowserView::GetBrowserViewForBrowser(browser());
  }

  void AddTab(bool child_of_active) {
    model().AddTab(
        std::make_unique<tabs::TabModel>(
            content::WebContents::Create(
                content::WebContents::CreateParams(browser()->GetProfile())),
            &model()),
        -1,
        child_of_active ? ui::PAGE_TRANSITION_LINK : ui::PAGE_TRANSITION_TYPED,
        child_of_active ? ADD_INHERIT_OPENER | ADD_ACTIVE : ADD_NONE);
  }

  views::View* GetTabView(tabs::TabInterface* tab) {
    TabCollectionNode* node = browser_view()
                                  ->vertical_tab_strip_region_view_for_testing()
                                  ->root_node_for_testing()
                                  ->GetNodeForHandle(tab->GetHandle());
    return node ? node->view() : nullptr;
  }

  auto NameTab(std::string_view name, int initial_index) {
    return NameView(name, base::BindLambdaForTesting([this, initial_index]() {
                      return GetTabView(tabs_[initial_index]);
                    }));
  }

  auto GetTabOrder() {
    return base::BindLambdaForTesting([this]() {
      std::vector<tabs::TabInterface*> order;
      for (int i = 0; i < model().count(); ++i) {
        order.push_back(model().GetTabAtIndex(i));
      }
      return order;
    });
  }

  auto RunScheduledLayout() {
    return Do([this]() { views::test::RunScheduledLayout(browser_view()); });
  }

  // Returns -1 if |tab| isn't wrapped by a tree tab node.
  int GetLevel(tabs::TabInterface* tab) {
    auto* collection =
        tabs::TreeTabNodeTabCollection::GetTreeTabNodeCollection(tab);
    if (!collection) {
      ADD_FAILURE() << "The tab isn't wrapped by a tree tab node.";
      return -1;
    }
    return collection->node().level();
  }

  auto StartDrag(std::string_view from, std::string_view to) {
    return Steps(MoveMouseTo(from),
                 ClickMouse(ui_controls::LEFT, /*release=*/false),
                 PollState(kDragStatePoller, base::BindRepeating([]() {
                             return TabDragController::IsActive();
                           })),
                 MoveMouseTo(to), RunScheduledLayout(),
                 WaitForState(kDragStatePoller, true));
  }

  BrowserWindowInterface* GetOtherBrowser() {
    BrowserWindowInterface* other = nullptr;
    GlobalBrowserCollection::GetInstance()->ForEach(
        [&](BrowserWindowInterface* b) {
          if (b != browser()) {
            other = b;
          }
          return true;
        });
    return other;
  }

  // Splits |split_tab| with |split_with|, which must be adjacent, and drags
  // the split out of the window.
  auto DetachSplitIntoNewWindow(int split_tab, int split_with) {
    return Steps(
        Do([this, split_tab, split_with]() {
          model().ActivateTabAt(split_tab);
          model().AddToNewSplit(
              {split_with}, {},
              split_tabs::SplitTabCreatedSource::kTabContextMenu);
          ASSERT_TRUE(tabs_[split_tab]->IsSplit());
        }),
        RunScheduledLayout(), NameTab("split tab", split_tab),
        MoveMouseTo("split tab"),
        ClickMouse(ui_controls::LEFT, /*release=*/false),
        PollState(kBrowserCountPoller, base::BindRepeating([]() {
                    return GlobalBrowserCollection::GetInstance()->GetSize();
                  })),
        MoveMouseTo(base::BindLambdaForTesting([this]() {
          return browser_view()->GetBoundsInScreen().top_right() +
                 gfx::Vector2d(150, 150);
        })),
        WaitForState(kBrowserCountPoller, 2u), ReleaseMouse(),
        PollState(kDragStatePoller, base::BindRepeating([]() {
                    return TabDragController::IsActive();
                  })),
        WaitForState(kDragStatePoller, false), Do([this, split_tab]() {
          BrowserWindowInterface* new_browser = GetOtherBrowser();
          ASSERT_TRUE(new_browser);
          TabStripModel* new_model = new_browser->GetTabStripModel();
          ASSERT_EQ(2, new_model->count());
          EXPECT_TRUE(new_model->GetTabAtIndex(0)->IsSplit());
          EXPECT_EQ(tabs_[split_tab], new_model->GetTabAtIndex(0));

          // The rest stays in the source window, in the tree.
          ASSERT_EQ(3, model().count());
          for (int i = 0; i < model().count(); ++i) {
            EXPECT_TRUE(
                tabs::TreeTabNodeTabCollection::GetNearestTreeTabNodeCollection(
                    model().GetTabAtIndex(i)))
                << "tab index: " << i;
          }
        }));
  }

  std::vector<raw_ptr<tabs::TabInterface>> tabs_;

 private:
  base::test::ScopedFeatureList feature_list_{tabs::kBraveTreeTab};
  gfx::AnimationTestApi::RenderModeResetter disable_animation_ =
      gfx::AnimationTestApi::SetRichAnimationRenderMode(
          gfx::Animation::RichAnimationRenderMode::FORCE_DISABLED);
};

// Dragging a tree tab parent moves its whole subtree, keeping it nested.
IN_PROC_BROWSER_TEST_F(UpstreamTreeTabsDragTest, DragParentMovesSubtree) {
  RunTestSequence(
      NameTab("parent", 0), NameTab("target", 4), StartDrag("parent", "target"),
      PollState(kTabOrderPoller, GetTabOrder()),
      WaitForState(kTabOrderPoller,
                   std::vector<tabs::TabInterface*>{
                       tabs_[3], tabs_[4], tabs_[0], tabs_[1], tabs_[2]}),
      ReleaseMouse(), WaitForState(kDragStatePoller, false), Do([&]() {
        EXPECT_EQ(0, GetLevel(tabs_[0]));
        EXPECT_EQ(1, GetLevel(tabs_[1]));
        EXPECT_EQ(2, GetLevel(tabs_[2]));
        views::test::RunScheduledLayout(browser_view());
        EXPECT_LT(GetTabView(tabs_[0])->x(), GetTabView(tabs_[1])->x());
        EXPECT_LT(GetTabView(tabs_[1])->x(), GetTabView(tabs_[2])->x());
      }));
}

// Dragging a tree tab child out of its parent moves it out of the subtree.
IN_PROC_BROWSER_TEST_F(UpstreamTreeTabsDragTest, DragChildOutOfParent) {
  RunTestSequence(
      NameTab("child", 2), NameTab("target", 4), StartDrag("child", "target"),
      PollState(kTabOrderPoller, GetTabOrder()),
      WaitForState(kTabOrderPoller,
                   std::vector<tabs::TabInterface*>{
                       tabs_[0], tabs_[1], tabs_[3], tabs_[4], tabs_[2]}),
      ReleaseMouse(), WaitForState(kDragStatePoller, false), Do([&]() {
        EXPECT_EQ(0, GetLevel(tabs_[2]));
        views::test::RunScheduledLayout(browser_view());
        EXPECT_EQ(GetTabView(tabs_[0])->x(), GetTabView(tabs_[2])->x());
      }));
}

// Dragging a collapsed tree tab parent moves its hidden subtree as well.
IN_PROC_BROWSER_TEST_F(UpstreamTreeTabsDragTest, DragCollapsedParent) {
  auto* collection =
      tabs::TreeTabNodeTabCollection::GetTreeTabNodeCollection(tabs_[0]);
  ASSERT_TRUE(collection);
  model().SetTreeTabNodeCollapsed(collection->node().id(), true);

  RunTestSequence(
      RunScheduledLayout(), NameTab("parent", 0), NameTab("target", 4),
      StartDrag("parent", "target"), PollState(kTabOrderPoller, GetTabOrder()),
      WaitForState(kTabOrderPoller,
                   std::vector<tabs::TabInterface*>{
                       tabs_[3], tabs_[4], tabs_[0], tabs_[1], tabs_[2]}),
      ReleaseMouse(), WaitForState(kDragStatePoller, false), Do([&]() {
        auto* moved_collection =
            tabs::TreeTabNodeTabCollection::GetTreeTabNodeCollection(tabs_[0]);
        ASSERT_TRUE(moved_collection);
        EXPECT_TRUE(moved_collection->node().collapsed());
        views::test::RunScheduledLayout(browser_view());
        EXPECT_TRUE(GetTabView(tabs_[0])->GetVisible());
        EXPECT_FALSE(GetTabView(tabs_[1])->GetVisible());
        EXPECT_FALSE(GetTabView(tabs_[2])->GetVisible());
      }));
}

// Dragging a split out of the tab strip detaches it into a new window, leaving
// the rest of the tree in the source window.
IN_PROC_BROWSER_TEST_F(UpstreamTreeTabsDragTest, DetachSplitIntoNewWindow) {
  RunTestSequence(DetachSplitIntoNewWindow(/*split_tab=*/3,
                                           /*split_with=*/4));
}

// Same as DetachSplitIntoNewWindow, for a split inside a tree.
IN_PROC_BROWSER_TEST_F(UpstreamTreeTabsDragTest,
                       DetachSplitIntoNewWindow_ChildSplit) {
  RunTestSequence(DetachSplitIntoNewWindow(/*split_tab=*/1,
                                           /*split_with=*/2));
}

// Same as DetachSplitIntoNewWindow, for a split with tree descendants.
IN_PROC_BROWSER_TEST_F(UpstreamTreeTabsDragTest,
                       DetachSplitIntoNewWindow_ParentSplit) {
  RunTestSequence(DetachSplitIntoNewWindow(/*split_tab=*/0,
                                           /*split_with=*/3));
}
