/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/external_protocol/external_protocol_handler.h"

#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

class ExternalProtocolHandlerTest : public testing::Test {
 protected:
  void SetUp() override {
    profile_.reset(new TestingProfile());
  }

  void TearDown() override {
    TestingBrowserProcess::GetGlobal()->SetLocalState(nullptr);
  }

  content::BrowserTaskEnvironment task_environment_;
  std::unique_ptr<TestingProfile> profile_;
};

TEST_F(ExternalProtocolHandlerTest, TestGetBlockStateMailto) {
  ExternalProtocolHandler::BlockState block_state =
      ExternalProtocolHandler::GetBlockState("mailto", nullptr, profile_.get());
  EXPECT_EQ(ExternalProtocolHandler::UNKNOWN, block_state);
}
