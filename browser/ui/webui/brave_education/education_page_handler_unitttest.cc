/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/strings/string_number_conversions.h"
#include "base/test/bind.h"
#include "base/test/test_future.h"
#include "brave/browser/ui/webui/brave_education/education_page_handler.h"
#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "chrome/test/base/scoped_testing_local_state.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/test/browser_task_environment.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_education {

class TestDelegate : public EducationPageHandler::Delegate {
 public:
  using AddActionCallback = base::RepeatingCallback<void(std::string)>;

  explicit TestDelegate(AddActionCallback add_action)
      : add_action_(std::move(add_action)) {}

  ~TestDelegate() override = default;

  void OpenURL(const GURL& url, WindowOpenDisposition disposition) override {
    add_action_.Run("open-url: " + url.spec());
  }

  void OpenRewardsPanel() override { add_action_.Run("open-rewards-panel"); }
  void OpenVPNPanel() override { add_action_.Run("open-vpn-panel"); }
  void OpenAIChat() override { add_action_.Run("open-ai-chat"); }

 private:
  AddActionCallback add_action_;
};

class EducationPageHandlerTest : public testing::Test {
 protected:
  mojo::Remote<mojom::EducationPageHandler>& CreateHandler(
      Profile* profile,
      std::optional<EducationContentType> content_type) {
    page_handler_ = std::make_unique<EducationPageHandler>(
        remote_.BindNewPipeAndPassReceiver(), profile, content_type);

    auto delegate = std::make_unique<TestDelegate>(base::BindLambdaForTesting(
        [this](std::string action) { actions_.push_back(std::move(action)); }));

    page_handler_->SetDelegateForTesting(std::move(delegate));  // IN-TEST

    return remote_;
  }

  const std::vector<std::string>& actions() { return actions_; }

 private:
  content::BrowserTaskEnvironment task_environment_;
  ScopedTestingLocalState local_state{TestingBrowserProcess::GetGlobal()};
  mojo::Remote<mojom::EducationPageHandler> remote_;
  std::unique_ptr<EducationPageHandler> page_handler_;
  std::vector<std::string> actions_;
};

TEST_F(EducationPageHandlerTest, OnlySupportedCommandsAreExecuted) {
  TestingProfile::Builder builder;
  auto profile = builder.Build();
  auto& handler = CreateHandler(profile.get(), std::nullopt);

  base::test::TestFuture<bool> future;
  handler->ExecuteCommand(mojom::Command::kOpenRewardsOnboarding,
                          mojom::ClickInfo::New(), future.GetCallback());

  ASSERT_FALSE(future.Get());
  ASSERT_TRUE(actions().empty());
}

TEST_F(EducationPageHandlerTest, BasicCommandsExecuted) {
  TestingProfile::Builder builder;
  auto profile = builder.Build();
  auto& handler =
      CreateHandler(profile.get(), EducationContentType::kGettingStarted);

  base::test::TestFuture<bool> future;

  handler->ExecuteCommand(mojom::Command::kOpenWalletOnboarding,
                          mojom::ClickInfo::New(), base::DoNothing());
  handler->ExecuteCommand(mojom::Command::kOpenRewardsOnboarding,
                          mojom::ClickInfo::New(), future.GetCallback());

  ASSERT_TRUE(future.Get());
  EXPECT_EQ(actions()[0], "open-url: chrome://wallet/");
  EXPECT_EQ(actions()[1], "open-rewards-panel");
}

TEST_F(EducationPageHandlerTest, VPNCommandsExecuted) {
  TestingProfile::Builder builder;
  auto profile = builder.Build();
  auto& handler =
      CreateHandler(profile.get(), EducationContentType::kGettingStarted);
  base::test::TestFuture<bool> future;

  handler->ExecuteCommand(mojom::Command::kOpenVPNOnboarding,
                          mojom::ClickInfo::New(), future.GetCallback());

#if BUILDFLAG(ENABLE_BRAVE_VPN)
  ASSERT_TRUE(future.Get());
  EXPECT_EQ(actions()[0], "open-vpn-panel");
#else
  ASSERT_FALSE(future.Get());
  EXPECT_TRUE(actions().empty());
#endif
}

TEST_F(EducationPageHandlerTest, ChatCommandsExecuted) {
  TestingProfile::Builder builder;
  auto profile = builder.Build();
  auto& handler =
      CreateHandler(profile.get(), EducationContentType::kGettingStarted);
  base::test::TestFuture<bool> future;

  handler->ExecuteCommand(mojom::Command::kOpenAIChat, mojom::ClickInfo::New(),
                          future.GetCallback());

#if BUILDFLAG(ENABLE_AI_CHAT)
  ASSERT_TRUE(future.Get());
  EXPECT_EQ(actions()[0], "open-ai-chat");
#else
  ASSERT_FALSE(future.Get());
  EXPECT_TRUE(actions().empty());
#endif
}

TEST_F(EducationPageHandlerTest, OffTheRecordProfile) {
  TestingProfile::Builder builder;
  auto profile = builder.Build();
  auto* otr_profile = profile->GetOffTheRecordProfile(
      Profile::OTRProfileID::CreateUniqueForTesting(),
      /*create_if_needed=*/true);

  auto& handler =
      CreateHandler(otr_profile, EducationContentType::kGettingStarted);

  base::test::TestFuture<bool> future;

  handler->ExecuteCommand(mojom::Command::kOpenWalletOnboarding,
                          mojom::ClickInfo::New(), base::DoNothing());
  handler->ExecuteCommand(mojom::Command::kOpenRewardsOnboarding,
                          mojom::ClickInfo::New(), base::DoNothing());
  handler->ExecuteCommand(mojom::Command::kOpenVPNOnboarding,
                          mojom::ClickInfo::New(), base::DoNothing());
  handler->ExecuteCommand(mojom::Command::kOpenAIChat, mojom::ClickInfo::New(),
                          future.GetCallback());

  ASSERT_FALSE(future.Get());
  EXPECT_TRUE(actions().empty());
}

}  // namespace brave_education
