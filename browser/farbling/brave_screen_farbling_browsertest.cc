/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <algorithm>

#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/brave_content_browser_client.h"
#include "brave/components/brave_shields/browser/brave_shields_util.h"
#include "brave/components/constants/brave_paths.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/extensions/extension_browsertest.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/common/chrome_content_client.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/http_request.h"
#include "third_party/blink/public/common/features.h"

using brave_shields::ControlType;

namespace {

const gfx::Rect kTestWindowBounds[] = {
    gfx::Rect(200, 100, 300, 200), gfx::Rect(50, 50, 200, 200),
    gfx::Rect(50, 50, 555, 444), gfx::Rect(0, 0, 200, 200)};

}  // namespace

class BraveScreenFarblingBrowserTest : public InProcessBrowserTest {
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
    embedded_test_server()->ServeFilesFromDirectory(test_data_dir);

    ASSERT_TRUE(embedded_test_server()->Start());

    top_level_page_url_ = embedded_test_server()->GetURL("a.com", "/");
    farbling_url_ = embedded_test_server()->GetURL("a.com", "/simple.html");
  }

  void TearDown() override {
    browser_content_client_.reset();
    content_client_.reset();
  }

  HostContentSettingsMap* ContentSettings() {
    return HostContentSettingsMapFactory::GetForProfile(browser()->profile());
  }

  void SetFingerprintingSetting(bool allow) {
    brave_shields::SetFingerprintingControlType(
        ContentSettings(), allow ? ControlType::ALLOW : ControlType::DEFAULT,
        top_level_page_url_);
  }

  content::WebContents* Contents() const {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  bool NavigateToURLUntilLoadStop(const GURL& url) {
    EXPECT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
    return WaitForLoadStop(Contents());
  }

  Browser* OpenPopup(const std::string& script) const {
    content::ExecuteScriptAsync(Contents(), script);
    Browser* popup = ui_test_utils::WaitForBrowserToOpen();
    EXPECT_NE(popup, browser());
    auto* popup_contents = popup->tab_strip_model()->GetActiveWebContents();
    EXPECT_TRUE(WaitForRenderFrameReady(popup_contents->GetPrimaryMainFrame()));
    return popup;
  }

  virtual bool IsFlagDisabled() const = 0;

  const GURL& FarblingUrl() { return farbling_url_; }

  gfx::Rect SetBounds(const gfx::Rect& bounds) {
    browser()->window()->SetBounds(bounds);
    return browser()->window()->GetBounds();
  }

  void FarbleScreenSize() {
    for (int j = 0; j < static_cast<int>(std::size(kTestWindowBounds)); ++j) {
      SetBounds(kTestWindowBounds[j]);
      for (bool allow_fingerprinting : {false, true}) {
        SetFingerprintingSetting(allow_fingerprinting);
        NavigateToURLUntilLoadStop(FarblingUrl());
        if (!allow_fingerprinting && !IsFlagDisabled()) {
          EXPECT_GE(
              8, EvalJs(Contents(), "window.outerWidth - window.innerWidth"));
          EXPECT_GE(
              8, EvalJs(Contents(), "window.outerHeight - window.innerHeight"));
          EXPECT_GE(8, EvalJs(Contents(),
                              "window.screen.availWidth - window.innerWidth"));
          EXPECT_GE(8,
                    EvalJs(Contents(),
                           "window.screen.availHeight - window.innerHeight"));
          EXPECT_GE(
              8, EvalJs(Contents(), "window.screen.width - window.innerWidth"));
          EXPECT_GE(8, EvalJs(Contents(),
                              "window.screen.height - window.innerHeight"));
        } else {
          EXPECT_LE(
              0, EvalJs(Contents(), "window.outerWidth - window.innerWidth"));
          EXPECT_LT(
              8, EvalJs(Contents(), "window.outerHeight - window.innerHeight"));
          EXPECT_LT(8, EvalJs(Contents(),
                              "window.screen.availWidth - window.innerWidth"));
          EXPECT_LT(8,
                    EvalJs(Contents(),
                           "window.screen.availHeight - window.innerHeight"));
          EXPECT_LT(
              8, EvalJs(Contents(), "window.screen.width - window.innerWidth"));
          EXPECT_LT(8, EvalJs(Contents(),
                              "window.screen.height - window.innerHeight"));
        }
      }
    }
  }

#define PREPARE_TEST_EVENT                                   \
  "let fakeScreenX = 100, fakeScreenY = 200; "               \
  "let fakeClientX = 300, fakeClientY = 400; "               \
  "let testEvent = document.createEvent('MouseEvent'); "     \
  "testEvent.initMouseEvent('click', true, true, window, 1," \
  "fakeScreenX + devicePixelRatio * fakeClientX,"            \
  "fakeScreenY + devicePixelRatio * fakeClientY,"            \
  "fakeClientX, fakeClientY, false, false, false, false, 0, null); "

  void FarbleWindowPosition() {
    for (bool allow_fingerprinting : {false, true}) {
      SetFingerprintingSetting(allow_fingerprinting);
      for (int i = 0; i < static_cast<int>(std::size(kTestWindowBounds)); ++i) {
        SetBounds(kTestWindowBounds[i]);
        NavigateToURLUntilLoadStop(FarblingUrl());
        if (!allow_fingerprinting && !IsFlagDisabled()) {
          EXPECT_GE(8, EvalJs(Contents(), "window.screenX"));
          EXPECT_GE(8, EvalJs(Contents(), "window.screenY"));
          EXPECT_GE(8, EvalJs(Contents(), "window.screen.availLeft"));
          EXPECT_GE(8, EvalJs(Contents(), "window.screen.availTop"));
          EXPECT_GE(
              8,
              EvalJs(
                  Contents(), PREPARE_TEST_EVENT
                  "testEvent.screenX - devicePixelRatio * testEvent.clientX"));
          EXPECT_GE(
              8,
              EvalJs(
                  Contents(), PREPARE_TEST_EVENT
                  "testEvent.screenY - devicePixelRatio * testEvent.clientY"));
        } else {
          if (kTestWindowBounds[i].x() > 8) {
            EXPECT_LT(8, EvalJs(Contents(), "window.screenX"));
          }
          if (kTestWindowBounds[i].y() > 8) {
            EXPECT_LT(8, EvalJs(Contents(), "window.screenY"));
          }
        }
      }
    }
  }

  void FarbleScreenMediaQuery() {
    for (int j = 0; j < static_cast<int>(std::size(kTestWindowBounds)); ++j) {
      SetBounds(kTestWindowBounds[j]);
      for (bool allow_fingerprinting : {false, true}) {
        SetFingerprintingSetting(allow_fingerprinting);
        NavigateToURLUntilLoadStop(FarblingUrl());
        EXPECT_EQ(
            !allow_fingerprinting && !IsFlagDisabled(),
            EvalJs(Contents(),
                   "matchMedia(`(max-device-width: ${innerWidth + 8}px) and "
                   "(min-device-width: ${innerWidth}px)`).matches"));
        EXPECT_EQ(
            !allow_fingerprinting && !IsFlagDisabled(),
            EvalJs(Contents(),
                   "matchMedia(`(max-device-height: ${innerHeight + 8}px) and "
                   "(min-device-height: ${innerHeight}px)`).matches"));
      }
    }
  }

  void FarbleScreenPopupPosition(int j) {
    gfx::Rect parent_bounds;
    // Make sure parent_bounds dimensions aren't unexpectedly large.
    do {
      parent_bounds = SetBounds(kTestWindowBounds[j]);
    } while (parent_bounds.width() > 600 || parent_bounds.height() > 600);
    for (bool allow_fingerprinting : {false, true}) {
      SetFingerprintingSetting(allow_fingerprinting);
      NavigateToURLUntilLoadStop(FarblingUrl());
      const char* script =
          "open('http://d.test/', '', `"
          "left=10,"
          "top=10,"
          "width=${outerWidth + 200},"
          "height=${outerHeight + 200}"
          "`);";
      Browser* popup = OpenPopup(script);
      base::RunLoop().RunUntilIdle();
      gfx::Rect child_bounds = popup->window()->GetBounds();
      auto* parent_contents = Contents();
      auto* popup_contents = popup->tab_strip_model()->GetActiveWebContents();
      const int popup_inner_width =
          EvalJs(popup_contents, "innerWidth").value.GetInt();
      const int popup_inner_height =
          EvalJs(popup_contents, "innerHeight").value.GetInt();
      if (!allow_fingerprinting && !IsFlagDisabled()) {
        EXPECT_GE(child_bounds.x(), parent_bounds.x());
        EXPECT_GE(child_bounds.y(), parent_bounds.y());
        EXPECT_LE(popup_inner_width, EvalJs(parent_contents, "innerWidth + 8"));
        EXPECT_LE(popup_inner_height,
                  EvalJs(parent_contents, "Math.max(100, innerHeight + 8)"));
      } else {
        EXPECT_LE(child_bounds.x(), std::max(80, 10 + parent_bounds.x()));
        EXPECT_LE(child_bounds.y(), std::max(80, 10 + parent_bounds.y()));
        EXPECT_GE(popup_inner_width, EvalJs(parent_contents, "innerWidth"));
        EXPECT_GE(popup_inner_height, EvalJs(parent_contents, "innerHeight"));
      }
    }
  }

 protected:
  base::test::ScopedFeatureList feature_list_;

 private:
  GURL top_level_page_url_;
  GURL farbling_url_;
  std::unique_ptr<ChromeContentClient> content_client_;
  std::unique_ptr<BraveContentBrowserClient> browser_content_client_;
};

