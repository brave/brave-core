// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "base/test/scoped_feature_list.h"
#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "brave/browser/ui/tabs/brave_tab_strip_model.h"
#include "brave/browser/ui/tabs/features.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_group_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/prefs/pref_service.h"
#include "components/tabs/public/tab_strip_collection.h"
#include "components/tabs/public/unpinned_tab_collection.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"

class TreeTabsBrowserTest : public InProcessBrowserTest {
 protected:
  TreeTabsBrowserTest() {
    feature_list_.InitAndEnableFeature(tabs::features::kBraveTreeTab);
  }
  ~TreeTabsBrowserTest() override = default;

  Profile* profile() { return browser()->profile(); }
  BraveTabStripModel& tab_strip_model() {
    return *static_cast<BraveTabStripModel*>(browser()->tab_strip_model());
  }
  tabs::TabStripCollection& tab_strip_collection() {
    return tab_strip_model().GetTabStripCollectionForTesting();
  }
  tabs::UnpinnedTabCollection& unpinned_collection() {
    return *tab_strip_collection().unpinned_collection();
  }
  void AddTab() {
    auto contents = content::WebContents::Create(
        content::WebContents::CreateParams(profile()));
    tab_strip_model().AppendWebContents(std::move(contents), true);
  }

  void SetTreeTabsEnabled(bool enabled) {
    profile()->GetPrefs()->SetBoolean(brave_tabs::kTreeTabsEnabled, enabled);
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    // Prerequisite for enabling tree tabs.
    profile()->GetPrefs()->SetBoolean(brave_tabs::kVerticalTabsEnabled, true);
  }

 private:
  base::test::ScopedFeatureList feature_list_;
};

IN_PROC_BROWSER_TEST_F(TreeTabsBrowserTest,
                       BuildTreeTabs_NormalTabsShouldBeWrappedWithTreeTabNode) {
  // Add multiple tabs to the browser.
  for (int i = 0; i < 3; ++i) {
    AddTab();
  }

  // Verify we have tabs in flat structure initially (3 added + 1 initial tab).
  ASSERT_EQ(4, tab_strip_model().count());
  for (int i = 0; i < tab_strip_model().count(); ++i) {
    ASSERT_EQ(tab_strip_model().GetTabAtIndex(i)->GetParentCollection()->type(),
              tabs::TabCollection::Type::UNPINNED);
  }

  SetTreeTabsEnabled(true);

  // Verify that each tab is wrapped in a TreeTabNode.
  for (int i = 0; i < tab_strip_model().count(); ++i) {
    auto* parent_collection =
        tab_strip_model().GetTabAtIndex(i)->GetParentCollection();
    EXPECT_EQ(parent_collection->type(), tabs::TabCollection::Type::TREE_NODE);
    EXPECT_EQ(parent_collection->GetParentCollection(), &unpinned_collection());
    EXPECT_EQ(parent_collection->ChildCount(), 1u);
  }

  // Verify that the unpinned collection has TreeTabNode with correct tab at
  // correct index.
  for (int i = 0; i < tab_strip_model().count(); ++i) {
    auto* tab_interface = tab_strip_model().GetTabAtIndex(i);
    EXPECT_EQ(
        static_cast<size_t>(i),
        *unpinned_collection().GetDirectChildIndexOfCollectionContainingTab(
            tab_interface));
  }
}

