/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <algorithm>
#include <array>
#include <string>

#include "base/files/file_path.h"
#include "base/memory/scoped_refptr.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/test/scoped_feature_list.h"
#include "base/values.h"
#include "brave/components/brave_shields/core/browser/brave_shields_utils.h"
#include "brave/components/constants/brave_paths.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/extensions/chrome_test_extension_loader.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "extensions/test/test_extension_dir.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "third_party/blink/public/common/features.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_observer.h"

using brave_shields::ControlType;
using content::TitleWatcher;

namespace {

constexpr auto kTestWindowBounds = std::to_array<const gfx::Rect>(
    {gfx::Rect(200, 100, 300, 200), gfx::Rect(50, 50, 200, 200),
     gfx::Rect(50, 50, 475, 460), gfx::Rect(0, 0, 200, 200)});

constexpr auto kAllowedScreenSizes = std::to_array<const gfx::Size>(
    {gfx::Size(1280, 800), gfx::Size(1366, 768), gfx::Size(1440, 900),
     gfx::Size(1680, 1050), gfx::Size(1920, 1080), gfx::Size(2560, 1440),
     gfx::Size(3840, 2160)});

struct ScreenValues {
  int width;
  int height;
  int avail_width;
  int avail_height;
  int avail_left;
  int avail_top;
  int screen_x;
  int screen_y;
};

}  // namespace

// A helper class to wait for widget bounds changes beyond given thresholds.
class WidgetBoundsChangeWaiter final : public views::WidgetObserver {
 public:
  WidgetBoundsChangeWaiter(views::Widget* widget, int threshold)
      : widget_(widget),
        threshold_(threshold),
        initial_bounds_(widget->GetWindowBoundsInScreen()) {
    widget_->AddObserver(this);
  }

  WidgetBoundsChangeWaiter(const WidgetBoundsChangeWaiter&) = delete;
  WidgetBoundsChangeWaiter& operator=(const WidgetBoundsChangeWaiter&) = delete;
  ~WidgetBoundsChangeWaiter() final { widget_->RemoveObserver(this); }

  // views::WidgetObserver:
  void OnWidgetBoundsChanged(views::Widget* widget,
                             const gfx::Rect& rect) final {
    if (BoundsChangeMeetsThreshold(rect)) {
      widget_->RemoveObserver(this);
      run_loop_.Quit();
    }
  }

  // Wait for changes to occur, or return immediately if they already have.
  void Wait() {
    if (!BoundsChangeMeetsThreshold(widget_->GetWindowBoundsInScreen())) {
      run_loop_.Run();
    }
  }

 private:
  bool BoundsChangeMeetsThreshold(const gfx::Rect& rect) const {
    return (std::abs(rect.x() - initial_bounds_.x()) >= threshold_ &&
            std::abs(rect.y() - initial_bounds_.y()) >= threshold_) ||
           (std::abs(rect.width() - initial_bounds_.width()) >= threshold_ &&
            std::abs(rect.height() - initial_bounds_.height()) >= threshold_);
  }

  const raw_ptr<views::Widget> widget_;
  const int threshold_;
  const gfx::Rect initial_bounds_;
  base::RunLoop run_loop_;
};

class BraveScreenFarblingBrowserTest : public InProcessBrowserTest {
 public:
  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    host_resolver()->AddRule("*", "127.0.0.1");
    content::SetupCrossSiteRedirector(embedded_test_server());

    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
    embedded_test_server()->ServeFilesFromDirectory(test_data_dir);

    ASSERT_TRUE(embedded_test_server()->Start());

