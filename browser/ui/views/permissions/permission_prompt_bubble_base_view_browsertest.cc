// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/ui/views/permissions/permission_prompt_bubble_base_view.h"

#include "base/test/run_until.h"
#include "build/build_config.h"
#include "chrome/browser/ui/views/chrome_widget_sublevel.h"
#include "chrome/browser/ui/views/permissions/permission_prompt_style.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/content_settings/core/common/content_settings_types.mojom.h"
#include "components/permissions/permission_request.h"
#include "components/permissions/resolvers/content_setting_permission_resolver.h"
#include "components/permissions/test/mock_permission_prompt.h"
#include "content/public/test/browser_test.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/views/widget/widget_observer.h"

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
  GeolocationAccuracy GetInitialGeolocationAccuracySelection() const override {
    NOTREACHED();
  }
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
    : public PermissionPromptBubbleBaseView,
      public views::WidgetObserver {
 public:
  MockPermissionPromptBubbleBaseView(
      Browser* browser,
      base::WeakPtr<permissions::PermissionPrompt::Delegate> delegate)
      : PermissionPromptBubbleBaseView(browser,
                                       delegate,
                                       PermissionPromptStyle::kBubbleOnly) {
    CreateWidget();
    GetWidget()->AddObserver(this);

    // Instead of using ShowWidget(), directly show the widget to avoid
    // dependency on browser window activation state.
    GetWidget()->Show();
  }
  ~MockPermissionPromptBubbleBaseView() override = default;

  // views::WidgetObserver:
  void OnWidgetCreated(views::Widget* widget) override {
    // Ensure the widget is active when created.
    widget->Activate();
  }

  void OnWidgetDestroyed(views::Widget* widget) override {
    widget->RemoveObserver(this);
  }
};

}  // namespace

#if BUILDFLAG(IS_MAC)
// Flaky on Mac CI.
#define MAYBE_ZOrderLevelShouldBeSecuritySurface \
  DISABLED_ZOrderLevelShouldBeSecuritySurface
#else
#define MAYBE_ZOrderLevelShouldBeSecuritySurface \
  ZOrderLevelShouldBeSecuritySurface
#endif  // BUILDFLAG(IS_MAC)

IN_PROC_BROWSER_TEST_F(PermissionPromptBubbleBaseViewBrowserTest,
                       MAYBE_ZOrderLevelShouldBeSecuritySurface) {
  // This test checks that the permission prompt bubble is created with the
  // correct z-order level, which should be kSecuritySurface.
  MockPermissionPromptDelegate mock_delegate;
  auto create_permission_prompt = [&]() {
    auto* permission_prompt = new MockPermissionPromptBubbleBaseView(
        browser(), mock_delegate.GetWeakPtr());
    // Wait until the prompt widget's native widget is created. Before that,
    // IsActive() will return false.
    EXPECT_TRUE(base::test::RunUntil(
        [&]() { return permission_prompt->GetWidget()->IsActive(); }));
    EXPECT_EQ(permission_prompt->GetWidget()->GetZOrderSublevel(),
              ChromeWidgetSublevel::kSublevelSecurity);
    EXPECT_EQ(permission_prompt->GetWidget()->GetZOrderLevel(),
              ui::ZOrderLevel::kSecuritySurface);

    // Also parent widget should have the same z-order level.
    auto* parent_widget = permission_prompt->GetWidget()->parent();
    EXPECT_TRUE(parent_widget);
    EXPECT_EQ(parent_widget->GetZOrderLevel(),
              ui::ZOrderLevel::kSecuritySurface);
    return permission_prompt;
  };

  // After closing the prompt widget, the parent widget should have the original
  // z-order level.
  auto* permission_prompt = create_permission_prompt();
  auto widget_weak_ptr = permission_prompt->GetWidget()->GetWeakPtr();
  ASSERT_TRUE(widget_weak_ptr);
  auto* parent_widget = widget_weak_ptr->parent();

  permission_prompt->GetWidget()->Close();

  ASSERT_TRUE(base::test::RunUntil([&]() { return !widget_weak_ptr; }));
  EXPECT_NE(parent_widget->GetZOrderLevel(), ui::ZOrderLevel::kSecuritySurface);

  // After Accepting the prompt, the parent widget should have the original
  // z-order level.
  permission_prompt = create_permission_prompt();
  widget_weak_ptr = permission_prompt->GetWidget()->GetWeakPtr();
  ASSERT_TRUE(widget_weak_ptr);
  parent_widget = widget_weak_ptr->parent();

  permission_prompt->AcceptDialog();

  ASSERT_TRUE(base::test::RunUntil([&]() { return !widget_weak_ptr; }));
  EXPECT_NE(parent_widget->GetZOrderLevel(), ui::ZOrderLevel::kSecuritySurface);

  // After Canceling the prompt, the parent widget should have the original
  // z-order level.
  permission_prompt = create_permission_prompt();
  widget_weak_ptr = permission_prompt->GetWidget()->GetWeakPtr();
  ASSERT_TRUE(widget_weak_ptr);
  parent_widget = widget_weak_ptr->parent();

  permission_prompt->CancelDialog();

  ASSERT_TRUE(base::test::RunUntil([&]() { return !widget_weak_ptr; }));
  EXPECT_NE(parent_widget->GetZOrderLevel(), ui::ZOrderLevel::kSecuritySurface);

  // After the prompt is deactivated, the parent widget should have the original
  // z-order level.
  permission_prompt = create_permission_prompt();
  widget_weak_ptr = permission_prompt->GetWidget()->GetWeakPtr();
  parent_widget = widget_weak_ptr->parent();

  widget_weak_ptr->Deactivate();
  parent_widget->Activate();  // To make it sure the deactivation

  ASSERT_TRUE(widget_weak_ptr);
  EXPECT_TRUE(base::test::RunUntil([&]() {
    return widget_weak_ptr->GetZOrderLevel() !=
           ui::ZOrderLevel::kSecuritySurface;
  }));
  EXPECT_TRUE(base::test::RunUntil([&]() {
    return parent_widget->GetZOrderLevel() != ui::ZOrderLevel::kSecuritySurface;
  }));

  // After the prompt is activated, the z-order level should be elevated again.
  widget_weak_ptr->Activate();
  EXPECT_TRUE(base::test::RunUntil([&]() {
    return widget_weak_ptr->GetZOrderLevel() ==
           ui::ZOrderLevel::kSecuritySurface;
  }));
  EXPECT_TRUE(base::test::RunUntil([&]() {
    return parent_widget->GetZOrderLevel() == ui::ZOrderLevel::kSecuritySurface;
  }));
}
