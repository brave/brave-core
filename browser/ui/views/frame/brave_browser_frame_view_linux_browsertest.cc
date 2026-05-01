/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <optional>

#include "base/memory/raw_ptr.h"
#include "base/memory/raw_ref.h"
#include "brave/browser/ui/browser_commands.h"
#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "brave/browser/ui/views/frame/brave_browser_frame_view_linux_native.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/themes/theme_service.h"
#include "chrome/browser/themes/theme_service_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/views/frame/browser_frame_view.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/frame/custom_corners_background.h"
#include "chrome/browser/ui/views/frame/top_container_view.h"
#include "chrome/browser/ui/views/toolbar/toolbar_view.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_test.h"
#include "ui/gfx/canvas.h"
#include "ui/linux/fake_linux_ui.h"
#include "ui/linux/linux_ui_getter.h"
#include "ui/linux/nav_button_provider.h"
#include "ui/linux/window_frame_provider.h"
#include "ui/native_theme/native_theme.h"
#include "ui/views/view_utils.h"

namespace {

// Minimal NavButtonProvider: returns empty images; RedrawImages is a no-op.
class FakeNavButtonProvider : public ui::NavButtonProvider {
 public:
  void RedrawImages(int, bool, bool) override {}
  gfx::ImageSkia GetImage(FrameButtonDisplayType, ButtonState) const override {
    return {};
  }
  gfx::Insets GetNavButtonMargin(FrameButtonDisplayType) const override {
    return {};
  }
  gfx::Insets GetTopAreaSpacing() const override { return {}; }
  int GetInterNavButtonSpacing() const override { return 0; }
};

// WindowFrameProvider that records the top_area_height argument of the most
// recent PaintWindowFrame call.  The crash (CairoSurface zero-height) occurs
// when top_area_height <= 0 reaches the real GTK provider; capturing the value
// lets the test verify the fix without requiring a real GTK environment.
class FakeWindowFrameProvider : public ui::WindowFrameProvider {
 public:
  int GetTopCornerRadiusDip() override { return 0; }
  bool IsTopFrameTranslucent() override { return false; }
  gfx::Insets GetFrameThicknessDip() override { return {}; }
  void PaintWindowFrame(gfx::Canvas*,
                        const gfx::Rect&,
                        int top_area_height,
                        bool,
                        const gfx::Insets&) override {
    last_top_area_height_ = top_area_height;
  }
  std::optional<int> last_top_area_height() const {
    return last_top_area_height_;
  }
  void reset() { last_top_area_height_.reset(); }

 private:
  std::optional<int> last_top_area_height_;
};

// LinuxUi that satisfies the factory's CreateNavButtonProvider() non-null
// requirement and routes GetWindowFrameProvider() to a fake we can inspect.
class FakeLinuxUiWithNavButtons : public ui::FakeLinuxUi {
 public:
  std::unique_ptr<ui::NavButtonProvider> CreateNavButtonProvider() override {
    return std::make_unique<FakeNavButtonProvider>();
  }
  ui::WindowFrameProvider* GetWindowFrameProvider(bool, bool, bool) override {
    return &frame_provider_;
  }
  // FakeLinuxUi returns nullptr; override to prevent null-dereference when
  // UseSystemTheme() triggers frame recreation on existing browser windows.
  ui::NativeTheme* GetNativeTheme() const override {
    return ui::NativeTheme::GetInstanceForNativeUi();
  }
  FakeWindowFrameProvider& frame_provider() { return frame_provider_; }

 private:
  FakeWindowFrameProvider frame_provider_;
};

// RAII wrapper that installs a LinuxUiGetter for the duration of a scope and
// restores the previous one on destruction, even if an assertion fires early.
class ScopedLinuxUiGetter {
 public:
  explicit ScopedLinuxUiGetter(ui::LinuxUiGetter* new_getter)
      : old_getter_(ui::LinuxUiGetter::instance()) {
    ui::LinuxUiGetter::set_instance(new_getter);
  }
  ~ScopedLinuxUiGetter() { ui::LinuxUiGetter::set_instance(old_getter_); }

 private:
  // Maybe null if no getter was installed before this scope.
  raw_ptr<ui::LinuxUiGetter> old_getter_ = nullptr;
};

// LinuxUiGetter that returns a fixed LinuxUiTheme for every window/profile.
class FakeLinuxUiGetter : public ui::LinuxUiGetter {
 public:
  explicit FakeLinuxUiGetter(ui::LinuxUiTheme& theme) : theme_(theme) {}
  ui::LinuxUiTheme* GetForWindow(aura::Window*) override { return &*theme_; }
  ui::LinuxUiTheme* GetForProfile(Profile*) override { return &*theme_; }

