/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/ipfs_ui.h"

#include "chrome/test/base/testing_profile.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/test_web_ui.h"
#include "testing/gtest/include/gtest/gtest.h"

class TestIPFSDomHandler : public IPFSDOMHandler {
 public:
  TestIPFSDomHandler() {
    TestingProfile::Builder builder;
    profile_ = builder.Build();
    web_contents_ = content::WebContents::Create(
        content::WebContents::CreateParams(profile_.get()));

    test_web_ui_.set_web_contents(web_contents_.get());
    set_web_ui(&test_web_ui_);
  }

  ~TestIPFSDomHandler() override {
    // The test handler unusually owns its own TestWebUI, so we make sure to
    // unbind it from the base class before the derived class is destroyed.
    set_web_ui(nullptr);
  }
  content::TestWebUI* web_ui() { return &test_web_ui_; }

 private:
  content::BrowserTaskEnvironment browser_task_environment;
  std::unique_ptr<TestingProfile> profile_;
  std::unique_ptr<content::WebContents> web_contents_;
  content::TestWebUI test_web_ui_;
};

TEST(TestIPFSDomHandler, AddComponentVersion) {
  TestIPFSDomHandler handler;
  ipfs::NodeInfo info;
  info.id = "id1";
  info.version = "version1";
  std::string component = "1.0.11";
  handler.SetIpfsClientUpdaterVersionForTesting(component);
  handler.OnGetNodeInfo(true, info);
  const auto& data = *handler.web_ui()->call_data()[0];
  ASSERT_TRUE(data.arg1()->is_dict());
  EXPECT_EQ(*data.arg1()->GetDict().FindString("id"), info.id);
  EXPECT_EQ(*data.arg1()->GetDict().FindString("version"), info.version);
  EXPECT_EQ(*data.arg1()->GetDict().FindString("component_version"), component);
}

TEST(TestIPFSDomHandler, ComponentNotRegistered) {
  TestIPFSDomHandler handler;
  ipfs::NodeInfo info;
  info.id = "id1";
  info.version = "version1";
  handler.OnGetNodeInfo(true, info);
  const auto& data = *handler.web_ui()->call_data()[0];
  ASSERT_TRUE(data.arg1()->is_dict());
  EXPECT_EQ(*data.arg1()->GetDict().FindString("id"), info.id);
  EXPECT_EQ(*data.arg1()->GetDict().FindString("version"), info.version);
  ASSERT_FALSE(data.arg1()->GetDict().FindString("component_version"));
}
