// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <variant>

#include "base/run_loop.h"
#include "base/test/run_until.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "brave/browser/ui/tabs/brave_tab_strip_model.h"
#include "brave/browser/ui/tabs/tree_tab_model.h"
#include "brave/components/tabs/public/tree_tab_node_id.h"
#include "brave/components/tabs/public/tree_tab_node_tab_collection.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/tab_group_sync/tab_group_sync_service_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/tabs/features.h"
#include "chrome/browser/ui/tabs/split_tab_metrics.h"
#include "chrome/browser/ui/tabs/tab_enums.h"
#include "chrome/browser/ui/tabs/tab_group_model.h"
#include "chrome/browser/ui/tabs/tab_model.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/prefs/pref_service.h"
#include "components/saved_tab_groups/public/tab_group_sync_service.h"
#include "components/split_tabs/split_tab_visual_data.h"
#include "components/tabs/public/pinned_tab_collection.h"
#include "components/tabs/public/split_tab_collection.h"
#include "components/tabs/public/tab_collection.h"
#include "components/tabs/public/tab_group_tab_collection.h"
#include "components/tabs/public/tab_strip_collection.h"
#include "components/tabs/public/unpinned_tab_collection.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

tree_tab::TreeTabNodeId GetTreeTabNodeIdForTab(tabs::TabInterface* tab) {
  const tabs::TabCollection* parent = tab->GetParentCollection();
  CHECK(parent);
  CHECK_EQ(parent->type(), tabs::TabCollection::Type::TREE_NODE);

  return static_cast<const tabs::TreeTabNodeTabCollection*>(parent)
      ->node()
      .id();
}

// Create a split containing the tabs at |index_a| and |index_b| by
// activating |index_a| and calling AddToNewSplit with |index_b|.
void CreateSplitWithTabs(TabStripModel* model, int index_a, int index_b) {
  ASSERT_NE(index_a, index_b);
  model->ActivateTabAt(index_a);
  // Note that we're passing only one index to AddToNewSplit, because the
  // implementation only takes one index, and then create split with active
  // index.
  model->AddToNewSplit({index_b}, split_tabs::SplitTabVisualData(),
                       split_tabs::SplitTabCreatedSource::kTabContextMenu);
}

void VerifySplitCreated(TabStripModel* model,
                        tabs::TabStripCollection* collection) {
  std::set<split_tabs::SplitTabId> splits = collection->ListSplits();
  ASSERT_GE(splits.size(), 1u);
  for (const auto& split_id : splits) {
    EXPECT_TRUE(model->ContainsSplit(split_id));
    tabs::SplitTabCollection* split_coll =
        collection->GetSplitTabCollection(split_id);
    ASSERT_TRUE(split_coll);
    EXPECT_EQ(2u, split_coll->TabCountRecursive());

    // Verifies two tabs in split collection have the same grand parent
    // collection.
    tabs::TabCollection* grand_parent_collection =
        split_coll->GetTabAtIndexRecursive(0)
            ->GetParentCollection()
            ->GetParentCollection();
    EXPECT_EQ(grand_parent_collection, split_coll->GetTabAtIndexRecursive(1)
                                           ->GetParentCollection()
                                           ->GetParentCollection());
    EXPECT_TRUE(grand_parent_collection->type() ==
                    tabs::TabCollection::Type::TREE_NODE ||
                grand_parent_collection->type() ==
                    tabs::TabCollection::Type::PINNED);
  }
}

