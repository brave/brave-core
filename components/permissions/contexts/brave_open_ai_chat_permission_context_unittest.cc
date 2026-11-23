// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/permissions/contexts/brave_open_ai_chat_permission_context.h"

#include <memory>
#include <utility>

#include "base/test/test_future.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/permissions/permission_request_data.h"
#include "components/permissions/permission_request_id.h"
#include "components/permissions/permission_request_manager.h"
#include "components/permissions/request_type.h"
#include "components/permissions/test/mock_permission_prompt_factory.h"
#include "components/permissions/test/test_permissions_client.h"
#include "content/public/browser/permission_descriptor_util.h"
#include "content/public/browser/permission_result.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/test_renderer_host.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/common/permissions/permission_utils.h"
#include "url/gurl.h"

namespace permissions {

class BraveOpenAIChatPermissionContextTest
    : public content::RenderViewHostTestHarness {
 public:
  BraveOpenAIChatPermissionContextTest() = default;
  ~BraveOpenAIChatPermissionContextTest() override = default;

  content::PermissionStatus RequestPermission(
      BraveOpenAIChatPermissionContext* permission_context,
      const GURL& url) {
    NavigateAndCommit(url);
    prompt_factory_->DocumentOnLoadCompletedInPrimaryMainFrame();

    const PermissionRequestID id(
        web_contents()->GetPrimaryMainFrame()->GetGlobalId(),
        PermissionRequestID::RequestLocalId());
    auto request_data = std::make_unique<PermissionRequestData>(
        blink::mojom::PermissionDescriptorPtr{}, id,
        /*user_gesture=*/true, url);
    request_data->request_type = RequestType::kBraveOpenAIChat;
    base::test::TestFuture<content::PermissionResult> future;
    permission_context->RequestPermission(std::move(request_data),
                                          future.GetCallback());
    return future.Get().status;
  }

 protected:
  void SetUp() override {
    content::RenderViewHostTestHarness::SetUp();
    PermissionRequestManager::CreateForWebContents(web_contents());
    PermissionRequestManager* manager =
        PermissionRequestManager::FromWebContents(web_contents());
    prompt_factory_ = std::make_unique<MockPermissionPromptFactory>(manager);
  }

  void TearDown() override {
    prompt_factory_.reset();
    content::RenderViewHostTestHarness::TearDown();
  }

  TestPermissionsClient client_;
  std::unique_ptr<MockPermissionPromptFactory> prompt_factory_;
};

TEST_F(BraveOpenAIChatPermissionContextTest, PromptForBraveSearch) {
  GURL brave_search_url("https://search.brave.com");

  prompt_factory_->set_response_type(PermissionRequestManager::ACCEPT_ALL);
  BraveOpenAIChatPermissionContext context(browser_context());
  EXPECT_EQ(content::PermissionStatus::GRANTED,
            RequestPermission(&context, brave_search_url));
  EXPECT_EQ(prompt_factory_->show_count(), 1);
}

TEST_F(BraveOpenAIChatPermissionContextTest, BlockForNonBraveSearch) {
  GURL brave_url("https://brave.com");

  prompt_factory_->set_response_type(PermissionRequestManager::ACCEPT_ALL);
  BraveOpenAIChatPermissionContext context(browser_context());
  EXPECT_EQ(content::PermissionStatus::DENIED,
            RequestPermission(&context, brave_url));
  EXPECT_EQ(prompt_factory_->show_count(), 0);
}

TEST_F(BraveOpenAIChatPermissionContextTest, NotAllowedInInsecureOrigins) {
  BraveOpenAIChatPermissionContext permission_context(browser_context());
  GURL insecure_url("http://search.brave.com");
  GURL secure_url("https://search.brave.com");

  EXPECT_EQ(content::PermissionStatus::DENIED,
            permission_context
                .GetPermissionStatus(
                    content::PermissionDescriptorUtil::
                        CreatePermissionDescriptorForPermissionType(
                            blink::PermissionType::NOTIFICATIONS),
                    nullptr /* render_frame_host */, insecure_url, insecure_url)
                .status);

  EXPECT_EQ(content::PermissionStatus::DENIED,
            permission_context
                .GetPermissionStatus(
                    content::PermissionDescriptorUtil::
                        CreatePermissionDescriptorForPermissionType(
                            blink::PermissionType::NOTIFICATIONS),
                    nullptr /* render_frame_host */, insecure_url, secure_url)
                .status);

  EXPECT_EQ(content::PermissionStatus::ASK,
            permission_context
                .GetPermissionStatus(
                    content::PermissionDescriptorUtil::
                        CreatePermissionDescriptorForPermissionType(
                            blink::PermissionType::NOTIFICATIONS),
                    nullptr /* render_frame_host */, secure_url, secure_url)
                .status);
}

}  // namespace permissions