IN_PROC_BROWSER_TEST_F(TreeTabsBrowserTest,
                       FlattenTreeTabs_ConvertTreeNodesToFlatStructure) {
  // Add multiple tabs to the browser.
  for (int i = 0; i < 3; ++i) {
    AddTab();
  }

  // Verify we have tabs in flat structure initially.
  ASSERT_EQ(4, tab_strip_model().count());  // 3 added + 1 initial tab
  for (int i = 0; i < tab_strip_model().count(); ++i) {
    ASSERT_EQ(tab_strip_model().GetTabAtIndex(i)->GetParentCollection()->type(),
              tabs::TabCollection::Type::UNPINNED);
  }

  SetTreeTabsEnabled(true);

  // Verify tree structure is created.
  for (int i = 0; i < tab_strip_model().count(); ++i) {
    auto* parent_collection =
        tab_strip_model().GetTabAtIndex(i)->GetParentCollection();
    ASSERT_EQ(parent_collection->type(), tabs::TabCollection::Type::TREE_NODE);
    ASSERT_EQ(parent_collection->GetParentCollection(), &unpinned_collection());
  }

  // Store tab pointers for verification after flattening.
  std::vector<tabs::TabInterface*> original_tabs;
  for (int i = 0; i < tab_strip_model().count(); ++i) {
    original_tabs.push_back(tab_strip_model().GetTabAtIndex(i));
  }

  SetTreeTabsEnabled(false);

  // Verify tabs are back to flat structure.
  EXPECT_EQ(4, tab_strip_model().count());
  for (int i = 0; i < tab_strip_model().count(); ++i) {
    EXPECT_EQ(tab_strip_model().GetTabAtIndex(i)->GetParentCollection()->type(),
              tabs::TabCollection::Type::UNPINNED);
    // Verify the same tabs are still present in correct order.
    EXPECT_EQ(original_tabs[i], tab_strip_model().GetTabAtIndex(i));
  }

  // Verify unpinned collection has correct number of tabs.
  EXPECT_EQ(4u, unpinned_collection().TabCountRecursive());
}

IN_PROC_BROWSER_TEST_F(TreeTabsBrowserTest,
                       BuildAndFlattenTreeTabs_RoundTripPreservesOrder) {
  // Add tabs with specific order.
  for (int i = 0; i < 5; ++i) {
    AddTab();
  }

  // Store original tab order.
  std::vector<tabs::TabInterface*> original_tab_order;
  for (int i = 0; i < tab_strip_model().count(); ++i) {
    original_tab_order.push_back(tab_strip_model().GetTabAtIndex(i));
  }

  // Verify initial flat structure.
  for (int i = 0; i < tab_strip_model().count(); ++i) {
    ASSERT_EQ(tab_strip_model().GetTabAtIndex(i)->GetParentCollection()->type(),
              tabs::TabCollection::Type::UNPINNED);
  }

  SetTreeTabsEnabled(true);

  // Verify tree structure.
  for (int i = 0; i < tab_strip_model().count(); ++i) {
    ASSERT_EQ(tab_strip_model().GetTabAtIndex(i)->GetParentCollection()->type(),
              tabs::TabCollection::Type::TREE_NODE);
  }

  // Flatten back to original structure.
  SetTreeTabsEnabled(false);

  // Verify original order is preserved.
  EXPECT_EQ(original_tab_order.size(),
            static_cast<size_t>(tab_strip_model().count()));
  for (int i = 0; i < tab_strip_model().count(); ++i) {
    EXPECT_EQ(original_tab_order[i], tab_strip_model().GetTabAtIndex(i));
    EXPECT_EQ(tab_strip_model().GetTabAtIndex(i)->GetParentCollection()->type(),
              tabs::TabCollection::Type::UNPINNED);
  }
}