    parent_url_ = embedded_test_server()->GetURL("a.com", "/iframe.html");
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
        parent_url());
  }

  content::WebContents* Contents() const {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  content::RenderFrameHost* Parent() const {
    return Contents()->GetPrimaryMainFrame();
  }

  content::RenderFrameHost* IFrame() const { return ChildFrameAt(Parent(), 0); }

  Browser* OpenPopup(const std::string& script, bool from_iframe) const {
    content::ExecuteScriptAsync(from_iframe ? Parent() : IFrame(), script);
    Browser* popup = ui_test_utils::WaitForBrowserToOpen();
    EXPECT_NE(popup, browser());
    auto* popup_contents = popup->tab_strip_model()->GetActiveWebContents();
    EXPECT_TRUE(WaitForRenderFrameReady(popup_contents->GetPrimaryMainFrame()));
    return popup;
  }

  virtual bool IsFlagDisabled() const = 0;

  gfx::Rect SetBounds(const gfx::Rect& bounds) {
    browser()->window()->SetBounds(bounds);
    content::RunAllPendingInMessageLoop();
    return browser()->window()->GetBounds();
  }

  bool IsAllowedScreenSize(const int width, const int height) {
    for (const auto& allowed_size : kAllowedScreenSizes) {
      if (allowed_size.width() == width && allowed_size.height() == height) {
        return true;
      }
    }
    return false;
  }

  void ExpectScreenSizeFarbling(content::RenderFrameHost* host,
                                bool expect_farbled,
                                const gfx::Size& minimum_screen_size,
                                const std::string& inner_width_expression,
                                const std::string& inner_height_expression) {
    if (expect_farbled) {
      EXPECT_GE(8,
                EvalJs(host, "window.outerWidth - " + inner_width_expression));
      EXPECT_GE(
          8, EvalJs(host, "window.outerHeight - " + inner_height_expression));
      EXPECT_EQ(0, EvalJs(host,
                          "window.screen.width - "
                          "window.screen.availWidth"));
      EXPECT_EQ(0, EvalJs(host,
                          "window.screen.height - "
                          "window.screen.availHeight"));
      const int screen_width = EvalJs(host, "window.screen.width").ExtractInt();
      const int screen_height =
          EvalJs(host, "window.screen.height").ExtractInt();
      EXPECT_TRUE(IsAllowedScreenSize(screen_width, screen_height));
      EXPECT_GE(screen_width, minimum_screen_size.width());
      EXPECT_GE(screen_height, minimum_screen_size.height());
      EXPECT_GE(8, EvalJs(host, "window.screenX"));
      EXPECT_GE(8, EvalJs(host, "window.screenY"));
    } else {
      EXPECT_LE(0,
                EvalJs(host, "window.outerWidth - " + inner_width_expression));
      EXPECT_LT(
          8, EvalJs(host, "window.outerHeight - " + inner_height_expression));
      EXPECT_LT(8, EvalJs(host, "window.screen.availWidth - " +
                                    inner_width_expression));
      EXPECT_LT(8, EvalJs(host, "window.screen.availHeight - " +
                                    inner_height_expression));
      EXPECT_LT(
          8, EvalJs(host, "window.screen.width - " + inner_width_expression));
      EXPECT_LT(
          8, EvalJs(host, "window.screen.height - " + inner_height_expression));
    }
  }

  void FarbleScreenSize(const GURL& url, bool content_scheme) {
    for (const auto& test_bounds : kTestWindowBounds) {
      SetBounds(test_bounds);
      for (bool allow_fingerprinting : {false, true}) {
        SetFingerprintingSetting(allow_fingerprinting);
        ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
        for (bool test_iframe : {false, true}) {
          if (!content_scheme) {
            continue;
          }
          content::RenderFrameHost* host = test_iframe ? Parent() : IFrame();
          ExpectScreenSizeFarbling(
              host, !allow_fingerprinting && !IsFlagDisabled(),
              test_bounds.size(), "parent.innerWidth", "parent.innerHeight");
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
      for (const auto& bounds : kTestWindowBounds) {
        SetBounds(bounds);
        ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), parent_url()));
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
            if (bounds.x() > 8) {
              EXPECT_LT(8, EvalJs(host, "window.screenX"));
            }
            if (bounds.y() > 8) {
              EXPECT_LT(8, EvalJs(host, "window.screenY"));
            }
          }
        }
      }
    }
  }

  void FarbleScreenMediaQuery() {
    for (const auto& bounds : kTestWindowBounds) {
      SetBounds(bounds);
      for (bool allow_fingerprinting : {false, true}) {
        SetFingerprintingSetting(allow_fingerprinting);
        ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), parent_url()));
        for (bool test_iframe : {false, true}) {
          content::RenderFrameHost* host = test_iframe ? Parent() : IFrame();
          // Allow for a 2px variance due to non-integer devicePixelRatio.
          EXPECT_EQ(
              !allow_fingerprinting && !IsFlagDisabled(),
              EvalJs(host,
                     "matchMedia(`(min-device-width: ${outerWidth - 2}px) and "
                     "(max-device-width: ${outerWidth + 2}px)`).matches"));
          EXPECT_EQ(
              !allow_fingerprinting && !IsFlagDisabled(),
              EvalJs(
                  host,
                  "matchMedia(`(min-device-height: ${outerHeight - 2}px) and "
                  "(max-device-height: ${outerHeight + 2}px)`).matches"));
        }
      }
    }
  }

  // This method checks the screen farbling behaviour for blob:// based URLs. It
  // does so by opening a blob:// window which reads the various screen
  // attribtues and then compares with the parent window. This comparison is
  // spliced on fingerinting being "allowed" and "blocked".
  void FarbleScreenBlobURL() {
    // Read the "real" blob values while fingerprinting is set to "allowed" as
    // a baseline for the blob checks below.
    SetFingerprintingSetting(/*allow=*/true);
    NavigateToBlob("real_blob");
    const ScreenValues real_blob = GetStoredScreenValues("real_blob");

    // Fingerprinting blocked.
    SetFingerprintingSetting(/*allow=*/false);
    NavigateToBlob("blob");
    const ScreenValues blob = GetStoredScreenValues("blob");

    // We check the screen farbling similar to how FarbleScreenSize tests.
    ExpectScreenSizeFarbling(Parent(), !IsFlagDisabled(),
                             browser()->window()->GetBounds().size(),
                             "window.innerWidth", "window.innerHeight");

    // TODO(https://github.com/brave/brave-browser/issues/56048): Blob aren't
    // getting farbled. Update these tests to EXPECT_NE once the support is
    // added.
    if (!IsFlagDisabled()) {
      EXPECT_EQ(real_blob.width, blob.width);
      EXPECT_EQ(real_blob.height, blob.height);
      EXPECT_EQ(real_blob.avail_width, blob.avail_width);
      EXPECT_EQ(real_blob.avail_height, blob.avail_height);
      EXPECT_EQ(real_blob.avail_left, blob.avail_left);
      EXPECT_EQ(real_blob.avail_top, blob.avail_top);
    }
  }

  // Navigates to a page which opens a blob:// popup window and writes the
  // various screen attributes to local storage.
  void NavigateToBlob(const std::string& prefix) {
    const GURL url =
        embedded_test_server()->GetURL("a.com", "/blob-fingerprinting.html");
    ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
    Browser* popup = OpenPopup(
        content::JsReplace("startBlobScreenFingerprintingTest($1)", prefix),
        /*from_iframe=*/true);
    auto* popup_contents = popup->tab_strip_model()->GetActiveWebContents();
    const std::u16string expected_title(u"blob-screen-values-written");
    TitleWatcher watcher(popup_contents, expected_title);
    EXPECT_EQ(expected_title, watcher.WaitAndGetTitle());
    // Close the popup but the parent window is still alive allowing us to
    // query the "shared" local storage.
    CloseBrowserSynchronously(popup);
  }

  ScreenValues GetStoredScreenValues(const std::string& prefix) {
    const content::EvalJsResult result = EvalJs(
        Parent(), content::JsReplace("readStoredScreenValues($1)", prefix));
    const base::DictValue& values = result.ExtractDict();
    return ScreenValuesFromDict(values);
  }

  ScreenValues ScreenValuesFromDict(const base::DictValue& values) {
    return {*values.FindInt("width"),      *values.FindInt("height"),
            *values.FindInt("availWidth"), *values.FindInt("availHeight"),
            *values.FindInt("availLeft"),  *values.FindInt("availTop"),
            *values.FindInt("screenX"),    *values.FindInt("screenY")};
  }

  enum class TestMode { kIframe, kWindowSize, kWindowPosition };

  void FarbleScreenPopupPosition(const gfx::Rect& bounds) {
    const gfx::Rect parent_bounds = SetBounds(bounds);
    for (bool allow_fingerprinting : {false, true}) {
      SetFingerprintingSetting(allow_fingerprinting);
      ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), parent_url()));
      for (TestMode test_mode : {TestMode::kIframe, TestMode::kWindowSize,
                                 TestMode::kWindowPosition}) {
        const char* script =
            "open('/simple.html', '', `"
            "left=30,"
            "top=30,"
            "width=${outerWidth + 20},"
            "height=${outerHeight + 20}"
            "`);";
        content::RenderFrameHost* host =
            test_mode == TestMode::kIframe ? Parent() : IFrame();
        Browser* popup = OpenPopup(script, test_mode == TestMode::kIframe);
        auto* popup_contents = popup->tab_strip_model()->GetActiveWebContents();
        content::WaitForLoadStop(popup_contents);
        gfx::Rect child_bounds = popup->window()->GetBounds();
        if (!allow_fingerprinting && !IsFlagDisabled()) {
          EXPECT_GE(child_bounds.x(), parent_bounds.x());
          EXPECT_GE(child_bounds.y(), parent_bounds.y());
        } else {
          EXPECT_LE(child_bounds.x(), std::max(80, 10 + parent_bounds.x()));
          EXPECT_LE(child_bounds.y(), std::max(80, 10 + parent_bounds.y()));
        }
        EXPECT_GE(EvalJs(host, "screen.width"), child_bounds.width());
        EXPECT_GE(EvalJs(host, "screen.height"), child_bounds.height());
        if (test_mode != TestMode::kIframe) {
          auto* widget = views::Widget::GetWidgetForNativeWindow(
              popup->window()->GetNativeWindow());
          auto bounds_before = popup->window()->GetBounds();
          auto waiter = WidgetBoundsChangeWaiter(widget, 10);
          if (test_mode == TestMode::kWindowSize) {
            ASSERT_TRUE(ExecJs(popup_contents,
                               "resizeTo(outerWidth - 13, outerHeight - 14)"));
          } else {  // test_mode == TestMode::kWindowPosition
            ASSERT_TRUE(
                ExecJs(popup_contents, "moveTo(screenX + 11, screenY + 12)"));
          }
          waiter.Wait();
          auto bounds_after = popup->window()->GetBounds();
          // Allow for a 2px variance due to non-integer devicePixelRatio.
          if (test_mode == TestMode::kWindowSize) {
            EXPECT_NEAR(bounds_after.width() - bounds_before.width(), -13, 2);
            EXPECT_NEAR(bounds_after.height() - bounds_before.height(), -14, 2);
          } else {  // test_mode == TestMode::kWindowPosition
            EXPECT_NEAR(bounds_after.x() - bounds_before.x(), 11, 2);
            EXPECT_NEAR(bounds_after.y() - bounds_before.y(), 12, 2);
          }
        }
      }
    }
  }

  GURL& parent_url() { return parent_url_; }

 protected:
  base::test::ScopedFeatureList feature_list_;

 private:
  GURL parent_url_;
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
  FarbleScreenSize(parent_url(), true);
}

