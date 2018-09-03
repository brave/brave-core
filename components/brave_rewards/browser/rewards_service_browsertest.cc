/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/path_service.h"
#include "brave/browser/brave_content_browser_client.h"
#include "brave/common/brave_paths.h"
#include "brave/components/brave_shields/common/brave_shield_constants.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/common/chrome_content_client.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/startup/startup_browser_creator.h"
#include "chrome/browser/ui/startup/startup_browser_creator_impl.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_navigation_observer.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/test/test_navigation_observer.h"
#include "chrome/common/webui_url_constants.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "net/dns/mock_host_resolver.h"
#include "google_apis/gaia/mock_url_fetcher_factory.h"
#include "net/url_request/url_fetcher_delegate.h"

namespace brave_test_resp {
  const std::string test_server_ = 
    "https://ledger-staging.mercury.basicattentiontoken.org/";
  const std::string balance_server_ =
    "https://balance-staging.mercury.basicattentiontoken.org/";
  const std::string registrarVK_ = 
    "{\"payload\":{\"adFree\":{\"currency\":\"BAT\","
    "\"fee\":{\"BAT\":20},\"choices\":{\"BAT\":[10,15,20,30,50,100]},\"range\""
    ":{\"BAT\":[10,100]},\"days\":30}},\"registrarVK\":\"==========ANONLOGIN_V"
    "K_BEG==========\\nx3EmZXFb2jD7OocZz6l7o638S45k2kKrX5BrWp1Ox+ ANJXSauJVPNH"
    "Kl/mmakFwxbkwJkJzfXe+c9+jxFtuX6 1\\n5xMuqWE8J7HIHbW/UEJwFELYjTWRF10x7LMd7"
    "s46MVT 4DC6LqGM8zLz1pCsHA3qab48gkpeiQpNZAdb9owFvU6 1\\n1tADCD6LdrEDQQDhRy"
    "1ijjAfhV9uKwlKKuhD6xXyPAZ BLOzUy+ZZh48riPnAHnUGal+ceCclccZXmoXXx92WHW 1"
    "\\n7VRDst4U4iaT/9QNCwajEaqgRNtKPV1Dp5QuMjP019h HucbxBMGGAZBNLXGzfKsTlF+wA"
    "dmOFAvBRAo8i2Azd 1\\n5SLk8SphICEkF+CNhN5g7IX2ih+Tb6w14LLlwupKw7y 96ANsdHz"
    "g0pwo2DDWOyAh1YPnION196pIT9xwISFZTA 3ef5G5d2c8cctdK4LuaxlSeEf1OZ100Sy5un5"
    "EjuHJB 77yFnY61GM7PHd6q3TLs2QS6c9PfrXD2idxaFq2DMd 1 0\\n===========ANONLO"
    "GIN_VK_END==========\"}";

  const std::string verification_ = 
    "{\"wallet\":{\"paymentId\":\"50e7b589-ea9b-48ac"
    "-9f74-d7eb85c9ff51\",\"addresses\":{\"BAT\":\"0x8A32f8560df7A5167E29ADc92"
    "0b5F01D135e1a01\",\"BTC\":\"mr5xMnn8TwdDQp7ijSK4F3sGbteS3STxbh\",\"CARD_I"
    "D\":\"3203f496-ef4e-4b5b-ace1-f914c41a241e\",\"ETH\":\"0x8A32f8560df7A516"
    "7E29ADc920b5F01D135e1a01\",\"LTC\":\"mkRxRoQPdoVm3CZqSPYPVqcQrzNfH2n54z\""
    "}},\"payload\":{\"adFree\":{\"currency\":\"BAT\",\"fee\":{\"BAT\":20},\"c"
    "hoices\":{\"BAT\":[10,15,20,30,50,100]},\"range\":{\"BAT\":[10,100]},\"da"
    "ys\":30}},\"verification\":\"9dTh21VGjNssqaruZb3wcp2ThFSq2Ft0mWf6C+f49g X"
    "LXrRC8OJo+SHNqB2lLnIFwqhdTQk9ixenOVJdpWwd 1 ATuhjeN/AQvYjrs5WaataUUa0dj68"
    "Unx3P32NOKB6gC\\n\"}";

