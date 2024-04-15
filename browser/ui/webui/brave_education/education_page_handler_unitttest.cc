/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>
#include <string>
#include <vector>

#include "base/strings/string_number_conversions.h"
#include "base/test/bind.h"
#include "base/test/test_future.h"
#include "brave/browser/ui/webui/brave_education/education_page_handler.h"
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
  void SetUp() override {
    TestingProfile::Builder builder;
    profile_ = builder.Build();
  }

  mojo::Remote<mojom::EducationPageHandler>& CreateHandler(
      EducationPageType page_type,
      Profile* profile = nullptr) {
    auto delegate = std::make_unique<TestDelegate>(base::BindLambdaForTesting(
        [this](std::string action) { actions_.push_back(std::move(action)); }));

    if (!profile) {
      profile = profile_.get();
    }

    page_handler_ = std::make_unique<EducationPageHandler>(
        remote_.BindNewPipeAndPassReceiver(), profile, page_type,
        std::move(delegate));

    return remote_;
  }

  Profile& profile() { return *profile_; }

  const std::vector<std::string>& actions() { return actions_; }

 private:
  content::BrowserTaskEnvironment task_environment_;
  ScopedTestingLocalState local_state_{TestingBrowserProcess::GetGlobal()};
  mojo::Remote<mojom::EducationPageHandler> remote_;
  std::unique_ptr<Profile> profile_;
  std::unique_ptr<EducationPageHandler> page_handler_;
  std::vector<std::string> actions_;
};

TEST_F(EducationPageHandlerTest, BasicCommandsExecuted) {
  auto& handler = CreateHandler(EducationPageType::kGettingStarted);

  base::test::TestFuture<bool> future;

  handler->ExecuteCommand(mojom::Command::kOpenWalletOnboarding,
                          base::DoNothing());
  handler->ExecuteCommand(mojom::Command::kOpenRewardsOnboarding,
                          future.GetCallback());

  ASSERT_TRUE(future.Get());
  EXPECT_EQ(actions()[0], "open-url: chrome://wallet/");
  EXPECT_EQ(actions()[1], "open-rewards-panel");
}

TEST_F(EducationPageHandlerTest, VPNCommandsExecuted) {
  auto& handler = CreateHandler(EducationPageType::kGettingStarted);
  base::test::TestFuture<bool> future;

  handler->ExecuteCommand(mojom::Command::kOpenVPNOnboarding,
                          future.GetCallback());

#if BUILDFLAG(ENABLE_BRAVE_VPN)
  ASSERT_TRUE(future.Get());
  EXPECT_EQ(actions()[0], "open-vpn-panel");
#else
  ASSERT_FALSE(future.Get());
  EXPECT_TRUE(actions().empty());
#endif
}

TEST_F(EducationPageHandlerTest, ChatCommandsExecuted) {
  auto& handler = CreateHandler(EducationPageType::kGettingStarted);
  base::test::TestFuture<bool> future;

  handler->ExecuteCommand(mojom::Command::kOpenAIChat, future.GetCallback());

  ASSERT_TRUE(future.Get());
  EXPECT_EQ(actions()[0], "open-ai-chat");
}

TEST_F(EducationPageHandlerTest, OffTheRecordProfile) {
  auto* otr_profile = profile().GetOffTheRecordProfile(
      Profile::OTRProfileID::CreateUniqueForTesting(),
      /*create_if_needed=*/true);

  auto& handler =
      CreateHandler(EducationPageType::kGettingStarted, otr_profile);

  base::test::TestFuture<bool> future;

  handler->ExecuteCommand(mojom::Command::kOpenWalletOnboarding,
                          base::DoNothing());
  handler->ExecuteCommand(mojom::Command::kOpenRewardsOnboarding,
                          base::DoNothing());
  handler->ExecuteCommand(mojom::Command::kOpenVPNOnboarding,
                          base::DoNothing());
  handler->ExecuteCommand(mojom::Command::kOpenAIChat, future.GetCallback());

  ASSERT_FALSE(future.Get());
  EXPECT_TRUE(actions().empty());
}

}  // namespace brave_education
