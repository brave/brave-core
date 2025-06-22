// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/side_panel/customize_chrome/customize_toolbar/list_action_modifiers.h"

#include <utility>

#include "base/containers/contains.h"
#include "base/containers/fixed_flat_set.h"
#include "testing/gtest/include/gtest/gtest.h"

using ActionId = side_panel::customize_chrome::mojom::ActionId;

class ListActionModifiersUnitTest : public testing::Test {
 protected:
  static constexpr auto kUnsupportedChromiumActions =
      base::MakeFixedFlatSet<ActionId>(
          {ActionId::kShowPaymentMethods, ActionId::kShowTranslate,
           ActionId::kShowReadAnything, ActionId::kShowAddresses});
};

TEST_F(ListActionModifiersUnitTest,
       FilterUnsupportedChromiumAction_IndividualActions) {
  // For each possible action, verify it's either filtered or preserved
  // correctly

  // Test each action individually to ensure proper filtering
  for (ActionId id = ActionId::kMinValue; id <= ActionId::kMaxValue;
       id = static_cast<ActionId>(static_cast<int>(id) + 1)) {
    std::vector<side_panel::customize_chrome::mojom::ActionPtr> single_action;
    auto action = side_panel::customize_chrome::mojom::Action::New();
    action->id = id;
    single_action.push_back(std::move(action));

    const auto filtered = customize_chrome::FilterUnsupportedChromiumActions(
        std::move(single_action));

    // Check if this action should be filtered out
    const bool should_be_filtered =
        base::Contains(kUnsupportedChromiumActions, id);

    if (should_be_filtered) {
      EXPECT_TRUE(filtered.empty()) << "Action ID " << static_cast<int>(id)
                                    << " should be filtered out but wasn't.";
    } else {
      EXPECT_EQ(filtered.size(), 1u) << "Action ID " << static_cast<int>(id)
                                     << " was incorrectly filtered out.";
      if (!filtered.empty()) {
        EXPECT_EQ(filtered[0]->id, id);
      }
    }
  }
}

TEST_F(ListActionModifiersUnitTest,
       FilterUnsupportedChromiumAction_PossibleActions) {
  // Create vector with all possible actions
  std::vector<side_panel::customize_chrome::mojom::ActionPtr> all_actions;
  for (ActionId id = ActionId::kMinValue; id <= ActionId::kMaxValue;
       id = static_cast<ActionId>(static_cast<int>(id) + 1)) {
    auto action = side_panel::customize_chrome::mojom::Action::New();
    action->id = id;
    all_actions.push_back(std::move(action));
  }

  // Test that the exact expected number of actions remain
  const auto filtered_all = customize_chrome::FilterUnsupportedChromiumActions(
      std::move(all_actions));

  const size_t expected_size = static_cast<int>(ActionId::kMaxValue) -
                               static_cast<int>(ActionId::kMinValue) + 1 -
                               kUnsupportedChromiumActions.size();

  EXPECT_EQ(expected_size, filtered_all.size())
      << "Wrong number of actions after filtering.";

  // Verify none of the unsupported actions remain
  for (const auto& action : filtered_all) {
    EXPECT_FALSE(base::Contains(kUnsupportedChromiumActions, action->id))
        << "Found unsupported action ID " << static_cast<int>(action->id)
        << " in filtered results.";
  }
}
