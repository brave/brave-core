// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/ui/actions/chrome_action_id.h"

#include <chrome/browser/ui/page_action/page_action_controller_unittest.cc>

namespace page_actions {
namespace {

class BravePageActionControllerTest : public testing::Test {
 public:
  BravePageActionControllerTest()
      : properties_provider_(PageActionPropertiesMap{
            {kActionBookmarkThisTab,
             PageActionProperties{.histogram_name = "Bookmark"}},
            {kFirstActionItemId,
             PageActionProperties{.histogram_name = "Test0"}}}) {}

  void SetUp() override {
    pinned_actions_model_ =
        std::make_unique<PinnedToolbarActionsModel>(&profile_);
    tab_interface_ = std::make_unique<FakeTabInterface>(&profile_);
    tab_interface_->Activate();
    controller_ = std::make_unique<PageActionControllerImpl>(
        *tab_interface_,
        std::vector<actions::ActionId>{kActionBookmarkThisTab,
                                       kFirstActionItemId},
        properties_provider_, pinned_actions_model_.get());
  }

  void TearDown() override {
    actions::ActionManager::Get().ResetForTesting();
    controller_.reset();
    pinned_actions_model_.reset();
    tab_interface_.reset();
  }

  PageActionControllerImpl* controller() { return controller_.get(); }

  std::unique_ptr<ActionItem> BuildActionItem(actions::ActionId action_id) {
    return ActionItem::Builder()
        .SetActionId(action_id)
        .SetVisible(true)
        .SetEnabled(true)
        .Build();
  }

 private:
  content::BrowserTaskEnvironment task_environment_;
  TestPageActionPropertiesProvider properties_provider_;
  TestingProfile profile_;
  std::unique_ptr<PageActionControllerImpl> controller_;
  std::unique_ptr<PinnedToolbarActionsModel> pinned_actions_model_;
  std::unique_ptr<FakeTabInterface> tab_interface_;
};

// Brave has its own bookmark button in the toolbar, so the omnibox one never
// shows, whichever `Show()` overload is called.
TEST_F(BravePageActionControllerTest, BookmarkPageActionNeverShows) {
  PageActionTestObserver bookmark_observer;
  TestPageActionModelObservation bookmark_observation(&bookmark_observer);
  auto bookmark_item = BuildActionItem(kActionBookmarkThisTab);
  base::CallbackListSubscription bookmark_subscription =
      controller()->CreateActionItemSubscription(bookmark_item.get());
  controller()->AddObserver(kActionBookmarkThisTab, bookmark_observation);

  PageActionTestObserver other_observer;
  TestPageActionModelObservation other_observation(&other_observer);
  auto other_item = BuildActionItem(kFirstActionItemId);
  base::CallbackListSubscription other_subscription =
      controller()->CreateActionItemSubscription(other_item.get());
  controller()->AddObserver(kFirstActionItemId, other_observation);

  controller()->Show(kActionBookmarkThisTab);
  controller()->Show(kActionBookmarkThisTab, {.should_animate = false});
  EXPECT_FALSE(bookmark_observer.visible());

  controller()->Show(kFirstActionItemId, {.should_animate = false});
  EXPECT_TRUE(other_observer.visible());
}

}  // namespace
}  // namespace page_actions
