/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>

#include "base/files/file_path.h"
#include "base/path_service.h"
#include "base/ranges/algorithm.h"
#include "base/strings/stringprintf.h"
#include "base/test/bind.h"
#include "base/test/mock_callback.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/constants/brave_paths.h"
#include "brave/components/constants/brave_services_key.h"
#include "brave/components/l10n/common/localization_util.h"
#include "brave/components/translate/core/common/brave_translate_features.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/translate/chrome_translate_client.h"
#include "chrome/browser/translate/translate_test_utils.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/views/translate/translate_bubble_controller.h"
#include "chrome/browser/ui/views/translate/translate_bubble_view.h"
#include "chrome/common/chrome_isolated_world_ids.h"
#include "chrome/grit/generated_resources.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/infobars/content/content_infobar_manager.h"
#include "components/infobars/core/infobar.h"
#include "components/translate/core/browser/translate_download_manager.h"
#include "components/translate/core/browser/translate_language_list.h"
#include "components/translate/core/browser/translate_manager.h"
#include "components/translate/core/browser/translate_script.h"
#include "components/translate/core/common/translate_util.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/test/embedded_test_server/http_response.h"
#include "services/network/public/cpp/network_switches.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

using ::testing::_;
using ::testing::MockFunction;
using ::testing::Return;

namespace translate {

namespace {
constexpr char kTestScript[] = R"(
var api_key = undefined;
var google = {};
google.translate = (function() {
  return {
    TranslateService: function(params) {
      api_key = params.key;
      return {
        isAvailable : function() {
          return true;
        },
        restore : function() {
          return;
        },
        getDetectedLanguage : function() {
          return "fr";
        },
        translatePage : function(sourceLang, targetLang,
                                 onTranslateProgress) {
          onTranslateProgress(100, true, false);
        }
      };
    }
  };
})();
cr.googleTranslate.onLoadCSS("https://translate.googleapis.com/translate_static/css/translateelement.css");

// Will call cr.googleTranslate.onTranslateElementLoad():
cr.googleTranslate.onLoadJavascript("https://translate.googleapis.com/translate_static/js/element/main.js");
)";

constexpr char kXhrPromiseTemplate[] = R"(
  new Promise((resolve) => {
    const xhr = new XMLHttpRequest();
    xhr.onload = () => resolve(xhr.%s);
    xhr.onerror = () => resolve(false);
    let url = '%s';
    if (%s && typeof api_key !== 'undefined') /* see kTestScript*/
      url += '&key=' + api_key;
    xhr.open('GET', url);
    xhr.send();
  })
)";
}  // namespace

class BraveTranslateBrowserTest : public InProcessBrowserTest {
 public:
  BraveTranslateBrowserTest()
      : https_server_(net::test_server::EmbeddedTestServer::TYPE_HTTPS) {}

  void SetUp() override {
    ASSERT_TRUE(https_server_.InitializeAndListen());
    InProcessBrowserTest::SetUp();
  }

  void SetUpOnMainThread() override {
    base::FilePath test_data_dir;
    CHECK(base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir));
    embedded_test_server()->ServeFilesFromDirectory(test_data_dir);
    CHECK(embedded_test_server()->Start());
    https_server_.RegisterRequestHandler(base::BindRepeating(
        &BraveTranslateBrowserTest::HandleRequest, base::Unretained(this)));
    https_server_.StartAcceptingConnections();

