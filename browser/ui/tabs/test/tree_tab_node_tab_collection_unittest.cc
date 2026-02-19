// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/tabs/public/tree_tab_node_tab_collection.h"

#include "base/functional/callback_helpers.h"
#include "base/test/task_environment.h"
#include "brave/components/tabs/public/tree_tab_node.h"
#include "chrome/browser/ui/tabs/tab_group_desktop.h"
#include "chrome/test/base/testing_profile.h"
#include "components/tabs/public/mock_tab_interface.h"
#include "components/tabs/public/pinned_tab_collection.h"
#include "components/tabs/public/split_tab_collection.h"
#include "components/tabs/public/split_tab_visual_data.h"
#include "components/tabs/public/tab_group_tab_collection.h"
#include "components/tabs/public/unpinned_tab_collection.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

class MockTabInterfaceWithWeakPtr : public tabs::MockTabInterface {
 public:
  using tabs::MockTabInterface::MockTabInterface;
  ~MockTabInterfaceWithWeakPtr() override = default;

  base::WeakPtr<tabs::TabInterface> GetWeakPtr() override {
    return weak_ptr_factory_.GetWeakPtr();
  }

 private:
  base::WeakPtrFactory<MockTabInterfaceWithWeakPtr> weak_ptr_factory_{this};
};

class TreeTabNodeTabCollectionUnitTest : public testing::Test {
 protected:
  TestingProfile* profile() { return testing_profile_.get(); }

 private:
  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<TestingProfile> testing_profile_;
};

TEST_F(TreeTabNodeTabCollectionUnitTest, Constructor) {
  // Constructing tabs::TreeTabNodeTabCollection with empty |tree_tab_node_id|
  // should fail.
  EXPECT_DEATH(
      tabs::TreeTabNodeTabCollection(
          tree_tab::TreeTabNodeId::CreateEmpty(),
          std::make_unique<MockTabInterfaceWithWeakPtr>(), base::DoNothing()),
      "");

  // Constructing tabs::TreeTabNodeTabCollection with nullptr |current_tab|
  // should fail.
  EXPECT_DEATH(
      tabs::TreeTabNodeTabCollection(tree_tab::TreeTabNodeId::GenerateNew(),
                                     nullptr, base::DoNothing()),
      "");

  // Valid construction should succeed.
  auto tree_tab_node_id = tree_tab::TreeTabNodeId::GenerateNew();
  auto mock_tab_interface = std::make_unique<MockTabInterfaceWithWeakPtr>();
  auto mock_tab_interface_ptr = mock_tab_interface.get();
  tabs::TreeTabNodeTabCollection tree_tab_node_tab_collection(
      tree_tab_node_id, std::move(mock_tab_interface), base::DoNothing());

  // Check that the tabs::TreeTabNodeTabCollection is constructed correctly.
  EXPECT_EQ(tree_tab_node_id, tree_tab_node_tab_collection.node().id());
  EXPECT_EQ(mock_tab_interface_ptr,
            tree_tab_node_tab_collection.current_tab().get());
  EXPECT_EQ(0,
            tree_tab_node_tab_collection.GetIndexOfTab(mock_tab_interface_ptr));
}

TEST_F(TreeTabNodeTabCollectionUnitTest, CanNotBeAddedToPinnedCollection) {
  // Create a tabs::TreeTabNodeTabCollection and try to add it to a
  // PinnedTabCollection.
  auto tree_tab_node = std::make_unique<tabs::TreeTabNodeTabCollection>(
      tree_tab::TreeTabNodeId::GenerateNew(),
      std::make_unique<MockTabInterfaceWithWeakPtr>(), base::DoNothing());
  tabs::PinnedTabCollection pinned_collection;

  // Verify that adding a tabs::TreeTabNodeTabCollection to a
  // PinnedTabCollection fails.
  EXPECT_DEATH(pinned_collection.AddCollection(std::move(tree_tab_node), 0),
               "");
}

TEST_F(TreeTabNodeTabCollectionUnitTest, CanBeAddedToUnpinnedCollection) {
  // Create a tabs::TreeTabNodeTabCollection and add it to an
  // UnpinnedTabCollection.
  auto tree_tab_node = std::make_unique<tabs::TreeTabNodeTabCollection>(
      tree_tab::TreeTabNodeId::GenerateNew(),
      std::make_unique<MockTabInterfaceWithWeakPtr>(), base::DoNothing());
  auto tree_tab_node_ptr = tree_tab_node.get();
  tabs::UnpinnedTabCollection unpinned_collection;
  unpinned_collection.AddCollection(std::move(tree_tab_node), 0);

  // Verify that the tabs::TreeTabNodeTabCollection was added correctly to the
  // UnpinnedTabCollection.
  EXPECT_EQ(0, unpinned_collection.GetIndexOfCollection(tree_tab_node_ptr));
}

