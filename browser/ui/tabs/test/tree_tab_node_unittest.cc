// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/tabs/public/tree_tab_node.h"

#include "base/test/task_environment.h"
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

class TreeTabNodeUnitTest : public testing::Test {
 protected:
  TestingProfile* profile() { return testing_profile_.get(); }

 private:
  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<TestingProfile> testing_profile_;
};

TEST_F(TreeTabNodeUnitTest, Constructor) {
  // Constructing TreeTabNode with empty |tree_tab_node_id| should fail.
  EXPECT_DEATH(
      TreeTabNodeTabCollection(tree_tab::TreeTabNodeId::CreateEmpty(),
                               std::make_unique<tabs::MockTabInterface>()),
      "");

  // Constructing TreeTabNode with nullptr |current_tab| should fail.
  EXPECT_DEATH(
      TreeTabNodeTabCollection(tree_tab::TreeTabNodeId::GenerateNew(), nullptr),
      "");

  // Valid construction should succeed.
  auto tree_tab_node_id = tree_tab::TreeTabNodeId::GenerateNew();
  auto mock_tab_interface = std::make_unique<tabs::MockTabInterface>();
  auto mock_tab_interface_ptr = mock_tab_interface.get();
  TreeTabNodeTabCollection tree_tab_node(tree_tab_node_id,
                                         std::move(mock_tab_interface));

  // Check that the TreeTabNode is constructed correctly.
  EXPECT_EQ(tree_tab_node_id, tree_tab_node.tree_tab_node_id());
  EXPECT_EQ(mock_tab_interface_ptr, tree_tab_node.current_tab());
  EXPECT_EQ(0, tree_tab_node.GetIndexOfTab(mock_tab_interface_ptr));
}

TEST_F(TreeTabNodeUnitTest, CanNotBeAddedToPinnedCollection) {
  // Create a TreeTabNode and try to add it to a PinnedTabCollection.
  auto tree_tab_node = std::make_unique<TreeTabNodeTabCollection>(
      tree_tab::TreeTabNodeId::GenerateNew(),
      std::make_unique<tabs::MockTabInterface>());
  tabs::PinnedTabCollection pinned_collection;

  // Verify that adding a TreeTabNode to a PinnedTabCollection fails.
  EXPECT_DEATH(pinned_collection.AddCollection(std::move(tree_tab_node), 0),
               "");
}

TEST_F(TreeTabNodeUnitTest, CanBeAddedToUnpinnedCollection) {
  // Create a TreeTabNode and add it to an UnpinnedTabCollection.
  auto tree_tab_node = std::make_unique<TreeTabNodeTabCollection>(
      tree_tab::TreeTabNodeId::GenerateNew(),
      std::make_unique<tabs::MockTabInterface>());
  auto tree_tab_node_ptr = tree_tab_node.get();
  tabs::UnpinnedTabCollection unpinned_collection;
  unpinned_collection.AddCollection(std::move(tree_tab_node), 0);

  // Verify that the TreeTabNode was added correctly to the
  // UnpinnedTabCollection.
  EXPECT_EQ(0, unpinned_collection.GetIndexOfCollection(tree_tab_node_ptr));
}

TEST_F(TreeTabNodeUnitTest, CanAddAnotherTreeTabNodeRecursively) {
  // Create a TreeTabNode and add another TreeTabNode as a child.
  auto parent_tree_tab_node = std::make_unique<TreeTabNodeTabCollection>(
      tree_tab::TreeTabNodeId::GenerateNew(),
      std::make_unique<tabs::MockTabInterface>());
  auto child_tree_tab_node = std::make_unique<TreeTabNodeTabCollection>(
      tree_tab::TreeTabNodeId::GenerateNew(),
      std::make_unique<tabs::MockTabInterface>());
  auto child_tree_tab_node_ptr = child_tree_tab_node.get();

  parent_tree_tab_node->AddCollection(std::move(child_tree_tab_node), 0);

  // Verify that the child TreeTabNode was added correctly.
  EXPECT_EQ(
      0, parent_tree_tab_node->GetIndexOfCollection(child_tree_tab_node_ptr));
}

TEST_F(TreeTabNodeUnitTest, CanAddGroupCollection) {
  TreeTabNodeTabCollection tree_tab_node(
      tree_tab::TreeTabNodeId::GenerateNew(),
      std::make_unique<tabs::MockTabInterface>());

  // Create a TabGroupTabCollection and add it to the TreeTabNode.
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

TEST_F(TreeTabNodeUnitTest, CanAddSplitTabCollection) {
  TreeTabNodeTabCollection tree_tab_node(
      tree_tab::TreeTabNodeId::GenerateNew(),
      std::make_unique<tabs::MockTabInterface>());

  // Create a SplitTabCollection and add it to the TreeTabNode.
  split_tabs::SplitTabId split_id = split_tabs::SplitTabId::GenerateNew();
  split_tabs::SplitTabVisualData visual_data;
  auto split_tab_collection =
      std::make_unique<tabs::SplitTabCollection>(split_id, visual_data);
  auto split_tab_collection_ptr = split_tab_collection.get();
  tree_tab_node.AddCollection(std::move(split_tab_collection), 0);

  // Verify that the SplitTabCollection was added correctly.
  EXPECT_EQ(0, tree_tab_node.GetIndexOfCollection(split_tab_collection_ptr));
}
