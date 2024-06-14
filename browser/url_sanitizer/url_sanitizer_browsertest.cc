/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>

#include "base/path_service.h"
#include "base/strings/string_util.h"
#include "brave/browser/url_sanitizer/url_sanitizer_service_factory.h"
#include "brave/components/constants/brave_paths.h"
#include "brave/components/url_sanitizer/browser/url_sanitizer_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "ui/base/clipboard/clipboard.h"
#include "ui/base/clipboard/scoped_clipboard_writer.h"

namespace {

constexpr const char kYoutubeRules[] = R"json(
    [{
        "include": [
            "*://youtu.be/*?*",
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

}  // namespace

class URLSanitizerTest : public InProcessBrowserTest {
 public:
  void SetUpOnMainThread() override {
    https_server_.ServeFilesFromDirectory(
        base::PathService::CheckedGet(brave::DIR_TEST_DATA));
    https_server_.StartAcceptingConnections();
    host_resolver()->AddRule("*", "127.0.0.1");
  }

  void SetUp() override {
    ASSERT_TRUE(https_server_.InitializeAndListen());
    InProcessBrowserTest::SetUp();
  }

  brave::URLSanitizerService* GetSanitizer() {
    return brave::URLSanitizerServiceFactory::GetForBrowserContext(
        browser()->profile());
  }

  void SetSanitizerRules(const std::string& json) {
    base::RunLoop loop;
    GetSanitizer()->SetInitializationCallbackForTesting(loop.QuitClosure());
    GetSanitizer()->Initialize(json);
    loop.Run();
  }

 protected:
  net::EmbeddedTestServer https_server_{net::EmbeddedTestServer::TYPE_HTTPS};
};

IN_PROC_BROWSER_TEST_F(URLSanitizerTest, JSApi) {
  SetSanitizerRules(kYoutubeRules);
  const GURL url = https_server_.GetURL("/url_sanitizer/js_api.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));

  auto check = [this](const std::string& name) {
    {
      ui::ScopedClipboardWriter clear_clipboard(
          ui::ClipboardBuffer::kCopyPaste);
      clear_clipboard.Reset();
    }

    constexpr const char kClickButton[] =
        R"js(
        (function() {
          const button = document.getElementById('$1');
          button.click();
        })();
    )js";
    const auto script =
        base::ReplaceStringPlaceholders(kClickButton, {name}, nullptr);
    ASSERT_TRUE(content::ExecJs(
        browser()->tab_strip_model()->GetActiveWebContents(), script));

    std::string text_from_clipboard;
    ui::Clipboard::GetForCurrentThread()->ReadAsciiText(
        ui::ClipboardBuffer::kCopyPaste, nullptr, &text_from_clipboard);

    EXPECT_TRUE(base::StartsWith(text_from_clipboard, "https://youtu.be/"))
        << name;
    EXPECT_EQ(std::string::npos, text_from_clipboard.find("si=")) << name;
  };

  check("test_1");
  check("test_2");
  check("test_3");
  check("test_4");
  check("test_5");
}