IN_PROC_BROWSER_TEST_F(TreeTabsBrowserTest, BuildTreeTabs_WithGroupedTabs) {
  // Add tabs to the browser.
  for (int i = 0; i < 4; ++i) {
    AddTab();
  }

  // Create a tab group with some tabs (indices 1-2).
  tab_groups::TabGroupId group_id = tab_strip_model().AddToNewGroup({1, 2});

  // Verify group is created.
  ASSERT_TRUE(tab_strip_model().group_model()->ContainsTabGroup(group_id));

  SetTreeTabsEnabled(true);

  // Verify tabs outside group are wrapped in TreeTabNodes within unpinned
  // collection.
  EXPECT_EQ(tab_strip_model().GetTabAtIndex(0)->GetParentCollection()->type(),
            tabs::TabCollection::Type::TREE_NODE);
  EXPECT_EQ(tab_strip_model().GetTabAtIndex(3)->GetParentCollection()->type(),
            tabs::TabCollection::Type::TREE_NODE);
  EXPECT_EQ(tab_strip_model().GetTabAtIndex(4)->GetParentCollection()->type(),
            tabs::TabCollection::Type::TREE_NODE);

  // Verify grouped tabs are wrapped in TreeTabNodes within the group.
  EXPECT_EQ(tab_strip_model().GetTabAtIndex(1)->GetParentCollection()->type(),
            tabs::TabCollection::Type::TREE_NODE);
  EXPECT_EQ(tab_strip_model().GetTabAtIndex(2)->GetParentCollection()->type(),
            tabs::TabCollection::Type::TREE_NODE);

  // Verify the TreeTabNodes containing grouped tabs are within a group
  // collection.
  auto* tab1_tree_node =
      tab_strip_model().GetTabAtIndex(1)->GetParentCollection();
  auto* tab2_tree_node =
      tab_strip_model().GetTabAtIndex(2)->GetParentCollection();
  EXPECT_EQ(tab1_tree_node->GetParentCollection()->type(),
            tabs::TabCollection::Type::GROUP);
  EXPECT_EQ(tab2_tree_node->GetParentCollection()->type(),
            tabs::TabCollection::Type::GROUP);
}

IN_PROC_BROWSER_TEST_F(TreeTabsBrowserTest, FlattenTreeTabs_WithGroupedTabs) {
  // Add tabs and create a group.
  for (int i = 0; i < 4; ++i) {
    AddTab();
  }

  std::vector<int> group_indices = {1, 2};
  tab_groups::TabGroupId group_id =
      tab_strip_model().AddToNewGroup(group_indices);

  SetTreeTabsEnabled(true);

  // Store original tabs for verification.
  std::vector<tabs::TabInterface*> original_tabs;
  for (int i = 0; i < tab_strip_model().count(); ++i) {
    original_tabs.push_back(tab_strip_model().GetTabAtIndex(i));
  }

  SetTreeTabsEnabled(false);

  // Verify tabs are preserved in correct order
  EXPECT_EQ(5, tab_strip_model().count());
  for (int i = 0; i < tab_strip_model().count(); ++i) {
    EXPECT_EQ(original_tabs[i], tab_strip_model().GetTabAtIndex(i));
  }

  // Verify group is still present.
  EXPECT_TRUE(tab_strip_model().group_model()->ContainsTabGroup(group_id));

  // Verify grouped tabs are back to being direct children of the group.
  EXPECT_EQ(tab_strip_model().GetTabAtIndex(1)->GetParentCollection()->type(),
            tabs::TabCollection::Type::GROUP);
  EXPECT_EQ(tab_strip_model().GetTabAtIndex(2)->GetParentCollection()->type(),
            tabs::TabCollection::Type::GROUP);

  // Verify ungrouped tabs are direct children of unpinned collection.
  EXPECT_EQ(tab_strip_model().GetTabAtIndex(0)->GetParentCollection()->type(),
            tabs::TabCollection::Type::UNPINNED);
  EXPECT_EQ(tab_strip_model().GetTabAtIndex(3)->GetParentCollection()->type(),
            tabs::TabCollection::Type::UNPINNED);
  EXPECT_EQ(tab_strip_model().GetTabAtIndex(4)->GetParentCollection()->type(),
            tabs::TabCollection::Type::UNPINNED);
}