// Verify that no splits remain and no tab is in a split. Optionally check
// |expected_tab_count| and |expected_unpinned_children|.
void VerifyUnsplit(TabStripModel* model,
                   tabs::TabStripCollection* collection,
                   int expected_tab_count,
                   size_t expected_unpinned_children) {
  EXPECT_TRUE(collection->ListSplits().empty());
  for (int i = 0; i < model->count(); ++i) {
    EXPECT_FALSE(model->GetTabAtIndex(i)->IsSplit())
        << "tab at index " << i << " should not be in a split after Unsplit";
  }
  EXPECT_EQ(model->count(), expected_tab_count);
  EXPECT_EQ(collection->unpinned_collection()->ChildCount(),
            expected_unpinned_children);
}

}  // namespace

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
  tabs::PinnedTabCollection& pinned_collection() {
    return *tab_strip_collection().pinned_collection();
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

  // Tab group APIs used in tests require the sync service to report
  // initialized.
  void EnsureTabGroupSyncServiceInitialized() {
    auto* tab_groups_service =
        tab_groups::TabGroupSyncServiceFactory::GetForProfile(profile());
    ASSERT_TRUE(tab_groups_service);
    tab_groups_service->SetIsInitializedForTesting(true);
  }

  void ExpectGroupModelTabListCount(tab_groups::TabGroupId group_id,
                                    size_t expected) {
    auto* group = tab_strip_model().group_model()->GetTabGroup(group_id);
    ASSERT_TRUE(group);
    EXPECT_EQ(expected, group->ListTabs().length());
  }

  // |split_id| is a split whose SplitTabCollection is a direct child of a
  // TabGroupTabCollection.
  void ExpectSplitCollectionChildOfGroup(split_tabs::SplitTabId split_id) {
    tabs::SplitTabCollection* split_coll =
        tab_strip_collection().GetSplitTabCollection(split_id);
    ASSERT_TRUE(split_coll);
    EXPECT_EQ(2u, split_coll->TabCountRecursive());
    EXPECT_EQ(split_coll->type(), tabs::TabCollection::Type::SPLIT);
    EXPECT_EQ(split_coll->GetParentCollection()->type(),
              tabs::TabCollection::Type::GROUP);
  }

  // |split_id| is wrapped in a TreeTabNode whose parent is the unpinned strip.
  void ExpectSplitWrappedInUnpinnedTreeNode(split_tabs::SplitTabId split_id) {
    tabs::SplitTabCollection* split_coll =
        tab_strip_collection().GetSplitTabCollection(split_id);
    ASSERT_TRUE(split_coll);
    EXPECT_EQ(2u, split_coll->TabCountRecursive());
    EXPECT_EQ(split_coll->type(), tabs::TabCollection::Type::SPLIT);
    const tabs::TabCollection* split_parent = split_coll->GetParentCollection();
    ASSERT_EQ(split_parent->type(), tabs::TabCollection::Type::TREE_NODE);
    EXPECT_EQ(split_parent->GetParentCollection(), &unpinned_collection());
    EXPECT_EQ(static_cast<const tabs::TreeTabNodeTabCollection*>(split_parent)
                  ->current_value_type(),
              tabs::TreeTabNodeTabCollection::CurrentValueType::kSplit);
  }

  void ExpectSplitParentGroupIs(split_tabs::SplitTabId split_id,
                                tab_groups::TabGroupId group_id) {
    tabs::SplitTabCollection* split_coll =
        tab_strip_collection().GetSplitTabCollection(split_id);
    ASSERT_TRUE(split_coll);
    const tabs::TabCollection* group_coll = split_coll->GetParentCollection();
    ASSERT_EQ(group_coll->type(), tabs::TabCollection::Type::GROUP);
    EXPECT_EQ(static_cast<const tabs::TabGroupTabCollection*>(group_coll)
                  ->GetTabGroupId(),
              group_id);
  }

  void SetSplitPinned(split_tabs::SplitTabId split, bool pinned) {
    tab_strip_model().SetSplitPinnedImplForTesting(split, pinned);
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

IN_PROC_BROWSER_TEST_F(TreeTabsBrowserTest,
                       BuildTreeTabsAndFlattenTreeTabs_WithSplitTabs) {
  // Start with flat structure: add two tabs (3 total with initial).
  AddTab();
  AddTab();
  ASSERT_EQ(3, tab_strip_model().count());

  // Create a split containing the first two tabs.
  CreateSplitWithTabs(&tab_strip_model(), 0, 1);
  ASSERT_EQ(1u, tab_strip_collection().ListSplits().size());
  ASSERT_EQ(3, tab_strip_model().count());

  // Store original tab order before BuildTreeTabs.
  std::vector<tabs::TabInterface*> original_tab_order;
  for (int i = 0; i < tab_strip_model().count(); ++i) {
    original_tab_order.push_back(tab_strip_model().GetTabAtIndex(i));
  }

  // Enable tree tabs: BuildTreeTabs() runs and must wrap the split in one tree
  // node (no crash). The split's tabs stay inside the same SplitTabCollection;
  // that collection is wrapped in a TreeTabNodeTabCollection.
  SetTreeTabsEnabled(true);

  VerifySplitCreated(&tab_strip_model(), &tab_strip_collection());

  // Unpinned should have two top-level children: one tree node (split), one
  // tree node (standalone tab).
  EXPECT_EQ(2u, unpinned_collection().ChildCount());
  EXPECT_EQ(tab_strip_model().GetTabAtIndex(2)->GetParentCollection()->type(),
            tabs::TabCollection::Type::TREE_NODE);
  EXPECT_EQ(tab_strip_model()
                .GetTabAtIndex(2)
                ->GetParentCollection()
                ->GetParentCollection(),
            &unpinned_collection());

  // Disable tree tabs: FlattenTreeTabs() runs and must restore the split and
  // preserve tab order.
  SetTreeTabsEnabled(false);

  EXPECT_EQ(3, tab_strip_model().count());
  EXPECT_EQ(1u, tab_strip_collection().ListSplits().size());
  for (int i = 0; i < tab_strip_model().count(); ++i) {
    EXPECT_EQ(original_tab_order[i], tab_strip_model().GetTabAtIndex(i))
        << "Tab order changed at index " << i;
  }

  // Split still contains two tabs.
  split_tabs::SplitTabId split_id =
      *tab_strip_collection().ListSplits().begin();
  tabs::SplitTabCollection* split_coll =
      tab_strip_collection().GetSplitTabCollection(split_id);
  ASSERT_TRUE(split_coll);
  EXPECT_EQ(2u, split_coll->TabCountRecursive());
}

// Verifies BuildTreeTabs() with a group in the strip: group is wrapped in a
// tree node, grouped tabs stay in the group, ungrouped tabs get tree nodes.
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
  const int tab_count_before = tab_strip_model().count();

  // Create a tab group with some tabs (indices 1-2).
  tab_groups::TabGroupId group_id = tab_strip_model().AddToNewGroup({1, 2});

  // Verify group is created.
  ASSERT_TRUE(tab_strip_model().group_model()->ContainsTabGroup(group_id));

  SetTreeTabsEnabled(true);  // Triggers BuildTreeTabs().

  EXPECT_EQ(tab_count_before, tab_strip_model().count());
  EXPECT_EQ(tab_strip_model().GetTabGroupForTab(1), group_id);
  EXPECT_EQ(tab_strip_model().GetTabGroupForTab(2), group_id);
  EXPECT_FALSE(tab_strip_model().GetTabGroupForTab(0).has_value());
  EXPECT_FALSE(tab_strip_model().GetTabGroupForTab(3).has_value());

  // Verify tabs outside group are wrapped in TreeTabNodes within unpinned
  // collection.
  EXPECT_EQ(tab_strip_model().GetTabAtIndex(0)->GetParentCollection()->type(),
            tabs::TabCollection::Type::TREE_NODE);
  EXPECT_EQ(tab_strip_model().GetTabAtIndex(3)->GetParentCollection()->type(),
            tabs::TabCollection::Type::TREE_NODE);
  EXPECT_EQ(tab_strip_model().GetTabAtIndex(4)->GetParentCollection()->type(),
            tabs::TabCollection::Type::TREE_NODE);

  // BuildTreeTabs wraps the group itself in a tree node; grouped tabs stay
  // direct children of the group.
  EXPECT_EQ(tab_strip_model().GetTabAtIndex(1)->GetParentCollection()->type(),
            tabs::TabCollection::Type::GROUP);
  EXPECT_EQ(tab_strip_model().GetTabAtIndex(2)->GetParentCollection()->type(),
            tabs::TabCollection::Type::GROUP);
  const tabs::TabCollection* group_collection =
      tab_strip_model().GetTabAtIndex(1)->GetParentCollection();
  const tabs::TabCollection* group_parent =
      group_collection->GetParentCollection();
  ASSERT_TRUE(group_parent);
  EXPECT_EQ(group_parent->type(), tabs::TabCollection::Type::TREE_NODE);
}

// Verifies FlattenTreeTabs() with a group: tab order and group membership are
// preserved; grouped tabs are direct children of the group, ungrouped of
// unpinned.
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

  SetTreeTabsEnabled(false);  // Triggers FlattenTreeTabs().

  // Verify tabs are preserved in correct order.
  for (size_t i = 0; i < original_tabs.size(); ++i) {
    EXPECT_EQ(original_tabs[i],
              tab_strip_model().GetTabAtIndex(static_cast<int>(i)))
        << "Tab order changed at index " << i;
  }

  // Verify group is still present.
  EXPECT_TRUE(tab_strip_model().group_model()->ContainsTabGroup(group_id));
  EXPECT_EQ(tab_strip_model().GetTabGroupForTab(1), group_id);
  EXPECT_EQ(tab_strip_model().GetTabGroupForTab(2), group_id);

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

// Round-trip: flat strip with group -> BuildTreeTabs() -> FlattenTreeTabs() ->
// order and group preserved (mirrors
// BuildTreeTabsAndFlattenTreeTabs_WithSplitTabs).
IN_PROC_BROWSER_TEST_F(TreeTabsBrowserTest,
                       BuildTreeTabsAndFlattenTreeTabs_WithGroups) {
  auto* tab_groups_service =
      tab_groups::TabGroupSyncServiceFactory::GetForProfile(
          browser()->profile());
  ASSERT_TRUE(tab_groups_service);
  tab_groups_service->SetIsInitializedForTesting(true);

  for (int i = 0; i < 4; ++i) {
    AddTab();
  }
  tab_groups::TabGroupId group_id = tab_strip_model().AddToNewGroup({1, 2});
  ASSERT_TRUE(tab_strip_model().group_model()->ContainsTabGroup(group_id));

  std::vector<tabs::TabInterface*> original_tab_order;
  for (int i = 0; i < tab_strip_model().count(); ++i) {
    original_tab_order.push_back(tab_strip_model().GetTabAtIndex(i));
  }

  SetTreeTabsEnabled(true);  // BuildTreeTabs().
  EXPECT_EQ(tab_strip_model().GetTabGroupForTab(1), group_id);
  EXPECT_EQ(tab_strip_model().GetTabAtIndex(1)->GetParentCollection()->type(),
            tabs::TabCollection::Type::GROUP);

  SetTreeTabsEnabled(false);  // FlattenTreeTabs().

  EXPECT_EQ(original_tab_order.size(),
            static_cast<size_t>(tab_strip_model().count()));
  for (size_t i = 0; i < original_tab_order.size(); ++i) {
    EXPECT_EQ(original_tab_order[i],
              tab_strip_model().GetTabAtIndex(static_cast<int>(i)))
        << "Tab order changed at index " << i;
  }
  EXPECT_TRUE(tab_strip_model().group_model()->ContainsTabGroup(group_id));
  EXPECT_EQ(tab_strip_model().GetTabGroupForTab(1), group_id);
  EXPECT_EQ(tab_strip_model().GetTabGroupForTab(2), group_id);
  EXPECT_EQ(tab_strip_model().GetTabAtIndex(1)->GetParentCollection()->type(),
            tabs::TabCollection::Type::GROUP);
  EXPECT_EQ(tab_strip_model().GetTabAtIndex(2)->GetParentCollection()->type(),
            tabs::TabCollection::Type::GROUP);
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
        // Verify the node has associated tab(s) (single tab or split; group
        // returns none).
        EXPECT_FALSE(created_change.node->GetTabs().empty());

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
        // Verify the node has associated tab(s) before destruction (single tab
        // or split; group returns none).
        EXPECT_FALSE(destroyed_change.node->GetTabs().empty());
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

IN_PROC_BROWSER_TEST_F(TreeTabsBrowserTest,
                       SetCollapsed_DoesBelongToCollapsedNode_UpdatesCache) {
  SetTreeTabsEnabled(true);
  ASSERT_TRUE(tab_strip_model().tree_model());

  // Initial tab at index 0; add a child tab so we have parent (tree node of
  // tab 0) and child (tree node containing the new tab).
  auto* parent_tab = tab_strip_model().GetTabAtIndex(0);
  auto tab_interface =
      std::make_unique<tabs::TabModel>(CreateWebContents(), &tab_strip_model());
  tab_interface->set_opener(parent_tab);
  tab_strip_model().AddTab(std::move(tab_interface), -1,
                           ui::PAGE_TRANSITION_AUTO_BOOKMARK, ADD_NONE);

  auto* child_tab =
      tab_strip_model().GetTabAtIndex(tab_strip_model().count() - 1);
  tree_tab::TreeTabNodeId parent_node_id = GetTreeTabNodeIdForTab(parent_tab);
  tree_tab::TreeTabNodeId child_node_id = GetTreeTabNodeIdForTab(child_tab);
  ASSERT_FALSE(parent_node_id.is_empty());
  ASSERT_FALSE(child_node_id.is_empty());

  TreeTabModel* model = tab_strip_model().tree_model();

  // Initially neither is under a collapsed node.
  EXPECT_FALSE(model->DoesBelongToCollapsedNode(parent_node_id));
  EXPECT_FALSE(model->DoesBelongToCollapsedNode(child_node_id));

  // Collapse parent: child should belong to collapsed node, parent should not.
  model->SetCollapsed(parent_node_id, true);
  EXPECT_FALSE(model->DoesBelongToCollapsedNode(parent_node_id));
  EXPECT_TRUE(model->DoesBelongToCollapsedNode(child_node_id));

  // Uncollapse: child should no longer belong to a collapsed node.
  model->SetCollapsed(parent_node_id, false);
  EXPECT_FALSE(model->DoesBelongToCollapsedNode(parent_node_id));
  EXPECT_FALSE(model->DoesBelongToCollapsedNode(child_node_id));
}

IN_PROC_BROWSER_TEST_F(TreeTabsBrowserTest,
                       DoesBelongToCollapsedNode_UnknownNodeId_ReturnsFalse) {
  SetTreeTabsEnabled(true);
  ASSERT_TRUE(tab_strip_model().tree_model());

  EXPECT_FALSE(tab_strip_model().tree_model()->DoesBelongToCollapsedNode(
      tree_tab::TreeTabNodeId::CreateEmpty()));
  EXPECT_FALSE(tab_strip_model().tree_model()->DoesBelongToCollapsedNode(
      tree_tab::TreeTabNodeId::GenerateNew()));
}

IN_PROC_BROWSER_TEST_F(TreeTabsBrowserTest,
                       MoveTab_OutOfCollapsedParent_UpdatesCache) {
  SetTreeTabsEnabled(true);
  ASSERT_TRUE(tab_strip_model().tree_model());

  // Parent tab at 0; add child tab (at index 1).
  auto* parent_tab = tab_strip_model().GetTabAtIndex(0);
  auto tab_interface =
      std::make_unique<tabs::TabModel>(CreateWebContents(), &tab_strip_model());
  tab_interface->set_opener(parent_tab);
  tab_strip_model().AddTab(std::move(tab_interface), -1,
                           ui::PAGE_TRANSITION_AUTO_BOOKMARK, ADD_NONE);

  auto* child_tab = tab_strip_model().GetTabAtIndex(1);
  tree_tab::TreeTabNodeId parent_node_id = GetTreeTabNodeIdForTab(parent_tab);
  tree_tab::TreeTabNodeId child_node_id = GetTreeTabNodeIdForTab(child_tab);
  ASSERT_FALSE(child_node_id.is_empty());

  TreeTabModel* model = tab_strip_model().tree_model();
  model->SetCollapsed(parent_node_id, true);
  EXPECT_TRUE(model->DoesBelongToCollapsedNode(child_node_id));

  // Move child tab to index 0 (before parent), reparenting its tree node from
  // under parent to a direct child of unpinned. OnReparented triggers
  // OnTreeTabNodeMoved which updates the cache.
  tab_strip_model().MoveWebContentsAt(1, 0, false);
  EXPECT_FALSE(model->DoesBelongToCollapsedNode(child_node_id));
}

IN_PROC_BROWSER_TEST_F(TreeTabsBrowserTest,
                       CommandNewTabToRight_TreeTabs_DoesNotCrash) {
  SetTreeTabsEnabled(true);

  // Create A(root)-B(leaf): one root tab and one child tab.
  auto* tab_a = tab_strip_model().GetTabAtIndex(0);
  auto tab_b_interface =
      std::make_unique<tabs::TabModel>(CreateWebContents(), &tab_strip_model());
  tab_b_interface->set_opener(tab_a);
  tab_strip_model().AddTab(std::move(tab_b_interface), -1,
                           ui::PAGE_TRANSITION_AUTO_BOOKMARK, ADD_NONE);

  ASSERT_EQ(2, tab_strip_model().count());
  auto* tab_b = tab_strip_model().GetTabAtIndex(1);

  // Activate A (index 0) and run "New tab to the right/below". This inserts at
  // index 1, so the result should be A(root) - new tab - B(leaf). Verifies no
  // crash.
  tab_strip_model().ActivateTabAt(0);
  chrome::NewTabToRight(browser());

  ASSERT_EQ(3, tab_strip_model().count());
  EXPECT_EQ(tab_a, tab_strip_model().GetTabAtIndex(0));
  auto* new_tab = tab_strip_model().GetTabAtIndex(1);
  EXPECT_EQ(tab_b, tab_strip_model().GetTabAtIndex(2));

  // The newly created tab should be a child of tab A's tree node (same
  // TreeTabNode parent as A's node).
  EXPECT_EQ(new_tab->GetParentCollection()->GetParentCollection(),
            tab_a->GetParentCollection());

  // And tab_b is also still a child of tab A's tree node.
  EXPECT_EQ(tab_b->GetParentCollection()->GetParentCollection(),
            tab_a->GetParentCollection());
}

IN_PROC_BROWSER_TEST_F(TreeTabsBrowserTest,
                       MoveTabWithChildOut_CacheUpdatedForLeftBehindChild) {
  SetTreeTabsEnabled(true);
  ASSERT_TRUE(tab_strip_model().tree_model());

  // Build A (parent) -> B (child of A) -> C (child of B). Collapse A.
  auto* tab_a = tab_strip_model().GetTabAtIndex(0);
  auto tab_b_interface =
      std::make_unique<tabs::TabModel>(CreateWebContents(), &tab_strip_model());
  tab_b_interface->set_opener(tab_a);
  tab_strip_model().AddTab(std::move(tab_b_interface), -1,
                           ui::PAGE_TRANSITION_AUTO_BOOKMARK, ADD_NONE);

  auto* tab_b = tab_strip_model().GetTabAtIndex(1);
  auto tab_c_interface =
      std::make_unique<tabs::TabModel>(CreateWebContents(), &tab_strip_model());
  tab_c_interface->set_opener(tab_b);
  tab_strip_model().AddTab(std::move(tab_c_interface), -1,
                           ui::PAGE_TRANSITION_AUTO_BOOKMARK, ADD_NONE);

  ASSERT_EQ(3, tab_strip_model().count());
  auto* tab_c = tab_strip_model().GetTabAtIndex(2);
  tree_tab::TreeTabNodeId node_a = GetTreeTabNodeIdForTab(tab_a);
  tree_tab::TreeTabNodeId node_b = GetTreeTabNodeIdForTab(tab_b);
  tree_tab::TreeTabNodeId node_c = GetTreeTabNodeIdForTab(tab_c);

  TreeTabModel* model = tab_strip_model().tree_model();
  model->SetCollapsed(node_a, true);
  EXPECT_TRUE(model->DoesBelongToCollapsedNode(node_b));
  EXPECT_TRUE(model->DoesBelongToCollapsedNode(node_c));

  // Move B out (to index 0). B's child C is no longer a child of B—it is
  // promoted to A's level. OnTreeTabNodeMoved is invoked for both B and C;
  // cache must reflect B not under collapsed and C still under collapsed A.
  tab_strip_model().MoveWebContentsAt(1, 0, false);
  EXPECT_EQ(3, tab_strip_model().count());

  EXPECT_FALSE(model->DoesBelongToCollapsedNode(node_b));
  EXPECT_TRUE(model->DoesBelongToCollapsedNode(node_c));

  // C is no longer under B: C's tree node's parent should be A's collection.
  tab_c = tab_strip_model().GetTabAtIndex(2);
  const tabs::TabCollection* c_parent = tab_c->GetParentCollection();
  ASSERT_EQ(c_parent->type(), tabs::TabCollection::Type::TREE_NODE);
  EXPECT_EQ(
      static_cast<const tabs::TreeTabNodeTabCollection*>(c_parent)->node().id(),
      node_c);
  const tabs::TabCollection* c_grandparent = c_parent->GetParentCollection();
  ASSERT_EQ(c_grandparent->type(), tabs::TabCollection::Type::TREE_NODE);
  EXPECT_EQ(static_cast<const tabs::TreeTabNodeTabCollection*>(c_grandparent)
                ->node()
                .id(),
            node_a);
}

IN_PROC_BROWSER_TEST_F(TreeTabsBrowserTest,
                       RemoveCollapsedParent_UpdatesCacheForChildren) {
  SetTreeTabsEnabled(true);
  ASSERT_TRUE(tab_strip_model().tree_model());

  // Parent tab at 0; add child tab (at index 1).
  auto* parent_tab = tab_strip_model().GetTabAtIndex(0);
  auto tab_interface =
      std::make_unique<tabs::TabModel>(CreateWebContents(), &tab_strip_model());
  tab_interface->set_opener(parent_tab);
  tab_strip_model().AddTab(std::move(tab_interface), -1,
                           ui::PAGE_TRANSITION_AUTO_BOOKMARK, ADD_NONE);

  auto* child_tab = tab_strip_model().GetTabAtIndex(1);
  tree_tab::TreeTabNodeId parent_node_id = GetTreeTabNodeIdForTab(parent_tab);
  tree_tab::TreeTabNodeId child_node_id = GetTreeTabNodeIdForTab(child_tab);
  ASSERT_FALSE(child_node_id.is_empty());

  TreeTabModel* model = tab_strip_model().tree_model();
  model->SetCollapsed(parent_node_id, true);
  EXPECT_TRUE(model->DoesBelongToCollapsedNode(child_node_id));

  // Close the parent tab. RemoveTabAtIndexRecursive removes the parent's tree
  // node and calls RemoveTreeTabNode(parent_node_id), which recomputes the
  // cache for nodes that had the removed node as closest collapsed ancestor;
  // the child is no longer under any collapsed node.
  tab_strip_model().CloseWebContentsAt(0, TabCloseTypes::CLOSE_NONE);
  EXPECT_EQ(1, tab_strip_model().count());
  EXPECT_FALSE(model->DoesBelongToCollapsedNode(child_node_id));
}

IN_PROC_BROWSER_TEST_F(TreeTabsBrowserTest,
                       MoveCollapsedParent_ChildRecalculatesCache) {
  SetTreeTabsEnabled(true);
  ASSERT_TRUE(tab_strip_model().tree_model());

  // Parent at 0, child at 1.
  auto* parent_tab = tab_strip_model().GetTabAtIndex(0);
  auto tab_interface =
      std::make_unique<tabs::TabModel>(CreateWebContents(), &tab_strip_model());
  tab_interface->set_opener(parent_tab);
  tab_strip_model().AddTab(std::move(tab_interface), -1,
                           ui::PAGE_TRANSITION_AUTO_BOOKMARK, ADD_NONE);

  auto* child_tab = tab_strip_model().GetTabAtIndex(1);
  tree_tab::TreeTabNodeId parent_node_id = GetTreeTabNodeIdForTab(parent_tab);
  tree_tab::TreeTabNodeId child_node_id = GetTreeTabNodeIdForTab(child_tab);
  ASSERT_FALSE(child_node_id.is_empty());

  TreeTabModel* model = tab_strip_model().tree_model();
  model->SetCollapsed(parent_node_id, true);
  EXPECT_TRUE(model->DoesBelongToCollapsedNode(child_node_id));

  // Move the collapsed parent tab to the end (index 1). The parent's tree
  // node is moved; OnTreeTabNodeMoved(parent_node_id) recomputes the parent and
  // its descendants. The child should no longer have the parent as closest
  // collapsed ancestor.
  tab_strip_model().MoveWebContentsAt(0, 1, false);
  EXPECT_EQ(2, tab_strip_model().count());
  EXPECT_FALSE(model->DoesBelongToCollapsedNode(child_node_id));
}

IN_PROC_BROWSER_TEST_F(TreeTabsBrowserTest,
                       MoveTabIntoCollapsedParent_UpdatesCache) {
  SetTreeTabsEnabled(true);
  ASSERT_TRUE(tab_strip_model().tree_model());

  // Parent at 0, child at 1; collapse parent.
  auto* parent_tab = tab_strip_model().GetTabAtIndex(0);
  auto tab_interface =
      std::make_unique<tabs::TabModel>(CreateWebContents(), &tab_strip_model());
  tab_interface->set_opener(parent_tab);
  tab_strip_model().AddTab(std::move(tab_interface), -1,
                           ui::PAGE_TRANSITION_AUTO_BOOKMARK, ADD_NONE);

  tree_tab::TreeTabNodeId parent_node_id = GetTreeTabNodeIdForTab(parent_tab);
  TreeTabModel* model = tab_strip_model().tree_model();
  model->SetCollapsed(parent_node_id, true);

  // Add a top-level tab (no opener) so it is not under the collapsed parent.
  auto standalone_tab =
      std::make_unique<tabs::TabModel>(CreateWebContents(), &tab_strip_model());
  tab_strip_model().AddTab(std::move(standalone_tab), -1,
                           ui::PAGE_TRANSITION_AUTO_BOOKMARK, ADD_NONE);
  ASSERT_EQ(3, tab_strip_model().count());
  auto* tab_to_move = tab_strip_model().GetTabAtIndex(2);
  tree_tab::TreeTabNodeId tab_to_move_node_id =
      GetTreeTabNodeIdForTab(tab_to_move);
  EXPECT_FALSE(model->DoesBelongToCollapsedNode(tab_to_move_node_id));

  // The tab is reparented under the parent's tree node; OnTreeTabNodeMoved
  // updates the cache so it is now under a collapsed node.
  tab_strip_model().MoveWebContentsAt(2, 1, false);
  EXPECT_TRUE(model->DoesBelongToCollapsedNode(tab_to_move_node_id));
}

IN_PROC_BROWSER_TEST_F(TreeTabsBrowserTest,
                       NestedCollapse_LeafKeepsCloserAncestorAsClosest) {
  SetTreeTabsEnabled(true);
  ASSERT_TRUE(tab_strip_model().tree_model());

  // Build A (root) -> B -> C (leaf).
  auto* tab_a = tab_strip_model().GetTabAtIndex(0);
  auto tab_b_interface =
      std::make_unique<tabs::TabModel>(CreateWebContents(), &tab_strip_model());
  tab_b_interface->set_opener(tab_a);
  tab_strip_model().AddTab(std::move(tab_b_interface), -1,
                           ui::PAGE_TRANSITION_AUTO_BOOKMARK, ADD_NONE);

  auto* tab_b = tab_strip_model().GetTabAtIndex(1);
  auto tab_c_interface =
      std::make_unique<tabs::TabModel>(CreateWebContents(), &tab_strip_model());
  tab_c_interface->set_opener(tab_b);
  tab_strip_model().AddTab(std::move(tab_c_interface), -1,
                           ui::PAGE_TRANSITION_AUTO_BOOKMARK, ADD_NONE);

  ASSERT_EQ(3, tab_strip_model().count());
  auto* tab_c = tab_strip_model().GetTabAtIndex(2);
  tree_tab::TreeTabNodeId node_a = GetTreeTabNodeIdForTab(tab_a);
  tree_tab::TreeTabNodeId node_b = GetTreeTabNodeIdForTab(tab_b);
  tree_tab::TreeTabNodeId node_c = GetTreeTabNodeIdForTab(tab_c);

  TreeTabModel* model = tab_strip_model().tree_model();

  // Collapse B first: C's closest collapsed ancestor is B.
  model->SetCollapsed(node_b, true);
  EXPECT_TRUE(model->DoesBelongToCollapsedNode(node_c));

  // Collapse A. C must still have B as closest (not A).
  // CollectUncollapseDescendantIds stops at collapsed B, so C is not assigned
  // A. After this, uncollapse A: only B is collapsed, so C should still be
  // under collapsed (B).
  model->SetCollapsed(node_a, true);
  EXPECT_TRUE(model->DoesBelongToCollapsedNode(node_c));

  model->SetCollapsed(node_a, false);
  EXPECT_FALSE(model->DoesBelongToCollapsedNode(node_a));
  EXPECT_FALSE(model->DoesBelongToCollapsedNode(
      node_b));  // B is collapsed itself, no collapsed ancestor
  EXPECT_TRUE(model->DoesBelongToCollapsedNode(node_c));

  model->SetCollapsed(node_b, false);
  EXPECT_FALSE(model->DoesBelongToCollapsedNode(node_c));
}

IN_PROC_BROWSER_TEST_F(TreeTabsBrowserTest,
                       SelectingNodeInCollapsedTreeTab_UncollapseAllAncestors) {
  SetTreeTabsEnabled(true);
  ASSERT_TRUE(tab_strip_model().tree_model());

  // Build A (root) -> B -> C (leaf).
  auto* tab_a = tab_strip_model().GetTabAtIndex(0);
  auto tab_b_interface =
      std::make_unique<tabs::TabModel>(CreateWebContents(), &tab_strip_model());
  tab_b_interface->set_opener(tab_a);
  tab_strip_model().AddTab(std::move(tab_b_interface), -1,
                           ui::PAGE_TRANSITION_AUTO_BOOKMARK, ADD_NONE);

  auto* tab_b = tab_strip_model().GetTabAtIndex(1);
  auto tab_c_interface =
      std::make_unique<tabs::TabModel>(CreateWebContents(), &tab_strip_model());
  tab_c_interface->set_opener(tab_b);
  tab_strip_model().AddTab(std::move(tab_c_interface), -1,
                           ui::PAGE_TRANSITION_AUTO_BOOKMARK, ADD_NONE);

  ASSERT_EQ(3, tab_strip_model().count());
  auto* tab_c = tab_strip_model().GetTabAtIndex(2);
  tree_tab::TreeTabNodeId node_a = GetTreeTabNodeIdForTab(tab_a);
  tree_tab::TreeTabNodeId node_b = GetTreeTabNodeIdForTab(tab_b);
  tree_tab::TreeTabNodeId node_c = GetTreeTabNodeIdForTab(tab_c);

  // collapse A and B.
  TreeTabModel* model = tab_strip_model().tree_model();
  model->SetCollapsed(node_a, true);
  model->SetCollapsed(node_b, true);
  ASSERT_TRUE(model->GetNode(node_a)->collapsed());
  ASSERT_TRUE(model->GetNode(node_b)->collapsed());
  EXPECT_TRUE(model->DoesBelongToCollapsedNode(node_b));
  EXPECT_TRUE(model->DoesBelongToCollapsedNode(node_c));

  // Select C. This should uncollapse A and B.
  tab_strip_model().ActivateTabAt(2);
  ASSERT_TRUE(base::test::RunUntil(
      [&]() { return !model->DoesBelongToCollapsedNode(node_c); }));
  EXPECT_FALSE(model->GetNode(node_a)->collapsed());
  EXPECT_FALSE(model->GetNode(node_b)->collapsed());
}

IN_PROC_BROWSER_TEST_F(TreeTabsBrowserTest,
                       AddingActiveNewTab_UncollapseAllAncestors) {
  SetTreeTabsEnabled(true);
  ASSERT_TRUE(tab_strip_model().tree_model());

  // Build A (root) -> B -> C -> D (leaf).
  auto* tab_a = tab_strip_model().GetTabAtIndex(0);
  auto tab_b_interface =
      std::make_unique<tabs::TabModel>(CreateWebContents(), &tab_strip_model());
  tab_b_interface->set_opener(tab_a);
  tab_strip_model().AddTab(std::move(tab_b_interface), -1,
                           ui::PAGE_TRANSITION_AUTO_BOOKMARK, ADD_NONE);

  auto* tab_b = tab_strip_model().GetTabAtIndex(1);

  auto tab_c_interface =
      std::make_unique<tabs::TabModel>(CreateWebContents(), &tab_strip_model());
  tab_c_interface->set_opener(tab_b);
  tab_strip_model().AddTab(std::move(tab_c_interface), -1,
                           ui::PAGE_TRANSITION_AUTO_BOOKMARK, ADD_NONE);
  auto* tab_c = tab_strip_model().GetTabAtIndex(2);

  // collapse A and B.
  tree_tab::TreeTabNodeId node_a = GetTreeTabNodeIdForTab(tab_a);
  tree_tab::TreeTabNodeId node_b = GetTreeTabNodeIdForTab(tab_b);
  TreeTabModel* model = tab_strip_model().tree_model();
  model->SetCollapsed(node_a, true);
  model->SetCollapsed(node_b, true);
  ASSERT_TRUE(model->GetNode(node_a)->collapsed());
  ASSERT_TRUE(model->GetNode(node_b)->collapsed());
  ASSERT_TRUE(model->DoesBelongToCollapsedNode(node_b));

  // Add a active new tab. This should uncollapse A and B.
  auto tab_d_interface =
      std::make_unique<tabs::TabModel>(CreateWebContents(), &tab_strip_model());
  tab_d_interface->set_opener(tab_c);
  tab_strip_model().AddTab(std::move(tab_d_interface), -1,
                           ui::PAGE_TRANSITION_AUTO_BOOKMARK, ADD_ACTIVE);
  auto* tab_d = tab_strip_model().GetTabAtIndex(2);
  tree_tab::TreeTabNodeId node_d = GetTreeTabNodeIdForTab(tab_d);

  ASSERT_TRUE(base::test::RunUntil(
      [&]() { return !model->DoesBelongToCollapsedNode(node_d); }));
  EXPECT_FALSE(model->GetNode(node_a)->collapsed());
  EXPECT_FALSE(model->GetNode(node_b)->collapsed());
}

IN_PROC_BROWSER_TEST_F(TreeTabsBrowserTest,
                       AddingInactiveNewTab_DoesNotUncollapseAllAncestors) {
  SetTreeTabsEnabled(true);
  ASSERT_TRUE(tab_strip_model().tree_model());

  // Build A (root) -> B -> C -> D(leaf).
  auto* tab_a = tab_strip_model().GetTabAtIndex(0);
  auto tab_b_interface =
      std::make_unique<tabs::TabModel>(CreateWebContents(), &tab_strip_model());
  tab_b_interface->set_opener(tab_a);
  tab_strip_model().AddTab(std::move(tab_b_interface), -1,
                           ui::PAGE_TRANSITION_AUTO_BOOKMARK, ADD_NONE);
  auto* tab_b = tab_strip_model().GetTabAtIndex(1);

  auto tab_c_interface =
      std::make_unique<tabs::TabModel>(CreateWebContents(), &tab_strip_model());
  tab_c_interface->set_opener(tab_b);
  tab_strip_model().AddTab(std::move(tab_c_interface), -1,
                           ui::PAGE_TRANSITION_AUTO_BOOKMARK, ADD_NONE);
  auto* tab_c = tab_strip_model().GetTabAtIndex(2);

  // collapse A and B.
  tree_tab::TreeTabNodeId node_a = GetTreeTabNodeIdForTab(tab_a);
  tree_tab::TreeTabNodeId node_b = GetTreeTabNodeIdForTab(tab_b);
  TreeTabModel* model = tab_strip_model().tree_model();
  model->SetCollapsed(node_a, true);
  model->SetCollapsed(node_b, true);
  ASSERT_TRUE(model->GetNode(node_a)->collapsed());
  ASSERT_TRUE(model->GetNode(node_b)->collapsed());

  // Add a inactive new tab. This should not uncollapse A and B.
  auto tab_d_interface =
      std::make_unique<tabs::TabModel>(CreateWebContents(), &tab_strip_model());
  tab_d_interface->set_opener(tab_c);
  tab_strip_model().AddTab(std::move(tab_d_interface), -1,
                           ui::PAGE_TRANSITION_AUTO_BOOKMARK, ADD_NONE);

  auto* tab_d = tab_strip_model().GetTabAtIndex(3);
  tree_tab::TreeTabNodeId node_d = GetTreeTabNodeIdForTab(tab_d);
  EXPECT_TRUE(model->DoesBelongToCollapsedNode(node_d));
  EXPECT_TRUE(model->GetNode(node_a)->collapsed());
  EXPECT_TRUE(model->GetNode(node_b)->collapsed());
}

IN_PROC_BROWSER_TEST_F(TreeTabsBrowserTest,
                       OnTreeTabChanged_CalledWhenTreeNodeWasCollapsed) {
  SetTreeTabsEnabled(true);
  ASSERT_TRUE(tab_strip_model().tree_model());

  // Build A (root) -> b(leaf).
  auto* tab_a = tab_strip_model().GetTabAtIndex(0);
  auto tab_b_interface =
      std::make_unique<tabs::TabModel>(CreateWebContents(), &tab_strip_model());
  tab_b_interface->set_opener(tab_a);
  tab_strip_model().AddTab(std::move(tab_b_interface), -1,
                           ui::PAGE_TRANSITION_AUTO_BOOKMARK, ADD_NONE);

  // Expect OnTreeTabChanged to be called kNodeCollapsedStateChanged type.
  // Also verify the CollapsedStateChangedChange contains valid node reference.
  MockTabStripModelObserver mock_observer;
  tab_strip_model().AddObserver(&mock_observer);
  EXPECT_CALL(mock_observer, OnTreeTabChanged(testing::Field(
                                 &TreeTabChange::type,
                                 TreeTabChange::kNodeCollapsedStateChanged)))
      .Times(1)
      .WillOnce([&](const TreeTabChange& change) {
        EXPECT_EQ(change.GetCollapsedStateChangedChange().node->id(),
                  GetTreeTabNodeIdForTab(tab_a));
        EXPECT_TRUE(change.GetCollapsedStateChangedChange().node->collapsed());
      });

  // Collapse tab_a.
  auto* model = static_cast<BraveTabStripModel*>(&tab_strip_model());
  model->SetTreeTabNodeCollapsed(GetTreeTabNodeIdForTab(tab_a), true);
  testing::Mock::VerifyAndClearExpectations(&mock_observer);

  // Also uncollapse tab_a should invoke OnTreeTabChanged with
  // kNodeCollapsedStateChanged type.
  EXPECT_CALL(mock_observer, OnTreeTabChanged(testing::Field(
                                 &TreeTabChange::type,
                                 TreeTabChange::kNodeCollapsedStateChanged)))
      .Times(1)
      .WillOnce([&](const TreeTabChange& change) {
        EXPECT_EQ(change.GetCollapsedStateChangedChange().node->id(),
                  GetTreeTabNodeIdForTab(tab_a));
        EXPECT_FALSE(change.GetCollapsedStateChangedChange().node->collapsed());
      });

  // Uncollapse tab_a.
  model->SetTreeTabNodeCollapsed(GetTreeTabNodeIdForTab(tab_a), false);
  testing::Mock::VerifyAndClearExpectations(&mock_observer);
}

IN_PROC_BROWSER_TEST_F(TreeTabsBrowserTest, CreateSplit_FromRootNode) {
  SetTreeTabsEnabled(true);

  // Two tabs without opener: each is a root (top-level) tree node.
  chrome::NewSplitTab(browser(),
                      split_tabs::SplitTabCreatedSource::kTabContextMenu);

  ASSERT_EQ(2, tab_strip_model().count());

  // Unpinned collection should have one child, the tree node wrapper containing
  // the split.
  EXPECT_EQ(unpinned_collection().ChildCount(), 1u);

  VerifySplitCreated(&tab_strip_model(), &tab_strip_collection());
}

IN_PROC_BROWSER_TEST_F(TreeTabsBrowserTest,
                       CreateSplit_SiblingCase_TwoAdjacentRootNodes) {
  SetTreeTabsEnabled(true);

  // Two tabs without opener: each is a root (top-level) tree node.
  auto tab1 =
      std::make_unique<tabs::TabModel>(CreateWebContents(), &tab_strip_model());
  tab_strip_model().AddTab(std::move(tab1), -1,
                           ui::PAGE_TRANSITION_AUTO_BOOKMARK, ADD_NONE);

  ASSERT_EQ(2, tab_strip_model().count());
  EXPECT_EQ(unpinned_collection().ChildCount(), 2u);

  CreateSplitWithTabs(&tab_strip_model(), 0, 1);

  VerifySplitCreated(&tab_strip_model(), &tab_strip_collection());
  EXPECT_EQ(1u, unpinned_collection().ChildCount());
}

IN_PROC_BROWSER_TEST_F(TreeTabsBrowserTest,
                       CreateSplit_FromLeafNode_TwoLeavesInDifferentBranches) {
  SetTreeTabsEnabled(true);

  // Build: A (root) -> B (leaf). C (root) -> D (leaf). B and D are leaves.
  auto* tab_a = tab_strip_model().GetTabAtIndex(0);
  auto tab_b =
      std::make_unique<tabs::TabModel>(CreateWebContents(), &tab_strip_model());
  tab_b->set_opener(tab_a);
  tab_strip_model().AddTab(std::move(tab_b), -1,
                           ui::PAGE_TRANSITION_AUTO_BOOKMARK, ADD_NONE);

  auto tab_c =
      std::make_unique<tabs::TabModel>(CreateWebContents(), &tab_strip_model());
  tab_strip_model().AddTab(std::move(tab_c), -1,
                           ui::PAGE_TRANSITION_AUTO_BOOKMARK, ADD_NONE);
  auto* tab_c_ptr = tab_strip_model().GetTabAtIndex(2);
  auto tab_d =
      std::make_unique<tabs::TabModel>(CreateWebContents(), &tab_strip_model());
  tab_d->set_opener(tab_c_ptr);
  tab_strip_model().AddTab(std::move(tab_d), -1,
                           ui::PAGE_TRANSITION_AUTO_BOOKMARK, ADD_NONE);

  ASSERT_EQ(4, tab_strip_model().count());

  CreateSplitWithTabs(&tab_strip_model(), 1, 3);
  VerifySplitCreated(&tab_strip_model(), &tab_strip_collection());

  EXPECT_EQ(4, tab_strip_model().count());
  EXPECT_EQ(2u, unpinned_collection().ChildCount());
}

IN_PROC_BROWSER_TEST_F(TreeTabsBrowserTest,
                       CreateSplit_FromMiddleNode_SplitIncludesMiddleTab) {
  SetTreeTabsEnabled(true);

  // Build: A (root) -> B (middle) -> C (leaf). B is a middle node.
  auto* tab_a = tab_strip_model().GetTabAtIndex(0);
  auto tab_b =
      std::make_unique<tabs::TabModel>(CreateWebContents(), &tab_strip_model());
  tab_b->set_opener(tab_a);
  tab_strip_model().AddTab(std::move(tab_b), -1,
                           ui::PAGE_TRANSITION_AUTO_BOOKMARK, ADD_NONE);
  auto* tab_b_ptr = tab_strip_model().GetTabAtIndex(1);
  auto tab_c =
      std::make_unique<tabs::TabModel>(CreateWebContents(), &tab_strip_model());
  tab_c->set_opener(tab_b_ptr);
  tab_strip_model().AddTab(std::move(tab_c), -1,
                           ui::PAGE_TRANSITION_AUTO_BOOKMARK, ADD_NONE);

  ASSERT_EQ(3, tab_strip_model().count());
  int index_b = tab_strip_model().GetIndexOfTab(tab_b_ptr);
  int index_c =
      tab_strip_model().GetIndexOfTab(tab_strip_model().GetTabAtIndex(2));
  ASSERT_EQ(1, index_b);
  ASSERT_EQ(2, index_c);

  CreateSplitWithTabs(&tab_strip_model(), index_b, index_c);

  VerifySplitCreated(&tab_strip_model(), &tab_strip_collection());
  EXPECT_EQ(3, tab_strip_model().count());
  EXPECT_EQ(1u, unpinned_collection().ChildCount());
}

IN_PROC_BROWSER_TEST_F(TreeTabsBrowserTest,
                       CreateSplit_AncestorDescendantCase_ParentAndChildTabs) {
  SetTreeTabsEnabled(true);

  // Build: A (root) -> B (child). A is ancestor, B is descendant.
  auto* tab_a = tab_strip_model().GetTabAtIndex(0);
  auto tab_b =
      std::make_unique<tabs::TabModel>(CreateWebContents(), &tab_strip_model());
  tab_b->set_opener(tab_a);
  tab_strip_model().AddTab(std::move(tab_b), -1,
                           ui::PAGE_TRANSITION_AUTO_BOOKMARK, ADD_NONE);

  ASSERT_EQ(2, tab_strip_model().count());

  CreateSplitWithTabs(&tab_strip_model(), 0, 1);
  VerifySplitCreated(&tab_strip_model(), &tab_strip_collection());

  EXPECT_EQ(1u, unpinned_collection().ChildCount());
}

IN_PROC_BROWSER_TEST_F(TreeTabsBrowserTest,
                       Unsplit_SiblingCase_TwoAdjacentRootNodes) {
  SetTreeTabsEnabled(true);

  auto tab1 =
      std::make_unique<tabs::TabModel>(CreateWebContents(), &tab_strip_model());
  tabs::TabInterface* tab1_interface = tab1.get();
  tab_strip_model().AddTab(std::move(tab1), -1,
                           ui::PAGE_TRANSITION_AUTO_BOOKMARK, ADD_NONE);
  ASSERT_EQ(2, tab_strip_model().count());
  ASSERT_EQ(unpinned_collection().ChildCount(), 2u);

  CreateSplitWithTabs(&tab_strip_model(), 0, 1);
  VerifySplitCreated(&tab_strip_model(), &tab_strip_collection());
  ASSERT_EQ(2, tab_strip_model().count());
  ASSERT_EQ(1u, unpinned_collection().ChildCount());

  split_tabs::SplitTabId split_id =
      *tab_strip_collection().ListSplits().begin();
  tab_strip_model().RemoveSplit(split_id);

  VerifyUnsplit(&tab_strip_model(), &tab_strip_collection(), 2,
                /*expected_unpinned_children=*/2u);

  EXPECT_EQ(tab1_interface->GetParentCollection()->type(),
            tabs::TabCollection::Type::TREE_NODE);
  EXPECT_EQ(
      tab1_interface->GetParentCollection()->GetParentCollection()->type(),
      tabs::TabCollection::Type::UNPINNED);
}

IN_PROC_BROWSER_TEST_F(TreeTabsBrowserTest, Unsplit_FromMiddleNode) {
  SetTreeTabsEnabled(true);

  // 1. - intial tab -> tab1
  //    - tab2
  auto tab1 =
      std::make_unique<tabs::TabModel>(CreateWebContents(), &tab_strip_model());
  tab1->set_opener(tab_strip_model().GetTabAtIndex(0));
  tab_strip_model().AddTab(std::move(tab1), -1,
                           ui::PAGE_TRANSITION_AUTO_BOOKMARK, ADD_NONE);
  ASSERT_EQ(2, tab_strip_model().count());
  ASSERT_EQ(unpinned_collection().ChildCount(), 1u);

  auto tab2 =
      std::make_unique<tabs::TabModel>(CreateWebContents(), &tab_strip_model());
  tab_strip_model().AddTab(std::move(tab2), -1,
                           ui::PAGE_TRANSITION_AUTO_BOOKMARK, ADD_NONE);
  ASSERT_EQ(3, tab_strip_model().count());
  ASSERT_EQ(unpinned_collection().ChildCount(), 2u);

  // 2. - initial tab -> (tab1, tab2) split
  CreateSplitWithTabs(&tab_strip_model(), 1, 2);
  VerifySplitCreated(&tab_strip_model(), &tab_strip_collection());
  ASSERT_EQ(3, tab_strip_model().count());
  ASSERT_EQ(unpinned_collection().ChildCount(), 1u);

  split_tabs::SplitTabId split_id =
      *tab_strip_collection().ListSplits().begin();
  tab_strip_model().RemoveSplit(split_id);

  // When unspliting in the middle, they should become child of the initial
  // tab.
  VerifyUnsplit(&tab_strip_model(), &tab_strip_collection(), 3,
                /*expected_unpinned_children=*/1u);

  const auto& root = std::get<std::unique_ptr<tabs::TabCollection>>(
      unpinned_collection().GetChildren().front());
  EXPECT_EQ(root->GetChildren().size(), 3u);
  EXPECT_EQ(
      std::get<std::unique_ptr<tabs::TabCollection>>(root->GetChildren()[1])
          ->type(),
      tabs::TabCollection::Type::TREE_NODE);
  EXPECT_EQ(
      std::get<std::unique_ptr<tabs::TabCollection>>(root->GetChildren()[2])
          ->type(),
      tabs::TabCollection::Type::TREE_NODE);
}

// Make tab group with existing tabs (tree on); group is wrapped in a tree node
// and tabs inside the group should be direct children of the group without
// any tree nodes.
IN_PROC_BROWSER_TEST_F(TreeTabsBrowserTest, AddToNewGroup_UnwrapsIntoGroup) {
  auto* tab_groups_service =
      tab_groups::TabGroupSyncServiceFactory::GetForProfile(
          browser()->profile());
  ASSERT_TRUE(tab_groups_service);
  tab_groups_service->SetIsInitializedForTesting(true);
  SetTreeTabsEnabled(true);

  for (int i = 0; i < 3; ++i) {
    AddTab();
  }
  ASSERT_EQ(4, tab_strip_model().count());
  // Added tabs are nested, so the unpinned collection should have one child
  // - tab0
  //     - tab1
  //         - tab2
  //             - tab3
  ASSERT_EQ(unpinned_collection().ChildCount(), 1u);

  for (int i = 0; i < tab_strip_model().count(); ++i) {
    EXPECT_EQ(tab_strip_model().GetTabAtIndex(i)->GetParentCollection()->type(),
              tabs::TabCollection::Type::TREE_NODE);
  }

  tab_groups::TabGroupId group_id = tab_strip_model().AddToNewGroup({0, 1});
  ASSERT_TRUE(tab_strip_model().group_model()->ContainsTabGroup(group_id));
  // After grouping two of the tabs, the unpinned collection should have two
  // child, as tab2 will be promoted up to the unpinned collection.
  // - Group(tab0, tab1)
  // - tab2
  //   - tab3
  ASSERT_EQ(unpinned_collection().ChildCount(), 2u);

  EXPECT_EQ(tab_strip_model().GetTabGroupForTab(0), group_id);
  EXPECT_EQ(tab_strip_model().GetTabGroupForTab(1), group_id);
  EXPECT_FALSE(tab_strip_model().GetTabGroupForTab(2).has_value());
  EXPECT_FALSE(tab_strip_model().GetTabGroupForTab(3).has_value());

  // Children of the group should be direct children of the group without any
  // tree nodes.
  EXPECT_EQ(tab_strip_model().GetTabAtIndex(0)->GetParentCollection()->type(),
            tabs::TabCollection::Type::GROUP);
  EXPECT_EQ(tab_strip_model().GetTabAtIndex(1)->GetParentCollection()->type(),
            tabs::TabCollection::Type::GROUP);
  EXPECT_EQ(tab_strip_model().GetTabAtIndex(2)->GetParentCollection()->type(),
            tabs::TabCollection::Type::TREE_NODE);
  EXPECT_EQ(tab_strip_model().GetTabAtIndex(3)->GetParentCollection()->type(),
            tabs::TabCollection::Type::TREE_NODE);

  // The group collection itself is wrapped in a tree node.
  const tabs::TabCollection* group_collection =
      tab_strip_model().GetTabAtIndex(0)->GetParentCollection();
  ASSERT_EQ(group_collection->type(), tabs::TabCollection::Type::GROUP);
  const tabs::TabCollection* group_parent =
      group_collection->GetParentCollection();
  ASSERT_TRUE(group_parent);
  EXPECT_EQ(group_parent->type(), tabs::TabCollection::Type::TREE_NODE);
}

// When removing a tab from a group, the tab should be wrapped in a tree node
IN_PROC_BROWSER_TEST_F(TreeTabsBrowserTest, RemoveFromGroup_WrapsInTreeNodes) {
  auto* tab_groups_service =
      tab_groups::TabGroupSyncServiceFactory::GetForProfile(
          browser()->profile());
  ASSERT_TRUE(tab_groups_service);
  tab_groups_service->SetIsInitializedForTesting(true);

  SetTreeTabsEnabled(true);

  for (int i = 0; i < 3; ++i) {
    AddTab();
  }
  ASSERT_EQ(4, tab_strip_model().count());

  // tab0, (Group(tab1, tab2)), tab3
  tab_groups::TabGroupId group_id = tab_strip_model().AddToNewGroup({1, 2});
  ASSERT_TRUE(tab_strip_model().group_model()->ContainsTabGroup(group_id));
  EXPECT_EQ(tab_strip_model().GetTabAtIndex(1)->GetParentCollection()->type(),
            tabs::TabCollection::Type::GROUP);
  EXPECT_EQ(tab_strip_model().GetTabAtIndex(2)->GetParentCollection()->type(),
            tabs::TabCollection::Type::GROUP);

  // tab0, tab1, tab2, tab3
  tab_strip_model().RemoveFromGroup({1});
  tab_strip_model().RemoveFromGroup({2});

  // The group should be removed.
  EXPECT_TRUE(tab_strip_model().group_model()->ListTabGroups().empty());

  // All tabs should be ungrouped.
  EXPECT_FALSE(tab_strip_model().GetTabGroupForTab(0).has_value());
  EXPECT_FALSE(tab_strip_model().GetTabGroupForTab(1).has_value());
  EXPECT_FALSE(tab_strip_model().GetTabGroupForTab(2).has_value());
  EXPECT_FALSE(tab_strip_model().GetTabGroupForTab(3).has_value());

  // The ungrouped tabs also should be wrapped in tree nodes.
  for (int i = 0; i < tab_strip_model().count(); ++i) {
    EXPECT_EQ(tab_strip_model().GetTabAtIndex(i)->GetParentCollection()->type(),
              tabs::TabCollection::Type::TREE_NODE);
  }
}

// Moving a tab from group A to group B - it should work well without any
// crashes.
IN_PROC_BROWSER_TEST_F(TreeTabsBrowserTest, MoveTab_FromGroupAToGroupB) {
  auto* tab_groups_service =
      tab_groups::TabGroupSyncServiceFactory::GetForProfile(
          browser()->profile());
  ASSERT_TRUE(tab_groups_service);
  tab_groups_service->SetIsInitializedForTesting(true);

  SetTreeTabsEnabled(true);
  for (int i = 0; i < 4; ++i) {
    AddTab();
  }
  ASSERT_EQ(5, tab_strip_model().count());

  // group A(tab0, tab1), tab2, group B(tab3, tab4)
  tab_groups::TabGroupId group_a = tab_strip_model().AddToNewGroup({0, 1});
  tab_groups::TabGroupId group_b = tab_strip_model().AddToNewGroup({3, 4});
  ASSERT_NE(group_a, group_b);

  tabs::TabInterface* tab_formerly_at_0 = tab_strip_model().GetTabAtIndex(0);
  tabs::TabInterface* tab_formerly_at_1 = tab_strip_model().GetTabAtIndex(1);
  tabs::TabInterface* tab_formerly_at_2 = tab_strip_model().GetTabAtIndex(2);
  tabs::TabInterface* tab_formerly_at_3 = tab_strip_model().GetTabAtIndex(3);
  tabs::TabInterface* tab_formerly_at_4 = tab_strip_model().GetTabAtIndex(4);

  // Move tab at index 1 from group A to group B (indices change after move).
  // group A(tab0), tab2, group B(tab1, tab3, tab4)
  tab_strip_model().AddToExistingGroup({1}, group_b, false);
  EXPECT_EQ(tab_strip_model().GetTabGroupForTab(
                tab_strip_model().GetIndexOfTab(tab_formerly_at_0)),
            group_a);
  EXPECT_FALSE(
      tab_strip_model()
          .GetTabGroupForTab(tab_strip_model().GetIndexOfTab(tab_formerly_at_2))
          .has_value());
  EXPECT_EQ(tab_strip_model().GetTabGroupForTab(
                tab_strip_model().GetIndexOfTab(tab_formerly_at_1)),
            group_b);
  EXPECT_EQ(tab_strip_model().GetIndexOfTab(tab_formerly_at_1), 2);
  EXPECT_EQ(tab_strip_model().GetTabGroupForTab(
                tab_strip_model().GetIndexOfTab(tab_formerly_at_3)),
            group_b);
  EXPECT_EQ(tab_strip_model().GetTabGroupForTab(
                tab_strip_model().GetIndexOfTab(tab_formerly_at_4)),
            group_b);

  EXPECT_EQ(1u, tab_strip_model()
                    .group_model()
                    ->GetTabGroup(group_a)
                    ->ListTabs()
                    .length());
  EXPECT_EQ(3u, tab_strip_model()
                    .group_model()
                    ->GetTabGroup(group_b)
                    ->ListTabs()
                    .length());
}

// Make a tab group with a nested tree hierarchy (parent and child in group).
IN_PROC_BROWSER_TEST_F(TreeTabsBrowserTest,
                       MakeTabGroup_WithNestedTreeHierarchy) {
  auto* tab_groups_service =
      tab_groups::TabGroupSyncServiceFactory::GetForProfile(
          browser()->profile());
  ASSERT_TRUE(tab_groups_service);
  tab_groups_service->SetIsInitializedForTesting(true);

  SetTreeTabsEnabled(true);
  // Build A (root) -> B (child).
  auto* tab_a = tab_strip_model().GetTabAtIndex(0);
  auto tab_b_interface =
      std::make_unique<tabs::TabModel>(CreateWebContents(), &tab_strip_model());
  tab_b_interface->set_opener(tab_a);
  tab_strip_model().AddTab(std::move(tab_b_interface), -1,
                           ui::PAGE_TRANSITION_AUTO_BOOKMARK, ADD_NONE);

  ASSERT_EQ(2, tab_strip_model().count());
  ASSERT_EQ(tab_strip_model().GetTabAtIndex(1)->GetParentCollection()->type(),
            tabs::TabCollection::Type::TREE_NODE);
  ASSERT_EQ(tab_strip_model()
                .GetTabAtIndex(1)
                ->GetParentCollection()
                ->GetParentCollection(),
            tab_a->GetParentCollection());

  // Build group with A and B
  tab_groups::TabGroupId group_id = tab_strip_model().AddToNewGroup({0, 1});

  // Then both should be direct children of the group.
  ASSERT_TRUE(tab_strip_model().group_model()->ContainsTabGroup(group_id));
  EXPECT_EQ(tab_strip_model().GetTabGroupForTab(0), group_id);
  EXPECT_EQ(tab_strip_model().GetTabGroupForTab(1), group_id);
  EXPECT_EQ(tab_strip_model().GetTabAtIndex(0)->GetParentCollection()->type(),
            tabs::TabCollection::Type::GROUP);
  EXPECT_EQ(tab_strip_model().GetTabAtIndex(1)->GetParentCollection()->type(),
            tabs::TabCollection::Type::GROUP);

  // Unpinned collection should have only one child - the tree node containing
  // the group.
  ASSERT_EQ(unpinned_collection().ChildCount(), 1u);
  const auto& tree_node_collection =
      std::get<std::unique_ptr<tabs::TabCollection>>(
          unpinned_collection().GetChildren().front());
  EXPECT_EQ(tree_node_collection->type(), tabs::TabCollection::Type::TREE_NODE);
  EXPECT_EQ(tree_node_collection->GetChildren().size(), 1u);

  EXPECT_EQ(
      static_cast<tabs::TreeTabNodeTabCollection*>(tree_node_collection.get())
          ->current_value_type(),
      tabs::TreeTabNodeTabCollection::CurrentValueType::kGroup);
}

// Move a tab from group A (which is nested under a tree node) to root. This
// should work well without any crashes.
IN_PROC_BROWSER_TEST_F(TreeTabsBrowserTest, MoveTab_FromNestedGroupToRoot) {
  auto* tab_groups_service =
      tab_groups::TabGroupSyncServiceFactory::GetForProfile(
          browser()->profile());
  ASSERT_TRUE(tab_groups_service);
  tab_groups_service->SetIsInitializedForTesting(true);

  SetTreeTabsEnabled(true);

  // Build A (root) -> B -> C.
  auto* tab_a = tab_strip_model().GetTabAtIndex(0);
  auto tab_b_interface =
      std::make_unique<tabs::TabModel>(CreateWebContents(), &tab_strip_model());
  tab_b_interface->set_opener(tab_a);
  tab_strip_model().AddTab(std::move(tab_b_interface), -1,
                           ui::PAGE_TRANSITION_AUTO_BOOKMARK, ADD_NONE);
  auto* tab_b = tab_strip_model().GetTabAtIndex(1);
  auto tab_c_interface =
      std::make_unique<tabs::TabModel>(CreateWebContents(), &tab_strip_model());
  tab_c_interface->set_opener(tab_b);
  tab_strip_model().AddTab(std::move(tab_c_interface), -1,
                           ui::PAGE_TRANSITION_AUTO_BOOKMARK, ADD_NONE);
  auto* tab_c = tab_strip_model().GetTabAtIndex(2);
  ASSERT_EQ(3, tab_strip_model().count());

  // Build groups with B and C (indices 1, 2).
  // Then A(Root) -> Group Wrapper(Group(B, C))
  tab_groups::TabGroupId group_id = tab_strip_model().AddToNewGroup({1, 2});
  ASSERT_TRUE(tab_strip_model().group_model()->ContainsTabGroup(group_id));
  EXPECT_EQ(tab_b->GetParentCollection()->type(),
            tabs::TabCollection::Type::GROUP);
  EXPECT_EQ(tab_c->GetParentCollection()->type(),
            tabs::TabCollection::Type::GROUP);
  EXPECT_EQ(tab_b->GetParentCollection(), tab_c->GetParentCollection());
  EXPECT_EQ(tab_strip_model().GetTabAtIndex(0)->GetParentCollection(),
            tab_b
                ->GetParentCollection()    // group
                ->GetParentCollection()    // tree wrapper of group
                ->GetParentCollection());  // root

  // Remove tab B from group so it moves to root (before the group).
  // Then A(Root) -> B
  //              -> GroupWrapper(Group(C))
  tab_strip_model().RemoveFromGroup({1});
  ASSERT_FALSE(tab_strip_model().GetTabGroupForTab(1).has_value());
  ASSERT_EQ(tab_strip_model().GetTabGroupForTab(2), group_id);

  EXPECT_EQ(tab_b->GetParentCollection()->type(),
            tabs::TabCollection::Type::TREE_NODE);
  EXPECT_EQ(tab_b
                ->GetParentCollection()   // wrapper
                ->GetParentCollection(),  // A(root)
            tab_a->GetParentCollection());
}

// Ungroup tabs (partial: one tab out of a group).
// This should work well without any crashes.
IN_PROC_BROWSER_TEST_F(TreeTabsBrowserTest, UngroupTabs_Partial) {
  auto* tab_groups_service =
      tab_groups::TabGroupSyncServiceFactory::GetForProfile(
          browser()->profile());
  ASSERT_TRUE(tab_groups_service);
  tab_groups_service->SetIsInitializedForTesting(true);

  SetTreeTabsEnabled(true);
  for (int i = 0; i < 3; ++i) {
    AddTab();
  }
  ASSERT_EQ(4, tab_strip_model().count());

  // Build group with tab1 and tab2
  // tab0, (Group(tab1, tab2)), tab3
  tab_groups::TabGroupId group_id = tab_strip_model().AddToNewGroup({1, 2});
  ASSERT_TRUE(tab_strip_model().group_model()->ContainsTabGroup(group_id));

  // Remove tab1 from group.
  tab_strip_model().RemoveFromGroup({1});
  ASSERT_FALSE(tab_strip_model().GetTabGroupForTab(1).has_value());
  ASSERT_EQ(tab_strip_model().GetTabGroupForTab(2), group_id);

  // Then tab0, tab1, (Group(tab2)), tab3
  EXPECT_EQ(tab_strip_model().GetTabAtIndex(1)->GetParentCollection()->type(),
            tabs::TabCollection::Type::TREE_NODE);
  EXPECT_EQ(tab_strip_model().GetTabAtIndex(2)->GetParentCollection()->type(),
            tabs::TabCollection::Type::GROUP);
  EXPECT_EQ(4, tab_strip_model().count());
}

// Ungroup all tabs in a group (single call).
IN_PROC_BROWSER_TEST_F(TreeTabsBrowserTest, UngroupAllTabs_InGroup) {
  auto* tab_groups_service =
      tab_groups::TabGroupSyncServiceFactory::GetForProfile(
          browser()->profile());
  ASSERT_TRUE(tab_groups_service);
  tab_groups_service->SetIsInitializedForTesting(true);

  SetTreeTabsEnabled(true);
  for (int i = 0; i < 3; ++i) {
    AddTab();
  }
  ASSERT_EQ(4, tab_strip_model().count());
  tab_groups::TabGroupId group_id = tab_strip_model().AddToNewGroup({1, 2});
  ASSERT_TRUE(tab_strip_model().group_model()->ContainsTabGroup(group_id));

  tab_strip_model().RemoveFromGroup({1, 2});

  EXPECT_FALSE(tab_strip_model().GetTabGroupForTab(1).has_value());
  EXPECT_FALSE(tab_strip_model().GetTabGroupForTab(2).has_value());
  EXPECT_FALSE(tab_strip_model().group_model()->ContainsTabGroup(group_id));
  for (int i = 0; i < tab_strip_model().count(); ++i) {
    EXPECT_EQ(tab_strip_model().GetTabAtIndex(i)->GetParentCollection()->type(),
              tabs::TabCollection::Type::TREE_NODE);
  }
  EXPECT_EQ(4, tab_strip_model().count());
  EXPECT_EQ(unpinned_collection().ChildCount(), 1u);
  EXPECT_TRUE(tab_strip_model().group_model()->ListTabGroups().empty());
}

// Remove tabs from multiple groups at once should work as expected.
IN_PROC_BROWSER_TEST_F(TreeTabsBrowserTest, RemoveTabsFromMultipleGroups) {
  auto* tab_groups_service =
      tab_groups::TabGroupSyncServiceFactory::GetForProfile(
          browser()->profile());
  ASSERT_TRUE(tab_groups_service);
  tab_groups_service->SetIsInitializedForTesting(true);

  SetTreeTabsEnabled(true);
  for (int i = 0; i < 3; ++i) {
    AddTab();
  }
  ASSERT_EQ(4, tab_strip_model().count());

  // Build groups with tab1 and tab2
  // tab0, (Group(tab1, tab2)), (Group(tab3))
  tab_groups::TabGroupId group_id_1 = tab_strip_model().AddToNewGroup({1, 2});
  tab_groups::TabGroupId group_id_2 = tab_strip_model().AddToNewGroup({3});
  ASSERT_TRUE(tab_strip_model().group_model()->ContainsTabGroup(group_id_1));
  ASSERT_TRUE(tab_strip_model().group_model()->ContainsTabGroup(group_id_2));

  // Remove tab1 and tab3 from group.
  tab_strip_model().RemoveFromGroup({1, 3});

  // Then tab0, tab1, (Group(tab2)), tab3
  EXPECT_EQ(tab_strip_model().GetTabAtIndex(1)->GetParentCollection()->type(),
            tabs::TabCollection::Type::TREE_NODE);
  EXPECT_EQ(tab_strip_model().GetTabAtIndex(2)->GetParentCollection()->type(),
            tabs::TabCollection::Type::GROUP);
  EXPECT_EQ(tab_strip_model().GetTabAtIndex(3)->GetParentCollection()->type(),
            tabs::TabCollection::Type::TREE_NODE);
  EXPECT_EQ(4, tab_strip_model().count());
}

// Pinned opener cannot host an unpinned child in the same tree node; the new
// tab must become a top-level tree node in the unpinned collection
IN_PROC_BROWSER_TEST_F(TreeTabsBrowserTest,
                       AddTab_OpenerIsPinned_NewTabIsTopLevelUnpinnedTreeNode) {
  SetTreeTabsEnabled(true);
  // Add a pinned tab.
  ASSERT_EQ(1, tab_strip_model().count());
  tab_strip_model().SetTabPinned(0, true);
  ASSERT_TRUE(tab_strip_model().IsTabPinned(0));

  // Create an unpinned tab from the pinned tab.
  tabs::TabInterface* pinned_opener = tab_strip_model().GetTabAtIndex(0);
  auto tab_interface =
      std::make_unique<tabs::TabModel>(CreateWebContents(), &tab_strip_model());
  tab_interface->set_opener(pinned_opener);
  tab_strip_model().AddTab(std::move(tab_interface), -1,
                           ui::PAGE_TRANSITION_AUTO_BOOKMARK, ADD_NONE);

  // The new tab should be an unpinned tree node.
  ASSERT_EQ(2, tab_strip_model().count());
  tabs::TabInterface* added = tab_strip_model().GetTabAtIndex(1);
  EXPECT_FALSE(tab_strip_model().IsTabPinned(1));
  EXPECT_EQ(added->GetParentCollection()->type(),
            tabs::TabCollection::Type::TREE_NODE);
  EXPECT_EQ(added->GetParentCollection()->GetParentCollection(),
            &unpinned_collection());
  EXPECT_EQ(static_cast<tabs::TabModel*>(added)->opener(), pinned_opener);
}

// PinTabs unwraps the tree node: children are promoted before the tab is moved
// to the pinned collection.
IN_PROC_BROWSER_TEST_F(TreeTabsBrowserTest,
                       PinTab_TreeNodeWithChild_PromotesChildToUnpinnedRoot) {
  SetTreeTabsEnabled(true);
  auto* parent_tab = tab_strip_model().GetTabAtIndex(0);
  auto child_interface =
      std::make_unique<tabs::TabModel>(CreateWebContents(), &tab_strip_model());
  child_interface->set_opener(parent_tab);
  tab_strip_model().AddTab(std::move(child_interface), -1,
                           ui::PAGE_TRANSITION_AUTO_BOOKMARK, ADD_NONE);

  ASSERT_EQ(2, tab_strip_model().count());
  tabs::TabInterface* child_tab = tab_strip_model().GetTabAtIndex(1);
  ASSERT_EQ(parent_tab->GetParentCollection()->ChildCount(), 2u);

  tab_strip_model().SetTabPinned(0, true);

  EXPECT_TRUE(tab_strip_model().IsTabPinned(
      tab_strip_model().GetIndexOfTab(parent_tab)));
  EXPECT_FALSE(tab_strip_model().IsTabPinned(
      tab_strip_model().GetIndexOfTab(child_tab)));
  EXPECT_EQ(child_tab->GetParentCollection()->type(),
            tabs::TabCollection::Type::TREE_NODE);
  EXPECT_EQ(child_tab->GetParentCollection()->GetParentCollection(),
            &unpinned_collection());
  EXPECT_EQ(1u, unpinned_collection().ChildCount());
}

// Split tabs pin/unpin as a unit; unpinned split is wrapped in a tree node
// again.
IN_PROC_BROWSER_TEST_F(TreeTabsBrowserTest,
                       PinUnpin_Split_WrapsTreeNodeWhenUnpinned) {
  // Create a split
  SetTreeTabsEnabled(true);
  AddTab();
  ASSERT_EQ(2, tab_strip_model().count());
  CreateSplitWithTabs(&tab_strip_model(), 0, 1);
  VerifySplitCreated(&tab_strip_model(), &tab_strip_collection());
  EXPECT_EQ(1u, unpinned_collection().ChildCount());

  // Pin the split
  tabs::TabInterface* tab0 = tab_strip_model().GetTabAtIndex(0);
  SetSplitPinned(tab0->GetSplit().value(), true);
  EXPECT_TRUE(tab_strip_model().IsTabPinned(0));
  EXPECT_TRUE(tab_strip_model().IsTabPinned(1));
  EXPECT_EQ(tab0->GetParentCollection()->type(),
            tabs::TabCollection::Type::SPLIT);
  EXPECT_EQ(tab0->GetParentCollection()->GetParentCollection()->type(),
            tabs::TabCollection::Type::PINNED);

  // Unpin the split
  SetSplitPinned(tab0->GetSplit().value(), false);
  EXPECT_FALSE(tab_strip_model().IsTabPinned(0));
  EXPECT_FALSE(tab_strip_model().IsTabPinned(1));
  EXPECT_EQ(tab0->GetParentCollection()->type(),
            tabs::TabCollection::Type::SPLIT);

  // Verify the split is wrapped in a tree node.
  const tabs::TabCollection* split_parent =
      tab0->GetParentCollection()->GetParentCollection();
  ASSERT_EQ(split_parent->type(), tabs::TabCollection::Type::TREE_NODE);
  EXPECT_EQ(split_parent->GetParentCollection(), &unpinned_collection());
  EXPECT_EQ(static_cast<const tabs::TreeTabNodeTabCollection*>(split_parent)
                ->current_value_type(),
            tabs::TreeTabNodeTabCollection::CurrentValueType::kSplit);
}

// Pinning a tab that is inside a tab group uses MoveTabsOutOfGroup with
// |new_pinned_state|; the tab lands in the pinned collection
IN_PROC_BROWSER_TEST_F(TreeTabsBrowserTest,
                       PinTab_FromGroupedTab_MovesToPinnedOutOfGroup) {
  auto* tab_groups_service =
      tab_groups::TabGroupSyncServiceFactory::GetForProfile(
          browser()->profile());
  ASSERT_TRUE(tab_groups_service);
  tab_groups_service->SetIsInitializedForTesting(true);

  SetTreeTabsEnabled(true);
  for (int i = 0; i < 3; ++i) {
    AddTab();
  }

  // Create a group
  // Group(tab0, tab1), tab2
  ASSERT_EQ(4, tab_strip_model().count());
  tab_groups::TabGroupId group_id = tab_strip_model().AddToNewGroup({0, 1});
  ASSERT_TRUE(tab_strip_model().group_model()->ContainsTabGroup(group_id));

  tabs::TabInterface* tab0 = tab_strip_model().GetTabAtIndex(0);
  tabs::TabInterface* tab1 = tab_strip_model().GetTabAtIndex(1);
  ASSERT_EQ(tab0->GetParentCollection()->type(),
            tabs::TabCollection::Type::GROUP);

  // Pin the tab in the group
  // [Pinned: tab0]
  // [Unpinned: Group(tab1), tab2]
  tab_strip_model().SetTabPinned(0, true);

  // Verify the tab is moved to the pinned collection.
  int pinned_idx = tab_strip_model().GetIndexOfTab(tab0);
  ASSERT_EQ(pinned_idx, 0);
  EXPECT_TRUE(tab_strip_model().IsTabPinned(pinned_idx));
  EXPECT_FALSE(tab_strip_model().GetTabGroupForTab(pinned_idx).has_value());
  EXPECT_EQ(tab0->GetParentCollection()->type(),
            tabs::TabCollection::Type::PINNED);

  // Verify the other tab is in the group.
  EXPECT_EQ(tab1->GetParentCollection()->type(),
            tabs::TabCollection::Type::GROUP);
}

// Pinning a tab while there is a split tabs in a pinned collection should work.
IN_PROC_BROWSER_TEST_F(TreeTabsBrowserTest,
                       PinTab_PinnedCollectionAlreadyHasSplit) {
  SetTreeTabsEnabled(true);
  AddTab();

  // Pin two tabs and they are in split collection
  ASSERT_EQ(2, tab_strip_model().count());
  CreateSplitWithTabs(&tab_strip_model(), 0, 1);
  SetSplitPinned(tab_strip_model().GetTabAtIndex(0)->GetSplit().value(), true);

  ASSERT_TRUE(tab_strip_model().IsTabPinned(0));
  ASSERT_TRUE(tab_strip_model().IsTabPinned(1));

  // Only one child with split collection.
  EXPECT_EQ(1u, pinned_collection().ChildCount());
  EXPECT_TRUE(std::holds_alternative<std::unique_ptr<tabs::TabCollection>>(
      pinned_collection().GetChildren()[0]));
  EXPECT_EQ(std::get<std::unique_ptr<tabs::TabCollection>>(
                pinned_collection().GetChildren()[0])
                ->type(),
            tabs::TabCollection::Type::SPLIT);
  ASSERT_TRUE(unpinned_collection().GetChildren().empty());

  // Pin a tab should work well.
  AddTab();
  ASSERT_EQ(3, tab_strip_model().count());
  tab_strip_model().SetTabPinned(2, true);
  EXPECT_TRUE(tab_strip_model().IsTabPinned(2));
  ASSERT_EQ(2u, pinned_collection().ChildCount());
  ASSERT_TRUE(unpinned_collection().GetChildren().empty());

  // Also pinning a tab from a group should work
  AddTab();
  ASSERT_EQ(4, tab_strip_model().count());
  tab_groups::TabGroupId group_id = tab_strip_model().AddToNewGroup({3});
  ASSERT_TRUE(tab_strip_model().group_model()->ContainsTabGroup(group_id));

  tab_strip_model().SetTabPinned(3, true);
  EXPECT_TRUE(tab_strip_model().IsTabPinned(3));
  EXPECT_FALSE(tab_strip_model().GetTabGroupForTab(3));
  ASSERT_FALSE(tab_strip_model().group_model()->ContainsTabGroup(group_id));
  ASSERT_EQ(3u, pinned_collection().ChildCount());
  ASSERT_TRUE(unpinned_collection().GetChildren().empty());

  // Also pinning another split tab should work
  AddTab();
  AddTab();
  ASSERT_EQ(6, tab_strip_model().count());
  CreateSplitWithTabs(&tab_strip_model(), 4, 5);
  VerifySplitCreated(&tab_strip_model(), &tab_strip_collection());

  SetSplitPinned(tab_strip_model().GetTabAtIndex(4)->GetSplit().value(), true);
  ASSERT_EQ(pinned_collection().ChildCount(), 4u);
  ASSERT_TRUE(unpinned_collection().GetChildren().empty());
  EXPECT_TRUE(tab_strip_model().IsTabPinned(4));
  EXPECT_TRUE(tab_strip_model().IsTabPinned(5));

  // So pinned tab collection now should be like
  // [Split(tab0, tab1), tab2, tab3(previously in group), Split(tab4, tab5)]
  EXPECT_TRUE(std::holds_alternative<std::unique_ptr<tabs::TabCollection>>(
      pinned_collection().GetChildren()[0]));
  EXPECT_EQ(std::get<std::unique_ptr<tabs::TabCollection>>(
                pinned_collection().GetChildren()[0])
                ->type(),
            tabs::TabCollection::Type::SPLIT);

  EXPECT_TRUE(std::holds_alternative<std::unique_ptr<tabs::TabInterface>>(
      pinned_collection().GetChildren()[1]));
  EXPECT_TRUE(std::holds_alternative<std::unique_ptr<tabs::TabInterface>>(
      pinned_collection().GetChildren()[2]));
  EXPECT_TRUE(std::holds_alternative<std::unique_ptr<tabs::TabCollection>>(

      pinned_collection().GetChildren()[3]));
  EXPECT_EQ(std::get<std::unique_ptr<tabs::TabCollection>>(
                pinned_collection().GetChildren()[3])
                ->type(),
            tabs::TabCollection::Type::SPLIT);
}

// Grouping the 2nd and 3rd pinned tabs (indices 1 and 2) used to
// crash in the tree-tabs delegate; completing the flow verifies stability.
IN_PROC_BROWSER_TEST_F(TreeTabsBrowserTest,
                       AddToNewGroup_SecondAndThirdPinnedTabs_NoCrash) {
  auto* tab_groups_service =
      tab_groups::TabGroupSyncServiceFactory::GetForProfile(
          browser()->profile());
  ASSERT_TRUE(tab_groups_service);
  tab_groups_service->SetIsInitializedForTesting(true);

  SetTreeTabsEnabled(true);

  AddTab();
  AddTab();
  ASSERT_EQ(3, tab_strip_model().count());

  tab_strip_model().SetTabPinned(0, true);
  tab_strip_model().SetTabPinned(1, true);
  tab_strip_model().SetTabPinned(2, true);
  ASSERT_TRUE(tab_strip_model().IsTabPinned(0));
  ASSERT_TRUE(tab_strip_model().IsTabPinned(1));
  ASSERT_TRUE(tab_strip_model().IsTabPinned(2));

  tabs::TabInterface* first_pinned = tab_strip_model().GetTabAtIndex(0);
  tabs::TabInterface* second_pinned = tab_strip_model().GetTabAtIndex(1);
  tabs::TabInterface* third_pinned = tab_strip_model().GetTabAtIndex(2);

  tab_groups::TabGroupId group_id = tab_strip_model().AddToNewGroup({1, 2});
  ASSERT_TRUE(tab_strip_model().group_model()->ContainsTabGroup(group_id));
  EXPECT_EQ(tab_strip_model().GetTabGroupForTab(
                tab_strip_model().GetIndexOfTab(second_pinned)),
            group_id);
  EXPECT_EQ(tab_strip_model().GetTabGroupForTab(
                tab_strip_model().GetIndexOfTab(third_pinned)),
            group_id);
  EXPECT_FALSE(
      tab_strip_model()
          .GetTabGroupForTab(tab_strip_model().GetIndexOfTab(first_pinned))
          .has_value());
}

// With tree tabs enabled, moving both tabs of a split into an existing group
// must keep the split as one collection inside the group
IN_PROC_BROWSER_TEST_F(TreeTabsBrowserTest,
                       MoveSplitTabsIntoExistingGroup_PreservesSplit) {
  EnsureTabGroupSyncServiceInitialized();
  SetTreeTabsEnabled(true);
  for (int i = 0; i < 3; ++i) {
    AddTab();
  }
  CreateSplitWithTabs(&tab_strip_model(), 0, 1);
  VerifySplitCreated(&tab_strip_model(), &tab_strip_collection());
  ASSERT_EQ(4, tab_strip_model().count());
  // So now we have:
  // [Split(tab0, tab1), tab2, tab3]

  tab_groups::TabGroupId group_id = tab_strip_model().AddToNewGroup({2, 3});
  ASSERT_TRUE(tab_strip_model().group_model()->ContainsTabGroup(group_id));
  EXPECT_EQ(tab_strip_model().GetTabGroupForTab(2), group_id);
  EXPECT_EQ(tab_strip_model().GetTabGroupForTab(3), group_id);
  //  Now we have:
  // [Split(tab0, tab1), Group(tab2, tab3)]

  // Try to add the split to the group
  const split_tabs::SplitTabId split_id =
      tab_strip_model().GetTabAtIndex(0)->GetSplit().value();
  ASSERT_TRUE(tab_strip_model().GetTabAtIndex(1)->GetSplit().has_value());
  EXPECT_EQ(tab_strip_model().GetTabAtIndex(1)->GetSplit().value(), split_id);

  tab_strip_model().AddToExistingGroup({0, 1}, group_id, false);

  // Checks if the model is in good shape.
  ASSERT_EQ(1u, tab_strip_collection().ListSplits().size());
  EXPECT_TRUE(tab_strip_model().ContainsSplit(split_id));

  ExpectGroupModelTabListCount(group_id, 4u);
  ExpectSplitCollectionChildOfGroup(split_id);
}

// Create a split in the unpinned strip, pin it, then add both tabs of the
// pinned split to a new group in one step; the split must stay one collection
// in the group.
IN_PROC_BROWSER_TEST_F(
    TreeTabsBrowserTest,
    AddToNewGroup_PinnedSplit_AfterUnpinnedSplit_PreservesSplit) {
  EnsureTabGroupSyncServiceInitialized();
  SetTreeTabsEnabled(true);
  AddTab();
  ASSERT_EQ(2, tab_strip_model().count());
  CreateSplitWithTabs(&tab_strip_model(), 0, 1);
  VerifySplitCreated(&tab_strip_model(), &tab_strip_collection());

  const split_tabs::SplitTabId split_id =
      tab_strip_model().GetTabAtIndex(0)->GetSplit().value();
  SetSplitPinned(split_id, true);
  ASSERT_TRUE(tab_strip_model().IsTabPinned(0));
  ASSERT_TRUE(tab_strip_model().IsTabPinned(1));
  ASSERT_TRUE(tab_strip_model().ContainsSplit(split_id));

  tab_groups::TabGroupId group_id = tab_strip_model().AddToNewGroup({0, 1});
  ASSERT_TRUE(tab_strip_model().group_model()->ContainsTabGroup(group_id));
  EXPECT_EQ(tab_strip_model().GetTabGroupForTab(0), group_id);
  EXPECT_EQ(tab_strip_model().GetTabGroupForTab(1), group_id);
  ASSERT_TRUE(tab_strip_model().ContainsSplit(split_id));
  ExpectGroupModelTabListCount(group_id, 2u);
  ExpectSplitCollectionChildOfGroup(split_id);
}

// Removing a split from a tab group (MoveTabsOutOfGroup) must keep the split
// intact and re-wrap it as a tree node in the unpinned strip, not destroy the
// split or leave the collection inconsistent.
IN_PROC_BROWSER_TEST_F(TreeTabsBrowserTest,
                       RemoveSplitFromGroup_PreservesSplitAsTreeNode) {
  EnsureTabGroupSyncServiceInitialized();
  SetTreeTabsEnabled(true);
  for (int i = 0; i < 3; ++i) {
    AddTab();
  }
  CreateSplitWithTabs(&tab_strip_model(), 0, 1);
  VerifySplitCreated(&tab_strip_model(), &tab_strip_collection());

  tabs::TabInterface* split_tab_a = tab_strip_model().GetTabAtIndex(0);
  tabs::TabInterface* split_tab_b = tab_strip_model().GetTabAtIndex(1);
  const split_tabs::SplitTabId split_id = split_tab_a->GetSplit().value();

  // Add the split to the group
  tab_groups::TabGroupId group_id = tab_strip_model().AddToNewGroup({2, 3});
  ASSERT_TRUE(tab_strip_model().group_model()->ContainsTabGroup(group_id));
  tab_strip_model().AddToExistingGroup({0, 1}, group_id, false);
  ExpectSplitCollectionChildOfGroup(split_id);

  // Remove the split from the group
  tab_strip_model().RemoveFromGroup({0, 1});

  ASSERT_EQ(1u, tab_strip_collection().ListSplits().size());
  EXPECT_TRUE(tab_strip_model().ContainsSplit(split_id));
  EXPECT_FALSE(split_tab_a->GetGroup().has_value());
  EXPECT_FALSE(split_tab_b->GetGroup().has_value());
  EXPECT_TRUE(split_tab_a->IsSplit());
  EXPECT_TRUE(split_tab_b->IsSplit());
  ExpectSplitWrappedInUnpinnedTreeNode(split_id);
  ExpectGroupModelTabListCount(group_id, 2u);
}

// Detach/reinsert a split uses TabStripCollection::InsertTabCollectionAt (Brave
// delegate): unpinned inserts should wrap the split in a tree node.
IN_PROC_BROWSER_TEST_F(TreeTabsBrowserTest,
                       InsertDetachedSplitTabAt_Unpinned_WrapsSplitInTreeNode) {
  SetTreeTabsEnabled(true);
  AddTab();
  AddTab();
  ASSERT_EQ(3, tab_strip_model().count());
  CreateSplitWithTabs(&tab_strip_model(), 0, 1);
  VerifySplitCreated(&tab_strip_model(), &tab_strip_collection());

  const split_tabs::SplitTabId split_id =
      tab_strip_model().GetTabAtIndex(0)->GetSplit().value();
  std::unique_ptr<DetachedTabCollection> detached =
      tab_strip_model().DetachSplitTabForInsertion(split_id);
  ASSERT_TRUE(detached);
  ASSERT_EQ(1, tab_strip_model().count());
  EXPECT_FALSE(tab_strip_model().ContainsSplit(split_id));

  tab_strip_model().InsertDetachedSplitTabAt(std::move(detached), 0, false);

  ASSERT_EQ(3, tab_strip_model().count());
  ASSERT_TRUE(tab_strip_model().ContainsSplit(split_id));
  EXPECT_EQ(tab_strip_model().GetTabAtIndex(0)->GetSplit().value(), split_id);
  EXPECT_EQ(tab_strip_model().GetTabAtIndex(1)->GetSplit().value(), split_id);

  ExpectSplitWrappedInUnpinnedTreeNode(split_id);
}

// InsertDetachedSplitTabAt with a target group uses InsertTabCollectionAt with
// |parent_group|; the split collection must stay a direct child of the group
// (no tree-node wrapper).
IN_PROC_BROWSER_TEST_F(TreeTabsBrowserTest,
                       InsertDetachedSplitTabAt_IntoGroup_SplitChildOfGroup) {
  EnsureTabGroupSyncServiceInitialized();

  SetTreeTabsEnabled(true);
  AddTab();
  AddTab();
  ASSERT_EQ(3, tab_strip_model().count());
  CreateSplitWithTabs(&tab_strip_model(), 0, 1);
  ASSERT_EQ(1u, tab_strip_collection().ListSplits().size());

  tab_groups::TabGroupId group_id = tab_strip_model().AddToNewGroup({2});
  ASSERT_TRUE(tab_strip_model().group_model()->ContainsTabGroup(group_id));
  EXPECT_EQ(tab_strip_model().GetTabGroupForTab(2), group_id);

  const split_tabs::SplitTabId split_id =
      tab_strip_model().GetTabAtIndex(0)->GetSplit().value();
  std::unique_ptr<DetachedTabCollection> detached =
      tab_strip_model().DetachSplitTabForInsertion(split_id);
  ASSERT_TRUE(detached);
  ASSERT_EQ(1, tab_strip_model().count());

  tab_strip_model().InsertDetachedSplitTabAt(std::move(detached), 0, false,
                                             group_id);

  ASSERT_EQ(3, tab_strip_model().count());
  ASSERT_TRUE(tab_strip_model().ContainsSplit(split_id));
  EXPECT_EQ(tab_strip_model().GetTabGroupForTab(0), group_id);
  EXPECT_EQ(tab_strip_model().GetTabGroupForTab(1), group_id);
  EXPECT_EQ(tab_strip_model().GetTabGroupForTab(2), group_id);

  ExpectSplitParentGroupIs(split_id, group_id);
}

// Selecting every tab in a tab group and using MoveSelectedTabsTo must move the
// group as one unit, not ungroup or move member tabs separately.
IN_PROC_BROWSER_TEST_F(
    TreeTabsBrowserTest,
    MoveSelectedTabsTo_SelectingFullGroup_MovesEntireGroupTogether) {
  EnsureTabGroupSyncServiceInitialized();

  SetTreeTabsEnabled(true);

  for (int i = 0; i < 3; ++i) {
    AddTab();
  }
  ASSERT_EQ(4, tab_strip_model().count());

  tabs::TabInterface* tab_b = tab_strip_model().GetTabAtIndex(1);
  tabs::TabInterface* tab_c = tab_strip_model().GetTabAtIndex(2);

  tab_groups::TabGroupId group_id = tab_strip_model().AddToNewGroup({1, 2});
  ASSERT_TRUE(tab_strip_model().group_model()->ContainsTabGroup(group_id));
  ExpectGroupModelTabListCount(group_id, 2u);

  // Select both grouped tabs (full group), then move the block to the front.
  // Note: MoveSelectedTabsTo(|index|) clamps the unpinned destination to
  // [count() - selected_unpinned_count], so with 4 tabs and 2 selected,
  // MoveSelectedTabsTo(3) becomes destination 2 and is a no-op for tabs at
  // indices 1–2. Use index 0 to get a real reorder.
  tab_strip_model().SelectTabAt(1);
  tab_strip_model().SelectTabAt(2);
  tab_strip_model().MoveSelectedTabsTo(0, std::nullopt);

  ASSERT_EQ(4, tab_strip_model().count());
  EXPECT_EQ(tab_strip_model().GetTabAtIndex(0), tab_b);
  EXPECT_EQ(tab_strip_model().GetTabAtIndex(1), tab_c);
  EXPECT_EQ(tab_strip_model().GetTabGroupForTab(0), group_id);
  EXPECT_EQ(tab_strip_model().GetTabGroupForTab(1), group_id);
  ExpectGroupModelTabListCount(group_id, 2u);
}