 private:
  const raw_ref<ui::LinuxUiTheme> theme_;
};

}  // namespace

class BraveBrowserFrameViewTest : public InProcessBrowserTest {
 protected:
  BrowserView* browser_view() {
    return BrowserView::GetBrowserViewForBrowser(browser());
  }

  BrowserFrameView* frame_view() {
    return browser_view()->browser_widget()->GetFrameView();
  }

  void ToggleVerticalTabStrip() {
    brave::ToggleVerticalTabStrip(browser());
    RunScheduledLayouts();
  }
};

// Verifies that PaintRestoredFrameBorder passes a positive top_area_height to
// the GTK frame provider when vertical tabs are active and no window title is
// shown.  The crash (CairoSurface zero-height assert) happens when
// top_area_height <= 0 reaches WindowFrameProviderGtk::PaintWindowFrame.
// The fix adds the toolbar height to guarantee a positive value; the fake
// provider captures the argument so we can assert it without needing real GTK.
IN_PROC_BROWSER_TEST_F(BraveBrowserFrameViewTest,
                       VerticalTabsGtkTopAreaHeightIsPositive) {
  FakeLinuxUiWithNavButtons fake_linux_ui;
  FakeLinuxUiGetter fake_getter(fake_linux_ui);
  ScopedLinuxUiGetter scoped_getter(&fake_getter);

  ThemeServiceFactory::GetForProfile(browser()->profile())->UseSystemTheme();

  Browser* gtk_browser = CreateBrowser(browser()->profile());
  auto* native_frame = views::AsViewClass<BraveBrowserFrameViewLinuxNative>(
      BrowserView::GetBrowserViewForBrowser(gtk_browser)
          ->browser_widget()
          ->GetFrameView());
  ASSERT_TRUE(native_frame) << "Expected BraveBrowserFrameViewLinuxNative; "
                               "fake GTK not picked up by the factory";

  // Enable vertical tabs with no title bar.  On Linux
  // kVerticalTabsShowTitleOnWindow defaults to true, so explicitly disable it.
  brave::ToggleVerticalTabStrip(gtk_browser);
  gtk_browser->profile()->GetPrefs()->SetBoolean(
      brave_tabs::kVerticalTabsShowTitleOnWindow, false);

  {
    gfx::Canvas canvas(native_frame->size(), 1.0f, false);
    native_frame->PaintRestoredFrameBorder(&canvas);
  }

  // Before the fix only GetTopAreaHeight() (shadow-only, tiny) was passed to
  // PaintWindowFrame; the fix adds the toolbar height.  Assert the captured
  // value is at least the toolbar height to confirm the addition happened.
  auto* toolbar = BrowserView::GetBrowserViewForBrowser(gtk_browser)->toolbar();
  ASSERT_TRUE(
      fake_linux_ui.frame_provider().last_top_area_height().has_value());
  EXPECT_GE(*fake_linux_ui.frame_provider().last_top_area_height(),
            toolbar->GetPreferredSize().height());

  // Close before the fake objects go out of scope; the frame view holds a raw
  // pointer to fake_linux_ui via the frame-provider getter.
  CloseBrowserSynchronously(gtk_browser);
}

// Verifies that with vertical tabs enabled and no title bar, both the toolbar
// and top container use the window corner radius so the frame's rounded arc
// shows through.
IN_PROC_BROWSER_TEST_F(
    BraveBrowserFrameViewTest,
    VerticalTabsTopAreaCornerRadiusReflectsWindowAndTitleBarState) {
  auto* toolbar_bg = static_cast<CustomCornersBackground*>(
      browser_view()->toolbar()->background());
  ASSERT_TRUE(toolbar_bg);
  auto* top_container_bg = static_cast<CustomCornersBackground*>(
      browser_view()->top_container()->background());
  ASSERT_TRUE(top_container_bg);

  // No title by default on linux.
  EXPECT_FALSE(browser()->profile()->GetPrefs()->GetBoolean(
      brave_tabs::kVerticalTabsShowTitleOnWindow));
  ToggleVerticalTabStrip();

  const auto window_corners = frame_view()->GetWindowRoundedCorners();
  const auto toolbar_radii = toolbar_bg->GetRoundedCornerRadii();
  ASSERT_TRUE(toolbar_radii.has_value());
  EXPECT_EQ(window_corners.upper_right(), toolbar_radii->upper_right());
  EXPECT_EQ(window_corners.upper_left(), toolbar_radii->upper_left());

  const auto tc_radii = top_container_bg->GetRoundedCornerRadii();
  ASSERT_TRUE(tc_radii.has_value());
  EXPECT_EQ(window_corners.upper_right(), tc_radii->upper_right());
  EXPECT_EQ(window_corners.upper_left(), tc_radii->upper_left());
}