    mock_cert_verifier_.mock_cert_verifier()->set_default_result(net::OK);
    ResetObserver();
  }

  void TearDownOnMainThread() override {
    language_determined_waiter_.reset();
    InProcessBrowserTest::TearDownOnMainThread();
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    InProcessBrowserTest::SetUpCommandLine(command_line);
    mock_cert_verifier_.SetUpCommandLine(command_line);

    // Remap translate.brave.com requests to the https test server.
    const std::string host_port = https_server_.host_port_pair().ToString();
    command_line->AppendSwitchASCII(network::switches::kHostResolverRules,
                                    "MAP translate.brave.com:443 " + host_port +
                                        ", MAP translate.google.com:443 " +
                                        host_port);
  }

  void SetUpInProcessBrowserTestFixture() override {
    InProcessBrowserTest::SetUpInProcessBrowserTestFixture();
    mock_cert_verifier_.SetUpInProcessBrowserTestFixture();
  }

  void TearDownInProcessBrowserTestFixture() override {
    mock_cert_verifier_.TearDownInProcessBrowserTestFixture();
    InProcessBrowserTest::TearDownInProcessBrowserTestFixture();
  }

  std::unique_ptr<net::test_server::HttpResponse> HandleRequest(
      const net::test_server::HttpRequest& request) {
    std::unique_ptr<net::test_server::BasicHttpResponse> http_response(
        new net::test_server::BasicHttpResponse);

    // We don't support CORS preflights on the backend.
    EXPECT_NE(request.method, net::test_server::METHOD_OPTIONS);

    EXPECT_FALSE(request.GetURL().DomainIs("translate.googleapis.com"))
        << "Found a request to google backend " << request.GetURL();

    if (request.GetURL().path() == "/translate") {
      const auto query = request.GetURL().query();
      EXPECT_NE(query.find(base::StringPrintf("&key=%s",
                                              BUILDFLAG(BRAVE_SERVICES_KEY))),
                std::string::npos)
          << "bad brave api key for request " << request.GetURL();
    }

    const auto response = backend_request_.Call(request.GetURL().path());
    if (std::get<0>(response) == 0)
      return nullptr;

    http_response->set_code(std::get<0>(response));
    http_response->set_content_type(std::get<1>(response));
    http_response->set_content(std::get<2>(response));
    http_response->AddCustomHeader("Access-Control-Allow-Origin", "*");
    return std::move(http_response);
  }

  ChromeTranslateClient* GetChromeTranslateClient() {
    return ChromeTranslateClient::FromWebContents(
        browser()->tab_strip_model()->GetActiveWebContents());
  }

  TranslateManager* GetTranslateManager() {
    return GetChromeTranslateClient()->GetTranslateManager();
  }

  void ResetObserver() {
    language_determined_waiter_ = translate::CreateTranslateWaiter(
        browser()->tab_strip_model()->GetActiveWebContents(),
        TranslateWaiter::WaitEvent::kLanguageDetermined);
  }

  // Set up expectations for the secondary test scripts and for the test css.
  void SetupTestScriptExpectations() {
    EXPECT_CALL(backend_request_, Call("/static/v1/element.js"))
        .WillOnce(Return(std::make_tuple(net::HttpStatusCode::HTTP_OK,
                                         "text/javascript", kTestScript)));

    EXPECT_CALL(backend_request_, Call("/static/v1/css/translateelement.css"))
        .WillRepeatedly(
            Return(std::make_tuple(net::HttpStatusCode::HTTP_OK, "text/css",
                                   "body{background-color:#AAA}")));

    EXPECT_CALL(backend_request_, Call("/static/v1/js/element/main.js"))
        .WillOnce(Return(
            std::make_tuple(net::HttpStatusCode::HTTP_OK, "text/javascript",
                            "cr.googleTranslate.onTranslateElementLoad()")));
  }

  void WaitUntilLanguageDetermined() { language_determined_waiter_->Wait(); }

  void WaitUntilPageTranslated() {
    translate::CreateTranslateWaiter(
        browser()->tab_strip_model()->GetActiveWebContents(),
        TranslateWaiter::WaitEvent::kPageTranslated)
        ->Wait();
  }

  content::EvalJsResult EvalTranslateJs(const std::string& script) {
    return content::EvalJs(
        browser()->tab_strip_model()->GetActiveWebContents(), script.c_str(),
        content::EXECUTE_SCRIPT_DEFAULT_OPTIONS, ISOLATED_WORLD_ID_TRANSLATE);
  }

  ::testing::AssertionResult HasNoBadFlagsInfobar() {
    auto* infobar_manager = infobars::ContentInfoBarManager::FromWebContents(
        browser()->tab_strip_model()->GetActiveWebContents());
    if (!infobar_manager)
      return ::testing::AssertionFailure() << "!infobar_manager";

    const auto it = base::ranges::find(
        infobar_manager->infobars(),
        infobars::InfoBarDelegate::BAD_FLAGS_INFOBAR_DELEGATE,
        &infobars::InfoBar::GetIdentifier);
    if (it != infobar_manager->infobars().cend()) {
      return ::testing::AssertionFailure() << "Bad flags infobar found.";
    }

    return ::testing::AssertionSuccess();
  }

  net::EmbeddedTestServer https_server_;
  MockFunction<std::tuple<net::HttpStatusCode, std::string, std::string>(
      std::string)>
      backend_request_;

 private:
  content::ContentMockCertVerifier mock_cert_verifier_;
  std::unique_ptr<TranslateWaiter> language_determined_waiter_;
  std::string script_;
};

IN_PROC_BROWSER_TEST_F(BraveTranslateBrowserTest, InternalTranslation) {
  ResetObserver();
  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(), embedded_test_server()->GetURL("/espanol_page.html")));
  WaitUntilLanguageDetermined();

  SetupTestScriptExpectations();

  auto* bubble = TranslateBubbleController::FromWebContents(
                     browser()->tab_strip_model()->GetActiveWebContents())
                     ->GetTranslateBubble();
  ASSERT_TRUE(bubble);

  // Check that the we see the translation bubble (not about the extension
  // installation).
  ASSERT_EQ(bubble->GetWindowTitle(),
            brave_l10n::GetLocalizedResourceUTF16String(
                IDS_TRANSLATE_BUBBLE_BEFORE_TRANSLATE_TITLE));

  // Translate the page. Note: the event onTranslateElementLoad() is
  // called from main.js (see SetupTestScriptExpectations()).
  GetTranslateManager()->TranslatePage("es", "en", true);
  WaitUntilPageTranslated();

  // Check that the test css styles were loaded and works.
  EXPECT_EQ("rgb(170, 170, 170)",
            EvalTranslateJs("getComputedStyle(document.body).getPropertyValue('"
                            "background-color')"));

  // Simulate a translate request to googleapis.com and check that the
  // redirections works well.
  EXPECT_CALL(backend_request_, Call("/translate_a/t"))
      .WillOnce(Return(std::make_tuple(net::HttpStatusCode::HTTP_OK,
                                       "application/json", "[\"This\"]")));
  EXPECT_EQ(
      "[\"This\"]",
      EvalTranslateJs(base::StringPrintf(
          kXhrPromiseTemplate, "response",
          "https://translate.googleapis.com/translate_a/t?query=something",
          "true")));

  // Check that we haven't tried to update the language lists.
  auto* language_list =
      TranslateDownloadManager::GetInstance()->language_list();
  language_list->RequestLanguageList();
  EXPECT_FALSE(language_list->HasOngoingLanguageListLoadingForTesting());

  // Check used urls.
  EXPECT_EQ(language_list->LanguageFetchURLForTesting().host(),
            "translate.brave.com");
  EXPECT_EQ(TranslateScript::GetTranslateScriptURL().host(),
            "translate.brave.com");

  // Check no bad flags infobar is shown (about the different translate
  // script/origin).
  EXPECT_TRUE(HasNoBadFlagsInfobar());

  // Chromium language list should be used by default.
  EXPECT_TRUE(TranslateDownloadManager::IsSupportedLanguage("ar"));
  EXPECT_TRUE(TranslateDownloadManager::IsSupportedLanguage("vi"));
}