IN_PROC_BROWSER_TEST_F(TreeTabsBrowserTest,
                       TreeTabNode_OnlyAddedToUnpinnedCollection) {
  // Add a tab and build tree structure.
  AddTab();
  tab_strip_model().SetTabPinned(0, /*pinned=*/true);

  // Verify we have 2 tabs (1 initial + 1 added).
  ASSERT_EQ(2, tab_strip_model().count());

  SetTreeTabsEnabled(true);

  // Verify that TreeTabNodes are created as children of unpinned collection.
  for (int i = 0; i < tab_strip_model().count(); ++i) {
    auto* parent_collection =
        tab_strip_model().GetTabAtIndex(i)->GetParentCollection();
    if (tab_strip_model().IsTabPinned(i)) {
      EXPECT_EQ(parent_collection->type(), tabs::TabCollection::Type::PINNED);
      // Verify TreeTabNode's parent is the pinned collection so it's not
      // wrapped in another TreeTabNode.
    } else {
      EXPECT_EQ(parent_collection->type(),
                tabs::TabCollection::Type::TREE_NODE);
      // Verify TreeTabNode's parent is the unpinned collection.
      EXPECT_EQ(parent_collection->GetParentCollection(),
                &unpinned_collection());
    }
  }

  // Verify unpinned collection contains TreeTabNodes as direct children.
  // The unpinned collection should have 1 TreeTabNode children.
  EXPECT_EQ(1u, unpinned_collection().ChildCount());
  EXPECT_EQ(1u, unpinned_collection().TabCountRecursive());
}

IN_PROC_BROWSER_TEST_F(TreeTabsBrowserTest,
                       TreeTabPrefsEnabled_AutomaticallyBuildsTreeStructure) {
  // Add multiple tabs to the browser.
  for (int i = 0; i < 3; ++i) {
    AddTab();
  }

  // Verify initial flat structure.
  ASSERT_EQ(4, tab_strip_model().count());
  for (int i = 0; i < tab_strip_model().count(); ++i) {
    ASSERT_EQ(tab_strip_model().GetTabAtIndex(i)->GetParentCollection()->type(),
              tabs::TabCollection::Type::UNPINNED);
  }

  // Enable both tree tabs and vertical tabs through preference change.
  // Both must be true for OnTreeTabRelatedPrefChanged to call BuildTreeTabs().
  profile()->GetPrefs()->SetBoolean(brave_tabs::kTreeTabsEnabled, true);
  profile()->GetPrefs()->SetBoolean(brave_tabs::kVerticalTabsEnabled, true);

  // Verify tree structure is created automatically via
  // OnTreeTabRelatedPrefChanged.
  for (int i = 0; i < tab_strip_model().count(); ++i) {
    auto* parent_collection =
        tab_strip_model().GetTabAtIndex(i)->GetParentCollection();
    EXPECT_EQ(parent_collection->type(), tabs::TabCollection::Type::TREE_NODE);
    EXPECT_EQ(parent_collection->GetParentCollection(), &unpinned_collection());
    EXPECT_EQ(parent_collection->ChildCount(), 1u);
  }
}

IN_PROC_BROWSER_TEST_F(TreeTabsBrowserTest,
                       TreeTabPrefsDisabled_AutomaticallyFlattensStructure) {
  // Add multiple tabs.
  for (int i = 0; i < 3; ++i) {
    AddTab();
  }

  // Enable tree tabs first to create tree structure.
  profile()->GetPrefs()->SetBoolean(brave_tabs::kTreeTabsEnabled, true);
  profile()->GetPrefs()->SetBoolean(brave_tabs::kVerticalTabsEnabled, true);

  // Verify tree structure is created.
  for (int i = 0; i < tab_strip_model().count(); ++i) {
    ASSERT_EQ(tab_strip_model().GetTabAtIndex(i)->GetParentCollection()->type(),
              tabs::TabCollection::Type::TREE_NODE);
  }

  // Store tab pointers for order verification.
  std::vector<tabs::TabInterface*> original_tabs;
  for (int i = 0; i < tab_strip_model().count(); ++i) {
    original_tabs.push_back(tab_strip_model().GetTabAtIndex(i));
  }

  // Disable tree tabs through preference change.
  // This should trigger OnTreeTabRelatedPrefChanged to call FlattenTreeTabs().
  profile()->GetPrefs()->SetBoolean(brave_tabs::kTreeTabsEnabled, false);

  // Verify structure is flattened automatically.
  EXPECT_EQ(4, tab_strip_model().count());
  for (int i = 0; i < tab_strip_model().count(); ++i) {
    EXPECT_EQ(tab_strip_model().GetTabAtIndex(i)->GetParentCollection()->type(),
              tabs::TabCollection::Type::UNPINNED);
    // Verify the same tabs are still present in correct order.
    EXPECT_EQ(original_tabs[i], tab_strip_model().GetTabAtIndex(i));
  }
}

