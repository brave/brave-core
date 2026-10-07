/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>
#include <vector>

#include "base/test/scoped_feature_list.h"
#include "base/test/test_future.h"
#include "brave/components/local_ai/buildflags/buildflags.h"
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

#if BUILDFLAG(ENABLE_LOCAL_AI)
#include "brave/components/local_ai/core/features.h"
#endif

namespace speech {

namespace {

using AvailabilityStatus = media::mojom::AvailabilityStatus;
using Quality = media::mojom::SpeechRecognitionQuality;

constexpr char kEnglish[] = "en-US";

}  // namespace

// Makes sure Web Speech's `available()` and `install()` offer nothing for any
// quality a page can ask for while Brave's feature is off or Local AI is not
// built, since Brave then serves no on-device speech model.
class OnDeviceSpeechUnavailableBrowserTest : public InProcessBrowserTest {
 public:
  OnDeviceSpeechUnavailableBrowserTest() {
    std::vector<base::test::FeatureRef> disabled;
#if BUILDFLAG(ENABLE_LOCAL_AI)
    disabled.push_back(local_ai::kBraveOnDeviceSpeechRecognition);
#endif
    // Every upstream on-device gate is enabled and Brave's own feature is
    // pinned off, so each case below measures what Brave answers instead of a
    // default a roll or a launch could flip.
    scoped_feature_list_.InitWithFeatures(
        {media::kOnDeviceWebSpeech, media::kOnDeviceWebSpeechSmallExpertModel,
         media::kOnDeviceWebSpeechGeminiNano},
        disabled);
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

IN_PROC_BROWSER_TEST_F(OnDeviceSpeechUnavailableBrowserTest,
                       CommandOffersNothing) {
  ExpectNothingOfferedOrInstallable(Quality::kCommand);
}

IN_PROC_BROWSER_TEST_F(OnDeviceSpeechUnavailableBrowserTest,
                       DictationOffersNothing) {
  ExpectNothingOfferedOrInstallable(Quality::kDictation);
}

IN_PROC_BROWSER_TEST_F(OnDeviceSpeechUnavailableBrowserTest,
                       ConversationOffersNothing) {
  ExpectNothingOfferedOrInstallable(Quality::kConversation);
}

}  // namespace speech