  const std::string wallet_ = "{\"altcurrency\":\"BAT\",\"probi\":\"0\",\"bala"
    "nce\":\"0.0000\",\"unconfirmed\":\"0.0000\",\"rates\":{\"BTC\":0.00003105"
    ",\"ETH\":0.0007520713830465265,\"XRP\":0.6385015608740894,\"BCH\":0.00039"
    "8527449465635,\"LTC\":0.003563298490127758,\"DASH\":0.0011736801836266257"
    ",\"BTG\":0.009819171067370777,\"USD\":0.214100307359946,\"EUR\":0.1835721"
    "7273398782},\"parameters\":{\"adFree\":{\"currency\":\"BAT\",\"fee\":{\"B"
    "AT\":20},\"choices\":{\"BAT\":[10,15,20,30,50,100]},\"range\":{\"BAT\":[1"
    "0,100]},\"days\":30}}}";

  const std::string grant_ = "{\"promotionId\":\"578de27e-174d-4fbe-bdf6-2e70b"
    "2b0ac86\",\"minimumReconcileTimestamp\":1525800234000,\"protocolVersion\""
    ":1,\"stateWallet\":{\"disabledWallet\":{\"notification\":{\"options\":{\""
    "persist\":false,\"style\":\"greetingStyle\"},\"messageAction\":\"optInPro"
    "motion\",\"greeting\":\"Hello!\",\"message\":\"Ready to support your favo"
    "rite sites? Brave will fill your wallet with tokens to get you started!\""
    ",\"buttons\":[{\"buttonActionId\":\"remindLater\",\"text\":\"Maybe later"
    "\"},{\"className\":\"primaryButton\",\"buttonActionId\":\"optInPromotion"
    "\",\"text\":\"I'm ready\"}]},\"panel\":{\"optedInButton\":\"Claim my free"
    " tokens\",\"optInMarkup\":{\"title\":\"Brave has created a simple wayfor "
    "you to contribute to the sites you use most.\",\"message\":[\"Now, for a "
    "limited time, Brave will fund yourwallet with tokens to get you started!"
    "\",\"To start using Brave Payments and accept your tokens, simply flip th"
    "e switch at the top of this window and then choose \\\"Claim my free Toke"
    "ns\\\".\",\"The rest is easy.\"]},\"disclaimer\":\"If these tokens are no"
    "t used within 90 days to support content creators, they will automaticall"
    "y return to the Brave User Growth Pool.\"}},\"emptyWallet\":{\"notificati"
    "on\":{\"options\":{\"persist\":false,\"style\":\"greetingStyle\"},\"messa"
    "geAction\":\"optInPromotion\",\"greeting\":\"Hello!\",\"message\":\"Brave"
    " is offering you free tokens to get you started with Brave Payments!\","
    "\"buttons\":[{\"buttonActionId\":\"remindLater\",\"text\":\"Maybe later\""
    "},{\"className\":\"primaryButton\",\"buttonActionId\":\"optInPromotion\","
    "\"text\":\"Claim my tokens...\"}]},\"panel\":{\"optedInButton\":\"Claim m"
    "y free tokens\",\"successText\":{\"title\":\"Bravo!\",\"message\":\"It's "
    "your lucky day. Your token grant is on its way.\"},\"disclaimer\":\"Your "
    "new tokens will appear in your account shortly. If these tokens are not u"
    "sed within 90 days to support content creators, they will automatically r"
    "eturn to the Brave User Growth Pool.\"}},\"fundedWallet\":{\"notification"
    "\":{\"options\":{\"persist\":false,\"style\":\"greetingStyle\"},\"message"
    "Action\":\"optInPromotion\",\"greeting\":\"Hello!\",\"message\":\"Thank y"
    "ou for being a great contributor! Here are someBAT tokens, on us!\",\"but"
    "tons\":[{\"buttonActionId\":\"remindLater\",\"text\":\"Maybe later\"},{\""
    "className\":\"primaryButton\",\"buttonActionId\":\"optInPromotion\",\"tex"
    "t\":\"Claim my tokens...\"}]},\"panel\":{\"optedInButton\":\"Claim my fre"
    "e tokens\",\"successText\":{\"greeting\":\"Bravo!\",\"message\":\"It's yo"
    "ur lucky day. Your token grant is on its way.\"},\"disclaimer\":\"Your ne"
    "w tokens will appear in your account shortly. If these tokens are not use"
    "d within 90 days to support content creators, they will automatically ret"
    "urn to the Brave User Growth Pool.\"}}}}";
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
  if (url.find(brave_test_resp::test_server_ + "v2/registrar/persona") == 0
    && tmp.size() == 6) {
    SetResponseString(brave_test_resp::registrarVK_);
  } else if (url.find(brave_test_resp::test_server_ + 
    "v2/registrar/persona") == 0
    && tmp.size() == 7) {
    SetResponseString(brave_test_resp::verification_);
  } else if (url.find(brave_test_resp::balance_server_ + "v2/wallet") == 0) {
    SetResponseString(brave_test_resp::wallet_);
  } else if (url.find(brave_test_resp::test_server_ + "v1/grant") == 0) {
    SetResponseString(brave_test_resp::grant_);
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
  }

