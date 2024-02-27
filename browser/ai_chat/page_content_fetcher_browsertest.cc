/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/files/file_path.h"
#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/memory/weak_ptr.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/strings/string_util.h"
#include "base/test/bind.h"
#include "brave/components/ai_chat/content/browser/ai_chat_tab_helper.h"
#include "brave/components/ai_chat/content/browser/page_content_fetcher.h"
#include "brave/components/constants/brave_paths.h"
#include "brave/components/l10n/common/test/scoped_default_locale.h"
#include "brave/components/text_recognition/common/buildflags/buildflags.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "net/dns/mock_host_resolver.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "ui/compositor/compositor_switches.h"

namespace {

constexpr char kEmbeddedTestServerDirectory[] = "leo";
constexpr char kGithubPatch[] = R"(diff --git a/file.cc b/file.cc
index 9e2e7d6ef96..4cdf7cc8ac8 100644
--- a/file.cc
+++ b/file.cc
@@ -7,6 +7,7 @@
 #include "file3.h"
 #include "file4.h"
+ 
+int main() {
+    std::cout << "This is the way" << std::endl;
+    return 0;
+})";
constexpr char kGithubUrlPath[] = "/brave/din_djarin/pull/1";
constexpr char kGithubUrlPathPatch[] = "/brave/din_djarin/pull/1.patch";
}  // namespace

class PageContentFetcherBrowserTest : public InProcessBrowserTest {
 public:
  PageContentFetcherBrowserTest()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {}

  void SetUpOnMainThread() override {
    mock_cert_verifier_.mock_cert_verifier()->set_default_result(net::OK);
    host_resolver()->AddRule("*", "127.0.0.1");
    content::SetupCrossSiteRedirector(&https_server_);

    brave::RegisterPathProvider();
    base::FilePath test_data_dir;
    test_data_dir = base::PathService::CheckedGet(brave::DIR_TEST_DATA);
    test_data_dir = test_data_dir.AppendASCII(kEmbeddedTestServerDirectory);
    https_server_.ServeFilesFromDirectory(test_data_dir);
    shared_url_loader_factory_ =
        base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
            &url_loader_factory_);
    // This handles the request for the pull request URL, but not the patch
    // file, the patch file must be handled via an interceptor
    https_server_.RegisterRequestHandler(
        base::BindRepeating(&PageContentFetcherBrowserTest::HandleGithubUrl,
                            base::Unretained(this)));
    ASSERT_TRUE(https_server_.Start());
    SetGithubInterceptor();
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    InProcessBrowserTest::SetUpCommandLine(command_line);
#if BUILDFLAG(ENABLE_TEXT_RECOGNITION)
    command_line->AppendSwitch(::switches::kEnablePixelOutputInTests);
#endif
    mock_cert_verifier_.SetUpCommandLine(command_line);
  }

  void SetUpInProcessBrowserTestFixture() override {
    InProcessBrowserTest::SetUpInProcessBrowserTestFixture();
    mock_cert_verifier_.SetUpInProcessBrowserTestFixture();
  }

  void TearDownInProcessBrowserTestFixture() override {
    mock_cert_verifier_.TearDownInProcessBrowserTestFixture();
    InProcessBrowserTest::TearDownInProcessBrowserTestFixture();
  }

  content::WebContents* GetActiveWebContents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  void NavigateURL(const GURL& url) {
    ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
    ASSERT_TRUE(WaitForLoadStop(GetActiveWebContents()));
  }

  void FetchPageContent(const base::Location& location,
                        const std::string& expected_text,
                        bool expected_is_video,
                        bool trim_whitespace = true) {
    SCOPED_TRACE(testing::Message() << location.ToString());
    base::RunLoop run_loop;
    ai_chat::FetchPageContent(
        browser()->tab_strip_model()->GetActiveWebContents(), "",
        base::BindLambdaForTesting([&run_loop, &expected_text,
                                    &expected_is_video, &trim_whitespace](
                                       std::string text, bool is_video,
                                       std::string invalidation_token) {
          EXPECT_EQ(expected_text,
                    trim_whitespace ? base::TrimWhitespaceASCII(
                                          text, base::TrimPositions::TRIM_ALL)
                                    : text);
          EXPECT_EQ(expected_is_video, is_video);
          run_loop.Quit();
        }),
        shared_url_loader_factory_);
    run_loop.Run();
  }

  // Handles returning a .patch file if the user is on a github.com pull request
  void SetGithubInterceptor() {
    GURL expected_patch_url =
        https_server_.GetURL("github.com", kGithubUrlPathPatch);
    url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [=](const network::ResourceRequest& request) {
          if (request.url == expected_patch_url) {
            url_loader_factory_.ClearResponses();
            url_loader_factory_.AddResponse(request.url.spec(), kGithubPatch);
          }
        }));
  }

  // Handles returning a 200 OK for the pull request URL to the test server
  std::unique_ptr<net::test_server::HttpResponse> HandleGithubUrl(
      const net::test_server::HttpRequest& request) {
    if (request.relative_url == kGithubUrlPath) {
      auto response = std::make_unique<net::test_server::BasicHttpResponse>();
      response->set_code(net::HTTP_OK);
      return response;
    }
    return nullptr;
  }

 protected:
  net::test_server::EmbeddedTestServer https_server_;

 private:
  content::ContentMockCertVerifier mock_cert_verifier_;
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
};

