/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <optional>

#include "base/memory/raw_ptr.h"
#include "base/path_service.h"
#include "base/test/run_until.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_shields/core/browser/brave_shields_utils.h"
#include "brave/components/constants/brave_paths.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "third_party/blink/public/common/features.h"

using brave_shields::ControlType;

namespace {

enum class BlobContainerType { kPopUpWindow = 0, kIFrame = 1, kUnset = 2 };

}  // namespace

// The driver of test is basically on the JS side which simplifies testing a
// lot. See brave_screen_farbling_browsertest for comparison.
// The JS test basically does the following:
// 1. Reads the various screen attributes from the parent window and writes to
// the localStorage keyed under parent_.
// 2. Opens a blob:// window or an iframe depending on the test.
// 3. Reads the various screen attributes from the blob window and then writes
// to the localStorage keyed under blob_.
// 4. Compares the two and they should match for passing tests.
// 4.1) For cases where fingerprinting was blocked: both the parent and the
// blob window sees the same "farbled" value. 4.2) For cases where
// fingerprinting was not blocked: both the parent and the blob window sees
// the same "un-farbled" value.
// The test design is taken from
// https://test-website-a.pages.dev/blob-farbling-bypass/ which relies on local
// storage as a medium to communicate the various attributes visible to blob and
// the main frame.
class BraveBlobScreenFarblingBrowserTest
    : public InProcessBrowserTest,
      public testing::WithParamInterface<bool> {
 public:
  BraveBlobScreenFarblingBrowserTest() {
    if (GetParam()) {
      feature_list_.InitAndEnableFeature(
          blink::features::kBraveBlockScreenFingerprinting);
    } else {
      feature_list_.InitAndDisableFeature(
          blink::features::kBraveBlockScreenFingerprinting);
    }
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    host_resolver()->AddRule("*", "127.0.0.1");
    content::SetupCrossSiteRedirector(embedded_test_server());
    embedded_test_server()->ServeFilesFromDirectory(
        base::PathService::CheckedGet(brave::DIR_TEST_DATA));
    ASSERT_TRUE(embedded_test_server()->Start());

    blob_test_url_ =
        embedded_test_server()->GetURL("a.com", "/blob-fingerprinting.html");

    // Setup the test bound.
    ui_test_utils::SetAndWaitForBounds(*browser(),
                                       gfx::Rect(100, 100, 400, 400));
  }

  void TearDownOnMainThread() override {
    if (pop_up_browser_) {
      ClosePopup();
    }
  }

  void AllowFingerprinting(bool allow) {
    fingerprinting_allowed_ = allow;
    brave_shields::SetFingerprintingControlType(
        HostContentSettingsMapFactory::GetForProfile(browser()->profile()),
        allow ? ControlType::ALLOW : ControlType::DEFAULT, blob_test_url_);
  }

  content::RenderFrameHost* MainFrame() const {
    return browser()
        ->tab_strip_model()
        ->GetActiveWebContents()
        ->GetPrimaryMainFrame();
  }

  void LoadPopupAndWait(ui_test_utils::BrowserCreatedObserver& observer) {
    pop_up_browser_ = observer.Wait();
    ASSERT_NE(pop_up_browser_, browser());
    auto* popup_contents =
        pop_up_browser_->tab_strip_model()->GetActiveWebContents();
    ASSERT_TRUE(WaitForRenderFrameReady(popup_contents->GetPrimaryMainFrame()));
  }

  void ClosePopup() {
    // Clear pop_up_browser_ before closing so the raw_ptr is not dangling
    // when the Browser object is destroyed inside CloseBrowserSynchronously.
    Browser* popup = pop_up_browser_;
    pop_up_browser_ = nullptr;
    CloseBrowserSynchronously(popup);
  }

  void NavigateToBlob() {
    ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), blob_test_url_));
    ASSERT_FALSE(blob_container_type_ == BlobContainerType::kUnset)
        << "Blob container type not set.";

    // Navigate to the main frame first.
    content::RenderFrameHost* parent = MainFrame();

    std::optional<ui_test_utils::BrowserCreatedObserver> popup_observer;
    if (blob_container_type_ == BlobContainerType::kPopUpWindow) {
      popup_observer.emplace();
    }

    // Navigate to the blob.
    ASSERT_TRUE(content::ExecJs(
        parent, blob_container_type_ == BlobContainerType::kPopUpWindow
                    ? "startBlobScreenFingerprintingPopUpTest()"
                    : "startBlobScreenFingerprintingIframeTest()"));

    // If pop-up we need to wait for the pop-up to open.
    if (blob_container_type_ == BlobContainerType::kPopUpWindow) {
      LoadPopupAndWait(*popup_observer);
    }

    // Wait for the blob to have emitted the 'blob_done' signal.
    ASSERT_TRUE(base::test::RunUntil([&]() {
      return content::EvalJs(parent,
                             "localStorage.getItem('blob_done') === 'true'")
          .ExtractBool();
    }));

    // Validate.
    bool should_match = true;
    // When the screen fingerprinting feature is enabled AND fingerprinting is
    // blocked (DEFAULT) AND the blob is in a popup, the popup bypasses farbling
    // and sees real values while the parent sees farbled values — so they won't
    // match.
    // TODO(https://github.com/brave/brave-browser/issues/56048): Remove this
    // override once shields are supported on blob:// URLs.
    if (GetParam() && !fingerprinting_allowed_ &&
        blob_container_type_ == BlobContainerType::kPopUpWindow) {
      should_match = false;
    }

    EXPECT_EQ(should_match,
              content::EvalJs(MainFrame(),
                              "storedScreenValuesMatch('parent', 'blob')")
                  .ExtractBool())
        << "param=" << GetParam()
        << ", container=" << static_cast<std::size_t>(blob_container_type_)
        << ", allowed=" << fingerprinting_allowed_;

    // Close the popup after validation.
    if (blob_container_type_ == BlobContainerType::kPopUpWindow) {
      ClosePopup();
    }
  }

  void RunTests() {
    AllowFingerprinting(/*allow=*/false);
    NavigateToBlob();

    AllowFingerprinting(/*allow=*/true);
    NavigateToBlob();
  }

  void set_blob_container_type(const BlobContainerType blob_container_type) {
    blob_container_type_ = blob_container_type;
  }

  void RunTests() {
    AllowFingerprinting(/*allow=*/false);
    NavigateToBlob();

    AllowFingerprinting(/*allow=*/true);
    NavigateToBlob();
  }

  void set_blob_container_type(const BlobContainerType blob_container_type) {
    blob_container_type_ = blob_container_type;
  }

 private:
  base::test::ScopedFeatureList feature_list_;
  GURL blob_test_url_;
  BlobContainerType blob_container_type_ = BlobContainerType::kUnset;
  bool fingerprinting_allowed_ = false;
  raw_ptr<Browser> pop_up_browser_ = nullptr;
};

INSTANTIATE_TEST_SUITE_P(
    /* no prefix */,
    BraveBlobScreenFarblingBrowserTest,
    testing::Bool(),
    [](const testing::TestParamInfo<bool>& info) {
      return info.param ? "BraveBlockScreenFingerprinting_Enabled"
                        : "BraveBlockScreenFingerprinting_Disabled";
    });

IN_PROC_BROWSER_TEST_P(BraveBlobScreenFarblingBrowserTest,
                       FarbleScreenBlobURLPopup) {
  set_blob_container_type(BlobContainerType::kPopUpWindow);
  RunTests();
}

IN_PROC_BROWSER_TEST_P(BraveBlobScreenFarblingBrowserTest,
                       FarbleScreenBlobURLIframe) {
  set_blob_container_type(BlobContainerType::kIFrame);
  RunTests();
}
