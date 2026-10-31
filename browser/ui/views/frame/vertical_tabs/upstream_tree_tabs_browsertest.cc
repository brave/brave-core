/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>
#include <vector>

#include "base/command_line.h"
#include "base/location.h"
#include "base/test/run_until.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "brave/browser/ui/tabs/brave_tab_strip_model.h"
#include "brave/browser/ui/tabs/public/switches.h"
#include "brave/browser/ui/tabs/tree_tab_model.h"
#include "brave/browser/ui/views/tabs/tree_tab_view_utils.h"
#include "brave/components/tabs/public/tree_tab_node.h"
#include "brave/components/tabs/public/tree_tab_node_tab_collection.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/tabs/features.h"
#include "chrome/browser/ui/tabs/split_tab_metrics.h"
#include "chrome/browser/ui/tabs/tab_enums.h"
#include "chrome/browser/ui/tabs/tab_model.h"
#include "chrome/browser/ui/tabs/vertical_tab_strip_state_controller.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/frame/vertical_tab_strip_region_view.h"
#include "chrome/browser/ui/views/tabs/common/root_tab_collection_node.h"
#include "chrome/browser/ui/views/tabs/common/tab_collection_node.h"
#include "chrome/browser/ui/views/tabs/common/tab_view.h"
#include "chrome/browser/ui/views/tabs/common/unpinned_tab_container_view.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/prefs/pref_service.h"
#include "components/tabs/public/tab_collection.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/page_transition_types.h"
#include "ui/events/event.h"
#include "ui/gfx/animation/animation.h"
#include "ui/gfx/animation/animation_test_api.h"
#include "ui/gfx/geometry/point_f.h"
#include "ui/views/test/button_test_api.h"
#include "ui/views/test/views_test_utils.h"
#include "ui/views/view_utils.h"

// Tests for tree tabs on top of Chromium's vertical tab strip.
class UpstreamTreeTabsBrowserTest : public InProcessBrowserTest {
 public:
  void SetUpCommandLine(base::CommandLine* command_line) override {
    InProcessBrowserTest::SetUpCommandLine(command_line);
    command_line->AppendSwitchASCII(
        tabs::switches::kVerticalTabMigrationSwitch,
        tabs::switches::kVerticalTabMigrationForceUpstreamValue);
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    auto* state_controller =
        tabs::VerticalTabStripStateController::From(browser());
    state_controller->SetVerticalTabsEnabled(true);
    ASSERT_TRUE(state_controller->ShouldDisplayVerticalTabs());
    state_controller->RequestCollapse(false);

    SetTreeTabsEnabled(true);
    ASSERT_TRUE(model().tree_model());
  }

  BraveTabStripModel& model() {
    return *static_cast<BraveTabStripModel*>(browser()->tab_strip_model());
  }

  BrowserView* browser_view() {
    return BrowserView::GetBrowserViewForBrowser(browser());
  }

  VerticalTabStripRegionView* region_view() {
    return browser_view()->vertical_tab_strip_region_view_for_testing();
  }

  RootTabCollectionNode* root_node() {
    return region_view()->root_node_for_testing();
  }

  void SetTreeTabsEnabled(bool enabled) {
    browser()->GetProfile()->GetPrefs()->SetBoolean(
        brave_tabs::kTreeTabsEnabled, enabled);
  }

  // Adds a tab opened from the active tab, i.e. a child of the active tab.
  void AddChildOfActiveTab() {
    model().AddTab(
        std::make_unique<tabs::TabModel>(
            content::WebContents::Create(
                content::WebContents::CreateParams(browser()->GetProfile())),
            &model()),
        -1, ui::PAGE_TRANSITION_LINK, ADD_INHERIT_OPENER | ADD_ACTIVE);
  }

  void AddTabWithoutOpener() {
    model().AppendWebContents(
        content::WebContents::Create(
            content::WebContents::CreateParams(browser()->GetProfile())),
        /*foreground=*/false);
  }