TEST_F(TreeTabNodeTabCollectionUnitTest, CanAddAnotherTreeTabNodeRecursively) {
  // Create a tabs::TreeTabNodeTabCollection and add another
  // tabs::TreeTabNodeTabCollection as a child.
  auto parent_tree_tab_node = std::make_unique<tabs::TreeTabNodeTabCollection>(
      tree_tab::TreeTabNodeId::GenerateNew(),
      std::make_unique<MockTabInterfaceWithWeakPtr>(), base::DoNothing());
  auto child_tree_tab_node = std::make_unique<tabs::TreeTabNodeTabCollection>(
      tree_tab::TreeTabNodeId::GenerateNew(),
      std::make_unique<MockTabInterfaceWithWeakPtr>(), base::DoNothing());
  auto child_tree_tab_node_ptr = child_tree_tab_node.get();

  parent_tree_tab_node->AddCollection(std::move(child_tree_tab_node), 0);

  // Verify that the child tabs::TreeTabNodeTabCollection was added correctly.
  EXPECT_EQ(
      0, parent_tree_tab_node->GetIndexOfCollection(child_tree_tab_node_ptr));
}

TEST_F(TreeTabNodeTabCollectionUnitTest, CanAddGroupCollection) {
  tabs::TreeTabNodeTabCollection tree_tab_node(
      tree_tab::TreeTabNodeId::GenerateNew(),
      std::make_unique<MockTabInterfaceWithWeakPtr>(), base::DoNothing());

  // Create a TabGroupTabCollection and add it to the
  // tabs::TreeTabNodeTabCollection.
  TabGroupDesktop::Factory tab_group_factory(profile());
  auto tab_group_tab_collection = std::make_unique<tabs::TabGroupTabCollection>(
      tab_group_factory, tab_groups::TabGroupId::GenerateNew(),
      tab_groups::TabGroupVisualData());
  auto tab_group_tab_collection_ptr = tab_group_tab_collection.get();
  tree_tab_node.AddCollection(std::move(tab_group_tab_collection), 0);

  // Verify that the TabGroupTabCollection was added correctly.
  EXPECT_EQ(0,
            tree_tab_node.GetIndexOfCollection(tab_group_tab_collection_ptr));
}

TEST_F(TreeTabNodeTabCollectionUnitTest, CanAddSplitTabCollection) {
  tabs::TreeTabNodeTabCollection tree_tab_node(
      tree_tab::TreeTabNodeId::GenerateNew(),
      std::make_unique<MockTabInterfaceWithWeakPtr>(), base::DoNothing());

  // Create a SplitTabCollection and add it to the
  // tabs::TreeTabNodeTabCollection.
  split_tabs::SplitTabId split_id = split_tabs::SplitTabId::GenerateNew();
  split_tabs::SplitTabVisualData visual_data;
  auto split_tab_collection =
      std::make_unique<tabs::SplitTabCollection>(split_id, visual_data);
  auto split_tab_collection_ptr = split_tab_collection.get();
  tree_tab_node.AddCollection(std::move(split_tab_collection), 0);

  // Verify that the SplitTabCollection was added correctly.
  EXPECT_EQ(0, tree_tab_node.GetIndexOfCollection(split_tab_collection_ptr));
}

// Tests for level and height calculation (root is level 0; leaf height is 0).
TEST_F(TreeTabNodeTabCollectionUnitTest, LevelAndHeight_SingleRootNode) {
  auto tree_tab_node = std::make_unique<tabs::TreeTabNodeTabCollection>(
      tree_tab::TreeTabNodeId::GenerateNew(),
      std::make_unique<MockTabInterfaceWithWeakPtr>(), base::DoNothing());
  auto* node_ptr = tree_tab_node.get();
  tabs::UnpinnedTabCollection unpinned_collection;
  unpinned_collection.AddCollection(std::move(tree_tab_node), 0);

  EXPECT_EQ(0, node_ptr->node().level());
  EXPECT_EQ(0, node_ptr->node().height());
  EXPECT_EQ(0, node_ptr->node().GetTreeHeight());
}

