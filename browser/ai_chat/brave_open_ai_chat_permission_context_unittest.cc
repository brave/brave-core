// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/permissions/contexts/brave_open_ai_chat_permission_context.h"

#include <memory>

#include "base/run_loop.h"
#include "base/test/bind.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/permissions/permission_request_data.h"
#include "components/permissions/permission_request_id.h"
#include "components/permissions/permission_request_manager.h"
#include "components/permissions/test/mock_permission_prompt_factory.h"
#include "content/public/browser/permission_result.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace permissions {

class BraveOpenAIChatPermissionContextTest
    : public ChromeRenderViewHostTestHarness {
 public:
  BraveOpenAIChatPermissionContextTest() = default;
  ~BraveOpenAIChatPermissionContextTest() override = default;

  ContentSetting RequestPermission(
      BraveOpenAIChatPermissionContext* permission_context,
      const GURL& url) {
    NavigateAndCommit(url);
    prompt_factory_->DocumentOnLoadCompletedInPrimaryMainFrame();

    const PermissionRequestID id(
        web_contents()->GetPrimaryMainFrame()->GetGlobalId(),
        PermissionRequestID::RequestLocalId());
    ContentSetting setting = ContentSetting::CONTENT_SETTING_DEFAULT;
    base::RunLoop run_loop;
    permission_context->RequestPermission(
        PermissionRequestData(permission_context, id, /*user_gesture=*/true,
                              url),
        base::BindLambdaForTesting([&](ContentSetting result) {
          setting = result;
          run_loop.Quit();
        }));
    run_loop.Run();

    return setting;
  }

 protected:
  void SetUp() override {
    ChromeRenderViewHostTestHarness::SetUp();
    PermissionRequestManager::CreateForWebContents(web_contents());
    PermissionRequestManager* manager =
        PermissionRequestManager::FromWebContents(web_contents());
    prompt_factory_ = std::make_unique<MockPermissionPromptFactory>(manager);
  }

  void TearDown() override {
    prompt_factory_.reset();
    ChromeRenderViewHostTestHarness::TearDown();
  }

  std::unique_ptr<MockPermissionPromptFactory> prompt_factory_;
};

TEST_F(BraveOpenAIChatPermissionContextTest, PromptForBraveSearch) {
  GURL brave_search_url("https://search.brave.com");

  prompt_factory_->set_response_type(PermissionRequestManager::ACCEPT_ALL);
  BraveOpenAIChatPermissionContext context(browser_context());
  EXPECT_EQ(ContentSetting::CONTENT_SETTING_ALLOW,
            RequestPermission(&context, brave_search_url));
  EXPECT_EQ(prompt_factory_->show_count(), 1);
}

TEST_F(BraveOpenAIChatPermissionContextTest, BlockForNonBraveSearch) {
  GURL brave_url("https://brave.com");

  prompt_factory_->set_response_type(PermissionRequestManager::ACCEPT_ALL);
  BraveOpenAIChatPermissionContext context(browser_context());
  EXPECT_EQ(ContentSetting::CONTENT_SETTING_BLOCK,
            RequestPermission(&context, brave_url));
  EXPECT_EQ(prompt_factory_->show_count(), 0);
}

TEST_F(BraveOpenAIChatPermissionContextTest, NotAllowedInInsecureOrigins) {
  BraveOpenAIChatPermissionContext permission_context(browser_context());
  GURL insecure_url("http://search.brave.com");
  GURL secure_url("https://search.brave.com");

  EXPECT_EQ(content::PermissionStatus::DENIED,
            permission_context
                .GetPermissionStatus(nullptr /* render_frame_host */,
                                     insecure_url, insecure_url)
                .status);

  EXPECT_EQ(content::PermissionStatus::DENIED,
            permission_context
                .GetPermissionStatus(nullptr /* render_frame_host */,
                                     insecure_url, secure_url)
                .status);

  EXPECT_EQ(content::PermissionStatus::ASK,
            permission_context
                .GetPermissionStatus(nullptr /* render_frame_host */,
                                     secure_url, secure_url)
                .status);
}

}  // namespace permissions