  // Returns true when the view model mirrors the collection hierarchy, with
  // tree tab node collections hoisted.
  bool IsViewModelSynced() {
    TabCollectionNode* unpinned =
        root_node()->GetChildNodeOfType(TabCollectionNode::Type::UNPINNED);
    if (!unpinned) {
      return false;
    }
    const auto* collection =
        std::get<tabs::ConstDanglingUntriagedTabCollection>(
            unpinned->GetNodeData())
            .get();
    const tabs::ConstChildrenPtrs expected =
        tree_tab_view_utils::GetChildrenForView(*collection);
    if (expected.size() != unpinned->children().size()) {
      return false;
    }
    for (size_t i = 0; i < expected.size(); ++i) {
      if (unpinned->children()[i]->GetNodeData() != expected[i]) {
        return false;
      }
    }
    return true;
  }

  void WaitForViewModelSynced(
      base::Location location = base::Location::Current()) {
    SCOPED_TRACE(location.ToString());
    ASSERT_TRUE(base::test::RunUntil([&]() { return IsViewModelSynced(); }))
        << "The view model doesn't mirror the tab collection hierarchy.";
    LayoutNow();
  }

  void LayoutNow() { views::test::RunScheduledLayout(browser_view()); }

  TabView* GetTabViewAt(int index) {
    TabCollectionNode* node = root_node()->GetNodeForHandle(
        model().GetTabAtIndex(index)->GetHandle());
    return node ? views::AsViewClass<TabView>(node->view()) : nullptr;
  }

  const tabs::TreeTabNode& GetTreeTabNodeAt(int index) {
    auto* collection = tabs::TreeTabNodeTabCollection::GetTreeTabNodeCollection(
        model().GetTabAtIndex(index));
    if (!collection) {
      ADD_FAILURE() << "Tab " << index << " isn't wrapped by a tree tab node.";
      return tabs::TreeTabNode::GetEmptyTreeTabNode();
    }
    return collection->node();
  }

  // Creates tabs as below and returns after the views are laid out.
  // - Tab 0
  //   - Tab 1
  //     - Tab 2
  // - Tab 3
  void CreateTree(base::Location location = base::Location::Current()) {
    SCOPED_TRACE(location.ToString());
    ASSERT_EQ(1, model().count());
    model().ActivateTabAt(0);
    AddChildOfActiveTab();
    AddChildOfActiveTab();
    AddTabWithoutOpener();
    ASSERT_EQ(4, model().count());
    ASSERT_EQ(1, GetTreeTabNodeAt(1).level());
    ASSERT_EQ(2, GetTreeTabNodeAt(2).level());
    ASSERT_EQ(0, GetTreeTabNodeAt(3).level());
    WaitForViewModelSynced();
  }

 private:
  base::test::ScopedFeatureList feature_list_{tabs::kBraveTreeTab};
  gfx::AnimationTestApi::RenderModeResetter disable_animation_ =
      gfx::AnimationTestApi::SetRichAnimationRenderMode(
          gfx::Animation::RichAnimationRenderMode::FORCE_DISABLED);
};

IN_PROC_BROWSER_TEST_F(UpstreamTreeTabsBrowserTest,
                       TreeIsBuiltWithUpstreamVerticalTabs) {
  // Brave's own vertical tabs are off. Upstream's vertical tabs alone should
  // build the tree.
  EXPECT_FALSE(browser()->GetProfile()->GetPrefs()->GetBoolean(
      brave_tabs::kVerticalTabsEnabled));
  EXPECT_TRUE(model().tree_model());

  browser()->GetProfile()->GetPrefs()->SetBoolean(prefs::kVerticalTabsEnabled,
                                                  false);
  EXPECT_FALSE(model().tree_model());
  browser()->GetProfile()->GetPrefs()->SetBoolean(prefs::kVerticalTabsEnabled,
                                                  true);
  EXPECT_TRUE(model().tree_model());

  // Toggling Brave's vertical tabs pref while upstream's is on keeps the tree.
  browser()->GetProfile()->GetPrefs()->SetBoolean(
      brave_tabs::kVerticalTabsEnabled, true);
  EXPECT_TRUE(model().tree_model());
  browser()->GetProfile()->GetPrefs()->SetBoolean(
      brave_tabs::kVerticalTabsEnabled, false);
  EXPECT_TRUE(model().tree_model());
}

