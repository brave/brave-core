/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/path_service.h"
#include "base/strings/stringprintf.h"
#include "base/task/post_task.h"
#include "base/test/thread_test_helper.h"
#include "brave/browser/brave_content_browser_client.h"
#include "brave/browser/extensions/brave_base_local_data_files_browsertest.h"
#include "brave/common/brave_paths.h"
#include "brave/common/pref_names.h"
#include "brave/components/brave_component_updater/browser/local_data_files_service.h"
#include "brave/components/brave_shields/browser/brave_shields_util.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/extensions/extension_browsertest.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/common/chrome_content_client.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"

using brave_shields::ControlType;

const char kEmbeddedTestServerDirectory[] = "webaudio";
const char kTitleScript[] = "domAutomationController.send(document.title);";
const char kExpectedWebAudioFarblingSum[] = "399";

class BraveWebAudioFarblingBrowserTest : public InProcessBrowserTest {
 public:
  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    content_client_.reset(new ChromeContentClient);
    content::SetContentClient(content_client_.get());
    browser_content_client_.reset(new BraveContentBrowserClient());
    content::SetBrowserClientForTesting(browser_content_client_.get());

    host_resolver()->AddRule("*", "127.0.0.1");
    content::SetupCrossSiteRedirector(embedded_test_server());

    brave::RegisterPathProvider();
    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
    test_data_dir = test_data_dir.AppendASCII(kEmbeddedTestServerDirectory);
    embedded_test_server()->ServeFilesFromDirectory(test_data_dir);

    ASSERT_TRUE(embedded_test_server()->Start());

    top_level_page_url_ = embedded_test_server()->GetURL("a.com", "/");
    farbling_url_ = embedded_test_server()->GetURL("a.com", "/farbling.html");
    copy_from_channel_url_ =
        embedded_test_server()->GetURL("a.com", "/copyFromChannel.html");
  }

  void TearDown() override {
    browser_content_client_.reset();
    content_client_.reset();
  }

  const GURL& copy_from_channel_url() { return copy_from_channel_url_; }

  const GURL& farbling_url() { return farbling_url_; }

  HostContentSettingsMap* content_settings() {
    return HostContentSettingsMapFactory::GetForProfile(browser()->profile());
  }

  void AllowFingerprinting() {
    brave_shields::SetFingerprintingControlType(
        content_settings(), ControlType::ALLOW, top_level_page_url_);
  }

  void BlockFingerprinting() {
    brave_shields::SetFingerprintingControlType(
        content_settings(), ControlType::BLOCK, top_level_page_url_);
  }

  void SetFingerprintingDefault() {
    brave_shields::SetFingerprintingControlType(
        content_settings(), ControlType::DEFAULT, top_level_page_url_);
  }

  template <typename T>
  std::string ExecScriptGetStr(const std::string& script, T* frame) {
    std::string value;
    EXPECT_TRUE(ExecuteScriptAndExtractString(frame, script, &value));
    return value;
  }

  content::WebContents* contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  bool NavigateToURLUntilLoadStop(const GURL& url) {
    ui_test_utils::NavigateToURL(browser(), url);
    return WaitForLoadStop(contents());
  }

 private:
  GURL top_level_page_url_;
  GURL copy_from_channel_url_;
  GURL farbling_url_;
  std::unique_ptr<ChromeContentClient> content_client_;
  std::unique_ptr<BraveContentBrowserClient> browser_content_client_;
};

// Tests for crash in copyFromChannel as reported in
// https://github.com/brave/brave-browser/issues/9552
// No crash indicates a successful test.
IN_PROC_BROWSER_TEST_F(BraveWebAudioFarblingBrowserTest,
                       CopyFromChannelNoCrash) {
  NavigateToURLUntilLoadStop(copy_from_channel_url());
}

// Tests results of farbling known values
IN_PROC_BROWSER_TEST_F(BraveWebAudioFarblingBrowserTest, FarbleWebAudio) {
  // Farbling level: maximum
  // web audio: pseudo-random data with no relation to underlying audio channel
  BlockFingerprinting();
  NavigateToURLUntilLoadStop(farbling_url());
  EXPECT_EQ(ExecScriptGetStr(kTitleScript, contents()),
            kExpectedWebAudioFarblingSum);
  // second time, same as the first (tests that the PRNG properly resets itself
  // at the beginning of each calculation)
  NavigateToURLUntilLoadStop(farbling_url());
  EXPECT_EQ(ExecScriptGetStr(kTitleScript, contents()),
            kExpectedWebAudioFarblingSum);

  // Farbling level: balanced (default)
  // web audio: farbled audio data
  SetFingerprintingDefault();
  NavigateToURLUntilLoadStop(farbling_url());
  EXPECT_EQ(ExecScriptGetStr(kTitleScript, contents()), "7968");

  // Farbling level: off
  // web audio: original audio data
  AllowFingerprinting();
  NavigateToURLUntilLoadStop(farbling_url());
  EXPECT_EQ(ExecScriptGetStr(kTitleScript, contents()), "8000");
}
