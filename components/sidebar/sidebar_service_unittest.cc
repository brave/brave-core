/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>

#include "base/feature_list.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/sidebar/features.h"
#include "brave/components/sidebar/sidebar_service.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace sidebar {

class SidebarServiceTest : public testing::Test,
                           public SidebarService::Observer {
 public:
  SidebarServiceTest() = default;

  ~SidebarServiceTest() override = default;

  void SetUp() override {
    // Disable by default till we implement all sidebar features.
    EXPECT_FALSE(base::FeatureList::IsEnabled(kSidebarFeature));

    scoped_feature_list_.InitAndEnableFeature(kSidebarFeature);
    SidebarService::RegisterProfilePrefs(prefs_.registry());
    service_.reset(new SidebarService(&prefs_));
    service_->AddObserver(this);
  }

  void TearDown() override { service_->RemoveObserver(this); }

  // SidebarServiceObserver overrides:
  void OnItemAdded(const SidebarItem& item, int index) override {
    item_index_on_called_ = index;
    on_item_added_called_ = true;
  }

  void OnWillRemoveItem(const SidebarItem& item, int index) override {
    item_index_on_called_ = index;
    on_will_remove_item_called_ = true;
  }

  void OnItemRemoved(const SidebarItem& item, int index) override {
    // Make sure OnWillRemoveItem() must be called before this.
    EXPECT_TRUE(on_will_remove_item_called_);
    item_index_on_called_ = index;
    on_item_removed_called_ = true;
  }

  void OnItemMoved(const SidebarItem& item, int from, int to) override {
    on_item_moved_called_ = true;
  }

  void ClearState() {
    item_index_on_called_ = -1;
    on_item_added_called_ = false;
    on_will_remove_item_called_ = false;
    on_item_removed_called_ = false;
    on_item_moved_called_ = false;
  }

  int item_index_on_called_ = -1;
  bool on_item_added_called_ = false;
  bool on_will_remove_item_called_ = false;
  bool on_item_removed_called_ = false;
  bool on_item_moved_called_ = false;

  TestingPrefServiceSimple prefs_;
  std::unique_ptr<SidebarService> service_;
  base::test::ScopedFeatureList scoped_feature_list_;
};

TEST_F(SidebarServiceTest, AddRemoveItems) {
  // Check the default items count.
  EXPECT_EQ(4UL, service_->items().size());
  EXPECT_EQ(0UL, service_->GetNotAddedDefaultSidebarItems().size());

  // Cache 1st item to compare after removing.
  const SidebarItem item = service_->items()[0];
  EXPECT_TRUE(IsBuiltInType(item));

  EXPECT_FALSE(on_will_remove_item_called_);
  EXPECT_FALSE(on_item_removed_called_);
  EXPECT_EQ(-1, item_index_on_called_);

  service_->RemoveItemAt(0);
  EXPECT_EQ(3UL, service_->items().size());
  EXPECT_EQ(1UL, service_->GetNotAddedDefaultSidebarItems().size());
  EXPECT_EQ(0, item_index_on_called_);
  EXPECT_TRUE(on_will_remove_item_called_);
  EXPECT_TRUE(on_item_removed_called_);
  ClearState();

  // Add again.
  EXPECT_FALSE(on_item_added_called_);
  service_->AddItem(item);
  // New item will be added at last.
  EXPECT_EQ(3, item_index_on_called_);
  EXPECT_EQ(4UL, service_->items().size());
  EXPECT_EQ(0UL, service_->GetNotAddedDefaultSidebarItems().size());
  EXPECT_TRUE(on_item_added_called_);
  ClearState();

  const SidebarItem item2 =
      SidebarItem::Create(GURL("https://www.brave.com/"), std::u16string(),
                          SidebarItem::Type::kTypeWeb, true);
  EXPECT_TRUE(IsWebType(item2));
  service_->AddItem(item2);
  EXPECT_EQ(4, item_index_on_called_);
  EXPECT_EQ(5UL, service_->items().size());
  // Default item count is not changed.
  EXPECT_EQ(4UL, service_->GetDefaultSidebarItemsFromCurrentItems().size());
}

TEST_F(SidebarServiceTest, MoveItem) {
  EXPECT_EQ(4UL, service_->items().size());

  // Move item at 0 to index 2.
  SidebarItem item = service_->items()[0];
  service_->MoveItem(0, 2);
  EXPECT_EQ(item.url, service_->items()[2].url);
  EXPECT_TRUE(on_item_moved_called_);

  // Move item at 0 to index 3.
  item = service_->items()[0];
  service_->MoveItem(0, 3);
  EXPECT_EQ(item.url, service_->items()[3].url);

  // Move item at 3 to index 0.
  item = service_->items()[3];
  service_->MoveItem(3, 0);
  EXPECT_EQ(item.url, service_->items()[0].url);

  // Move item at 2 to index 1.
  item = service_->items()[2];
  service_->MoveItem(2, 1);
  EXPECT_EQ(item.url, service_->items()[1].url);
}

}  // namespace sidebar
