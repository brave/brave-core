// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "base/run_loop.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "brave/browser/ui/tabs/brave_tab_strip_model.h"
#include "brave/components/tabs/public/tree_tab_node_tab_collection.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/tab_group_sync/tab_group_sync_service_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/features.h"
#include "chrome/browser/ui/tabs/tab_group_model.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/prefs/pref_service.h"
#include "components/saved_tab_groups/public/tab_group_sync_service.h"
#include "components/tabs/public/tab_strip_collection.h"
#include "components/tabs/public/unpinned_tab_collection.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

class TreeTabsBrowserTest : public InProcessBrowserTest {
 protected:
  TreeTabsBrowserTest() {
    feature_list_.InitAndEnableFeature(tabs::kBraveTreeTab);
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
    tab_strip_model().AppendWebContents(CreateWebContents(), true);
  }
  std::unique_ptr<content::WebContents> CreateWebContents() {
    return content::WebContents::Create(
        content::WebContents::CreateParams(profile()));
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
  auto* tab_groups_service =
      tab_groups::TabGroupSyncServiceFactory::GetForProfile(
          browser()->profile());
  ASSERT_TRUE(tab_groups_service);
  tab_groups_service->SetIsInitializedForTesting(true);

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
  auto* tab_groups_service =
      tab_groups::TabGroupSyncServiceFactory::GetForProfile(
          browser()->profile());
  ASSERT_TRUE(tab_groups_service);
  tab_groups_service->SetIsInitializedForTesting(true);

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

IN_PROC_BROWSER_TEST_F(TreeTabsBrowserTest, AddTabRecursive) {
  // 1. Not in tree mode: should call base method ------------------------------
  // Ensure tree tab mode is disabled.
  SetTreeTabsEnabled(false);

  // Create a tab to add.
  auto tab_interface =
      std::make_unique<tabs::TabModel>(CreateWebContents(), &tab_strip_model());

  // Add tab when not in tree mode.
  tab_strip_model().AddTab(std::move(tab_interface), -1 /*to the last*/,
                           ui::PAGE_TRANSITION_AUTO_BOOKMARK,
                           ADD_INHERIT_OPENER | ADD_ACTIVE);

  // Verify tab was added normally (not wrapped in TreeTabNode).
  auto* added_tab =
      tab_strip_model().GetTabAtIndex(tab_strip_model().count() - 1);
  EXPECT_TRUE(static_cast<tabs::TabModel*>(added_tab)->opener())
      << "ADD_INHERIT_OPENER forces opener";
  EXPECT_EQ(added_tab->GetParentCollection()->type(),
            tabs::TabCollection::Type::UNPINNED);

  // 2. In tree mode without opener: should wrap in TreeTabNode but shouldn't be
  // nested in other tree tab node ---------------------------------------------
  // Enable tree tabs to enter tree mode.
  SetTreeTabsEnabled(true);

  // Create a tab with no opener.
  tab_interface =
      std::make_unique<tabs::TabModel>(CreateWebContents(), &tab_strip_model());
  ASSERT_EQ(nullptr, tab_interface->opener());

  // Add tab in tree mode without opener.
  // note that PAGE_TRANSITION_AUTO_BOOKMARK is used as LINK or TYPED transition
  // type would be treated as having an opener (the current active tab).
  tab_strip_model().AddTab(std::move(tab_interface), -1 /*to the last*/,
                           ui::PAGE_TRANSITION_AUTO_BOOKMARK, ADD_ACTIVE);

  // Verify tab was added and wrapped in TreeTabNode but not child of other
  // TreeTabNode.
  added_tab = tab_strip_model().GetTabAtIndex(tab_strip_model().count() - 1);
  EXPECT_EQ(added_tab->GetParentCollection()->type(),
            tabs::TabCollection::Type::TREE_NODE);
  EXPECT_EQ(added_tab->GetParentCollection()->GetParentCollection(),
            &unpinned_collection());

  // 3. In tree mode with opener as previous tab: should be added as child of
  // opener's TreeTabNode ------------------------------------------------------
  //  Create a tab with the previous tab as opener.
  tab_interface =
      std::make_unique<tabs::TabModel>(CreateWebContents(), &tab_strip_model());
  auto* opener_tab =
      tab_strip_model().GetTabAtIndex(tab_strip_model().count() - 1);
  tab_interface->set_opener(opener_tab);  // previous tab is the opener.

  // Add tab in tree mode with opener as previous tab.
  tab_strip_model().AddTab(std::move(tab_interface), -1 /*to the last*/,
                           ui::PAGE_TRANSITION_AUTO_BOOKMARK, ADD_NONE);

  // Verify tab was added as child of opener's TreeTabNode.
  added_tab = tab_strip_model().GetTabAtIndex(tab_strip_model().count() - 1);
  EXPECT_EQ(added_tab->GetParentCollection()->type(),
            tabs::TabCollection::Type::TREE_NODE);

  EXPECT_EQ(added_tab->GetParentCollection()->GetParentCollection()->type(),
            tabs::TabCollection::Type::TREE_NODE);

  EXPECT_EQ(added_tab->GetParentCollection()->GetParentCollection(),
            opener_tab->GetParentCollection());

  // The opener's TreeTabNode should now have 2 children (1 original + 1 added).
  EXPECT_EQ(2u, opener_tab->GetParentCollection()->ChildCount());

  // 4. In tree mode with opener. The previous tab is not the opener but the
  // previous tab is a child of opener. In this case, the new tab should be
  // added as a child of the opener's TreeTabNode. -----------------------------
  // add tab in tree mode with opener as the tab before the previous tab.
  opener_tab = tab_strip_model().GetTabAtIndex(tab_strip_model().count() - 2);
  ASSERT_EQ(opener_tab->GetParentCollection()->type(),
            tabs::TabCollection::Type::TREE_NODE);
  ASSERT_EQ(opener_tab->GetParentCollection()->ChildCount(), 2u);

  tab_interface =
      std::make_unique<tabs::TabModel>(CreateWebContents(), &tab_strip_model());
  tab_interface->set_opener(opener_tab);

  tab_strip_model().AddTab(std::move(tab_interface), -1 /*to the last*/,
                           ui::PAGE_TRANSITION_AUTO_BOOKMARK, ADD_NONE);

  added_tab = tab_strip_model().GetTabAtIndex(tab_strip_model().count() - 1);
  EXPECT_EQ(opener_tab, static_cast<tabs::TabModel*>(added_tab)->opener());
  EXPECT_EQ(added_tab->GetParentCollection()->type(),
            tabs::TabCollection::Type::TREE_NODE);
  ASSERT_EQ(opener_tab->GetParentCollection()->ChildCount(), 3u);
  EXPECT_EQ(static_cast<const tabs::TreeTabNodeTabCollection*>(
                opener_tab->GetParentCollection())
                ->GetTopLevelAncestor(),
            static_cast<const tabs::TreeTabNodeTabCollection*>(
                added_tab->GetParentCollection())
                ->GetTopLevelAncestor());

  EXPECT_EQ(opener_tab->GetParentCollection(),
            added_tab->GetParentCollection()->GetParentCollection());
  EXPECT_EQ(opener_tab->GetParentCollection()
                ->GetDirectChildIndexOfCollectionContainingTab(added_tab),
            opener_tab->GetParentCollection()->ChildCount() - 1);

  // 5. In tree mode with opener not as previous tab: should wrap in new
  // TreeTabNode but should not be a child of the opener's TreeTabNode. -------

  // Sets the opener as the first tab so that we have another tree tab node
  // between opener and newly added tab.
  opener_tab = tab_strip_model().GetTabAtIndex(0);
  ASSERT_EQ(opener_tab->GetParentCollection()->type(),
            tabs::TabCollection::Type::TREE_NODE);
  tab_interface =
      std::make_unique<tabs::TabModel>(CreateWebContents(), &tab_strip_model());
  tab_interface->set_opener(opener_tab);
  tab_strip_model().AddTab(std::move(tab_interface), -1 /*to the last*/,
                           ui::PAGE_TRANSITION_AUTO_BOOKMARK, ADD_NONE);

  // Verify tab was added and wrapped in its own TreeTabNode, but not as child
  // of the opener's TreeTabNode.
  added_tab = tab_strip_model().GetTabAtIndex(tab_strip_model().count() - 1);
  EXPECT_EQ(opener_tab, static_cast<tabs::TabModel*>(added_tab)->opener());
  EXPECT_EQ(added_tab->GetParentCollection()->type(),
            tabs::TabCollection::Type::TREE_NODE);
  EXPECT_EQ(added_tab->GetParentCollection()->GetParentCollection(),
            &unpinned_collection());
}

// Mock observer for testing OnTreeTabChanged callback.
class MockTabStripModelObserver : public TabStripModelObserver {
 public:
  MOCK_METHOD(void, OnTreeTabChanged, (const TreeTabChange& change), ());
};

IN_PROC_BROWSER_TEST_F(TreeTabsBrowserTest,
                       OnTreeTabChanged_CalledWhenTreeNodeCreated) {
  // Add tabs to the browser.
  for (int i = 0; i < 2; ++i) {
    AddTab();
  }

  // Create and register mock observer.
  MockTabStripModelObserver mock_observer;
  tab_strip_model().AddObserver(&mock_observer);

  // Verify initial flat structure.
  ASSERT_EQ(3, tab_strip_model().count());
  for (int i = 0; i < tab_strip_model().count(); ++i) {
    ASSERT_EQ(tab_strip_model().GetTabAtIndex(i)->GetParentCollection()->type(),
              tabs::TabCollection::Type::UNPINNED);
  }

  // Use RunLoop to wait for all callbacks to be invoked.
  base::RunLoop run_loop;
  int call_count = 0;
  const int expected_calls = 3;

  // Expect OnTreeTabChanged to be called with kNodeCreated for each tab.
  // We have 3 tabs, so expect 3 calls with kNodeCreated type.
  // Also verify the CreatedChange contains valid node reference.
  EXPECT_CALL(mock_observer,
              OnTreeTabChanged(testing::Field(&TreeTabChange::type,
                                              TreeTabChange::kNodeCreated)))
      .Times(expected_calls)
      .WillRepeatedly([&](const TreeTabChange& change) {
        // Verify we can get the CreatedChange delta.
        const auto& created_change = change.GetCreatedChange();
        // Verify the node reference is valid - TreeTabNode should have a valid
        // ID.
        EXPECT_EQ(created_change.node->id(), change.id);
        // Verify the node has an associated tab.
        EXPECT_NE(created_change.node->GetTab(), nullptr);

        // Quit the run loop after all expected callbacks are received.
        if (++call_count == expected_calls) {
          run_loop.Quit();
        }
      });

  // Enable tree tabs - this should trigger OnTreeTabChanged callbacks.
  SetTreeTabsEnabled(true);

  // Wait for all callbacks to be invoked.
  run_loop.Run();

  // Verify tree structure is created.
  for (int i = 0; i < tab_strip_model().count(); ++i) {
    EXPECT_EQ(tab_strip_model().GetTabAtIndex(i)->GetParentCollection()->type(),
              tabs::TabCollection::Type::TREE_NODE);
  }

  tab_strip_model().RemoveObserver(&mock_observer);
}

IN_PROC_BROWSER_TEST_F(TreeTabsBrowserTest,
                       OnTreeTabChanged_CalledWhenTreeNodeDestroyed) {
  // Add tabs and enable tree structure first.
  for (int i = 0; i < 2; ++i) {
    AddTab();
  }

  // Use RunLoop to wait for tree creation callbacks.
  base::RunLoop create_run_loop;
  int create_call_count = 0;
  const int expected_create_calls = 3;

  // Temporarily add observer to wait for tree creation to complete.
  MockTabStripModelObserver temp_observer;
  tab_strip_model().AddObserver(&temp_observer);
  EXPECT_CALL(temp_observer,
              OnTreeTabChanged(testing::Field(&TreeTabChange::type,
                                              TreeTabChange::kNodeCreated)))
      .Times(expected_create_calls)
      .WillRepeatedly([&](const TreeTabChange&) {
        if (++create_call_count == expected_create_calls) {
          create_run_loop.Quit();
        }
      });

  SetTreeTabsEnabled(true);
  create_run_loop.Run();
  tab_strip_model().RemoveObserver(&temp_observer);

  // Verify tree structure is created.
  ASSERT_EQ(3, tab_strip_model().count());
  for (int i = 0; i < tab_strip_model().count(); ++i) {
    ASSERT_EQ(tab_strip_model().GetTabAtIndex(i)->GetParentCollection()->type(),
              tabs::TabCollection::Type::TREE_NODE);
  }

  // Create and register mock observer after tree is built.
  MockTabStripModelObserver mock_observer;
  tab_strip_model().AddObserver(&mock_observer);

  // Expect OnTreeTabChanged to be called with kNodeWillBeDestroyed for each
  // tree node. We have 3 tabs wrapped in tree nodes, so expect 3 calls.
  // Also verify the WillBeDestroyedChange contains valid node reference.
  EXPECT_CALL(mock_observer,
              OnTreeTabChanged(testing::Field(
                  &TreeTabChange::type, TreeTabChange::kNodeWillBeDestroyed)))
      .Times(3)
      .WillRepeatedly([](const TreeTabChange& change) {
        // Verify we can get the WillBeDestroyedChange delta.
        const auto& destroyed_change = change.GetWillBeDestroyedChange();
        // Verify the node reference is valid - TreeTabNode should have a valid
        // ID.
        EXPECT_EQ(destroyed_change.node->id(), change.id);
        // Verify the node has an associated tab before destruction.
        EXPECT_NE(destroyed_change.node->GetTab(), nullptr);
      });

  // Disable tree tabs - this should trigger OnTreeTabChanged callbacks.
  // Note: kNodeWillBeDestroyed is called synchronously, not posted.
  SetTreeTabsEnabled(false);

  // Verify structure is flattened.
  for (int i = 0; i < tab_strip_model().count(); ++i) {
    EXPECT_EQ(tab_strip_model().GetTabAtIndex(i)->GetParentCollection()->type(),
              tabs::TabCollection::Type::UNPINNED);
  }

  tab_strip_model().RemoveObserver(&mock_observer);
}

IN_PROC_BROWSER_TEST_F(
    TreeTabsBrowserTest,
    RemoveTabAtIndexRecursive_WithChildren_MovesChildrenToParent) {
  SetTreeTabsEnabled(true);

  auto* parent_tab = tab_strip_model().GetTabAtIndex(0);

  // Create a child node
  auto tab_interface =
      std::make_unique<tabs::TabModel>(CreateWebContents(), &tab_strip_model());
  tab_interface->set_opener(parent_tab);
  tab_strip_model().AddTab(std::move(tab_interface), -1,
                           ui::PAGE_TRANSITION_AUTO_BOOKMARK, ADD_NONE);

  // Verify tree structure: tab 0 has a child
  // The child tab should be at the last index.
  auto* child_tab =
      tab_strip_model().GetTabAtIndex(tab_strip_model().count() - 1);
  ASSERT_EQ(child_tab->GetParentCollection()->type(),
            tabs::TabCollection::Type::TREE_NODE);
  ASSERT_EQ(child_tab->GetParentCollection()->GetParentCollection(),
            parent_tab->GetParentCollection());
  // Parent's TreeTabNode should have 2 children: parent tab itself + child tab
  // Note: ChildCount includes the parent tab itself as one child
  ASSERT_GE(parent_tab->GetParentCollection()->ChildCount(), 2u);

  int initial_count = tab_strip_model().count();

  // Remove the parent tab (index 0).
  tab_strip_model().CloseWebContentsAt(0, TabCloseTypes::CLOSE_NONE);
  EXPECT_EQ(initial_count - 1, tab_strip_model().count());

  // The child tab should now be at index 0 (where parent was).
  auto* moved_child = tab_strip_model().GetTabAtIndex(0);
  EXPECT_EQ(child_tab, moved_child);

  // The child should now be a direct child of unpinned collection.
  EXPECT_EQ(moved_child->GetParentCollection()->type(),
            tabs::TabCollection::Type::TREE_NODE);
  EXPECT_EQ(moved_child->GetParentCollection()->GetParentCollection(),
            &unpinned_collection());
}

IN_PROC_BROWSER_TEST_F(
    TreeTabsBrowserTest,
    RemoveTabAtIndexRecursive_WithMultipleChildren_MovesAllChildren) {
  SetTreeTabsEnabled(true);

  auto* parent_tab = tab_strip_model().GetTabAtIndex(0);

  // Create multiple children for the parent tab.
  std::vector<tabs::TabInterface*> child_tabs;
  for (int i = 0; i < 3; ++i) {
    auto tab_interface = std::make_unique<tabs::TabModel>(CreateWebContents(),
                                                          &tab_strip_model());
    tab_interface->set_opener(parent_tab);
    child_tabs.push_back(tab_interface.get());

    tab_strip_model().AddTab(std::move(tab_interface), -1,
                             ui::PAGE_TRANSITION_AUTO_BOOKMARK, ADD_NONE);
  }

  // Verify tree structure: parent's TreeTabNode has 4 children
  // (parent tab itself + 3 child tabs)
  ASSERT_EQ(parent_tab->GetParentCollection()->ChildCount(), 4u);

  int initial_count = tab_strip_model().count();

  // Remove the parent tab (index 0).
  tab_strip_model().CloseWebContentsAt(0, TabCloseTypes::CLOSE_NONE);
  EXPECT_EQ(initial_count - 1, tab_strip_model().count());

  // All children should be preserved and moved to unpinned collection level.
  for (size_t i = 0; i < child_tabs.size(); ++i) {
    auto* moved_child = tab_strip_model().GetTabAtIndex(i);
    EXPECT_EQ(child_tabs[i], moved_child);
    EXPECT_EQ(moved_child->GetParentCollection()->type(),
              tabs::TabCollection::Type::TREE_NODE);
    EXPECT_EQ(moved_child->GetParentCollection()->GetParentCollection(),
              &unpinned_collection());
  }
}

IN_PROC_BROWSER_TEST_F(
    TreeTabsBrowserTest,
    RemoveTabAtIndexRecursive_NestedStructure_PreservesTree) {
  SetTreeTabsEnabled(true);

  // The initial tab is the level 0 tab.
  auto* level0_tab = tab_strip_model().GetTabAtIndex(0);

  // Create level 1 child.
  auto tab_interface =
      std::make_unique<tabs::TabModel>(CreateWebContents(), &tab_strip_model());
  tab_interface->set_opener(level0_tab);
  tab_strip_model().AddTab(std::move(tab_interface), -1,
                           ui::PAGE_TRANSITION_AUTO_BOOKMARK, ADD_NONE);

  // Find level1_tab - it should be a child of level0_tab's TreeTabNode
  auto* level1_tab =
      tab_strip_model().GetTabAtIndex(tab_strip_model().count() - 1);
  ASSERT_EQ(level1_tab->GetParentCollection()->GetParentCollection(),
            level0_tab->GetParentCollection());

  // Create level 2 child (child of level 1).
  tab_interface =
      std::make_unique<tabs::TabModel>(CreateWebContents(), &tab_strip_model());
  tab_interface->set_opener(level1_tab);
  tab_strip_model().AddTab(std::move(tab_interface), -1,
                           ui::PAGE_TRANSITION_AUTO_BOOKMARK, ADD_NONE);
  auto* level2_tab =
      tab_strip_model().GetTabAtIndex(tab_strip_model().count() - 1);

  // Verify nested structure: level0 -> level1 -> level2
  // level2's parent collection should be a TreeTabNode, and its parent should
  // be level1's TreeTabNode
  ASSERT_EQ(level2_tab->GetParentCollection()->type(),
            tabs::TabCollection::Type::TREE_NODE);
  ASSERT_EQ(level2_tab->GetParentCollection()->GetParentCollection(),
            level1_tab->GetParentCollection());
  ASSERT_EQ(level1_tab->GetParentCollection()->GetParentCollection(),
            level0_tab->GetParentCollection());

  int level1_index = tab_strip_model().GetIndexOfTab(level1_tab);
  ASSERT_GE(level1_index, 0);

  // Remove level 1 tab (middle node).
  tab_strip_model().CloseWebContentsAt(level1_index, TabCloseTypes::CLOSE_NONE);

  // Verify structure after removal:
  // - level2 should be moved to level0's children
  // - level0 should still exist
  EXPECT_EQ(2, tab_strip_model().count());
  EXPECT_EQ(level0_tab, tab_strip_model().GetTabAtIndex(0));
  EXPECT_EQ(level2_tab, tab_strip_model().GetTabAtIndex(1));

  // level2 should now be a direct child of level0's tree node.
  auto* moved_level2 = tab_strip_model().GetTabAtIndex(1);
  EXPECT_EQ(moved_level2->GetParentCollection()->GetParentCollection(),
            level0_tab->GetParentCollection());

  // level0 should have 2 children now (itself + level2).
  EXPECT_EQ(level0_tab->GetParentCollection()->ChildCount(), 2u);
}

IN_PROC_BROWSER_TEST_F(TreeTabsBrowserTest,
                       RemoveTabAtIndexRecursive_NoChildren_RemovesNormally) {
  SetTreeTabsEnabled(true);

  // Add tabs without opener
  for (int i = 0; i < 2; ++i) {
    auto tab_interface = std::make_unique<tabs::TabModel>(CreateWebContents(),
                                                          &tab_strip_model());
    tab_strip_model().AddTab(std::move(tab_interface), -1,
                             ui::PAGE_TRANSITION_AUTO_BOOKMARK, ADD_NONE);
  }

  // Verify initial structure.
  ASSERT_EQ(3, tab_strip_model().count());
  for (int i = 0; i < tab_strip_model().count(); ++i) {
    ASSERT_EQ(tab_strip_model().GetTabAtIndex(i)->GetParentCollection()->type(),
              tabs::TabCollection::Type::TREE_NODE);
  }

  // Store remaining tab pointers.
  auto* tab0 = tab_strip_model().GetTabAtIndex(0);
  auto* tab2 = tab_strip_model().GetTabAtIndex(2);

  // Remove tab at index 1 (no children).
  tab_strip_model().CloseWebContentsAt(1, TabCloseTypes::CLOSE_NONE);

  // Verify tab was removed and remaining tabs are preserved.
  EXPECT_EQ(2, tab_strip_model().count());
  EXPECT_EQ(tab0, tab_strip_model().GetTabAtIndex(0));
  EXPECT_EQ(tab2, tab_strip_model().GetTabAtIndex(1));

  // Also unpinned collection should have 2 tree nodes for 0 and 2
  EXPECT_EQ(unpinned_collection().ChildCount(), 2u);
  EXPECT_EQ(unpinned_collection().GetTabAtIndexRecursive(0), tab0);
  EXPECT_EQ(unpinned_collection().GetTabAtIndexRecursive(1), tab2);

  // Remaining tabs should still be in tree structure.
  for (int i = 0; i < tab_strip_model().count(); ++i) {
    EXPECT_EQ(tab_strip_model().GetTabAtIndex(i)->GetParentCollection()->type(),
              tabs::TabCollection::Type::TREE_NODE);
  }
}

IN_PROC_BROWSER_TEST_F(TreeTabsBrowserTest,
                       MoveTabsRecursive_FlatTabs_ReordersAtUnpinnedLevel) {
  SetTreeTabsEnabled(true);

  // Add tabs without opener so each is a top-level tree node.
  for (int i = 0; i < 3; ++i) {
    auto tab_interface = std::make_unique<tabs::TabModel>(CreateWebContents(),
                                                          &tab_strip_model());
    tab_strip_model().AddTab(std::move(tab_interface), -1,
                             ui::PAGE_TRANSITION_AUTO_BOOKMARK, ADD_NONE);
  }

  // 4 tabs total: initial + 3 added. Order: tab0, tab1, tab2, tab3.
  ASSERT_EQ(4, tab_strip_model().count());
  auto* tab0 = tab_strip_model().GetTabAtIndex(0);
  auto* tab1 = tab_strip_model().GetTabAtIndex(1);
  auto* tab2 = tab_strip_model().GetTabAtIndex(2);
  auto* tab3 = tab_strip_model().GetTabAtIndex(3);

  for (int i = 0; i < tab_strip_model().count(); ++i) {
    ASSERT_EQ(tab_strip_model().GetTabAtIndex(i)->GetParentCollection()->type(),
              tabs::TabCollection::Type::TREE_NODE);
    ASSERT_EQ(tab_strip_model()
                  .GetTabAtIndex(i)
                  ->GetParentCollection()
                  ->GetParentCollection(),
              &unpinned_collection());
  }

  // Move tab at index 0 to index 2. Triggers MoveTabsRecursive.
  // Expected order after move: tab1, tab2, tab0, tab3.
  tab_strip_model().MoveWebContentsAt(0, 2, false);

  EXPECT_EQ(4, tab_strip_model().count());
  EXPECT_EQ(tab_strip_model().GetTabAtIndex(0), tab1);
  EXPECT_EQ(tab_strip_model().GetTabAtIndex(1), tab2);
  EXPECT_EQ(tab_strip_model().GetTabAtIndex(2), tab0);
  EXPECT_EQ(tab_strip_model().GetTabAtIndex(3), tab3);

  // All should still be tree nodes under unpinned.
  for (int i = 0; i < tab_strip_model().count(); ++i) {
    EXPECT_EQ(tab_strip_model().GetTabAtIndex(i)->GetParentCollection()->type(),
              tabs::TabCollection::Type::TREE_NODE);
    EXPECT_EQ(tab_strip_model()
                  .GetTabAtIndex(i)
                  ->GetParentCollection()
                  ->GetParentCollection(),
              &unpinned_collection());
  }
}

IN_PROC_BROWSER_TEST_F(TreeTabsBrowserTest,
                       MoveTabsRecursive_MoveTabWithChildren_PromotesChildren) {
  SetTreeTabsEnabled(true);

  auto* parent_tab = tab_strip_model().GetTabAtIndex(0);

  // Add two children under the first tab.
  for (int i = 0; i < 2; ++i) {
    auto tab_interface = std::make_unique<tabs::TabModel>(CreateWebContents(),
                                                          &tab_strip_model());
    tab_interface->set_opener(parent_tab);
    tab_strip_model().AddTab(std::move(tab_interface), -1,
                             ui::PAGE_TRANSITION_AUTO_BOOKMARK, ADD_NONE);
  }

  // Order: parent_tab, child0, child1. Parent's TreeTabNode has 3 children.
  ASSERT_EQ(3, tab_strip_model().count());
  auto* child0 = tab_strip_model().GetTabAtIndex(1);
  auto* child1 = tab_strip_model().GetTabAtIndex(2);
  ASSERT_EQ(parent_tab->GetParentCollection()->ChildCount(), 3u);

  // Move the parent tab (index 0) to the end (index 2).
  // MoveTabsRecursive should move the tree node; children are first moved to
  // the parent's parent (unpinned), then the node is moved.
  // Expected: child0, child1 become top-level; then parent moves to end.
  // So final order: child0, child1, parent_tab.
  tab_strip_model().MoveWebContentsAt(0, 2, false);

  EXPECT_EQ(3, tab_strip_model().count());
  EXPECT_EQ(tab_strip_model().GetTabAtIndex(0), child0);
  EXPECT_EQ(tab_strip_model().GetTabAtIndex(1), child1);
  EXPECT_EQ(tab_strip_model().GetTabAtIndex(2), parent_tab);

  // All three should be direct children of unpinned (each in own tree node).
  EXPECT_EQ(child0->GetParentCollection()->GetParentCollection(),
            &unpinned_collection());
  EXPECT_EQ(child1->GetParentCollection()->GetParentCollection(),
            &unpinned_collection());
  EXPECT_EQ(parent_tab->GetParentCollection()->GetParentCollection(),
            &unpinned_collection());
}

IN_PROC_BROWSER_TEST_F(TreeTabsBrowserTest,
                       MoveTabsRecursive_MoveToEnd_InsertAfterLast) {
  SetTreeTabsEnabled(true);

  for (int i = 0; i < 2; ++i) {
    auto tab_interface = std::make_unique<tabs::TabModel>(CreateWebContents(),
                                                          &tab_strip_model());
    tab_strip_model().AddTab(std::move(tab_interface), -1,
                             ui::PAGE_TRANSITION_AUTO_BOOKMARK, ADD_NONE);
  }

  ASSERT_EQ(3, tab_strip_model().count());
  auto* tab0 = tab_strip_model().GetTabAtIndex(0);
  auto* tab1 = tab_strip_model().GetTabAtIndex(1);
  auto* tab2 = tab_strip_model().GetTabAtIndex(2);

  // Move first tab to the end (destination_index >= count means insert after
  // last).
  tab_strip_model().MoveWebContentsAt(0, 3, false);

  EXPECT_EQ(3, tab_strip_model().count());
  EXPECT_EQ(tab_strip_model().GetTabAtIndex(0), tab1);
  EXPECT_EQ(tab_strip_model().GetTabAtIndex(1), tab2);
  EXPECT_EQ(tab_strip_model().GetTabAtIndex(2), tab0);
}
