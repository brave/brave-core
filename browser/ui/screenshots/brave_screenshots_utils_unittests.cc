// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <optional>

#include "base/test/scoped_feature_list.h"
#include "brave/app/brave_command_ids.h"
#include "brave/browser/ui/brave_ui_features.h"
#include "chrome/browser/autocomplete/autocomplete_classifier_factory.h"
#include "chrome/browser/renderer_context_menu/render_view_context_menu.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/scoped_testing_local_state.h"
#include "chrome/test/base/test_browser_window.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/browser/context_menu_params.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/clipboard/clipboard.h"

namespace {
content::ContextMenuParams CreateDefaultParams() {
  content::ContextMenuParams params;
  params.page_url = GURL("http://test.page/");
  return params;
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

  void SetUp() override {
    TestingProfile::Builder builder;

    builder.AddTestingFactory(
        TemplateURLServiceFactory::GetInstance(),
        base::BindRepeating(&TemplateURLServiceFactory::BuildInstanceFor));

    profile_ = builder.Build();

    AutocompleteClassifierFactory::GetInstance()->SetTestingFactoryAndUse(
        profile_.get(),
        base::BindRepeating(&AutocompleteClassifierFactory::BuildInstanceFor));

    web_contents_ = content::WebContents::Create(
        content::WebContents::CreateParams(profile_.get()));
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

  std::unique_ptr<BraveRenderViewContextMenuMock> CreateContextMenu() {
    content::ContextMenuParams params = CreateDefaultParams();
    auto menu = std::make_unique<BraveRenderViewContextMenuMock>(
        *web_contents_->GetPrimaryMainFrame(), params);

    Browser::CreateParams create_params(Browser::Type::TYPE_NORMAL,
                                        profile_.get(), true);
    auto test_window = std::make_unique<TestBrowserWindow>();
    create_params.window = test_window.get();
    browser_.reset(Browser::Create(create_params));
    menu->SetBrowser(browser_.get());

    menu->Init();
    return menu;
  }

  content::WebContents* GetWebContents() { return web_contents_.get(); }

 private:
  content::BrowserTaskEnvironment task_environment_;
  ScopedTestingLocalState testing_local_state_;
  std::unique_ptr<TestingProfile> profile_;
  std::unique_ptr<Browser> browser_;
  std::unique_ptr<content::WebContents> web_contents_;
};

TEST_F(BraveScreenshotsContextMenuTest, FeatureFlagControlsContextMenu) {
  base::test::ScopedFeatureList scoped_feature_list_;

  for (bool enabled : {true, false}) {
    scoped_feature_list_.InitWithFeatureState(features::kBraveScreenshots,
                                              enabled);

    auto context_menu = CreateContextMenu();
    EXPECT_TRUE(context_menu);

    std::optional<size_t> screenshot_tools_index =
        context_menu->menu_model().GetIndexOfCommandId(
            IDC_BRAVE_UTILS_SCREENSHOT_TOOLS);

    EXPECT_EQ(screenshot_tools_index.has_value(), enabled);

    if (screenshot_tools_index.has_value()) {
      EXPECT_EQ(
          context_menu->IsCommandIdEnabled(IDC_BRAVE_UTILS_SCREENSHOT_TOOLS),
          enabled);
    }

    scoped_feature_list_.Reset();
  }
}
