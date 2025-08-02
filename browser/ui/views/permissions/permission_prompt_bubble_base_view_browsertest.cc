// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/ui/views/permissions/permission_prompt_bubble_base_view.h"

#include "chrome/browser/ui/views/permissions/permission_prompt_style.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/content_settings/core/common/content_settings_types.mojom.h"
#include "components/permissions/permission_request.h"
#include "components/permissions/resolvers/content_setting_permission_resolver.h"
#include "components/permissions/test/mock_permission_prompt.h"
#include "content/public/test/browser_test.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using PermissionPromptBubbleBaseViewBrowserTest = InProcessBrowserTest;

namespace {

class MockPermissionPromptDelegate
    : public permissions::PermissionPrompt::Delegate {
 public:
  MockPermissionPromptDelegate() {
    request_.push_back(std::make_unique<permissions::PermissionRequest>(
        std::make_unique<permissions::PermissionRequestData>(
            std::make_unique<permissions::ContentSettingPermissionResolver>(
                ContentSettingsType::MEDIASTREAM_CAMERA),
            true, GURL()),
        base::DoNothing()));
  }
  ~MockPermissionPromptDelegate() override = default;

  const std::vector<std::unique_ptr<permissions::PermissionRequest>>& Requests()
      override {
    return request_;
  }

  GURL GetRequestingOrigin() const override { return GURL(); }

  GURL GetEmbeddingOrigin() const override { return GURL(); }

  void Accept() override {}
  void AcceptThisTime() override {}
  void Deny() override {}
  void Dismiss() override {}
  void Ignore() override {}
  void SetPromptOptions(PromptOptions prompt_options) override {}
  void FinalizeCurrentRequests() override {}
  void OpenHelpCenterLink(const ui::Event& event) override {}
  void PreIgnoreQuietPrompt() override {}
  void SetManageClicked() override {}
  void SetLearnMoreClicked() override {}
  void SetHatsShownCallback(base::OnceCallback<void()> callback) override {}

  bool WasCurrentRequestAlreadyDisplayed() override { return false; }
  bool ShouldDropCurrentRequestIfCannotShowQuietly() const override {
    return false;
  }
  bool ShouldCurrentRequestUseQuietUI() const override { return false; }
  std::optional<permissions::PermissionUiSelector::QuietUiReason>
  ReasonForUsingQuietUi() const override {
    return std::nullopt;
  }
  void SetDismissOnTabClose() override {}
  void SetPromptShown() override {}
  void SetDecisionTime() override {}
  bool RecreateView() override { return false; }
  const permissions::PermissionPrompt* GetCurrentPrompt() const override {
    return nullptr;
  }

  base::WeakPtr<permissions::PermissionPrompt::Delegate> GetWeakPtr() override {
    return weak_ptr_factory_.GetWeakPtr();
  }

  content::WebContents* GetAssociatedWebContents() override { return nullptr; }

 private:
  std::vector<std::unique_ptr<permissions::PermissionRequest>> request_;

  base::WeakPtrFactory<MockPermissionPromptDelegate> weak_ptr_factory_{this};
};

class MockPermissionPromptBubbleBaseView
    : public PermissionPromptBubbleBaseView {
 public:
  MockPermissionPromptBubbleBaseView(
      Browser* browser,
      base::WeakPtr<permissions::PermissionPrompt::Delegate> delegate)
      : PermissionPromptBubbleBaseView(browser,
                                       delegate,
                                       base::TimeTicks::Now(),
                                       PermissionPromptStyle::kBubbleOnly) {
    CreateWidget();
    ShowWidget();
  }
};

}  // namespace

IN_PROC_BROWSER_TEST_F(PermissionPromptBubbleBaseViewBrowserTest,
                       ZOrderLevelShouldBeSecuritySurface) {
  // This test checks that the permission prompt bubble is created with the
  // correct z-order level, which should be kSecuritySurface.
  MockPermissionPromptDelegate mock_delegate;
  auto* permission_prompt = new MockPermissionPromptBubbleBaseView(
      browser(), mock_delegate.GetWeakPtr());
  EXPECT_EQ(permission_prompt->GetWidget()->GetZOrderLevel(),
            ui::ZOrderLevel::kSecuritySurface);
  permission_prompt->GetWidget()->CloseNow();
}