  void TearDown() override {
    InProcessBrowserTest::TearDown();
  }

  GURL rewards_url() {
    GURL rewards_url("chrome://rewards");
    return rewards_url;
  }

  content::WebContents* contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  bool EnableRewards() {
    // Load rewards page
    ui_test_utils::NavigateToURL(browser(), rewards_url());
    WaitForLoadStop(contents());
    // inject fake URLFetcher objects.
    MockURLFetcherFactory<brave_net::BraveURLFetcher> factory;
    //opt in and create wallet to enable rewards
    bool result;
    return ExecuteScriptAndExtractBool(contents(), 
      "document.querySelector(\"[data-test-id='optInAction']\").click();"
      "var interval = setInterval(function() {"
      "if (document.querySelector(\"[data-test-id2='enableMain']\") != null) {"
        "clearInterval(interval);"
        "domAutomationController.send(true);"
      "}"
    "}, 1000);", &result);
  }
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
  bool result;
  ASSERT_TRUE(ExecuteScriptAndExtractBool(contents(),
    "document.querySelector(\"[data-test-id2='enableMain']\").click();"
    "domAutomationController.send(true);", &result));
  EXPECT_TRUE(result);

  // Toggle rewards back on
  bool enableResult;
  ASSERT_TRUE(ExecuteScriptAndExtractBool(contents(),
    "document.querySelector(\"[data-test-id2='enableMain']\").click();"
    "domAutomationController.send(true);", &enableResult));
  EXPECT_TRUE(enableResult);
}

IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest, ToggleAutoContribute) {
  EnableRewards();

  // once rewards has loaded, reload page to activate auto-contribute
  contents()->GetController().Reload(content::ReloadType::NORMAL, true);
  EXPECT_TRUE(WaitForLoadStop(contents()));

  // toggle auto contribute off
  bool result;
  ASSERT_TRUE(ExecuteScriptAndExtractBool(contents(),
    "var interval = setInterval(function() {"
      "if (document.querySelector(\"[data-test-id2='autoContribution']\")"
        " != null) {"
        "document.querySelector(\"[data-test-id2='autoContribution']\").click();"
        "clearInterval(interval);"
        "domAutomationController.send(true);"
      "}"
    "}, 1000);", &result));
  EXPECT_TRUE(result);

  // toggle auto contribute back on
  ASSERT_TRUE(ExecuteScriptAndExtractBool(contents(),
    "document.querySelector(\"[data-test-id2='autoContribution']\").click();"
    "domAutomationController.send(true);", &result));
  EXPECT_TRUE(result);
}

IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest, ActivateSettingsModal) {
  EnableRewards();

  bool result;  
  ASSERT_TRUE(ExecuteScriptAndExtractBool(contents(),
    "document.querySelector(\"[data-test-id='settingsButton']\").click();"
    "domAutomationController.send(true);", &result));
  EXPECT_TRUE(result);

  bool modalResult;
  ASSERT_TRUE(ExecuteScriptAndExtractBool(contents(),
    "var interval = setInterval(function() {"
      "if (document.getElementById('modal') != null) {"
        "clearInterval(interval);"
        "domAutomationController.send(true);"
      "}"
    "}, 1000);", &modalResult));
  EXPECT_TRUE(modalResult);
}
