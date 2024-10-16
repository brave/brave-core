/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifdef UNSAFE_BUFFERS_BUILD
// TODO(https://github.com/brave/brave-browser/issues/41661): Remove this and
// convert code to safer constructs.
#pragma allow_unsafe_buffers
#endif

#include <algorithm>

#include "base/files/file_path.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_shields/content/browser/brave_shields_util.h"
#include "brave/components/constants/brave_paths.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/extensions/chrome_test_extension_loader.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/http_request.h"
#include "third_party/blink/public/common/features.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_observer.h"

using brave_shields::ControlType;

namespace {

const gfx::Rect kTestWindowBounds[] = {
    gfx::Rect(200, 100, 300, 200), gfx::Rect(50, 50, 200, 200),
    gfx::Rect(50, 50, 475, 460), gfx::Rect(0, 0, 200, 200)};

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

  void FarbleScreenSize(const GURL& url, bool content_scheme) {
    for (int j = 0; j < static_cast<int>(std::size(kTestWindowBounds)); ++j) {
      SetBounds(kTestWindowBounds[j]);
      for (bool allow_fingerprinting : {false, true}) {
        SetFingerprintingSetting(allow_fingerprinting);
        ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
        for (bool test_iframe : {false, true}) {
          if (!content_scheme) {
            continue;
          }
          content::RenderFrameHost* host = test_iframe ? Parent() : IFrame();
          if (!allow_fingerprinting && !IsFlagDisabled() && content_scheme) {
            EXPECT_GE(8, EvalJs(host, "window.outerWidth - parent.innerWidth"));
            EXPECT_GE(8,
                      EvalJs(host, "window.outerHeight - parent.innerHeight"));
            EXPECT_GE(8, EvalJs(host,
                                "window.screen.availWidth - Math.max(450, "
                                "parent.innerWidth)"));
            EXPECT_GE(8, EvalJs(host,
                                "window.screen.availHeight -  Math.max(450, "
                                "parent.innerHeight)"));
            EXPECT_GE(
                8,
                EvalJs(
                    host,
                    "window.screen.width - Math.max(450, parent.innerWidth)"));
            EXPECT_GE(8, EvalJs(host,
                                "window.screen.height - Math.max(450, "
                                "parent.innerHeight)"));
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
        ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), parent_url()));
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
    parent_bounds = SetBounds(kTestWindowBounds[j]);
    for (bool allow_fingerprinting : {false, true}) {
      SetFingerprintingSetting(allow_fingerprinting);
      ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), parent_url()));
      for (bool test_iframe : {false, true}) {
        const char* script =
            "open('/simple.html', '', `"
            "left=30,"
            "top=30,"
            "width=${outerWidth + 20},"
            "height=${outerHeight + 20}"
            "`);";
        Browser* popup = OpenPopup(script, test_iframe);
        auto* popup_contents = popup->tab_strip_model()->GetActiveWebContents();
        content::WaitForLoadStop(popup_contents);
        gfx::Rect child_bounds = popup->window()->GetBounds();
        if (!allow_fingerprinting && !IsFlagDisabled()) {
          EXPECT_GE(child_bounds.x(), parent_bounds.x());
          EXPECT_GE(child_bounds.y(), parent_bounds.y());
          int maxWidth = 10 + std::max(450, parent_bounds.width());
          int maxHeight = 10 + std::max(450, parent_bounds.width());
          EXPECT_GE(maxWidth, child_bounds.width());
          EXPECT_GE(maxHeight, child_bounds.height());
        } else {
          EXPECT_LE(child_bounds.x(), std::max(80, 10 + parent_bounds.x()));
          EXPECT_LE(child_bounds.y(), std::max(80, 10 + parent_bounds.y()));
          EXPECT_LE(parent_bounds.width(), child_bounds.width());
          EXPECT_LE(parent_bounds.height(), child_bounds.height());
        }
        if (!test_iframe) {
          auto* widget = views::Widget::GetWidgetForNativeWindow(
              popup->window()->GetNativeWindow());

          auto bounds_before = popup->window()->GetBounds();
          auto waiter1 = WidgetBoundsChangeWaiter(widget, 10);
          ASSERT_TRUE(ExecJs(popup_contents,
                             "resizeTo(outerWidth - 13, outerHeight - 14)"));
          waiter1.Wait();
          auto bounds_after = popup->window()->GetBounds();
          EXPECT_EQ(-13, bounds_after.width() - bounds_before.width());
          EXPECT_EQ(-14, bounds_after.height() - bounds_before.height());

          bounds_before = popup->window()->GetBounds();
          auto waiter2 = WidgetBoundsChangeWaiter(widget, 10);
          ASSERT_TRUE(
              ExecJs(popup_contents, "moveTo(screenX + 11, screenY + 12)"));
          waiter2.Wait();
          bounds_after = popup->window()->GetBounds();
          EXPECT_EQ(11, bounds_after.x() - bounds_before.x());
          EXPECT_EQ(12, bounds_after.y() - bounds_before.y());
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