IN_PROC_BROWSER_TEST_F(UpstreamTreeTabsBrowserTest, IndentedByLevel) {
  CreateTree();

  // Tree tab node collections aren't mirrored, so all tabs are direct
  // children of the unpinned container.
  for (int i = 0; i < model().count(); ++i) {
    SCOPED_TRACE(testing::Message() << "tab index: " << i);
    TabView* tab_view = GetTabViewAt(i);
    ASSERT_TRUE(tab_view);
    EXPECT_EQ(region_view()->GetUnpinnedTabsContainer(), tab_view->parent());
    EXPECT_TRUE(tab_view->GetVisible());
  }

  EXPECT_LT(GetTabViewAt(0)->x(), GetTabViewAt(1)->x());
  EXPECT_LT(GetTabViewAt(1)->x(), GetTabViewAt(2)->x());
  EXPECT_EQ(GetTabViewAt(0)->x(), GetTabViewAt(3)->x());
  // Indented tabs share the same trailing edge.
  EXPECT_EQ(GetTabViewAt(0)->bounds().right(),
            GetTabViewAt(2)->bounds().right());
  // Tabs are stacked in model order.
  EXPECT_LT(GetTabViewAt(0)->y(), GetTabViewAt(1)->y());
  EXPECT_LT(GetTabViewAt(1)->y(), GetTabViewAt(2)->y());
  EXPECT_LT(GetTabViewAt(2)->y(), GetTabViewAt(3)->y());
}

IN_PROC_BROWSER_TEST_F(UpstreamTreeTabsBrowserTest, CollapseAndExpand) {
  CreateTree();
  model().ActivateTabAt(3);

  model().SetTreeTabNodeCollapsed(GetTreeTabNodeAt(0).id(), true);
  WaitForViewModelSynced();
  EXPECT_TRUE(GetTabViewAt(0)->GetVisible());
  EXPECT_FALSE(GetTabViewAt(1)->GetVisible());
  EXPECT_FALSE(GetTabViewAt(2)->GetVisible());
  EXPECT_TRUE(GetTabViewAt(3)->GetVisible());
  // Tab 3 comes right after tab 0.
  EXPECT_LT(GetTabViewAt(3)->y() - GetTabViewAt(0)->bounds().bottom(),
            GetTabViewAt(0)->height());

  model().SetTreeTabNodeCollapsed(GetTreeTabNodeAt(0).id(), false);
  WaitForViewModelSynced();
  for (int i = 0; i < model().count(); ++i) {
    EXPECT_TRUE(GetTabViewAt(i)->GetVisible()) << "tab index: " << i;
  }
}

IN_PROC_BROWSER_TEST_F(UpstreamTreeTabsBrowserTest, ToggleButton) {
  CreateTree();
  model().ActivateTabAt(0);
  LayoutNow();

  // Only tabs with descendants have the toggle button.
  EXPECT_TRUE(GetTabViewAt(0)->ShouldShowTreeToggleButton());
  model().ActivateTabAt(2);
  LayoutNow();
  EXPECT_FALSE(GetTabViewAt(2)->ShouldShowTreeToggleButton());

  model().ActivateTabAt(0);
  LayoutNow();
  views::ImageButton* button =
      GetTabViewAt(0)->tree_toggle_button_for_testing();
  ASSERT_TRUE(button);
  EXPECT_TRUE(button->GetVisible());
  EXPECT_EQ(u"Collapse tab tree", button->GetTooltipText());
  EXPECT_EQ(u"Collapse tab tree",
            button->GetViewAccessibility().GetCachedName());

  // The toggle button takes the close button's place.
  views::test::ButtonTestApi(button).NotifyClick(
      ui::MouseEvent(ui::EventType::kMousePressed, gfx::PointF(), gfx::PointF(),
                     base::TimeTicks::Now(), ui::EF_LEFT_MOUSE_BUTTON,
                     ui::EF_LEFT_MOUSE_BUTTON));
  EXPECT_TRUE(GetTreeTabNodeAt(0).collapsed());
  WaitForViewModelSynced();
  EXPECT_FALSE(GetTabViewAt(1)->GetVisible());
  EXPECT_EQ(u"Expand tab tree", button->GetTooltipText());

  // The toggle button stays visible while collapsed, even for an inactive
  // tab.
  model().ActivateTabAt(3);
  LayoutNow();
  EXPECT_TRUE(GetTabViewAt(0)->ShouldShowTreeToggleButton());

  views::test::ButtonTestApi(button).NotifyClick(
      ui::MouseEvent(ui::EventType::kMousePressed, gfx::PointF(), gfx::PointF(),
                     base::TimeTicks::Now(), ui::EF_LEFT_MOUSE_BUTTON,
                     ui::EF_LEFT_MOUSE_BUTTON));
  EXPECT_FALSE(GetTreeTabNodeAt(0).collapsed());
}