class BraveScreenFarblingBrowserTest_EnableFlag
    : public BraveScreenFarblingBrowserTest {
 public:
  BraveScreenFarblingBrowserTest_EnableFlag() {
    feature_list_.InitAndEnableFeature(
        blink::features::kBraveBlockScreenFingerprinting);
  }

  bool IsFlagDisabled() const override { return false; }
};

class BraveScreenFarblingBrowserTest_DisableFlag
    : public BraveScreenFarblingBrowserTest {
 public:
  BraveScreenFarblingBrowserTest_DisableFlag() {
    feature_list_.InitAndDisableFeature(
        blink::features::kBraveBlockScreenFingerprinting);
  }

  bool IsFlagDisabled() const override { return true; }
};

IN_PROC_BROWSER_TEST_F(BraveScreenFarblingBrowserTest_EnableFlag,
                       FarbleScreenSize_EnableFlag) {
  FarbleScreenSize();
}

IN_PROC_BROWSER_TEST_F(BraveScreenFarblingBrowserTest_DisableFlag,
                       FarbleScreenSize_DisableFlag) {
  FarbleScreenSize();
}

IN_PROC_BROWSER_TEST_F(BraveScreenFarblingBrowserTest_EnableFlag,
                       FarbleWindowPosition_EnableFlag) {
  FarbleWindowPosition();
}

