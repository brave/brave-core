/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>

#include "base/path_service.h"
#include "base/strings/string_util.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/brave_browser_features.h"
#include "brave/browser/url_sanitizer/url_sanitizer_service_factory.h"
#include "brave/components/constants/brave_paths.h"
#include "brave/components/url_sanitizer/browser/url_sanitizer_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "services/network/public/cpp/network_switches.h"
#include "ui/base/clipboard/clipboard.h"
#include "ui/base/clipboard/scoped_clipboard_writer.h"

namespace {

constexpr char kYoutubeRules[] = R"json(
    [{
        "include": [
            "*://youtu.be/*",
            "*://*.youtube.com/watch?*"
        ],
        "exclude": [ ],
        "params": [
            "app",
            "embeds_euri",
            "embeds_loader_url_for_pings",
            "embeds_origin",
            "feature",
            "pp",
            "si",
            "source_ve_path"
        ]
    }]
  )json";

constexpr char kYoutubePermissions[] = R"json(
    {
      "js_api": [ "*://*.youtube.com/*" ]
    }
  )json";

}  // namespace

class URLSanitizerTestBase : public InProcessBrowserTest {
 public:
  explicit URLSanitizerTestBase(bool enable_feature) {
    if (enable_feature) {
      feature_list_.InitAndEnableFeature(features::kBraveCopyCleanLinkFromJs);
    } else {
      feature_list_.InitAndDisableFeature(features::kBraveCopyCleanLinkFromJs);
    }
  }

  void SetUpOnMainThread() override {
    https_server_.ServeFilesFromDirectory(
        base::PathService::CheckedGet(brave::DIR_TEST_DATA));
    https_server_.StartAcceptingConnections();
    host_resolver()->AddRule("*", "127.0.0.1");
    mock_cert_verifier_.mock_cert_verifier()->set_default_result(net::OK);
  }

  void SetUp() override {
    ASSERT_TRUE(https_server_.InitializeAndListen());
    InProcessBrowserTest::SetUp();
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    InProcessBrowserTest::SetUpCommandLine(command_line);
    mock_cert_verifier_.SetUpCommandLine(command_line);
    command_line->AppendSwitchASCII(
        network::switches::kHostResolverRules,
        "MAP * " + https_server_.host_port_pair().ToString());
  }

  void SetUpInProcessBrowserTestFixture() override {
    InProcessBrowserTest::SetUpInProcessBrowserTestFixture();
    mock_cert_verifier_.SetUpInProcessBrowserTestFixture();
  }

  void TearDownInProcessBrowserTestFixture() override {
    mock_cert_verifier_.TearDownInProcessBrowserTestFixture();
    InProcessBrowserTest::TearDownInProcessBrowserTestFixture();
  }

  brave::URLSanitizerService* GetSanitizer() {
    return brave::URLSanitizerServiceFactory::GetForBrowserContext(
        browser()->profile());
  }

  void SetSanitizerRules(const std::string& matchers,
                         const std::string& permissions) {
    base::RunLoop loop;

    GetSanitizer()->SetInitializationCallbackForTesting(loop.QuitClosure());
    brave::URLSanitizerComponentInstaller::RawConfig config;
    config.matchers = matchers;
    config.permissions = permissions;
    auto* component_intaller =
        static_cast<brave::URLSanitizerComponentInstaller::Observer*>(
            GetSanitizer());
    component_intaller->OnConfigReady(config);

    loop.Run();
  }

  void NonBlockingDelay(base::TimeDelta delay) {
    base::RunLoop run_loop(base::RunLoop::Type::kNestableTasksAllowed);
    base::SingleThreadTaskRunner::GetCurrentDefault()->PostDelayedTask(
        FROM_HERE, run_loop.QuitWhenIdleClosure(), delay);
    run_loop.Run();
  }

  void WaitClipboardEmpty() {
    std::string text_from_clipboard;
    while (text_from_clipboard != "empty") {
      ui::Clipboard::GetForCurrentThread()->ReadAsciiText(
          ui::ClipboardBuffer::kCopyPaste, nullptr, &text_from_clipboard);
      NonBlockingDelay(base::Microseconds(10));
    }
  }