IN_PROC_BROWSER_TEST_F(
    TreeTabsBrowserTest,
    VerticalTabPrefsDisabled_AutomaticallyFlattensStructure) {
  // Add tabs.
  for (int i = 0; i < 2; ++i) {
    AddTab();
  }

  // Enable both preferences to create tree structure.
  profile()->GetPrefs()->SetBoolean(brave_tabs::kTreeTabsEnabled, true);
  profile()->GetPrefs()->SetBoolean(brave_tabs::kVerticalTabsEnabled, true);

  // Verify tree structure is created.
  for (int i = 0; i < tab_strip_model().count(); ++i) {
    ASSERT_EQ(tab_strip_model().GetTabAtIndex(i)->GetParentCollection()->type(),
              tabs::TabCollection::Type::TREE_NODE);
  }

  // Disable vertical tabs while keeping tree tabs enabled.
  // This should trigger OnTreeTabRelatedPrefChanged to call FlattenTreeTabs()
  // because both prefs must be true for tree structure.
  profile()->GetPrefs()->SetBoolean(brave_tabs::kVerticalTabsEnabled, false);

  // Should flatten because both prefs must be true for tree structure.
  for (int i = 0; i < tab_strip_model().count(); ++i) {
    EXPECT_EQ(tab_strip_model().GetTabAtIndex(i)->GetParentCollection()->type(),
              tabs::TabCollection::Type::UNPINNED);
  }
}

IN_PROC_BROWSER_TEST_F(TreeTabsBrowserTest,
                       OnlyVerticalTabsEnabled_KeepsFlatStructure) {
  // Add tabs.
  for (int i = 0; i < 2; ++i) {
    AddTab();
  }

  // Initially tree tabs pref is disabled, but vertical tabs are enabled in the
  // test
  ASSERT_FALSE(profile()->GetPrefs()->GetBoolean(brave_tabs::kTreeTabsEnabled));
  ASSERT_TRUE(
      profile()->GetPrefs()->GetBoolean(brave_tabs::kVerticalTabsEnabled));

  // Verify flat structure.
  for (int i = 0; i < tab_strip_model().count(); ++i) {
    ASSERT_EQ(tab_strip_model().GetTabAtIndex(i)->GetParentCollection()->type(),
              tabs::TabCollection::Type::UNPINNED);
  }
}

IN_PROC_BROWSER_TEST_F(TreeTabsBrowserTest,
                       OnlyTreeTabsEnabled_KeepsFlatStructure) {
  // Add tabs.
  for (int i = 0; i < 2; ++i) {
    AddTab();
  }

  // Initially tree tabs are disabled, but vertical tabs are enabled.
  ASSERT_FALSE(profile()->GetPrefs()->GetBoolean(brave_tabs::kTreeTabsEnabled));
  ASSERT_TRUE(
      profile()->GetPrefs()->GetBoolean(brave_tabs::kVerticalTabsEnabled));

  // Verify initial flat structure.
  for (int i = 0; i < tab_strip_model().count(); ++i) {
    ASSERT_EQ(tab_strip_model().GetTabAtIndex(i)->GetParentCollection()->type(),
              tabs::TabCollection::Type::UNPINNED);
  }

  // Enable only tree tabs (not vertical tabs).
  profile()->GetPrefs()->SetBoolean(brave_tabs::kVerticalTabsEnabled, false);
  profile()->GetPrefs()->SetBoolean(brave_tabs::kTreeTabsEnabled, true);

  // Should remain flat because both prefs must be true for tree structure.
  for (int i = 0; i < tab_strip_model().count(); ++i) {
    EXPECT_EQ(tab_strip_model().GetTabAtIndex(i)->GetParentCollection()->type(),
              tabs::TabCollection::Type::UNPINNED);
  }
}