IN_PROC_BROWSER_TEST_F(BraveTranslateBrowserTest, NoAutoTranslate) {
  // Set auto translate from es to en.
  GetChromeTranslateClient()
      ->GetTranslatePrefs()
      ->AddLanguagePairToAlwaysTranslateList("es", "en");
  ResetObserver();
  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(), embedded_test_server()->GetURL("/espanol_page.html")));
  WaitUntilLanguageDetermined();

  auto* bubble = TranslateBubbleController::FromWebContents(
                     browser()->tab_strip_model()->GetActiveWebContents())
                     ->GetTranslateBubble();
  ASSERT_TRUE(bubble);

  // Check that the we see BEFORE translation bubble (not in-progress bubble).
  ASSERT_EQ(bubble->GetWindowTitle(),
            brave_l10n::GetLocalizedResourceUTF16String(
                IDS_TRANSLATE_BUBBLE_BEFORE_TRANSLATE_TITLE));
}

class BraveTranslateBrowserGoogleRedirectTest
    : public BraveTranslateBrowserTest {
 public:
  void SetUpCommandLine(base::CommandLine* command_line) override {
    BraveTranslateBrowserTest::SetUpCommandLine(command_line);
    const std::string host_port = https_server_.host_port_pair().ToString();
    // Add translate.google.com redirection to the https test server.
    command_line->AppendSwitchASCII(network::switches::kHostResolverRules,
                                    "MAP translate.brave.com:443 " + host_port +
                                        ", MAP translate.google.com:443 " +
                                        host_port);
  }
};

IN_PROC_BROWSER_TEST_F(BraveTranslateBrowserGoogleRedirectTest,
                       JsRedirectionsSelectivity) {
  ResetObserver();

  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(), embedded_test_server()->GetURL("/espanol_page.html")));
  WaitUntilLanguageDetermined();

  SetupTestScriptExpectations();
  GetTranslateManager()->TranslatePage("es", "en", true);
  WaitUntilPageTranslated();

  const char kTestURL[] = "https://translate.google.com/something.svg";
  const char kTestSvg[] = R"(
    <svg xmlns="http://www.w3.org/2000/svg" width="300" height="300"></svg>
  )";

  EXPECT_CALL(backend_request_, Call("/something.svg"))
      .WillRepeatedly(Return(std::make_tuple(net::HttpStatusCode::HTTP_OK,
                                             "image/svg+xml", kTestSvg)));

  const auto do_xhr_and_get_final_url =
      base::StringPrintf(kXhrPromiseTemplate, "responseURL", kTestURL, "false");

  // Check that a page request is unaffected by the js redirections.
  EXPECT_EQ(kTestURL, content::EvalJs(
                          browser()->tab_strip_model()->GetActiveWebContents(),
                          do_xhr_and_get_final_url));

  // Check that the same page request from translate world will be redirected.
  EXPECT_EQ("https://translate.brave.com/something.svg",
            EvalTranslateJs(do_xhr_and_get_final_url));

  const char kLoadImageTemplate[] = R"(
    new Promise((resolve) => {
      let p = new Image();
      p.onload = () => resolve(true);
      p.onerror = () => resolve(false);
      p.src = '%s';
    });
  )";

  const auto load_image = base::StringPrintf(kLoadImageTemplate, kTestURL);

  // Check that the image is loaded in the main world correctly.
  EXPECT_EQ(true, content::EvalJs(
                      browser()->tab_strip_model()->GetActiveWebContents(),
                      load_image));
  ::testing::Mock::VerifyAndClearExpectations(&backend_request_);

  // Check that an image request will blocked by CSP in translate world (because
  // that are not held by js redirections).
  EXPECT_CALL(backend_request_, Call(_)).Times(0);
  EXPECT_EQ(false, EvalTranslateJs(load_image));
}

}  // namespace translate