IN_PROC_BROWSER_TEST_F(BraveScreenFarblingBrowserTest_DisableFlag,
                       FarbleScreenSize_DisableFlag) {
  FarbleScreenSize(parent_url(), true);
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

IN_PROC_BROWSER_TEST_F(BraveScreenFarblingBrowserTest_EnableFlag,
                       FarbleScreenBlobURL_EnableFlag) {
  FarbleScreenBlobURL();
}

IN_PROC_BROWSER_TEST_F(BraveScreenFarblingBrowserTest_DisableFlag,
                       FarbleScreenBlobURL_DisableFlag) {
  FarbleScreenBlobURL();
}

// Run each window size as a separate test because on linux
// the browser window does not properly resized within
// a single test.

IN_PROC_BROWSER_TEST_F(BraveScreenFarblingBrowserTest_EnableFlag,
                       FarbleScreenPopupPosition_EnableFlag_0) {
  FarbleScreenPopupPosition(kTestWindowBounds[0]);
}

IN_PROC_BROWSER_TEST_F(BraveScreenFarblingBrowserTest_DisableFlag,
                       FarbleScreenPopupPosition_DisableFlag_0) {
  FarbleScreenPopupPosition(kTestWindowBounds[0]);
}

IN_PROC_BROWSER_TEST_F(BraveScreenFarblingBrowserTest_EnableFlag,
                       FarbleScreenPopupPosition_EnableFlag_1) {
  FarbleScreenPopupPosition(kTestWindowBounds[1]);
}

IN_PROC_BROWSER_TEST_F(BraveScreenFarblingBrowserTest_DisableFlag,
                       FarbleScreenPopupPosition_DisableFlag_1) {
  FarbleScreenPopupPosition(kTestWindowBounds[1]);
}

IN_PROC_BROWSER_TEST_F(BraveScreenFarblingBrowserTest_EnableFlag,
                       FarbleScreenPopupPosition_EnableFlag_2) {
  FarbleScreenPopupPosition(kTestWindowBounds[2]);
}

IN_PROC_BROWSER_TEST_F(BraveScreenFarblingBrowserTest_DisableFlag,
                       FarbleScreenPopupPosition_DisableFlag_2) {
  FarbleScreenPopupPosition(kTestWindowBounds[2]);
}

IN_PROC_BROWSER_TEST_F(BraveScreenFarblingBrowserTest_EnableFlag,
                       FarbleScreenPopupPosition_EnableFlag_3) {
  FarbleScreenPopupPosition(kTestWindowBounds[3]);
}

IN_PROC_BROWSER_TEST_F(BraveScreenFarblingBrowserTest_DisableFlag,
                       FarbleScreenPopupPosition_DisableFlag_3) {
  FarbleScreenPopupPosition(kTestWindowBounds[3]);
}

IN_PROC_BROWSER_TEST_F(BraveScreenFarblingBrowserTest_EnableFlag,
                       FarbleScreenSize_Schemes) {
  // chrome: URI (don't farble)
  FarbleScreenSize(GURL("chrome:version"), false);

  // chrome-extension: URI (don't farble)
  extensions::TestExtensionDir test_extension_dir;
  test_extension_dir.WriteManifest(R"({
    "name": "Browser Action Popup",
    "manifest_version": 3,
    "version": "0.1",
    "action": { "default_popup": "popup.html" }
  })");
  test_extension_dir.WriteFile(FILE_PATH_LITERAL("popup.html"),
                               "<!doctype html><html>This is a popup</html>");
  std::string extension_id = LoadExtension(test_extension_dir.UnpackedPath());
  base::RunLoop().RunUntilIdle();  // Ensure the extension is fully loaded.
  const GURL extension_url("chrome-extension://" + extension_id +
                           "/popup.html");
  FarbleScreenSize(extension_url, false);

  // devtools: URI (don't farble)
  const GURL devtools_url("devtools://devtools/bundled/devtools_app.html");
  FarbleScreenSize(devtools_url, false);
}
