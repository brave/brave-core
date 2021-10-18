/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/files/file_path.h"
#include "base/path_service.h"
#include "base/strings/stringprintf.h"
#include "base/test/bind.h"
#include "base/test/mock_callback.h"
#include "base/test/scoped_feature_list.h"
#include "brave/common/brave_paths.h"
#include "brave/components/translate/core/browser/brave_translate_features.h"
#include "brave/components/translate/core/browser/buildflags.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/translate/chrome_translate_client.h"
#include "chrome/browser/translate/translate_test_utils.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/views/translate/translate_bubble_view.h"
#include "chrome/common/chrome_isolated_world_ids.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/network_session_configurator/common/network_switches.h"
#include "components/translate/core/browser/translate_download_manager.h"
#include "components/translate/core/browser/translate_language_list.h"
#include "components/translate/core/browser/translate_manager.h"
#include "components/translate/core/browser/translate_script.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/test/embedded_test_server/http_response.h"
#include "services/network/public/cpp/network_switches.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "ui/base/l10n/l10n_util.h"
#include "url/gurl.h"

using ::testing::_;
using ::testing::MockFunction;
using ::testing::Return;

namespace translate {

namespace {
const char kTestScript[] = R"(
var google = {};
google.translate = (function() {
  return {
    TranslateService: function() {
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

const char kXhrPromiseTemplate[] = R"(
  new Promise((resolve) => {
    const xhr = new XMLHttpRequest();
    xhr.onload = () => resolve(xhr.%s);
    xhr.onerror = () => resolve(false);
    xhr.open("GET", '%s');
    xhr.send();
  })
)";
}  // namespace

class BraveTranslateBrowserTest : public InProcessBrowserTest {
 public:
  BraveTranslateBrowserTest() {
    scoped_feature_list_.InitAndEnableFeature(features::kUseBraveTranslateGo);

    https_server_.reset(new net::EmbeddedTestServer(
        net::test_server::EmbeddedTestServer::TYPE_HTTPS));
    https_server_->SetSSLConfig(net::EmbeddedTestServer::CERT_OK);

    brave::RegisterPathProvider();
    base::FilePath test_data_dir;
    CHECK(base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir));
    embedded_test_server()->ServeFilesFromDirectory(test_data_dir);
    CHECK(embedded_test_server()->Start());

    https_server_->RegisterRequestHandler(base::BindRepeating(
        &BraveTranslateBrowserTest::HandleRequest, base::Unretained(this)));
    CHECK(https_server_->Start());
  }

  void SetUpOnMainThread() override { ResetObserver(); }

  void TearDownOnMainThread() override {
    language_determined_waiter_.reset();
    InProcessBrowserTest::TearDownOnMainThread();
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    // HTTPS server only serves a valid cert for localhost, so this is needed
    // to load pages from other hosts without an error.
    command_line->AppendSwitch(::switches::kIgnoreCertificateErrors);

    // Remap translate-relay requests to the https test server.
    const std::string host_port = https_server_->host_port_pair().ToString();
    command_line->AppendSwitchASCII(
        network::switches::kHostResolverRules,
        "MAP translate-relay.brave.com:443 " + host_port);
  }

  std::unique_ptr<net::test_server::HttpResponse> HandleRequest(
      const net::test_server::HttpRequest& request) {
    const auto response = backend_request_.Call(request.GetURL().path());

    if (std::get<0>(response) == 0)
      return nullptr;
    std::unique_ptr<net::test_server::BasicHttpResponse> http_response(
        new net::test_server::BasicHttpResponse);
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
    EXPECT_CALL(backend_request_, Call("/translate_a/element.js"))
        .WillOnce(Return(std::make_tuple(net::HttpStatusCode::HTTP_OK,
                                         "text/javascript", kTestScript)));

    EXPECT_CALL(backend_request_,
                Call("/translate_static/css/translateelement.css"))
        .WillRepeatedly(
            Return(std::make_tuple(net::HttpStatusCode::HTTP_OK, "text/css",
                                   "body{background-color:#AAA}")));

    EXPECT_CALL(backend_request_, Call("/translate_static/js/element/main.js"))
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

  std::unique_ptr<net::EmbeddedTestServer> https_server_;
  MockFunction<std::tuple<net::HttpStatusCode, std::string, std::string>(
      std::string)>
      backend_request_;

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
  std::unique_ptr<TranslateWaiter> language_determined_waiter_;
  std::string script_;
};

#if BUILDFLAG(ENABLE_BRAVE_TRANSLATE_GO)
IN_PROC_BROWSER_TEST_F(BraveTranslateBrowserTest, InternalTranslation) {
  ResetObserver();
  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(), embedded_test_server()->GetURL("/espanol_page.html")));
  WaitUntilLanguageDetermined();

  SetupTestScriptExpectations();

  auto* bubble = TranslateBubbleView::GetCurrentBubble();
  ASSERT_TRUE(bubble);

  // Check that the we see the translation bubble (not about the extension
  // installation).
  ASSERT_EQ(bubble->GetWindowTitle(),
            l10n_util::GetStringUTF16(
                IDS_BRAVE_TRANSLATE_BUBBLE_BEFORE_TRANSLATE_TITLE));

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
          "https://translate.googleapis.com/translate_a/t?query=something")));

  // Check that we haven't tried to update the language lists.
  auto* language_list =
      TranslateDownloadManager::GetInstance()->language_list();
  EXPECT_FALSE(language_list->HasOngoingLanguageListLoadingForTesting());

  // Check used urls.
  EXPECT_EQ(language_list->LanguageFetchURLForTesting().host(),
            "translate-relay.brave.com");
  EXPECT_EQ(TranslateScript::GetTranslateScriptURL().host(),
            "translate-relay.brave.com");
}
#endif  // BUILDFLAG(ENABLE_BRAVE_TRANSLATE_GO)

