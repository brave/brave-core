// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/gestures/gesture_command_browser_mapping.h"

#include "chrome/app/chrome_command_ids.h"
#include "components/grit/brave_components_strings.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/l10n/l10n_util.h"

namespace gestures {

TEST(GestureCommandBrowserMappingTest, BackCommandMapsToTheBackBrowserId) {
  EXPECT_EQ(IDC_BACK, MaybeToGestureCommandId(GestureCommandType::kBack));
}

TEST(GestureCommandBrowserMappingTest, UnassignedCommandHasNoBrowserId) {
  EXPECT_FALSE(MaybeToGestureCommandId(GestureCommandType::kUnassigned));
}

TEST(GestureCommandBrowserMappingTest, NavigateToUrlCommandHasNoBrowserId) {
  EXPECT_FALSE(MaybeToGestureCommandId(GestureCommandType::kNavigateToUrl));
}

TEST(GestureCommandBrowserMappingTest,
     BackCommandDisplayNameMatchesItsLocalizedString) {
  EXPECT_EQ(l10n_util::GetStringUTF16(IDS_BRAVE_GESTURES_COMMAND_BACK),
            ToDisplayName(GestureCommandType::kBack));
}

TEST(GestureCommandBrowserMappingTest,
     DifferentCommandsHaveDifferentDisplayNames) {
  EXPECT_NE(ToDisplayName(GestureCommandType::kBack),
            ToDisplayName(GestureCommandType::kForward));
}

}  // namespace gestures
