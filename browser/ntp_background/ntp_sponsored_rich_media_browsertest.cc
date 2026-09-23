/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/functional/bind.h"
#include "base/path_service.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/components/constants/brave_paths.h"
#include "brave/components/ntp_background_images/browser/ntp_background_images_service.h"
#include "brave/components/ntp_background_images/browser/ntp_background_images_service_waiter.h"
#include "brave/components/ntp_background_images/browser/switches.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "chrome/test/base/platform_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_navigation_observer.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace ntp_background_images {

namespace {
constexpr char kRichMediaUrl[] =
    R"(chrome-untrusted://new-tab-takeover/aa0b561e-9eed-4aaa-8999-5627bc6b14fd/index.html)";
}  // namespace

class NTPSponsoredRichMediaBrowserTest : public PlatformBrowserTest {
 protected:
  void SetUpOnMainThread() override {
    PlatformBrowserTest::SetUpOnMainThread();

    const base::FilePath test_data_file_path =
        base::PathService::CheckedGet(brave::DIR_TEST_DATA);

    const base::FilePath component_file_path =
        test_data_file_path.AppendASCII("components")
            .AppendASCII("ntp_sponsored_images")
            .AppendASCII("rich_media");
    base::CommandLine::ForCurrentProcess()->AppendSwitchPath(
        switches::kOverrideSponsoredImagesComponentPath, component_file_path);

    NTPBackgroundImagesService* const ntp_background_images_service =
        g_brave_browser_process->ntp_background_images_service();
    ASSERT_TRUE(ntp_background_images_service);

    NTPBackgroundImagesServiceWaiter waiter(*ntp_background_images_service);
    ntp_background_images_service->Init();
    waiter.WaitForOnSponsoredContentDidUpdate();
  }

  content::WebContents* GetActiveWebContents() {
    return chrome_test_utils::GetActiveWebContents(this);
  }
};

IN_PROC_BROWSER_TEST_F(NTPSponsoredRichMediaBrowserTest,
                       LoadResourceAndClickButton) {
  content::WebContentsConsoleObserver console_observer(GetActiveWebContents());
  console_observer.SetFilter(base::BindRepeating(
      [](const content::WebContentsConsoleObserver::Message& message) {
        return message.log_level == blink::mojom::ConsoleMessageLevel::kError;
      }));
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), GURL(kRichMediaUrl)));
  EXPECT_TRUE(console_observer.messages().empty());

  ASSERT_TRUE(content::ExecJs(GetActiveWebContents(),
                              "document.querySelector('.button').click();"));
  content::EvalJsResult result = content::EvalJs(
      GetActiveWebContents(), "document.querySelector('.button').textContent;");
  EXPECT_EQ(result.ExtractString(), "🚀");
}

}  // namespace ntp_background_images