TEST_F(TreeTabNodeTabCollectionUnitTest, LevelAndHeight_RootWithOneTreeChild) {
  auto parent = std::make_unique<tabs::TreeTabNodeTabCollection>(
      tree_tab::TreeTabNodeId::GenerateNew(),
      std::make_unique<MockTabInterfaceWithWeakPtr>(), base::DoNothing());
  auto child = std::make_unique<tabs::TreeTabNodeTabCollection>(
      tree_tab::TreeTabNodeId::GenerateNew(),
      std::make_unique<MockTabInterfaceWithWeakPtr>(), base::DoNothing());
  tabs::TreeTabNodeTabCollection* child_ptr = child.get();

  parent->AddCollection(std::move(child), 0);

  EXPECT_EQ(0, parent->node().level());
  EXPECT_EQ(1, parent->node().height());
  EXPECT_EQ(1, child_ptr->node().level());
  EXPECT_EQ(0, child_ptr->node().height());
  EXPECT_EQ(1, parent->node().GetTreeHeight());
  EXPECT_EQ(1, child_ptr->node().GetTreeHeight());
}

TEST_F(TreeTabNodeTabCollectionUnitTest, LevelAndHeight_ChainOfThree) {
  auto root = std::make_unique<tabs::TreeTabNodeTabCollection>(
      tree_tab::TreeTabNodeId::GenerateNew(),
      std::make_unique<MockTabInterfaceWithWeakPtr>(), base::DoNothing());
  auto child1 = std::make_unique<tabs::TreeTabNodeTabCollection>(
      tree_tab::TreeTabNodeId::GenerateNew(),
      std::make_unique<MockTabInterfaceWithWeakPtr>(), base::DoNothing());
  auto child2 = std::make_unique<tabs::TreeTabNodeTabCollection>(
      tree_tab::TreeTabNodeId::GenerateNew(),
      std::make_unique<MockTabInterfaceWithWeakPtr>(), base::DoNothing());
  tabs::TreeTabNodeTabCollection* child1_ptr = child1.get();
  tabs::TreeTabNodeTabCollection* child2_ptr = child2.get();

  root->AddCollection(std::move(child1), 0);
  child1_ptr->AddCollection(std::move(child2), 0);

  EXPECT_EQ(0, root->node().level());
  EXPECT_EQ(2, root->node().height());
  EXPECT_EQ(1, child1_ptr->node().level());
  EXPECT_EQ(1, child1_ptr->node().height());
  EXPECT_EQ(2, child2_ptr->node().level());
  EXPECT_EQ(0, child2_ptr->node().height());
  EXPECT_EQ(2, root->node().GetTreeHeight());
  EXPECT_EQ(2, child2_ptr->node().GetTreeHeight());
}

// Reparenting: moving a node from parent1 to parent2 should recalculate level,
// height, and tree height correctly for the moved node, parent1, and parent2.
TEST_F(TreeTabNodeTabCollectionUnitTest, LevelAndHeight_Reparenting) {
  tabs::UnpinnedTabCollection unpinned;
  auto parent1 = std::make_unique<tabs::TreeTabNodeTabCollection>(
      tree_tab::TreeTabNodeId::GenerateNew(),
      std::make_unique<MockTabInterfaceWithWeakPtr>(), base::DoNothing());
  auto parent2 = std::make_unique<tabs::TreeTabNodeTabCollection>(
      tree_tab::TreeTabNodeId::GenerateNew(),
      std::make_unique<MockTabInterfaceWithWeakPtr>(), base::DoNothing());
  auto node = std::make_unique<tabs::TreeTabNodeTabCollection>(
      tree_tab::TreeTabNodeId::GenerateNew(),
      std::make_unique<MockTabInterfaceWithWeakPtr>(), base::DoNothing());

  tabs::TreeTabNodeTabCollection* parent1_ptr = parent1.get();
  tabs::TreeTabNodeTabCollection* parent2_ptr = parent2.get();
  tabs::TreeTabNodeTabCollection* node_ptr = node.get();

  unpinned.AddCollection(std::move(parent1), 0);
  unpinned.AddCollection(std::move(parent2), 1);
  parent1_ptr->AddCollection(std::move(node), 0);

  // Initial state: parent1 has node as child.
  EXPECT_EQ(0, parent1_ptr->node().level());
  EXPECT_EQ(1, parent1_ptr->node().height());
  EXPECT_EQ(0, parent2_ptr->node().level());
  EXPECT_EQ(0, parent2_ptr->node().height());
  EXPECT_EQ(1, node_ptr->node().level());
  EXPECT_EQ(0, node_ptr->node().height());
  EXPECT_EQ(1, parent1_ptr->node().GetTreeHeight());
  EXPECT_EQ(0, parent2_ptr->node().GetTreeHeight());
  EXPECT_EQ(1, node_ptr->node().GetTreeHeight());

  // Reparent: move node from parent1 to parent2.
  auto removed = parent1_ptr->MaybeRemoveCollection(node_ptr);
  ASSERT_NE(removed.get(), nullptr);
  parent2_ptr->AddCollection(std::move(removed), 0);

  // After reparenting: level, height, and tree height must be recalculated.
  EXPECT_EQ(0, parent1_ptr->node().level());
  EXPECT_EQ(0, parent1_ptr->node().height());
  EXPECT_EQ(0, parent1_ptr->node().GetTreeHeight());

  EXPECT_EQ(0, parent2_ptr->node().level());
  EXPECT_EQ(1, parent2_ptr->node().height());
  EXPECT_EQ(1, parent2_ptr->node().GetTreeHeight());

  EXPECT_EQ(1, node_ptr->node().level());
  EXPECT_EQ(0, node_ptr->node().height());
  EXPECT_EQ(1, node_ptr->node().GetTreeHeight());
}

