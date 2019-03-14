/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/run_loop.h"
#include "base/path_service.h"
#include "bat/ledger/ledger.h"
#include "brave/common/brave_paths.h"
#include "brave/components/brave_rewards/browser/rewards_service_impl.h"
#include "brave/components/brave_rewards/browser/rewards_service_factory.h"
#include "bat/ledger/internal/bat_helper.h"
#include "bat/ledger/internal/static_values.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test_utils.h"
#include "google_apis/gaia/mock_url_fetcher_factory.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_test_resp {
  std::string registrarVK_;
  std::string verification_;
  std::string wallet_;
  std::string grant_;
}

namespace brave_net {
class BraveURLFetcher : public net::TestURLFetcher {
 public:
  BraveURLFetcher(bool success,
                 const GURL& url,
                 const std::string& results,
                 net::URLFetcher::RequestType request_type,
                 net::URLFetcherDelegate* d);
  ~BraveURLFetcher() override;

  void Start() override;
  void DetermineURLResponsePath(std::string url);

 private:
  void RunDelegate();

  base::WeakPtrFactory<BraveURLFetcher> weak_factory_{this};
  DISALLOW_COPY_AND_ASSIGN(BraveURLFetcher);
};

void split(std::vector<std::string>* tmp, std::string query, char delimiter) {
  std::stringstream ss(query);
  std::string item;
  while (std::getline(ss, item, delimiter)) {
    if (query[0] != '\n') {
      tmp->push_back(item);
    }
  }
}

void BraveURLFetcher::DetermineURLResponsePath(std::string url) {
  std::vector<std::string> tmp;
  brave_net::split(&tmp, url, '/');
  if (url.find(braveledger_bat_helper::buildURL(REGISTER_PERSONA, PREFIX_V2,
    braveledger_bat_helper::SERVER_TYPES::LEDGER)) == 0
    && tmp.size() == 6) {
    SetResponseString(brave_test_resp::registrarVK_);
  } else if (url.find(braveledger_bat_helper::buildURL(REGISTER_PERSONA,
    PREFIX_V2, braveledger_bat_helper::SERVER_TYPES::LEDGER)) == 0
    && tmp.size() == 7) {
    SetResponseString(brave_test_resp::verification_);
  } else if (url.find(braveledger_bat_helper::buildURL(WALLET_PROPERTIES,
    PREFIX_V2, braveledger_bat_helper::SERVER_TYPES::BALANCE)) == 0) {
    SetResponseString(brave_test_resp::wallet_);
  } else if (url.find(braveledger_bat_helper::buildURL(GET_SET_PROMOTION,
    PREFIX_V2, braveledger_bat_helper::SERVER_TYPES::LEDGER)) == 0) {
    SetResponseString(brave_test_resp::grant_);
  } else if (url.find(braveledger_bat_helper::buildURL(GET_PUBLISHERS_LIST_V1,
    "", braveledger_bat_helper::SERVER_TYPES::PUBLISHER)) == 0) {
    SetResponseString("[[\"duckduckgo.com\",true,false]]");
  }
}

BraveURLFetcher::BraveURLFetcher(bool success,
  const GURL& url,
  const std::string& results,
  net::URLFetcher::RequestType request_type,
  net::URLFetcherDelegate* d)
  : net::TestURLFetcher(0, url, d) {
  set_url(url);
  set_status(net::URLRequestStatus(net::URLRequestStatus::SUCCESS, 0));
  set_response_code(net::HTTP_OK);
  DetermineURLResponsePath(url.spec());
}

BraveURLFetcher::~BraveURLFetcher() = default;

void BraveURLFetcher::Start() {
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::BindOnce(&BraveURLFetcher::RunDelegate,
      weak_factory_.GetWeakPtr()));
}

void BraveURLFetcher::RunDelegate() {
  delegate()->OnURLFetchComplete(this);
}

}  // namespace brave_net

