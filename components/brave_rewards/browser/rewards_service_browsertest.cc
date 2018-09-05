/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/path_service.h"
#include "brave/common/brave_paths.h"
#include "brave/vendor/bat-native-ledger/src/bat_helper.h"
#include "brave/vendor/bat-native-ledger/src/static_values.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test_utils.h"
#include "google_apis/gaia/mock_url_fetcher_factory.h"
#include "net/url_request/url_fetcher_delegate.h"

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

void split(std::vector<std::string>& tmp, std::string query, char delimiter) {
  std::stringstream ss(query);
  std::string item;
  while (std::getline(ss, item, delimiter)) {
    if (query[0] != '\n') {
      tmp.push_back(item);
    }
  }
}

void BraveURLFetcher::DetermineURLResponsePath(std::string url) {
  std::vector<std::string> tmp;
  brave_net::split(tmp, url, '/');
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
    PREFIX_V1, braveledger_bat_helper::SERVER_TYPES::LEDGER)) == 0) {
    SetResponseString(brave_test_resp::grant_);
  } else if (url.find(braveledger_bat_helper::buildURL(GET_PUBLISHERS_LIST_V1,
    "", braveledger_bat_helper::SERVER_TYPES::PUBLISHER)) == 0) {
    SetResponseString("[]");
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
  }

  void TearDown() override {
    InProcessBrowserTest::TearDown();
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
    GURL rewards_url("chrome://rewards");
    return rewards_url;
  }

  content::WebContents* contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  void EnableRewards() {
    // Load rewards page
    ui_test_utils::NavigateToURL(browser(), rewards_url());
    WaitForLoadStop(contents());
    //opt in and create wallet to enable rewards
    bool result = false;
    ASSERT_TRUE(ExecuteScriptAndExtractBool(contents(),
        "document.querySelector(\"[data-test-id='optInAction']\").click()",
        &result));
    ASSERT_TRUE(result);

    result = false;
    ASSERT_TRUE(ExecuteScriptAndExtractBool(contents(),
      "var count = 10;"
      "var interval = setInterval(function() {"
      "  if (count == 0) {"
      "    clearInterval(interval);"
      "    domAutomationController.send(false);"
      "  } else {"
      "    count -= 1;"
      "  }"
      "  if (document.querySelector(\"[data-test-id2='enableMain']\")) {"
      "    clearInterval(interval);"
      "    domAutomationController.send(true);"
      "  }"
      "}, 500);", &result));
    ASSERT_TRUE(result);
  }

  MockURLFetcherFactory<brave_net::BraveURLFetcher> factory;
};

IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest, RenderWelcome) {
  // Enable Rewards
  EnableRewards();
  EXPECT_STREQ(contents()->GetLastCommittedURL().spec().c_str(),
    rewards_url().spec().c_str());
}

IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest, ToggleRewards) {
  // Enable Rewards
  EnableRewards();

  // Toggle rewards off
  bool result = false;
  ASSERT_TRUE(ExecuteScriptAndExtractBool(contents(),
      "document.querySelector(\"[data-test-id2='enableMain']\").click()",
      &result));
  EXPECT_TRUE(result);

  // Toggle rewards back on
  bool enableResult = false;
  ASSERT_TRUE(ExecuteScriptAndExtractBool(contents(),
      "document.querySelector(\"[data-test-id2='enableMain']\").click()",
      &enableResult));
  EXPECT_TRUE(enableResult);
}

IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest, ToggleAutoContribute) {
  EnableRewards();

  // once rewards has loaded, reload page to activate auto-contribute
  contents()->GetController().Reload(content::ReloadType::NORMAL, true);
  EXPECT_TRUE(WaitForLoadStop(contents()));

  // toggle auto contribute off
  bool result = false;
  ASSERT_TRUE(ExecuteScriptAndExtractBool(contents(),
    "var count = 10;"
    "var interval = setInterval(function() {"
    "  if (count == 0) {"
    "    clearInterval(interval);"
    "    domAutomationController.send(false);"
    "  } else {"
    "    count -= 1;"
    "  }"
    "  if (document.querySelector(\"[data-test-id2='autoContribution']\")) {"
    "    document.querySelector(\"[data-test-id2='autoContribution']\").click();"
    "    clearInterval(interval);"
    "    domAutomationController.send(true);"
    "  }"
    "}, 500);", &result));
  EXPECT_TRUE(result);

  // toggle auto contribute back on
  ASSERT_TRUE(ExecuteScriptAndExtractBool(contents(),
      "document.querySelector(\"[data-test-id2='autoContribution']\").click()",
      &result));
  EXPECT_TRUE(result);
}

IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest, ActivateSettingsModal) {
  EnableRewards();

  bool result = false;
  ASSERT_TRUE(ExecuteScriptAndExtractBool(contents(),
      "document.querySelector(\"[data-test-id='settingsButton']\").click()",
      &result));
  EXPECT_TRUE(result);

  bool modalResult = false;
  ASSERT_TRUE(ExecuteScriptAndExtractBool(contents(),
    "var count = 10;"
    "var interval = setInterval(function() {"
    "  if (count == 0) {"
    "    clearInterval(interval);"
    "    domAutomationController.send(false);"
    "  } else {"
    "    count -= 1;"
    "  }"
    "  if (document.getElementById('modal')) {"
    "    clearInterval(interval);"
    "    domAutomationController.send(true);"
    "  }"
    "}, 500);", &modalResult));
  EXPECT_TRUE(modalResult);
}