// Reparenting a subtree (node with children): level and height of the whole
// subtree should be recalculated under the new parent.
TEST_F(TreeTabNodeTabCollectionUnitTest, LevelAndHeight_ReparentingSubtree) {
  tabs::UnpinnedTabCollection unpinned;
  auto parent1 = std::make_unique<tabs::TreeTabNodeTabCollection>(
      tree_tab::TreeTabNodeId::GenerateNew(),
      std::make_unique<MockTabInterfaceWithWeakPtr>(), base::DoNothing());
  auto parent2 = std::make_unique<tabs::TreeTabNodeTabCollection>(
      tree_tab::TreeTabNodeId::GenerateNew(),
      std::make_unique<MockTabInterfaceWithWeakPtr>(), base::DoNothing());
  auto middle = std::make_unique<tabs::TreeTabNodeTabCollection>(
      tree_tab::TreeTabNodeId::GenerateNew(),
      std::make_unique<MockTabInterfaceWithWeakPtr>(), base::DoNothing());
  auto leaf = std::make_unique<tabs::TreeTabNodeTabCollection>(
      tree_tab::TreeTabNodeId::GenerateNew(),
      std::make_unique<MockTabInterfaceWithWeakPtr>(), base::DoNothing());

  tabs::TreeTabNodeTabCollection* parent1_ptr = parent1.get();
  tabs::TreeTabNodeTabCollection* parent2_ptr = parent2.get();
  tabs::TreeTabNodeTabCollection* middle_ptr = middle.get();
  tabs::TreeTabNodeTabCollection* leaf_ptr = leaf.get();

  unpinned.AddCollection(std::move(parent1), 0);
  unpinned.AddCollection(std::move(parent2), 1);
  parent1_ptr->AddCollection(std::move(middle), 0);
  middle_ptr->AddCollection(std::move(leaf), 0);

  // Initial: parent1 (0, height 2) -> middle (1, height 1) -> leaf (2, height
  // 0).
  EXPECT_EQ(0, parent1_ptr->node().level());
  EXPECT_EQ(2, parent1_ptr->node().height());
  EXPECT_EQ(1, middle_ptr->node().level());
  EXPECT_EQ(1, middle_ptr->node().height());
  EXPECT_EQ(2, leaf_ptr->node().level());
  EXPECT_EQ(0, leaf_ptr->node().height());
  EXPECT_EQ(2, parent1_ptr->node().GetTreeHeight());
  EXPECT_EQ(2, leaf_ptr->node().GetTreeHeight());

  // Reparent middle (and its subtree) from parent1 to parent2.
  auto removed = parent1_ptr->MaybeRemoveCollection(middle_ptr);
  ASSERT_NE(removed.get(), nullptr);
  parent2_ptr->AddCollection(std::move(removed), 0);

  // After reparenting: parent1 is now a leaf; parent2 has middle->leaf.
  EXPECT_EQ(0, parent1_ptr->node().level());
  EXPECT_EQ(0, parent1_ptr->node().height());
  EXPECT_EQ(0, parent1_ptr->node().GetTreeHeight());

  EXPECT_EQ(0, parent2_ptr->node().level());
  EXPECT_EQ(2, parent2_ptr->node().height());
  EXPECT_EQ(2, parent2_ptr->node().GetTreeHeight());

  EXPECT_EQ(1, middle_ptr->node().level());
  EXPECT_EQ(1, middle_ptr->node().height());
  EXPECT_EQ(2, middle_ptr->node().GetTreeHeight());

  EXPECT_EQ(2, leaf_ptr->node().level());
  EXPECT_EQ(0, leaf_ptr->node().height());
  EXPECT_EQ(2, leaf_ptr->node().GetTreeHeight());
}
