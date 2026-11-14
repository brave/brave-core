/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "base/command_line.h"
#include "base/location.h"
#include "base/strings/string_number_conversions.h"
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
#include "chrome/browser/ui/tabs/tab_group_model.h"
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
#include "components/split_tabs/split_tab_id.h"
#include "components/split_tabs/split_tab_visual_data.h"
#include "components/tab_groups/tab_group_id.h"
#include "components/tabs/public/tab_collection.h"
#include "components/tabs/public/tab_group.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/page_transition_types.h"
#include "ui/gfx/animation/animation.h"
#include "ui/gfx/animation/animation_test_api.h"
#include "ui/gfx/range/range.h"
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
  // tree tab node collections unwrapped.
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
    const auto expected =
        tree_tab_view_utils::GetUnwrappedChildren(*collection);
    if (expected.size() != unpinned->children().size()) {
      return false;
    }
    for (size_t i = 0; i < expected.size(); ++i) {
      const tabs::TabCollectionNodeHandle expected_handle = std::visit(
          [](const auto& child) -> tabs::TabCollectionNodeHandle {
            return child->GetHandle();
          },
          expected[i].get());
      if (unpinned->children()[i]->GetHandle() != expected_handle) {
        return false;
      }
    }
    return true;
  }

  // Describes a node handle as "tab<index>" or "collection<type>", for
  // diagnostics.
  std::string DescribeHandle(const tabs::TabCollectionNodeHandle& handle) {
    if (const auto* tab_handle = std::get_if<tabs::TabHandle>(&handle)) {
      const tabs::TabInterface* tab = tab_handle->Get();
      return tab ? "tab" + base::NumberToString(model().GetIndexOfTab(tab))
                 : "dead-tab";
    }
    const tabs::TabCollection* collection =
        std::get<tabs::TabCollectionHandle>(handle).Get();
    return collection
               ? "collection" +
                     base::NumberToString(static_cast<int>(collection->type()))
               : "dead-collection";
  }

  // Describes the expected and the actual children of the unpinned node.
  std::string DescribeViewModel() {
    TabCollectionNode* unpinned =
        root_node()->GetChildNodeOfType(TabCollectionNode::Type::UNPINNED);
    if (!unpinned) {
      return "no unpinned node";
    }
    const auto* collection =
        std::get<tabs::ConstDanglingUntriagedTabCollection>(
            unpinned->GetNodeData())
            .get();
    std::string result = "expected:";
    for (const auto& child :
         tree_tab_view_utils::GetUnwrappedChildren(*collection)) {
      result += " " + DescribeHandle(std::visit(
                          [](const auto& c) -> tabs::TabCollectionNodeHandle {
                            return c->GetHandle();
                          },
                          child.get()));
    }
    result += " / actual:";
    for (const auto& child : unpinned->children()) {
      result += " " + DescribeHandle(child->GetHandle());
    }
    return result;
  }

  void WaitForViewModelSynced(
      base::Location location = base::Location::Current()) {
    SCOPED_TRACE(location.ToString());
    ASSERT_TRUE(base::test::RunUntil([&]() { return IsViewModelSynced(); }))
        << "The view model doesn't mirror the tab collection hierarchy. "
        << DescribeViewModel();
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

IN_PROC_BROWSER_TEST_F(UpstreamTreeTabsBrowserTest, FlattenAndRebuild) {
  CreateTree();

  SetTreeTabsEnabled(false);
  ASSERT_FALSE(model().tree_model());
  WaitForViewModelSynced();
  TabView* first_view = GetTabViewAt(0);
  ASSERT_TRUE(first_view);
  for (int i = 1; i < model().count(); ++i) {
    TabView* view = GetTabViewAt(i);
    ASSERT_TRUE(view) << "tab index: " << i;
    EXPECT_EQ(first_view->x(), view->x()) << "tab index: " << i;
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
                       ViewModelSyncedOnCloseTabs) {
  CreateTree();

  // Close a leaf.
  model().CloseWebContentsAt(2, TabCloseTypes::CLOSE_NONE);
  WaitForViewModelSynced();

  // Close the last tab and then the root of a subtree.
  model().CloseWebContentsAt(2, TabCloseTypes::CLOSE_NONE);
  WaitForViewModelSynced();
  model().CloseWebContentsAt(0, TabCloseTypes::CLOSE_NONE);
  WaitForViewModelSynced();
  EXPECT_EQ(1, model().count());
}

IN_PROC_BROWSER_TEST_F(UpstreamTreeTabsBrowserTest,
                       ViewModelSyncedOnMoveSelectedSubtree) {
  CreateTree();

  // Move a parent together with its descendants, keeping the subtree.
  model().ActivateTabAt(0);
  model().ExtendSelectionTo(2);
  model().MoveSelectedTabsTo(3, std::nullopt);
  WaitForViewModelSynced();

  // Move only a child out of the middle of the tree.
  model().ActivateTabAt(1);
  model().MoveSelectedTabsTo(0, std::nullopt);
  WaitForViewModelSynced();
}

IN_PROC_BROWSER_TEST_F(UpstreamTreeTabsBrowserTest,
                       ViewModelSyncedOnSplitAndUnsplit) {
  CreateTree();

  // Make a split of a parent and its child.
  model().ActivateTabAt(1);
  const split_tabs::SplitTabId split_id =
      model().AddToNewSplit({2}, split_tabs::SplitTabVisualData(),
                            split_tabs::SplitTabCreatedSource::kTabContextMenu);
  WaitForViewModelSynced();

  model().RemoveSplit(split_id);
  WaitForViewModelSynced();

  // Make a split of tabs in different trees.
  model().ActivateTabAt(0);
  model().AddToNewSplit({model().count() - 1}, split_tabs::SplitTabVisualData(),
                        split_tabs::SplitTabCreatedSource::kTabContextMenu);
  WaitForViewModelSynced();
}

IN_PROC_BROWSER_TEST_F(UpstreamTreeTabsBrowserTest,
                       ViewModelSyncedOnGroupOperations) {
  CreateTree();

  // Group a tab that has descendants. The group is wrapped by a tree node.
  const tab_groups::TabGroupId group = model().AddToNewGroup({0});
  WaitForViewModelSynced();

  // Add tabs to the existing group, from the tree and from outside of it.
  model().AddToExistingGroup({2}, group);
  WaitForViewModelSynced();
  model().AddToExistingGroup({model().count() - 1}, group);
  WaitForViewModelSynced();

  model().MoveGroupTo(group, 1);
  WaitForViewModelSynced();

  // Empty the group. Its descendants are moved to the group's parent.
  // The group is destroyed when it gets empty, so don't touch it after
  // collecting the tabs.
  std::vector<tabs::TabInterface*> tabs_in_group;
  const gfx::Range range =
      model().group_model()->GetTabGroup(group)->ListTabs();
  for (size_t i = range.start(); i < range.end(); ++i) {
    tabs_in_group.push_back(model().GetTabAtIndex(static_cast<int>(i)));
  }
  ASSERT_FALSE(tabs_in_group.empty());
  for (tabs::TabInterface* tab : tabs_in_group) {
    model().RemoveFromGroup({model().GetIndexOfTab(tab)});
    WaitForViewModelSynced();
  }
}