IN_PROC_BROWSER_TEST_F(UpstreamTreeTabsBrowserTest,
                       ActivatingHiddenTabExpandsAncestors) {
  CreateTree();
  model().ActivateTabAt(3);
  model().SetTreeTabNodeCollapsed(GetTreeTabNodeAt(1).id(), true);
  model().SetTreeTabNodeCollapsed(GetTreeTabNodeAt(0).id(), true);
  WaitForViewModelSynced();

  // Expanded asynchronously, not in the middle of the model's notifications.
  model().ActivateTabAt(2);
  ASSERT_TRUE(base::test::RunUntil([&]() {
    return !GetTreeTabNodeAt(0).collapsed() && !GetTreeTabNodeAt(1).collapsed();
  }));
  WaitForViewModelSynced();
  EXPECT_TRUE(GetTabViewAt(2)->GetVisible());
}

IN_PROC_BROWSER_TEST_F(UpstreamTreeTabsBrowserTest, FlattenAndRebuild) {
  CreateTree();

  SetTreeTabsEnabled(false);
  ASSERT_FALSE(model().tree_model());
  WaitForViewModelSynced();
  for (int i = 1; i < model().count(); ++i) {
    EXPECT_EQ(GetTabViewAt(0)->x(), GetTabViewAt(i)->x()) << "tab index: " << i;
  }

  // Normal operations work after flattening.
  AddTabWithoutOpener();
  model().MoveWebContentsAt(0, 2, /*select_after_move=*/false);
  model().CloseWebContentsAt(1, TabCloseTypes::CLOSE_NONE);
  WaitForViewModelSynced();

  SetTreeTabsEnabled(true);
  ASSERT_TRUE(model().tree_model());
  WaitForViewModelSynced();
}

IN_PROC_BROWSER_TEST_F(UpstreamTreeTabsBrowserTest,
                       ViewModelSyncedOnModelChanges) {
  CreateTree();

  // Move a tree parent.
  model().MoveWebContentsAt(0, 3, /*select_after_move=*/false);
  WaitForViewModelSynced();

  // Close a tree parent. Its children are moved to its parent.
  model().ActivateTabAt(0);
  AddChildOfActiveTab();
  WaitForViewModelSynced();
  model().CloseWebContentsAt(0, TabCloseTypes::CLOSE_NONE);
  WaitForViewModelSynced();

  // Pin and unpin.
  model().SetTabPinned(0, true);
  WaitForViewModelSynced();
  model().SetTabPinned(0, false);
  WaitForViewModelSynced();

  // Group and ungroup.
  tabs::TabInterface* tab = model().GetTabAtIndex(1);
  model().AddToNewGroup({1});
  WaitForViewModelSynced();
  ASSERT_TRUE(tab->GetGroup());
  model().RemoveFromGroup({model().GetIndexOfTab(tab)});
  WaitForViewModelSynced();
}