IN_PROC_BROWSER_TEST_F(PageContentFetcherBrowserTest, FetchPageContent) {
  // Simple page with text
  NavigateURL(https_server_.GetURL("a.com", "/text.html"));
  FetchPageContent(FROM_HERE, "I have spoken", false);
  // Main element
  NavigateURL(https_server_.GetURL("a.com", "/text_with_main.html"));
  FetchPageContent(FROM_HERE, "Only this text", false);
  // Main element with ignored
  NavigateURL(https_server_.GetURL("a.com", "/text_with_main.html"));
  FetchPageContent(FROM_HERE, "Only this text", false);
  // Not a page extraction host and page with no text
  NavigateURL(https_server_.GetURL("a.com", "/canvas.html"));
  FetchPageContent(FROM_HERE, "", false);
#if BUILDFLAG(ENABLE_TEXT_RECOGNITION)
  // Page recognition host with a canvas element
  NavigateURL(https_server_.GetURL("docs.google.com", "/canvas.html"));
  FetchPageContent(FROM_HERE, "this is the way", false);
#if BUILDFLAG(IS_WIN)
  // Unsupported locale should return no content for Windows only
  // Other platforms do not use locale for extraction
  const brave_l10n::test::ScopedDefaultLocale locale("xx_XX");
  NavigateURL(https_server_.GetURL("docs.google.com", "/canvas.html"));
  FetchPageContent(FROM_HERE, "", false);
#endif  // #if BUILDFLAG(IS_WIN)
  NavigateURL(https_server_.GetURL("github.com", kGithubUrlPath));
  FetchPageContent(FROM_HERE, kGithubPatch, false);
#endif  // #if BUILDFLAG(ENABLE_TEXT_RECOGNITION)
}

IN_PROC_BROWSER_TEST_F(PageContentFetcherBrowserTest, FetchPageContentPDF) {
  auto* chat_tab_helper =
      ai_chat::AIChatTabHelper::FromWebContents(GetActiveWebContents());
  ASSERT_TRUE(chat_tab_helper);
  chat_tab_helper->SetUserOptedIn(true);
  auto run_loop = std::make_unique<base::RunLoop>();
  chat_tab_helper->SetOnPDFA11yInfoLoadedCallbackForTesting(
      base::BindLambdaForTesting([this, &run_loop]() {
        FetchPageContent(FROM_HERE, "This is the way\nI have spoken", false,
                         false);
        run_loop->Quit();
      }));
  NavigateURL(https_server_.GetURL("a.com", "/dummy.pdf"));
  run_loop->Run();

  run_loop = std::make_unique<base::RunLoop>();
  chat_tab_helper->SetOnPDFA11yInfoLoadedCallbackForTesting(
      base::BindLambdaForTesting([this, &run_loop]() {
        FetchPageContent(FROM_HERE, "", false, false);
        run_loop->Quit();
      }));
  NavigateURL(https_server_.GetURL("a.com", "/empty_pdf.pdf"));
  run_loop->Run();
}
