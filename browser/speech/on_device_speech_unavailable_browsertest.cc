/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>

#include "base/test/scoped_feature_list.h"
#include "base/test/test_future.h"
#include "chrome/browser/speech/on_device_speech_recognition_impl.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "media/base/media_switches.h"
#include "media/mojo/mojom/speech_recognizer.mojom.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "testing/gtest/include/gtest/gtest.h"

// Brave serves no on-device speech model by default, so Web Speech's
// `available()` and `install()` offer nothing for every quality a page can ask
// for. Brave's `SodaInstaller` replacement enforces that for `kCommand`. Built
// by a target `enable_local_ai` does not guard, because the replacement is in
// every build.

namespace speech {

namespace {

using AvailabilityStatus = media::mojom::AvailabilityStatus;
using Quality = media::mojom::SpeechRecognitionQuality;

constexpr char kEnglish[] = "en-US";

}  // namespace

class OnDeviceSpeechUnavailableBrowserTest : public InProcessBrowserTest {
 public:
  OnDeviceSpeechUnavailableBrowserTest() {
    // Every upstream quality gate is enabled, so each case below measures what
    // Brave ships instead of an upstream default a roll could flip.
    // `kOnDeviceWebSpeech` also guards `GetSodaAvailabilityStatus`, which the
    // `kCommand` case has to reach to see Brave's installer at all.
    scoped_feature_list_.InitWithFeatures(
        {media::kOnDeviceWebSpeech, media::kOnDeviceWebSpeechSmallExpertModel,
         media::kOnDeviceWebSpeechGeminiNano},
        {});
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    host_resolver()->AddRule("*", "127.0.0.1");
    embedded_https_test_server().ServeFilesFromSourceDirectory(
        GetChromeTestDataDir());
    ASSERT_TRUE(embedded_https_test_server().Start());
  }

 protected:
  void ExpectNothingOfferedOrInstallable(Quality quality) {
    ASSERT_TRUE(ui_test_utils::NavigateToURL(
        browser(),
        embedded_https_test_server().GetURL("foo.com", "/empty.html")));

    base::test::TestFuture<AvailabilityStatus> available;
    on_device_speech_recognition()->Available({kEnglish}, quality,
                                              available.GetCallback());
    EXPECT_EQ(AvailabilityStatus::kUnavailable, available.Get());

    base::test::TestFuture<bool> installed;
    on_device_speech_recognition()->Install({kEnglish}, quality,
                                            installed.GetCallback());
    EXPECT_FALSE(installed.Get());
  }

 private:
  OnDeviceSpeechRecognitionImpl* on_device_speech_recognition() {
    return OnDeviceSpeechRecognitionImpl::GetOrCreateForCurrentDocument(
        chrome_test_utils::GetActiveWebContents(this)->GetPrimaryMainFrame());
  }

  base::test::ScopedFeatureList scoped_feature_list_;
};

// The only quality Brave's installer governs. Without the replacement upstream
// reports SODA's en-US as downloadable and then parks the install() that
// follows on a download that never arrives.
IN_PROC_BROWSER_TEST_F(OnDeviceSpeechUnavailableBrowserTest,
                       CommandOffersNothing) {
  ExpectNothingOfferedOrInstallable(Quality::kCommand);
}

// The two qualities that never reach Brave's installer.
// `GetOnDeviceSpeechRecognitionAvailabilityStatusAsync` routes both to the
// optimization guide, which reports no eligible on-device model in a Brave
// build, so these pin the answer Brave gives once upstream's own gates are
// open.
IN_PROC_BROWSER_TEST_F(OnDeviceSpeechUnavailableBrowserTest,
                       DictationOffersNothing) {
  ExpectNothingOfferedOrInstallable(Quality::kDictation);
}

IN_PROC_BROWSER_TEST_F(OnDeviceSpeechUnavailableBrowserTest,
                       ConversationOffersNothing) {
  ExpectNothingOfferedOrInstallable(Quality::kConversation);
}

}  // namespace speech
