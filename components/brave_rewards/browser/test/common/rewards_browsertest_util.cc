/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_util.h"

#include <optional>
#include <utility>

#include "base/base64.h"
#include "base/files/file_util.h"
#include "base/json/json_writer.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/strings/stringprintf.h"
#include "base/task/sequenced_task_runner.h"
#include "base/test/bind.h"
#include "brave/browser/brave_rewards/rewards_service_factory.h"
#include "brave/components/brave_rewards/browser/rewards_service_observer.h"
#include "brave/components/brave_rewards/common/mojom/rewards.mojom.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "brave/components/constants/brave_paths.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/os_crypt/sync/os_crypt.h"
#include "components/prefs/pref_service.h"

namespace brave_rewards::test_util {

namespace {

class PublisherUpdatedWaiter : public RewardsServiceObserver {
 public:
  explicit PublisherUpdatedWaiter(RewardsService* rewards_service)
      : rewards_service_(rewards_service) {
    rewards_service_->AddObserver(this);
  }

  ~PublisherUpdatedWaiter() override { rewards_service_->RemoveObserver(this); }

  void OnPublisherUpdated(const std::string& publisher_id) override {
    run_loop_.Quit();
  }

  void Wait() { run_loop_.Run(); }

 private:
  base::RunLoop run_loop_;
  raw_ptr<RewardsService> rewards_service_;
};

}  // namespace

void GetTestDataDir(base::FilePath* test_data_dir) {
  base::ScopedAllowBlockingForTesting allow_blocking;
  ASSERT_TRUE(base::PathService::Get(brave::DIR_TEST_DATA, test_data_dir));
  *test_data_dir = test_data_dir->AppendASCII("rewards-data");
  ASSERT_TRUE(base::PathExists(*test_data_dir));
}

GURL GetRewardsUrl() {
  GURL url("brave://rewards");
  return url;
}

GURL GetRewardsInternalsUrl() {
  GURL url("brave://rewards-internals");
  return url;
}

GURL GetNewTabUrl() {
  GURL url("brave://newtab");
  return url;
}

void StartProcess(RewardsServiceImpl* rewards_service) {
  DCHECK(rewards_service);
  base::RunLoop run_loop;
  rewards_service->StartProcessForTesting(
      base::BindLambdaForTesting([&]() { run_loop.Quit(); }));
  run_loop.Run();
}

void StartProcessWithConnectedUser(Profile* profile) {
  auto* prefs = profile->GetPrefs();

  prefs->SetString(prefs::kDeclaredGeo, "US");

  constexpr char kRewardsWalletJSON[] = R"(
      {"payment_id":"2b6e71a6-f3c7-5999-9235-11605a60ec93",
       "recovery_seed":"QgcQHdg6fo53/bGKVwZlL1UkLiql8X7U68jaWgz6FWQ="})";

  prefs->SetString(prefs::kWalletBrave, kRewardsWalletJSON);

  base::Value::Dict wallet;
  wallet.Set("token", "token");
  wallet.Set("address", GetUpholdExternalAddress());
  wallet.Set("status", static_cast<int>(mojom::WalletStatus::kConnected));
  wallet.Set("user_name", "Brave Test");

  std::string json;
  base::JSONWriter::Write(wallet, &json);
  auto encrypted = EncryptPrefString(json);
  ASSERT_TRUE(encrypted);

  prefs->SetString(prefs::kExternalWalletType, "uphold");
  prefs->SetString(prefs::kWalletUphold, *encrypted);

  auto* rewards_service = RewardsServiceFactory::GetForProfile(profile);
  DCHECK(rewards_service);

  test_util::StartProcess(static_cast<RewardsServiceImpl*>(rewards_service));

  {
    // Verify that the payment ID was read correctly.
    std::string payment_id;
    base::RunLoop run_loop;
    rewards_service->GetRewardsWallet(
        base::BindLambdaForTesting([&](mojom::RewardsWalletPtr rewards_wallet) {
          payment_id = rewards_wallet->payment_id;
          run_loop.Quit();
        }));
    run_loop.Run();
    ASSERT_EQ(payment_id, "2b6e71a6-f3c7-5999-9235-11605a60ec93");
  }

