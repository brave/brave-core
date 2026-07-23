/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include "base/path_service.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_component_updater/browser/local_data_files_service.h"
#include "brave/components/brave_shields/core/browser/brave_shields_utils.h"
#include "brave/components/brave_shields/core/common/features.h"
#include "brave/components/constants/brave_paths.h"
#include "brave/components/webcompat/core/common/features.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"

using brave_shields::ControlType;

namespace {
constexpr char kEmbeddedTestServerDirectory[] = "speech";
constexpr char kTitleScript[] = "document.title";

// The voices list is returned as a comma-separated "name (voiceURI)" string.
// Its ordering is not stable across navigations: on macOS the platform
// default voice is resolved asynchronously and, once known, is moved to the
// front of the list (see TtsPlatformImplMac::Voices). The first navigation in
// this test can read getVoices() before that async resolution completes, so an
// unfarbled list may be plain alphabetical on one domain and default-voice-
// first on another, even though the set of voices is identical. Farbling being
// off only guarantees the same voices on every domain, not the same order, so
// compare order-independently. (Voice names never contain a comma, so
// splitting on ',' is safe here.)
std::string SortedVoices(const std::string& voices) {
  std::vector<std::string> parts = base::SplitString(
      voices, ",", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
  std::sort(parts.begin(), parts.end());
  return base::JoinString(parts, ",");
}
}  // namespace

class BraveSpeechSynthesisFarblingBrowserTest : public InProcessBrowserTest {
 public:
  BraveSpeechSynthesisFarblingBrowserTest() {
    scoped_feature_list_.InitWithFeatures(
        {
            brave_shields::features::kBraveShowStrictFingerprintingMode,
            webcompat::features::kBraveWebcompatExceptionsService,
        },
        {});
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    host_resolver()->AddRule("*", "127.0.0.1");
    content::SetupCrossSiteRedirector(embedded_test_server());

    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
    test_data_dir = test_data_dir.AppendASCII(kEmbeddedTestServerDirectory);
    embedded_test_server()->ServeFilesFromDirectory(test_data_dir);

    ASSERT_TRUE(embedded_test_server()->Start());
  }

  HostContentSettingsMap* content_settings() {
    return HostContentSettingsMapFactory::GetForProfile(browser()->profile());
  }

  void AllowFingerprinting(std::string domain) {
    brave_shields::SetFingerprintingControlType(
        content_settings(), ControlType::ALLOW,
        embedded_test_server()->GetURL(domain, "/"));
  }

  void BlockFingerprinting(std::string domain) {
    brave_shields::SetFingerprintingControlType(
        content_settings(), ControlType::BLOCK,
        embedded_test_server()->GetURL(domain, "/"));
  }

  void SetFingerprintingDefault(std::string domain) {
    brave_shields::SetFingerprintingControlType(
        content_settings(), ControlType::DEFAULT,
        embedded_test_server()->GetURL(domain, "/"));
  }

  content::WebContents* web_contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
};

// Tests results of farbling voices list
IN_PROC_BROWSER_TEST_F(BraveSpeechSynthesisFarblingBrowserTest, FarbleVoices) {
  std::string domain_b = "b.com";
  std::string domain_z = "z.com";
  GURL url_b =
      embedded_test_server()->GetURL(domain_b, "/voices-farbling.html");
  GURL url_z =
      embedded_test_server()->GetURL(domain_z, "/voices-farbling.html");
  // Farbling level: off
  // The voices list should be the real voices list.
  AllowFingerprinting(domain_b);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url_b));
  std::string off_voices_b =
      EvalJs(web_contents(), kTitleScript).ExtractString();
  ASSERT_NE("failed", off_voices_b);

  // On platforms without any voices, the rest of this test is invalid.
  if (off_voices_b == "") {
    return;
  }

  AllowFingerprinting(domain_z);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url_z));
  std::string off_voices_z =
      EvalJs(web_contents(), kTitleScript).ExtractString();
  ASSERT_NE("failed", off_voices_z);
  // The voices list should be the same on every domain if farbling is off
  // (order-independent; see SortedVoices).
  EXPECT_EQ(SortedVoices(off_voices_b), SortedVoices(off_voices_z));

  // Farbling level: default
  // The voices list is farbled per domain.
  SetFingerprintingDefault(domain_b);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url_b));
  std::string default_voices_b =
      EvalJs(web_contents(), kTitleScript).ExtractString();
  SetFingerprintingDefault(domain_z);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url_z));
  std::string default_voices_z =
      EvalJs(web_contents(), kTitleScript).ExtractString();
  // The farbled voices list should be different from the unfarbled voices
  // list, and each domain's lists should be different from each other.
  // (That is not true of all domains, because there are a finite number of
  // farbling choices, but it should be true of these two domains.)
  // Compare order-independently (see SortedVoices): otherwise these could pass
  // merely because the platform reorders the default voice between navigations,
  // masking a regression where farbling silently stops adding the fake voice.
  EXPECT_NE(SortedVoices(off_voices_b), SortedVoices(default_voices_b));
  EXPECT_NE(SortedVoices(off_voices_z), SortedVoices(default_voices_z));
  EXPECT_NE(SortedVoices(default_voices_b), SortedVoices(default_voices_z));

  // Farbling level: maximum
  // The voices list is empty.
  BlockFingerprinting(domain_b);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url_b));
  auto max_voices_b = EvalJs(web_contents(), kTitleScript);
  EXPECT_EQ("", max_voices_b);
  BlockFingerprinting(domain_z);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url_z));
  auto max_voices_z = EvalJs(web_contents(), kTitleScript);
  EXPECT_EQ("", max_voices_z);

  // Farbling level: default, but webcompat exception enabled
  SetFingerprintingDefault(domain_z);
  brave_shields::SetWebcompatEnabled(
      content_settings(), ContentSettingsType::BRAVE_WEBCOMPAT_SPEECH_SYNTHESIS,
      true, embedded_test_server()->GetURL(domain_z, "/"), nullptr);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url_z));
  std::string off_voices_z2 =
      EvalJs(web_contents(), kTitleScript).ExtractString();
  ASSERT_NE("failed", off_voices_z2);
  // The voices list should be the same on every domain if farbling is off
  // (order-independent; see SortedVoices).
  EXPECT_EQ(SortedVoices(off_voices_b), SortedVoices(off_voices_z2));
}
