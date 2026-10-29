// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/ui/webui/cr_components/searchbox/searchbox_handler.h"

#include "brave/components/vector_icons/vector_icons.h"
#include "testing/gtest/include/gtest/gtest.h"

TEST(BraveSearchboxHandlerTest, LeoIconsMapToBraveResources) {
  // `OpenHereAction`'s icon.
  EXPECT_EQ(SearchboxHandler::VectorIconToResourceName(kLeoWindowTabNewIcon),
            "//resources/brave-icons/window-tab-new.svg");
  // The Ask Brave Search starter pack's icon.
  EXPECT_EQ(
      SearchboxHandler::VectorIconToResourceName(kLeoMessageBubbleAskIcon),
      "//resources/brave-icons/message-bubble-ask.svg");
}

TEST(BraveSearchboxHandlerTest, IconWithoutResourceFallsBackToDefault) {
  // Commander matches use an icon with no resource, which would otherwise hit
  // `DUMP_WILL_BE_NOTREACHED`, fatal outside official builds.
  EXPECT_EQ(SearchboxHandler::VectorIconToResourceName(kLeoCaratRightIcon), "");
}