  {
    // Verify that the external wallet data was read correctly.
    mojom::ExternalWalletPtr external_wallet;
    base::RunLoop run_loop;
    rewards_service->GetExternalWallet(
        base::BindLambdaForTesting([&](mojom::ExternalWalletPtr wallet) {
          external_wallet = std::move(wallet);
          run_loop.Quit();
        }));
    run_loop.Run();
    ASSERT_TRUE(external_wallet);
    ASSERT_EQ(external_wallet->address, GetUpholdExternalAddress());
  }
}

GURL GetUrl(
    net::EmbeddedTestServer* https_server,
    const std::string& publisher_key,
    const std::string& path) {
  DCHECK(https_server);
  std::string new_path = path;
  if (new_path.empty()) {
    new_path = "/index.html";
  }
  return https_server->GetURL(publisher_key, new_path);
}

void ActivateTabAtIndex(Browser* browser, const int32_t index) {
  DCHECK(browser);
  browser->tab_strip_model()->ActivateTabAt(
      index, TabStripUserGestureDetails(
                 TabStripUserGestureDetails::GestureType::kOther));
}

std::string BalanceDoubleToString(double amount) {
  return base::StringPrintf("%.3f", amount);
}

std::string GetUpholdExternalAddress() {
  return "abe5f454-fedd-4ea9-9203-470ae7315bb3";
}

std::string GetGeminiExternalAddress() {
  return "00471311-fc4d-463b-9317-579e82b0a6b8";
}

void NavigateToPublisherPage(
    Browser* browser,
    net::EmbeddedTestServer* https_server,
    const std::string& publisher_key,
    const std::string& path) {
  DCHECK(browser && https_server);
  ui_test_utils::NavigateToURLWithDisposition(
      browser,
      GetUrl(https_server, publisher_key, path),
      WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP);
}

void NavigateToPublisherAndWaitForUpdate(Browser* browser,
                                         net::EmbeddedTestServer* https_server,
                                         const std::string& publisher_key) {
  DCHECK(browser);
  auto* rewards_service =
      RewardsServiceFactory::GetForProfile(browser->profile());
  PublisherUpdatedWaiter waiter(rewards_service);
  NavigateToPublisherPage(browser, https_server, publisher_key);
  waiter.Wait();
}

void WaitForEngineStop(RewardsServiceImpl* rewards_service) {
  base::RunLoop run_loop;
  rewards_service->StopEngine(base::BindLambdaForTesting(
      [&](const mojom::Result) { run_loop.Quit(); }));
  run_loop.Run();
}

void WaitForAutoContributeVisitTime() {
  base::RunLoop run_loop;
  base::SequencedTaskRunner::GetCurrentDefault()->PostDelayedTask(
      FROM_HERE, base::BindLambdaForTesting([&]() { run_loop.Quit(); }),
      base::Seconds(2.1));
  run_loop.Run();
}

void CreateRewardsWallet(RewardsServiceImpl* rewards_service,
                         const std::string& country) {
  DCHECK(rewards_service);

  // Ensure that the utility process is started before attempting to create a
  // rewards payment ID.
  StartProcess(rewards_service);

  base::RunLoop run_loop;
  bool success = false;
  rewards_service->CreateRewardsWallet(
      "US",
      base::BindLambdaForTesting([&](mojom::CreateRewardsWalletResult result) {
        success = result == mojom::CreateRewardsWalletResult::kSuccess;
        run_loop.Quit();
      }));

  run_loop.Run();

  ASSERT_TRUE(success);
}

void SetOnboardingBypassed(Browser* browser, bool bypassed) {
  DCHECK(browser);
  // Rewards onboarding will be skipped if the rewards enabled flag is set
  PrefService* prefs = browser->profile()->GetPrefs();
  prefs->SetBoolean(prefs::kEnabled, bypassed);
}

std::optional<std::string> EncryptPrefString(const std::string& value) {
  std::string encrypted;
  if (!OSCrypt::EncryptString(value, &encrypted)) {
    return {};
  }
  return base::Base64Encode(encrypted);
}

std::optional<std::string> DecryptPrefString(const std::string& value) {
  std::string decoded;
  if (!base::Base64Decode(value, &decoded)) {
    return {};
  }
  std::string decrypted;
  if (!OSCrypt::DecryptString(decoded, &decrypted)) {
    return {};
  }
  return decrypted;
}

}  // namespace brave_rewards::test_util
