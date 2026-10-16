// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/gestures/gesture_command_browser_mapping.h"

#include "base/strings/utf_string_conversions.h"
#include "brave/app/command_utils.h"
#include "brave/components/gestures/core/common/actions/gesture_command_constants.h"
#include "chrome/app/chrome_command_ids.h"
#include "components/grit/brave_components_strings.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/l10n/l10n_util.h"

namespace gestures {

TEST(GestureCommandBrowserMappingTest, RealCommandIdPassesThroughUnchanged) {
  EXPECT_EQ(IDC_BACK, MaybeToGestureCommandId(IDC_BACK));
}

TEST(GestureCommandBrowserMappingTest, UnassignedCommandHasNoBrowserId) {
  EXPECT_FALSE(MaybeToGestureCommandId(kUnassigned));
}

TEST(GestureCommandBrowserMappingTest, NavigateToUrlCommandHasNoBrowserId) {
  EXPECT_FALSE(MaybeToGestureCommandId(kNavigateToUrl));
}

TEST(GestureCommandBrowserMappingTest, ScrollToTopCommandHasNoBrowserId) {
  EXPECT_FALSE(MaybeToGestureCommandId(kScrollToTop));
}

TEST(GestureCommandBrowserMappingTest, SwitchToTabCommandHasNoBrowserId) {
  EXPECT_FALSE(MaybeToGestureCommandId(kSwitchToTab));
}

TEST(GestureCommandBrowserMappingTest,
     SentinelDisplayNameMatchesItsLocalizedString) {
  EXPECT_EQ(l10n_util::GetStringUTF16(IDS_BRAVE_GESTURES_COMMAND_UNASSIGNED),
            ToDisplayName(kUnassigned));
}

TEST(GestureCommandBrowserMappingTest,
     RealCommandIdDisplayNameComesFromCommandUtils) {
  EXPECT_EQ(base::UTF8ToUTF16(commands::GetCommandName(IDC_BACK)),
            ToDisplayName(IDC_BACK));
}

TEST(GestureCommandBrowserMappingTest,
     DifferentRealCommandsHaveDifferentDisplayNames) {
  EXPECT_NE(ToDisplayName(IDC_BACK), ToDisplayName(IDC_FORWARD));
}

}  // namespace gestures
