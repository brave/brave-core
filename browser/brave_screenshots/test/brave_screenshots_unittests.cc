/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <optional>

#include "base/test/scoped_feature_list.h"
#include "brave/app/brave_command_ids.h"
#include "brave/browser/brave_screenshots/features.h"
#include "chrome/browser/autocomplete/autocomplete_classifier_factory.h"
#include "chrome/browser/autocomplete/chrome_autocomplete_provider_client.h"
#include "chrome/browser/custom_handlers/protocol_handler_registry_factory.h"
#include "chrome/browser/renderer_context_menu/render_view_context_menu.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/scoped_testing_local_state.h"
#include "chrome/test/base/test_browser_window.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "components/custom_handlers/protocol_handler_registry.h"
#include "components/custom_handlers/test_protocol_handler_registry_delegate.h"
#include "components/search_engines/template_url_service.h"
#include "content/public/browser/context_menu_params.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/clipboard/clipboard.h"

namespace {

content::ContextMenuParams CreateNormalPageParams() {
  content::ContextMenuParams rv;
  rv.page_url = GURL("http://test.page/");
  return rv;
}

content::ContextMenuParams CreateDevToolsPageParams() {
  content::ContextMenuParams rv;
  rv.page_url = GURL("devtools://devtools/bundled/inspector.html");
  return rv;
}

}  // namespace

class BraveRenderViewContextMenuMock : public BraveRenderViewContextMenu {
 public:
  using BraveRenderViewContextMenu::BraveRenderViewContextMenu;

  void Show() override {}
  void SetBrowser(Browser* browser) { browser_ = browser; }

  Browser* GetBrowser() const override {
    if (browser_) {
      return browser_;
    }
    return BraveRenderViewContextMenu::GetBrowser();
  }

 private:
  raw_ptr<Browser> browser_ = nullptr;
};

class BraveScreenshotsContextMenuTest : public testing::Test {
 protected:
  BraveScreenshotsContextMenuTest()
      : testing_local_state_(TestingBrowserProcess::GetGlobal()) {}

  content::WebContents* GetWebContents() { return web_contents_.get(); }

  // Returns a test context menu.
  std::unique_ptr<BraveRenderViewContextMenuMock> CreateContextMenu(
      content::WebContents* web_contents,
      content::ContextMenuParams params,
      bool is_pwa_browser = false) {
    auto menu = std::make_unique<BraveRenderViewContextMenuMock>(
        *web_contents->GetPrimaryMainFrame(), params);

    Browser::CreateParams create_params(
        is_pwa_browser ? Browser::Type::TYPE_APP : Browser::Type::TYPE_NORMAL,
        profile_.get(), true);
    auto test_window = std::make_unique<TestBrowserWindow>();
    create_params.window = test_window.get();
    browser_.reset(Browser::Create(create_params));
    menu->SetBrowser(browser_.get());

    menu->Init();
    return menu;
  }

  void SetUp() override {
    TestingProfile::Builder builder;
    profile_ = builder.Build();
    web_contents_ = content::WebContents::Create(
        content::WebContents::CreateParams(profile_.get()));
  }

  void SetBraveScreenshotsFeatureState(bool enabled) {
    features_.Reset();
    features_.InitWithFeatureState(
        brave_screenshots::features::kBraveScreenshots, enabled);
  }

  void TearDown() override {
    web_contents_.reset();
    browser_.reset();
    profile_.reset();

    // We run into a DCHECK on Windows. The scenario is addressed explicitly
    // in Chromium's source for MessageWindow::WindowClass::~WindowClass().
    // See base/win/message_window.cc for more information.
    ui::Clipboard::DestroyClipboardForCurrentThread();
  }

  PrefService* GetPrefs() { return profile_->GetPrefs(); }

 private:
  content::BrowserTaskEnvironment browser_task_environment;
  ScopedTestingLocalState testing_local_state_;
  base::test::ScopedFeatureList features_;
  std::unique_ptr<TestingProfile> profile_;
  std::unique_ptr<Browser> browser_;
  std::unique_ptr<content::WebContents> web_contents_;
};

// We expect screenshot menu items to be present only when enabled
TEST_F(BraveScreenshotsContextMenuTest, MenuForWebPage) {
  content::ContextMenuParams params = CreateNormalPageParams();

  for (auto enabled : {true, false}) {
    SetBraveScreenshotsFeatureState(enabled);
    auto context_menu = CreateContextMenu(GetWebContents(), params);
    EXPECT_TRUE(context_menu);

    // Check for the main submenu label
    std::optional<size_t> index =
        context_menu->menu_model().GetIndexOfCommandId(
            IDC_BRAVE_SCREENSHOT_TOOLS);

    if (enabled) {
      EXPECT_TRUE(index.has_value());
      EXPECT_TRUE(context_menu->menu_model().GetSubmenuModelAt(*index));
    } else {
      EXPECT_FALSE(index.has_value());
    }
  }
}

// We expect all menu items to be absent within developer tools' context menu
TEST_F(BraveScreenshotsContextMenuTest, MenuForDevTools) {
  SetBraveScreenshotsFeatureState(true);
  content::ContextMenuParams params = CreateDevToolsPageParams();
  auto context_menu = CreateContextMenu(GetWebContents(), params, true);
  EXPECT_TRUE(context_menu);
  EXPECT_FALSE(context_menu->menu_model()
                   .GetIndexOfCommandId(IDC_BRAVE_SCREENSHOT_TOOLS)
                   .has_value());
}