class BraveTranslateBrowserGoogleRedirectTest
    : public BraveTranslateBrowserTest {
 public:
  void SetUpCommandLine(base::CommandLine* command_line) override {
    BraveTranslateBrowserTest::SetUpCommandLine(command_line);
    const std::string host_port = https_server_->host_port_pair().ToString();
    // Add translate.google.com redirection to the https test server.
    command_line->AppendSwitchASCII(
        network::switches::kHostResolverRules,
        "MAP translate-relay.brave.com:443 " + host_port +
            ", MAP translate.google.com:443 " + host_port);
  }
};

#if BUILDFLAG(ENABLE_BRAVE_TRANSLATE_GO)
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
      base::StringPrintf(kXhrPromiseTemplate, "responseURL", kTestURL);

  // Check that a page request is unaffected by the js redirections.
  EXPECT_EQ(kTestURL, content::EvalJs(
                          browser()->tab_strip_model()->GetActiveWebContents(),
                          do_xhr_and_get_final_url));

  // Check that the same page request from translate world will be redirected.
  EXPECT_EQ("https://translate-relay.brave.com/something.svg",
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

  // Check that an request that are not held by redirections will blocked by
  // CSP in translate world.
  EXPECT_EQ(true, content::EvalJs(
                      browser()->tab_strip_model()->GetActiveWebContents(),
                      load_image));
  EXPECT_EQ(false, EvalTranslateJs(load_image));
}
#endif  // BUILDFLAG(ENABLE_BRAVE_TRANSLATE_GO)

class BraveTranslateBrowserDisabledFeatureTest
    : public BraveTranslateBrowserGoogleRedirectTest {
 public:
  BraveTranslateBrowserDisabledFeatureTest() {
    scoped_feature_list_.InitAndDisableFeature(features::kUseBraveTranslateGo);
  }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
};

IN_PROC_BROWSER_TEST_F(BraveTranslateBrowserDisabledFeatureTest,
                       FeatureDisabled) {
  EXPECT_CALL(backend_request_, Call(_)).Times(0);
  ResetObserver();
  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(), embedded_test_server()->GetURL("/espanol_page.html")));
  WaitUntilLanguageDetermined();

  auto* bubble = TranslateBubbleView::GetCurrentBubble();
  ASSERT_TRUE(bubble);

  // The that we see a bubble that suggests Google translate extension
  // installation.
  ASSERT_EQ(bubble->GetWindowTitle(),
            l10n_util::GetStringUTF16(
                IDS_BRAVE_TRANSLATE_BUBBLE_BEFORE_TRANSLATE_INSTALL_TITLE));

  // Check the we don't download the translate scripts
  base::MockCallback<TranslateScript::RequestCallback> mock_callback;
  EXPECT_CALL(mock_callback, Run(false));
  TranslateDownloadManager::GetInstance()->script()->Request(
      mock_callback.Get(), false);

  // The resulting callback must be postted immediately, so simply use
  // RunUtilIdle() to wait for it.
  base::RunLoop().RunUntilIdle();
}

}  // namespace translate