IN_PROC_BROWSER_TEST_F(BraveScreenFarblingBrowserTest_DisableFlag,
                       FarbleWindowPosition_DisableFlag) {
  FarbleWindowPosition();
}

IN_PROC_BROWSER_TEST_F(BraveScreenFarblingBrowserTest_EnableFlag,
                       FarbleScreenMediaQuery_EnableFlag) {
  FarbleScreenMediaQuery();
}

IN_PROC_BROWSER_TEST_F(BraveScreenFarblingBrowserTest_DisableFlag,
                       FarbleScreenMediaQuery_DisableFlag) {
  FarbleScreenMediaQuery();
}

// Run each window size as a separate test because on linux
// the browser window does not properly resized within
// a single test.

IN_PROC_BROWSER_TEST_F(BraveScreenFarblingBrowserTest_EnableFlag,
                       FarbleScreenPopupPosition_EnableFlag_0) {
  FarbleScreenPopupPosition(0);
}

IN_PROC_BROWSER_TEST_F(BraveScreenFarblingBrowserTest_DisableFlag,
                       FarbleScreenPopupPosition_DisableFlag_0) {
  FarbleScreenPopupPosition(0);
}

IN_PROC_BROWSER_TEST_F(BraveScreenFarblingBrowserTest_EnableFlag,
                       FarbleScreenPopupPosition_EnableFlag_1) {
  FarbleScreenPopupPosition(1);
}

IN_PROC_BROWSER_TEST_F(BraveScreenFarblingBrowserTest_DisableFlag,
                       FarbleScreenPopupPosition_DisableFlag_1) {
  FarbleScreenPopupPosition(1);
}

IN_PROC_BROWSER_TEST_F(BraveScreenFarblingBrowserTest_EnableFlag,
                       FarbleScreenPopupPosition_EnableFlag_2) {
  FarbleScreenPopupPosition(2);
}

IN_PROC_BROWSER_TEST_F(BraveScreenFarblingBrowserTest_DisableFlag,
                       FarbleScreenPopupPosition_DisableFlag_2) {
  FarbleScreenPopupPosition(2);
}

IN_PROC_BROWSER_TEST_F(BraveScreenFarblingBrowserTest_EnableFlag,
                       FarbleScreenPopupPosition_EnableFlag_3) {
  FarbleScreenPopupPosition(3);
}

IN_PROC_BROWSER_TEST_F(BraveScreenFarblingBrowserTest_DisableFlag,
                       FarbleScreenPopupPosition_DisableFlag_3) {
  FarbleScreenPopupPosition(3);
}
