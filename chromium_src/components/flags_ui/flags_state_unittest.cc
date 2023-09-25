/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "src/components/flags_ui/flags_state_unittest.cc"

#include <string_view>

#include "base/ranges/algorithm.h"

namespace flags_ui {

TEST_F(FlagsStateTest, ShowDefaultState) {
  // Choose a non-default option for kFlags8.
  flags_state_->SetFeatureEntryEnabled(&flags_storage_,
                                       std::string(kFlags8).append("@1"), true);

  // Disable a feature that is mapped to kFlags10.
  auto feature_list = std::make_unique<base::FeatureList>();
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kTestFeature2);

  // Get flag feature entries.
  base::Value::List supported_entries;
  base::Value::List unsupported_entries;
  flags_state_->GetFlagFeatureEntries(&flags_storage_, kGeneralAccessFlagsOnly,
                                      supported_entries, unsupported_entries,
                                      base::BindRepeating(&SkipFeatureEntry));
  ASSERT_EQ(11u, supported_entries.size());

  auto check_default_option_description =
      [&](std::string_view name, std::string_view expected_description) {
        SCOPED_TRACE(name);
        auto entry_it = base::ranges::find_if(
            supported_entries, [&](const base::Value& entry) {
              return *entry.GetDict().FindString("internal_name") == name;
            });
        ASSERT_TRUE(entry_it != supported_entries.end());
        auto* options = entry_it->GetDict().FindList("options");
        ASSERT_TRUE(options && !options->empty());
        auto* description = (*options)[0].GetDict().FindString("description");
        ASSERT_TRUE(description);
        EXPECT_EQ(*description, expected_description);
      };

  check_default_option_description(kFlags7, "Default (Enabled)");
  check_default_option_description(kFlags8, "Default");
  check_default_option_description(kFlags9, "Default (Enabled)");
  check_default_option_description(kFlags10, "Default (Disabled*)");
  check_default_option_description(kFlags12, "Default (Disabled)");
}

}  // namespace flags_ui
