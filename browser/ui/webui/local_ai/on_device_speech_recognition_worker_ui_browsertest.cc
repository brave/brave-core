// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <string>
#include <string_view>

#include "base/strings/strcat.h"
#include "base/test/metrics/histogram_tester.h"
#include "base/test/run_until.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/local_ai/core/features.h"
#include "brave/components/local_ai/core/url_constants.h"
#include "brave/components/local_ai/resources/grit/on_device_speech_recognition_worker_generated_map.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "services/network/public/cpp/connection_allowlist.h"
#include "services/network/public/cpp/connection_allowlist_metrics.h"
#include "url/gurl.h"

namespace local_ai {

namespace {

// The page's Trusted Types policy lets `new Worker()` load only ORT's worker
// glue, `ort.wasm.bundle.min.<contenthash>.js`.
std::string OrtWorkerScriptPath() {
  for (const auto& resource : kOnDeviceSpeechRecognitionWorkerGenerated) {
    const std::string_view path = resource.path;
    if (path.starts_with("ort.wasm.bundle.min.") && path.ends_with(".js")) {
      return base::StrCat({"/", path});
    }
  }
  return std::string();
}

}  // namespace

class OnDeviceSpeechRecognitionWorkerUIBrowserTest
    : public InProcessBrowserTest {
 public:
  OnDeviceSpeechRecognitionWorkerUIBrowserTest() {
    feature_list_.InitAndEnableFeature(kBraveOnDeviceSpeechRecognition);
  }

 protected:
  content::RenderFrameHost* NavigateToWorkerPage() {
    content::RenderFrameHost* rfh = ui_test_utils::NavigateToURL(
        browser(), GURL(kOnDeviceSpeechRecognitionWorkerURL));
    EXPECT_TRUE(rfh && !rfh->IsErrorDocument());
    return rfh;
  }

 private:
  base::test::ScopedFeatureList feature_list_;
};

IN_PROC_BROWSER_TEST_F(OnDeviceSpeechRecognitionWorkerUIBrowserTest,
                       CommitsEmptyEnforcedAllowlist) {
  content::RenderFrameHost* rfh = NavigateToWorkerPage();
  ASSERT_TRUE(rfh);

  const auto& enforced = rfh->GetConnectionAllowlists().enforced;
  ASSERT_TRUE(enforced.has_value());
  EXPECT_TRUE(enforced->allowlist.empty());
  EXPECT_EQ(enforced->webrtc_behavior,
            network::ConnectionAllowlist::WebRtcBehavior::kBlock);
}

IN_PROC_BROWSER_TEST_F(OnDeviceSpeechRecognitionWorkerUIBrowserTest,
                       PageLoadsItsOwnFiles) {
  content::RenderFrameHost* rfh = NavigateToWorkerPage();
  ASSERT_TRUE(rfh);

  EXPECT_EQ(true, content::EvalJs(rfh, "fetch(location.href).then(r => r.ok)"));
}

// Worker scripts load with no WebContents, unlike the page. A worker does not
// inherit the page's allowlist, so its script response must have one too.
IN_PROC_BROWSER_TEST_F(OnDeviceSpeechRecognitionWorkerUIBrowserTest,
                       WorkerGetsEnforcedAllowlist) {
  const std::string worker_script = OrtWorkerScriptPath();
  ASSERT_FALSE(worker_script.empty());

  base::HistogramTester histograms;
  auto enforced_count = [&] {
    return histograms.GetBucketCount(
        network::kConnectionAllowlistTypeHistogram,
        network::ConnectionAllowlistType::kEnforced);
  };

  content::RenderFrameHost* rfh = NavigateToWorkerPage();
  ASSERT_TRUE(rfh);
  EXPECT_EQ(1, enforced_count());

  ASSERT_TRUE(content::ExecJs(
      rfh, content::JsReplace("(() => { window.worker = new Worker($1); })()",
                              worker_script)));
  EXPECT_TRUE(base::test::RunUntil([&] { return enforced_count() == 2; }));
}

}  // namespace local_ai
