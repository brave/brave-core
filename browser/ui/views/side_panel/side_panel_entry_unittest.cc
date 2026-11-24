/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/side_panel/side_panel_entry.h"

#include <memory>
#include <utility>

#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/functional/callback_helpers.h"
#include "base/test/bind.h"
#include "chrome/browser/ui/side_panel/side_panel_entry_key.h"
#include "chrome/browser/ui/side_panel/side_panel_native_view.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/views/view.h"

namespace brave {

namespace {

std::unique_ptr<SidePanelEntry> MakeEntry(
    base::RepeatingCallback<int()> default_content_width_callback) {
  return std::make_unique<SidePanelEntry>(
      SidePanelEntryKey(SidePanelEntry::Id::kReadingList),
      base::BindRepeating([](SidePanelEntryScope&) {
        return SidePanelNativeView(std::make_unique<views::View>());
      }),
      std::move(default_content_width_callback));
}

}  // namespace

// The literal 400 is intentional: it guards Brave's override of
// SidePanelEntry::kSidePanelDefaultContentWidth.
TEST(SidePanelEntryTest, DefaultContentWidthWithoutCallback) {
  EXPECT_EQ(400, MakeEntry(base::NullCallback())->GetDefaultContentWidth());
}

TEST(SidePanelEntryTest, PreferredWidth) {
  struct {
    const char* name;
    int preferred;
    int expected;
  } const kTestCases[] = {
      {"Below minimum is ignored", 399, 400},
      {"At minimum is used", 400, 400},
      {"Above minimum is used", 520, 520},
  };

  for (const auto& tc : kTestCases) {
    SCOPED_TRACE(tc.name);
    const int preferred = tc.preferred;
    EXPECT_EQ(tc.expected,
              MakeEntry(base::BindLambdaForTesting([preferred] {
                return preferred;
              }))->GetDefaultContentWidth());
  }
}

}  // namespace brave