  std::string WaitClipboard() {
    std::string text_from_clipboard = "empty";
    while (text_from_clipboard == "empty") {
      ui::Clipboard::GetForCurrentThread()->ReadAsciiText(
          ui::ClipboardBuffer::kCopyPaste, nullptr, &text_from_clipboard);
      NonBlockingDelay(base::Microseconds(10));
    }
    return text_from_clipboard;
  }

  void Check() {
    SetSanitizerRules(kYoutubeRules, kYoutubePermissions);
    const GURL url("https://www.YoUtUbE.com/url_sanitizer/js_api.html");
    ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));

    auto check = [this](const std::string& name, bool should_sanitize,
                        const std::string& expected_text) {
      {
        ui::ScopedClipboardWriter clear_clipboard(
            ui::ClipboardBuffer::kCopyPaste);
        clear_clipboard.Reset();
        clear_clipboard.WriteText(u"empty");
      }
      WaitClipboardEmpty();

      auto* web_contents = browser()->tab_strip_model()->GetActiveWebContents();

      static constexpr char kClickButton[] =
          R"js(
            (function() {
              const button = document.getElementById('$1');
              button.click();
            })();
        )js";
      const auto script =
          base::ReplaceStringPlaceholders(kClickButton, {name}, nullptr);
      web_contents->Focus();
      ASSERT_TRUE(content::ExecJs(web_contents, script));

      const std::string text_from_clipboard = WaitClipboard();

      EXPECT_EQ(expected_text, text_from_clipboard)
          << name << " " << should_sanitize << " " << text_from_clipboard;
    };

    const bool should_sanitize =
        base::FeatureList::IsEnabled(features::kBraveCopyCleanLinkFromJs);

    const std::string sanitized = "https://youtu.be/B";
    const std::string unsanitized = "hTtPs://Youtu.Be/B?si=oLb865I64uJlLRJX";
    const std::string& expected = (should_sanitize) ? sanitized : unsanitized;

    check("test_1", should_sanitize, expected);
    check("test_2", should_sanitize, expected);
    check("test_3", should_sanitize, expected);
    check("test_4", should_sanitize, expected);
    check("test_5", should_sanitize, expected);
    // We cannot distinguish the context, so even if a password similar to the
    // URL is copied, we will sanitize it.
    check("test_sanitizable_password", should_sanitize, expected);
    // Not sanitazable password should be copied as is.
    check("test_not_sanitizable_password_1", should_sanitize, "Pa$$w0rd");
    check("test_not_sanitizable_password_2", should_sanitize, "A:^C,D");
    check("test_not_sanitizable_password_3", should_sanitize,
          "Ftp://Example.Com/?si=12345");

    const GURL no_permission_url(
        "https://no_permission.com/url_sanitizer/js_api.html");
    ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), no_permission_url));

    check("test_1", false, unsanitized);
    check("test_2", false, unsanitized);
    check("test_3", false, unsanitized);
    check("test_4", false, unsanitized);
    check("test_5", false, unsanitized);
    check("test_sanitizable_password", false, unsanitized);
    check("test_not_sanitizable_password_1", false, "Pa$$w0rd");
    check("test_not_sanitizable_password_2", false, "A:^C,D");
    check("test_not_sanitizable_password_3", false,
          "Ftp://Example.Com/?si=12345");
  }

 protected:
  base::test::ScopedFeatureList feature_list_;
  content::ContentMockCertVerifier mock_cert_verifier_;
  net::EmbeddedTestServer https_server_{net::EmbeddedTestServer::TYPE_HTTPS};
};

class EnabledURLSanitizerTest : public URLSanitizerTestBase {
 public:
  EnabledURLSanitizerTest() : URLSanitizerTestBase(true) {}
};

// Different name to prevent running in parallel.
class DisabledURLSanitizerTest : public URLSanitizerTestBase {
 public:
  DisabledURLSanitizerTest() : URLSanitizerTestBase(false) {}
};

IN_PROC_BROWSER_TEST_F(EnabledURLSanitizerTest, JSApi) {
  Check();
}

IN_PROC_BROWSER_TEST_F(DisabledURLSanitizerTest, JSApi) {
  Check();
}