IN_PROC_BROWSER_TEST_F(UpstreamTreeTabsBrowserTest,
                       PressingTreeParentSelectsDescendants) {
  CreateTree();
  model().ActivateTabAt(3);
  LayoutNow();

  // Pressing a tree parent selects its descendants as well, so that dragging
  // it moves the whole subtree.
  TabView* tab_view = GetTabViewAt(0);
  const gfx::PointF point(tab_view->width() / 2, tab_view->height() / 2);
  static_cast<views::View*>(tab_view)->OnMousePressed(ui::MouseEvent(
      ui::EventType::kMousePressed, point, point, base::TimeTicks::Now(),
      ui::EF_LEFT_MOUSE_BUTTON, ui::EF_LEFT_MOUSE_BUTTON));
  EXPECT_EQ(0, model().active_index());
  EXPECT_TRUE(model().IsTabSelected(0));
  EXPECT_TRUE(model().IsTabSelected(1));
  EXPECT_TRUE(model().IsTabSelected(2));
  EXPECT_FALSE(model().IsTabSelected(3));

  // Releasing without dragging selects the tab only.
  static_cast<views::View*>(tab_view)->OnMouseReleased(ui::MouseEvent(
      ui::EventType::kMouseReleased, point, point, base::TimeTicks::Now(),
      ui::EF_LEFT_MOUSE_BUTTON, ui::EF_LEFT_MOUSE_BUTTON));
  EXPECT_TRUE(model().IsTabSelected(0));
  EXPECT_FALSE(model().IsTabSelected(1));
  EXPECT_FALSE(model().IsTabSelected(2));

  // A modified click edits the selection as usual.
  model().ActivateTabAt(3);
  static_cast<views::View*>(tab_view)->OnMousePressed(ui::MouseEvent(
      ui::EventType::kMousePressed, point, point, base::TimeTicks::Now(),
      ui::EF_LEFT_MOUSE_BUTTON | ui::EF_PLATFORM_ACCELERATOR,
      ui::EF_LEFT_MOUSE_BUTTON));
  static_cast<views::View*>(tab_view)->OnMouseReleased(ui::MouseEvent(
      ui::EventType::kMouseReleased, point, point, base::TimeTicks::Now(),
      ui::EF_LEFT_MOUSE_BUTTON | ui::EF_PLATFORM_ACCELERATOR,
      ui::EF_LEFT_MOUSE_BUTTON));
  EXPECT_TRUE(model().IsTabSelected(0));
  EXPECT_FALSE(model().IsTabSelected(1));
  EXPECT_FALSE(model().IsTabSelected(2));
}

// Moving the selected subtree, as dragging does, keeps it nested and the view
// model in sync.
IN_PROC_BROWSER_TEST_F(UpstreamTreeTabsBrowserTest, MoveSelectedSubtree) {
  CreateTree();
  std::vector<tabs::TabInterface*> tabs;
  for (int i = 0; i < model().count(); ++i) {
    tabs.push_back(model().GetTabAtIndex(i));
  }
  model().ActivateTabAt(1);
  model().SelectTabAt(2);
  ASSERT_TRUE(model().IsTabSelected(1));
  ASSERT_TRUE(model().IsTabSelected(2));

  // Move tab 1 and 2 to the end.
  model().MoveSelectedTabsTo(2, std::nullopt);
  WaitForViewModelSynced();
  EXPECT_EQ(tabs[0], model().GetTabAtIndex(0));
  EXPECT_EQ(tabs[3], model().GetTabAtIndex(1));
  EXPECT_EQ(tabs[1], model().GetTabAtIndex(2));
  EXPECT_EQ(tabs[2], model().GetTabAtIndex(3));
  EXPECT_EQ(GetTreeTabNodeAt(2).level() + 1, GetTreeTabNodeAt(3).level());
  EXPECT_LT(GetTabViewAt(2)->x(), GetTabViewAt(3)->x());
}

namespace {

// Returns true if every unpinned tab is wrapped by a tree tab node.
bool AreUnpinnedTabsInTree(TabStripModel* model) {
  for (int i = model->IndexOfFirstNonPinnedTab(); i < model->count(); ++i) {
    if (!tabs::TreeTabNodeTabCollection::GetNearestTreeTabNodeCollection(
            model->GetTabAtIndex(i))) {
      return false;
    }
  }
  return true;
}

}  // namespace

// Detaching a split in a group, as dragging it out of the window does.
IN_PROC_BROWSER_TEST_F(UpstreamTreeTabsBrowserTest, DetachSplitInGroup) {
  CreateTree();
  AddTabWithoutOpener();
  AddTabWithoutOpener();
  ASSERT_EQ(6, model().count());
  model().ActivateTabAt(4);
  model().AddToNewSplit({5}, {},
                        split_tabs::SplitTabCreatedSource::kTabContextMenu);
  ASSERT_TRUE(model().GetTabAtIndex(4)->IsSplit());
  model().AddToNewGroup({3, 4, 5});
  ASSERT_TRUE(model().GetTabAtIndex(4)->GetGroup());
  ASSERT_TRUE(model().GetTabAtIndex(4)->IsSplit());
  EXPECT_TRUE(AreUnpinnedTabsInTree(&model()));

  auto detached = model().DetachTabsAndCollectionsForInsertion({4, 5});
  EXPECT_EQ(1u, detached.size());
  EXPECT_TRUE(AreUnpinnedTabsInTree(&model()));
}
