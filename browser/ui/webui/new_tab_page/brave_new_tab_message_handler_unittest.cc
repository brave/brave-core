// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/new_tab_page/brave_new_tab_message_handler.h"

#include "base/test/simple_test_clock.h"
#include "base/time/time.h"
#include "chrome/browser/first_run/first_run.h"
#include "testing/gtest/include/gtest/gtest.h"

TEST(BraveNewTabMessageHandlerTest, TalkPrompt) {
  auto* clock = new base::SimpleTestClock();
  clock->SetNow(first_run::GetFirstRunSentinelCreationTime());
  EXPECT_EQ(BraveNewTabMessageHandler::CanPromptBraveTalk(clock->Now()), false);
  clock->Advance(base::TimeDelta::FromDays(2));
  EXPECT_EQ(BraveNewTabMessageHandler::CanPromptBraveTalk(clock->Now()), false);
  clock->Advance(base::TimeDelta::FromDays(1));
  EXPECT_EQ(BraveNewTabMessageHandler::CanPromptBraveTalk(clock->Now()), true);
}