class BraveRewardsBrowserTest : public InProcessBrowserTest {
 public:
  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    brave::RegisterPathProvider();
    ReadTestData();
    rewards_service_ = static_cast<brave_rewards::RewardsServiceImpl*>(
        brave_rewards::RewardsServiceFactory::GetForProfile(
            browser()->profile()));
    rewards_service_->SetLedgerEnvForTesting();
  }

  void TearDown() override {
    InProcessBrowserTest::TearDown();
  }

  void RunUntilIdle() {
    base::RunLoop loop;
    loop.RunUntilIdle();
  }

  void GetReconcileTime() {
    rewards_service()->GetReconcileTime(
        base::Bind(&BraveRewardsBrowserTest::OnGetReconcileTime,
          base::Unretained(this)));
  }

  void GetShortRetries() {
    rewards_service()->GetShortRetries(
        base::Bind(&BraveRewardsBrowserTest::OnGetShortRetries,
          base::Unretained(this)));
  }

  void GetProduction() {
    rewards_service()->GetProduction(
        base::Bind(&BraveRewardsBrowserTest::OnGetProduction,
          base::Unretained(this)));
  }

  void GetDebug() {
    rewards_service()->GetDebug(
        base::Bind(&BraveRewardsBrowserTest::OnGetDebug,
          base::Unretained(this)));
  }

  void ReadTestData() {
    base::FilePath path;
    ASSERT_TRUE(base::PathService::Get(brave::DIR_TEST_DATA, &path));
    path = path.AppendASCII("rewards-data");
    ASSERT_TRUE(base::PathExists(path));
    base::ReadFileToString(path.AppendASCII("register_persona_resp.json"),
      &brave_test_resp::registrarVK_);
    ReadFileToString(path.AppendASCII("verify_persona_resp.json"),
      &brave_test_resp::verification_);
    ReadFileToString(path.AppendASCII("wallet_balance_resp.json"),
      &brave_test_resp::wallet_);
    ReadFileToString(path.AppendASCII("ugp_grant_resp.json"),
      &brave_test_resp::grant_);
  }

  GURL rewards_url() {
    GURL rewards_url("brave://rewards");
    return rewards_url;
  }

  content::WebContents* contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  void EnableRewards() {
    // Load rewards page
    ui_test_utils::NavigateToURL(browser(), rewards_url());
    WaitForLoadStop(contents());
    // opt in and create wallet to enable rewards
    ASSERT_TRUE(ExecJs(contents(),
      "document.querySelector(\"[data-test-id='optInAction']\").click();",
      content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
      content::ISOLATED_WORLD_ID_CONTENT_END));
    content::EvalJsResult jsResult = EvalJs(contents(),
      "new Promise((resolve) => {"
      "var count = 10;"
      "var interval = setInterval(function() {"
      "  if (count == 0) {"
      "    clearInterval(interval);"
      "    resolve(false);"
      "  } else {"
      "    count -= 1;"
      "  }"
      "  if (document.querySelector(\"[data-test-id2='enableMain']\")) {"
      "    clearInterval(interval);"
      "    resolve(true);"
      "  }"
      "}, 500);});",
      content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
      content::ISOLATED_WORLD_ID_CONTENT_END);
    ASSERT_TRUE(jsResult.ExtractBool());
  }

  brave_rewards::RewardsServiceImpl* rewards_service() {
    return rewards_service_;
  }

  MOCK_METHOD1(OnGetProduction, void(bool));
  MOCK_METHOD1(OnGetDebug, void(bool));
  MOCK_METHOD1(OnGetReconcileTime, void(int32_t));
  MOCK_METHOD1(OnGetShortRetries, void(bool));

  brave_rewards::RewardsServiceImpl* rewards_service_;
  MockURLFetcherFactory<brave_net::BraveURLFetcher> factory;
};

IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest, RenderWelcome) {
  // Enable Rewards
  EnableRewards();
  EXPECT_STREQ(contents()->GetLastCommittedURL().spec().c_str(),
      // actual url is always chrome://
      "chrome://rewards/");
}

IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest, ToggleRewards) {
  // Enable Rewards
  EnableRewards();

  // Toggle rewards off
  content::EvalJsResult toggleOffResult = EvalJs(contents(),
      "document.querySelector(\"[data-test-id2='enableMain']\").click();"
      "new Promise((resolve) => {"
      "var count = 10;"
      "var interval = setInterval(function() {"
      "  if (count == 0) {"
      "    clearInterval(interval);"
      "    resolve(false);"
      "  } else {"
      "    count -= 1;"
      "  }"
      "  if (document.querySelector(\"[data-test-id2='enableMain']\")) {"
      "    clearInterval(interval);"
      "    resolve(document.querySelector(\"[data-test-id2='enableMain']\")"
      "      .getAttribute(\"data-toggled\") === 'false');"
      "  }"
      "}, 500);});",
      content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
      content::ISOLATED_WORLD_ID_CONTENT_END);
  ASSERT_TRUE(toggleOffResult.ExtractBool());

  // Toggle rewards back on
  content::EvalJsResult toggleOnResult = EvalJs(contents(),
      "document.querySelector(\"[data-test-id2='enableMain']\").click();"
      "new Promise((resolve) => {"
      "var count = 10;"
      "var interval = setInterval(function() {"
      "  if (count == 0) {"
      "    clearInterval(interval);"
      "    resolve(false);"
      "  } else {"
      "    count -= 1;"
      "  }"
      "  if (document.querySelector(\"[data-test-id2='enableMain']\")) {"
      "    clearInterval(interval);"
      "    resolve(document.querySelector(\"[data-test-id2='enableMain']\")"
      "      .getAttribute(\"data-toggled\") === 'true');"
      "  }"
      "}, 500);});",
      content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
      content::ISOLATED_WORLD_ID_CONTENT_END);
  ASSERT_TRUE(toggleOnResult.ExtractBool());
}

IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest, ToggleAutoContribute) {
  EnableRewards();

  // once rewards has loaded, reload page to activate auto-contribute
  contents()->GetController().Reload(content::ReloadType::NORMAL, true);
  EXPECT_TRUE(WaitForLoadStop(contents()));

  // toggle auto contribute off
  content::EvalJsResult toggleOffResult = EvalJs(contents(),
    "let toggleClicked = false;"
    "new Promise((resolve) => {"
    "var count = 10;"
    "var interval = setInterval(function() {"
    "  if (count == 0) {"
    "    clearInterval(interval);"
    "    resolve(false);"
    "  } else {"
    "    count -= 1;"
    "  }"
    "  if (document.querySelector(\"[data-test-id2='autoContribution']\")) {"
    "    if (!toggleClicked) {"
    "      toggleClicked = true;"
    "      document.querySelector("
    "          \"[data-test-id2='autoContribution']\").click();"
    "    } else {"
    "      clearInterval(interval);"
    "      resolve(document.querySelector("
    "          \"[data-test-id2='autoContribution']\")"
    "        .getAttribute(\"data-toggled\") === 'false');"
    "    }"
    "  }"
    "}, 500);});",
    content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
    content::ISOLATED_WORLD_ID_CONTENT_END);
  ASSERT_TRUE(toggleOffResult.ExtractBool());

  // toggle auto contribute back on
  content::EvalJsResult toggleOnResult = EvalJs(contents(),
    "document.querySelector(\"[data-test-id2='autoContribution']\").click();"
    "new Promise((resolve) => {"
    "var count = 10;"
    "var interval = setInterval(function() {"
    "  if (count == 0) {"
    "    clearInterval(interval);"
    "    resolve(false);"
    "  } else {"
    "    count -= 1;"
    "  }"
    "  if (document.querySelector(\"[data-test-id2='autoContribution']\")) {"
    "    clearInterval(interval);"
    "    resolve(document.querySelector(\"[data-test-id2='autoContribution']\")"
    "      .getAttribute(\"data-toggled\") === 'true');"
    "  }"
    "}, 500);});",
      content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
      content::ISOLATED_WORLD_ID_CONTENT_END);
  ASSERT_TRUE(toggleOnResult.ExtractBool());
}

IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest, ActivateSettingsModal) {
  EnableRewards();

  content::EvalJsResult modalResult = EvalJs(contents(),
    "document.querySelector(\"[data-test-id='settingsButton']\").click();"
    "new Promise((resolve) => {"
    "var count = 10;"
    "var interval = setInterval(function() {"
    "  if (count == 0) {"
    "    clearInterval(interval);"
    "    resolve(false);"
    "  } else {"
    "    count -= 1;"
    "  }"
    "  if (document.getElementById('modal')) {"
    "    clearInterval(interval);"
    "    resolve(document.getElementById('modal') != null);"
    "  }"
    "}, 500);});",
      content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
      content::ISOLATED_WORLD_ID_CONTENT_END);
  ASSERT_TRUE(modalResult.ExtractBool());
}

IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest, HandleFlagsSingleArg) {
  testing::InSequence s;
  // SetProduction(true)
  EXPECT_CALL(*this, OnGetProduction(true));
  // Staging - true and 1
  EXPECT_CALL(*this, OnGetProduction(false)).Times(2);
  // Staging - false and random
  EXPECT_CALL(*this, OnGetProduction(true)).Times(2);

  rewards_service()->SetProduction(true);
  GetProduction();
  RunUntilIdle();

  // Staging - true
  rewards_service()->SetProduction(true);
  rewards_service()->HandleFlags("staging=true");
  GetProduction();
  RunUntilIdle();

  // Staging - 1
  rewards_service()->SetProduction(true);
  rewards_service()->HandleFlags("staging=1");
  GetProduction();
  RunUntilIdle();

  // Staging - false
  rewards_service()->SetProduction(false);
  rewards_service()->HandleFlags("staging=false");
  GetProduction();
  RunUntilIdle();

  // Staging - random
  rewards_service()->SetProduction(false);
  rewards_service()->HandleFlags("staging=werwe");
  GetProduction();
  RunUntilIdle();

  // SetDebug(true)
  EXPECT_CALL(*this, OnGetDebug(true));
  // Debug - true and 1
  EXPECT_CALL(*this, OnGetDebug(true)).Times(2);
  // Debug - false and random
  EXPECT_CALL(*this, OnGetDebug(false)).Times(2);

  rewards_service()->SetDebug(true);
  GetDebug();
  RunUntilIdle();

  // Debug - true
  rewards_service()->SetDebug(false);
  rewards_service()->HandleFlags("debug=true");
  GetDebug();
  RunUntilIdle();

  // Debug - 1
  rewards_service()->SetDebug(false);
  rewards_service()->HandleFlags("debug=1");
  GetDebug();
  RunUntilIdle();

  // Debug - false
  rewards_service()->SetDebug(true);
  rewards_service()->HandleFlags("debug=false");
  GetDebug();
  RunUntilIdle();

  // Debug - random
  rewards_service()->SetDebug(true);
  rewards_service()->HandleFlags("debug=werwe");
  GetDebug();
  RunUntilIdle();

  // positive number
  EXPECT_CALL(*this, OnGetReconcileTime(10));
  // negative number and string
  EXPECT_CALL(*this, OnGetReconcileTime(0)).Times(2);

  // Reconcile interval - positive number
  rewards_service()->SetReconcileTime(0);
  rewards_service()->HandleFlags("reconcile-interval=10");
  GetReconcileTime();
  RunUntilIdle();

  // Reconcile interval - negative number
  rewards_service()->SetReconcileTime(0);
  rewards_service()->HandleFlags("reconcile-interval=-1");
  GetReconcileTime();
  RunUntilIdle();

  // Reconcile interval - string
  rewards_service()->SetReconcileTime(0);
  rewards_service()->HandleFlags("reconcile-interval=sdf");
  GetReconcileTime();
  RunUntilIdle();

  EXPECT_CALL(*this, OnGetShortRetries(true));  // on
  EXPECT_CALL(*this, OnGetShortRetries(false));  // off

  // Short retries - on
  rewards_service()->SetShortRetries(false);
  rewards_service()->HandleFlags("short-retries=true");
  GetShortRetries();
  RunUntilIdle();

  // Short retries - off
  rewards_service()->SetShortRetries(true);
  rewards_service()->HandleFlags("short-retries=false");
  GetShortRetries();
  RunUntilIdle();
}

IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest, HandleFlagsMultipleFlags) {
  EXPECT_CALL(*this, OnGetProduction(false));
  EXPECT_CALL(*this, OnGetDebug(true));
  EXPECT_CALL(*this, OnGetReconcileTime(10));
  EXPECT_CALL(*this, OnGetShortRetries(true));

  rewards_service()->SetProduction(true);
  rewards_service()->SetDebug(true);
  rewards_service()->SetReconcileTime(0);
  rewards_service()->SetShortRetries(false);

  rewards_service()->HandleFlags(
      "staging=true,debug=true,short-retries=true,reconcile-interval=10");

  GetReconcileTime();
  GetShortRetries();
  GetProduction();
  GetDebug();
  RunUntilIdle();
}

IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest, HandleFlagsWrongInput) {
  EXPECT_CALL(*this, OnGetProduction(true));
  EXPECT_CALL(*this, OnGetDebug(false));
  EXPECT_CALL(*this, OnGetReconcileTime(0));
  EXPECT_CALL(*this, OnGetShortRetries(false));

  rewards_service()->SetProduction(true);
  rewards_service()->SetDebug(false);
  rewards_service()->SetReconcileTime(0);
  rewards_service()->SetShortRetries(false);

  rewards_service()->HandleFlags(
      "staging=,debug=,shortretries=true,reconcile-interval");

  GetReconcileTime();
  GetShortRetries();
  GetDebug();
  GetProduction();
  RunUntilIdle();
}
