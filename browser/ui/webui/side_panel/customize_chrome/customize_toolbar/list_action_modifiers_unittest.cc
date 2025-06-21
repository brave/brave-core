// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/side_panel/customize_chrome/customize_toolbar/list_action_modifiers.h"

#include <utility>

#include "testing/gtest/include/gtest/gtest.h"

using ListActionModifiersUnitTest = ::testing::Test;

TEST_F(ListActionModifiersUnitTest, FilterUnsupportedChromiumAction) {
  // We have four actions that are not supported.
  //  * ActionId::kShowPaymentMethods
  //  * ActionId::kShowTranslate
  //  * ActionId::kShowReadAnything
  //  * ActionId::kShowAddresses
  // Checks if customize_chrome::FilterUnsupportedChromiumActions() filters them
  // out.
  using ActionId = side_panel::customize_chrome::mojom::ActionId;
  side_panel::customize_chrome::mojom::ActionPtr action =
      side_panel::customize_chrome::mojom::Action::New();
  action->id = ActionId::kShowPaymentMethods;
  std::vector<side_panel::customize_chrome::mojom::ActionPtr> actions;
  actions.push_back(std::move(action));
  action = side_panel::customize_chrome::mojom::Action::New();
  action->id = ActionId::kShowTranslate;
  actions.push_back(std::move(action));
  action = side_panel::customize_chrome::mojom::Action::New();
  action->id = ActionId::kShowReadAnything;
  actions.push_back(std::move(action));
  action = side_panel::customize_chrome::mojom::Action::New();
  action->id = ActionId::kShowAddresses;
  actions.push_back(std::move(action));

  actions =
      customize_chrome::FilterUnsupportedChromiumActions(std::move(actions));
  // The actions vector should be empty now.
  EXPECT_TRUE(actions.empty());

  // Now let's add a supported action and check if it remains.
  action = side_panel::customize_chrome::mojom::Action::New();
  action->id = ActionId::kShowBookmarks;
  actions.push_back(std::move(action));
  actions =
      customize_chrome::FilterUnsupportedChromiumActions(std::move(actions));
  // The actions vector should contain the supported action.
  EXPECT_EQ(actions.size(), 1u);
  EXPECT_EQ(actions[0]->id, ActionId::kShowBookmarks);
}
