/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <algorithm>

#include "base/files/file_path.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/brave_content_browser_client.h"
#include "brave/components/brave_shields/browser/brave_shields_util.h"
#include "brave/components/constants/brave_paths.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/extensions/chrome_test_extension_loader.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/common/chrome_content_client.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
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
    farbling_url_ = embedded_test_server()->GetURL("a.com", "/iframe.html");
  }

  void TearDown() override {
    browser_content_client_.reset();
    content_client_.reset();
  }

  std::string LoadExtension(const base::FilePath& path) {
    extensions::ChromeTestExtensionLoader loader(browser()->profile());
    scoped_refptr<const extensions::Extension> extension =
        loader.LoadExtension(path);
    EXPECT_TRUE(extension);
    return extension->id();
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

  content::RenderFrameHost* Parent() const {
    return Contents()->GetPrimaryMainFrame();
  }

  content::RenderFrameHost* IFrame() const { return ChildFrameAt(Parent(), 0); }

  bool NavigateToURLUntilLoadStop(const GURL& url) {
    EXPECT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
    return WaitForLoadStop(Contents());
  }

  Browser* OpenPopup(const std::string& script, bool from_iframe) const {
    content::ExecuteScriptAsync(from_iframe ? Parent() : IFrame(), script);
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

  void FarbleScreenSize(const GURL& url, bool content_scheme) {
    for (int j = 0; j < static_cast<int>(std::size(kTestWindowBounds)); ++j) {
      SetBounds(kTestWindowBounds[j]);
      for (bool allow_fingerprinting : {false, true}) {
        SetFingerprintingSetting(allow_fingerprinting);
        NavigateToURLUntilLoadStop(url);
        for (bool test_iframe : {false, true}) {
          if (!content_scheme) {
            continue;
          }
          content::RenderFrameHost* host = test_iframe ? Parent() : IFrame();
          if (!allow_fingerprinting && !IsFlagDisabled() && content_scheme) {
            EXPECT_GE(8, EvalJs(host, "window.outerWidth - parent.innerWidth"));
            EXPECT_GE(8,
                      EvalJs(host, "window.outerHeight - parent.innerHeight"));
            EXPECT_GE(
                8,
                EvalJs(host, "window.screen.availWidth - parent.innerWidth"));
            EXPECT_GE(
                8,
                EvalJs(host, "window.screen.availHeight - parent.innerHeight"));
            EXPECT_GE(8,
                      EvalJs(host, "window.screen.width - parent.innerWidth"));
            EXPECT_GE(
                8, EvalJs(host, "window.screen.height - parent.innerHeight"));
          } else {
            EXPECT_LE(0, EvalJs(host, "window.outerWidth - parent.innerWidth"));
            EXPECT_LT(8,
                      EvalJs(host, "window.outerHeight - parent.innerHeight"));
            EXPECT_LT(
                8,
                EvalJs(host, "window.screen.availWidth - parent.innerWidth"));
            EXPECT_LT(
                8,
                EvalJs(host, "window.screen.availHeight - parent.innerHeight"));
            EXPECT_LT(8,
                      EvalJs(host, "window.screen.width - parent.innerWidth"));
            EXPECT_LT(
                8, EvalJs(host, "window.screen.height - parent.innerHeight"));
          }
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
        for (bool test_iframe : {false, true}) {
          content::RenderFrameHost* host = test_iframe ? Parent() : IFrame();
          if (!allow_fingerprinting && !IsFlagDisabled()) {
            EXPECT_GE(8, EvalJs(host, "window.screenX"));
            EXPECT_GE(8, EvalJs(host, "window.screenY"));
            EXPECT_GE(8, EvalJs(host, "window.screen.availLeft"));
            EXPECT_GE(8, EvalJs(host, "window.screen.availTop"));
            EXPECT_GE(8, EvalJs(host, PREPARE_TEST_EVENT
                                "testEvent.screenX - devicePixelRatio * "
                                "testEvent.clientX"));
            EXPECT_GE(8, EvalJs(host, PREPARE_TEST_EVENT
                                "testEvent.screenY - devicePixelRatio * "
                                "testEvent.clientY"));
          } else {
            if (kTestWindowBounds[i].x() > 8) {
              EXPECT_LT(8, EvalJs(host, "window.screenX"));
            }
            if (kTestWindowBounds[i].y() > 8) {
              EXPECT_LT(8, EvalJs(host, "window.screenY"));
            }
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
        for (bool test_iframe : {false, true}) {
          content::RenderFrameHost* host = test_iframe ? Parent() : IFrame();
          EXPECT_EQ(
              !allow_fingerprinting && !IsFlagDisabled(),
              EvalJs(host,
                     "matchMedia(`(device-width: ${outerWidth}px)`).matches"));
          EXPECT_EQ(
              !allow_fingerprinting && !IsFlagDisabled(),
              EvalJs(
                  host,
                  "matchMedia(`(device-height: ${outerHeight}px)`).matches"));
        }
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
      for (bool test_iframe : {false, true}) {
        const char* script =
            "open('http://d.test/', '', `"
            "left=10,"
            "top=10,"
            "width=${outerWidth + 200},"
            "height=${outerHeight + 200}"
            "`);";
        Browser* popup = OpenPopup(script, test_iframe);
        auto* popup_contents = popup->tab_strip_model()->GetActiveWebContents();
        content::WaitForLoadStop(popup_contents);
        gfx::Rect child_bounds = popup->window()->GetBounds();
        if (!allow_fingerprinting && !IsFlagDisabled()) {
          EXPECT_GE(child_bounds.x(), parent_bounds.x());
          EXPECT_GE(child_bounds.y(), parent_bounds.y());
          EXPECT_GE(10 + parent_bounds.width(), child_bounds.width());
          EXPECT_GE(10 + parent_bounds.height(), child_bounds.height());
        } else {
          EXPECT_LE(child_bounds.x(), std::max(80, 10 + parent_bounds.x()));
          EXPECT_LE(child_bounds.y(), std::max(80, 10 + parent_bounds.y()));
          EXPECT_LE(parent_bounds.width(), child_bounds.width());
          EXPECT_LE(parent_bounds.height(), child_bounds.height());
        }
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
  FarbleScreenSize(FarblingUrl(), true);
}

IN_PROC_BROWSER_TEST_F(BraveScreenFarblingBrowserTest_DisableFlag,
                       FarbleScreenSize_DisableFlag) {
  FarbleScreenSize(FarblingUrl(), true);
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

IN_PROC_BROWSER_TEST_F(BraveScreenFarblingBrowserTest_EnableFlag,
                       FarbleScreenSize_Schemes) {
  // chrome: URI (don't farble)
  FarbleScreenSize(GURL("chrome:version"), false);

  // chrome-extension: URI (don't farble)
  base::FilePath test_data_dir;
  base::PathService::Get(chrome::DIR_TEST_DATA, &test_data_dir);
  std::string extension_id =
      LoadExtension(test_data_dir.AppendASCII("extensions")
                        .AppendASCII("ui")
                        .AppendASCII("browser_action_popup"));
  base::RunLoop().RunUntilIdle();  // Ensure the extension is fully loaded.
  const GURL extension_url("chrome-extension://" + extension_id +
                           "/popup.html");
  FarbleScreenSize(extension_url, false);

  // devtools: URI (don't farble)
  const GURL devtools_url("devtools://devtools/bundled/devtools_app.html");
  FarbleScreenSize(devtools_url, false);
}
