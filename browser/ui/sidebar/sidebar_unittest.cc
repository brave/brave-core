/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>

#include "base/feature_list.h"
#include "base/scoped_observation.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/ui/sidebar/sidebar_model.h"
#include "brave/browser/ui/sidebar/sidebar_service_factory.h"
#include "brave/components/sidebar/features.h"
#include "brave/components/sidebar/sidebar_service.h"
#include "chrome/test/base/testing_profile.h"
#include "components/prefs/pref_service.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace sidebar {

class SidebarModelTest : public testing::Test, public SidebarModel::Observer {
 public:
  SidebarModelTest() = default;

  ~SidebarModelTest() override = default;

  void SetUp() override {
    scoped_feature_list_.InitAndEnableFeature(kSidebarFeature);
    profile_ = std::make_unique<TestingProfile>();
    service_ = SidebarServiceFactory::GetForProfile(profile_.get());
    model_.reset(new SidebarModel(profile_.get()));
    observation_.Observe(model_.get());
  }

  // SidebarModel::Observer overrides:
  void OnItemMoved(const SidebarItem& item, int from, int to) override {
    on_item_moved_called_ = true;
  }
  void OnActiveIndexChanged(int old_index, int new_index) override {
    on_active_index_changed_called_ = true;
  }

  void ClearState() {
    on_item_moved_called_ = false;
    on_active_index_changed_called_ = false;
  }

  Profile* profile() { return profile_.get(); }
  SidebarModel* model() { return model_.get(); }
  SidebarService* service() { return service_; }

  content::BrowserTaskEnvironment browser_task_environment_;
  bool on_item_moved_called_ = false;
  bool on_active_index_changed_called_ = false;
  SidebarService* service_ = nullptr;
  std::unique_ptr<TestingProfile> profile_;
  std::unique_ptr<SidebarModel> model_;
  base::ScopedObservation<SidebarModel, SidebarModel::Observer> observation_{
      this};
  base::test::ScopedFeatureList scoped_feature_list_;
};

TEST_F(SidebarModelTest, ItemsChangedTest) {
  EXPECT_EQ(0UL, model()->data_.size());
  model()->Init(nullptr);
  EXPECT_EQ(service()->items().size(), model()->data_.size());

  EXPECT_EQ(-1, model()->active_index());

  // Move item at 1 to at index 2.
  // Total size and active index is not changed when currently active index is
  // -1.
  const size_t model_size = model()->data_.size();
  // Cache data at index 1.
  auto* model_data = model()->data_[1].get();
  EXPECT_FALSE(on_item_moved_called_);
  EXPECT_FALSE(on_active_index_changed_called_);

  service()->MoveItem(1, 2);

  EXPECT_TRUE(on_item_moved_called_);
  EXPECT_FALSE(on_active_index_changed_called_);
  EXPECT_EQ(model_data, model()->data_[2].get());
  EXPECT_EQ(-1, model()->active_index());
  EXPECT_EQ(model_size, model()->data_.size());

  model()->SetActiveIndex(1, false);
  EXPECT_EQ(1, model()->active_index());

  // Move item at 1 to 2. This causes active index change because item at 1 was
  // active item. After moving, active item index should be 2.
  ClearState();
  service()->MoveItem(1, 2);
  EXPECT_TRUE(on_item_moved_called_);
  EXPECT_TRUE(on_active_index_changed_called_);
  EXPECT_EQ(2, model()->active_index());

  // Moving item from 1 to 0 doesn't affect active index.
  ClearState();
  service()->MoveItem(1, 0);
  EXPECT_TRUE(on_item_moved_called_);
  EXPECT_FALSE(on_active_index_changed_called_);
  EXPECT_EQ(2, model()->active_index());

  // Moving item from 3 to 0 affect active index. Items behind the active
  // item(at 2) to the front of active index. So, active item is also moved from
  // 2 to 3 index.
  ClearState();
  service()->MoveItem(3, 0);
  EXPECT_TRUE(on_item_moved_called_);
  EXPECT_TRUE(on_active_index_changed_called_);
  EXPECT_EQ(3, model()->active_index());
}

}  // namespace sidebar
