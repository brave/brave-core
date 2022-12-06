/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/url_sanitizer/url_sanitizer_service_factory.h"
#include "brave/components/url_sanitizer/browser/url_sanitizer_service.h"
#include "build/build_config.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/location_bar/location_bar_view.h"
#include "chrome/browser/ui/views/omnibox/omnibox_view_views.h"
#include "chrome/browser/ui/views/toolbar/toolbar_view.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test.h"
#include "ui/base/clipboard/clipboard.h"
#include "ui/strings/grit/ui_strings.h"
#include "ui/views/controls/textfield/textfield.h"
#include "url/gurl.h"

class BraveOmniboxViewViewsTest : public InProcessBrowserTest {
 public:
  LocationBarView* location_bar() {
    auto* browser_view = BrowserView::GetBrowserViewForBrowser(browser());
    return browser_view->toolbar()->location_bar();
  }
  OmniboxViewViews* omnibox_view() { return location_bar()->omnibox_view(); }
};

// Load brave url and check copied url also has brave scheme.
IN_PROC_BROWSER_TEST_F(BraveOmniboxViewViewsTest, CopyURLToClipboardTest) {
  const std::string test_url("brave://version/");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), GURL(test_url)));

  omnibox_view()->SelectAll(true);
  omnibox_view()->ExecuteCommand(views::Textfield::kCopy, 0);
  ui::Clipboard* clipboard = ui::Clipboard::GetForCurrentThread();
  std::string text_from_clipboard;
  clipboard->ReadAsciiText(ui::ClipboardBuffer::kCopyPaste,
                           /* data_dst = */ nullptr,
                           &text_from_clipboard);
  EXPECT_EQ(test_url, text_from_clipboard);

#if BUILDFLAG(IS_LINUX)
  clipboard->ReadAsciiText(ui::ClipboardBuffer::kSelection,
                           /* data_dst = */ nullptr,
                           &text_from_clipboard);
  EXPECT_EQ(test_url, text_from_clipboard);
#endif
}

IN_PROC_BROWSER_TEST_F(BraveOmniboxViewViewsTest, CopyCleanURLToClipboardTest) {
  brave::URLSanitizerServiceFactory::GetForBrowserContext(browser()->profile())
      ->Initialize(R"([
    { "include": [ "*://*/*"], "params": ["utm_content"] }
  ])");
  const std::string test_url(
      "https://dev-pages.bravesoftware.com/clean-urls/"
      "exempted/"
      "?brave_testing1=foo&brave_testing2=bar&brave_testing3=keep&&;b&"
      "d&utm_content=removethis&e=&f=g&=end");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), GURL(test_url)));

  omnibox_view()->SelectAll(true);
  omnibox_view()->ExecuteCommand(IDC_COPY_CLEAN_LINK, 0);
  ui::Clipboard* clipboard = ui::Clipboard::GetForCurrentThread();
  std::string text_from_clipboard;
  clipboard->ReadAsciiText(ui::ClipboardBuffer::kCopyPaste,
                           /* data_dst = */ nullptr, &text_from_clipboard);
  EXPECT_EQ(text_from_clipboard,
            "https://dev-pages.bravesoftware.com/clean-urls/exempted/"
            "?brave_testing1=foo&brave_testing2=bar&brave_testing3=keep&&;b&d&"
            "e=&f=g&=end");
}

IN_PROC_BROWSER_TEST_F(BraveOmniboxViewViewsTest, DoNotSanitizeInternalURLS) {
  const std::string test_url("brave://settings/?utm_ad=1");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), GURL(test_url)));
  brave::URLSanitizerServiceFactory::GetForBrowserContext(browser()->profile())
      ->Initialize(R"([
    { "include": [ "*://*/*"], "params": ["utm_ad"] }
  ])");
  base::RunLoop().RunUntilIdle();

  omnibox_view()->SelectAll(true);
  omnibox_view()->ExecuteCommand(IDC_COPY_CLEAN_LINK, 0);
  ui::Clipboard* clipboard = ui::Clipboard::GetForCurrentThread();
  std::string text_from_clipboard;
  clipboard->ReadAsciiText(ui::ClipboardBuffer::kCopyPaste,
                           /* data_dst = */ nullptr, &text_from_clipboard);
  EXPECT_EQ(text_from_clipboard, "brave://settings/?utm_ad=1");
}

IN_PROC_BROWSER_TEST_F(BraveOmniboxViewViewsTest,
                       CopyCleanedURLToClipboardByHotkey) {
  brave::URLSanitizerServiceFactory::GetForBrowserContext(browser()->profile())
      ->Initialize(R"([
    { "include": [ "*://*/*"], "params": ["utm_content"] }
  ])");
  const std::string test_url(
      "https://dev-pages.bravesoftware.com/clean-urls/"
      "exempted/"
      "?brave_testing1=foo&brave_testing2=bar&brave_testing3=keep&&;b&"
      "d&utm_content=removethis&e=&f=g&=end");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), GURL(test_url)));

  omnibox_view()->SelectAll(true);

  auto* textfield = static_cast<views::Textfield*>(omnibox_view());
  textfield->AcceleratorPressed(
      ui::Accelerator(ui::VKEY_C, ui::EF_PLATFORM_ACCELERATOR));
  ui::Clipboard* clipboard = ui::Clipboard::GetForCurrentThread();
  std::string text_from_clipboard;
  clipboard->ReadAsciiText(ui::ClipboardBuffer::kCopyPaste,
                           /* data_dst = */ nullptr, &text_from_clipboard);
  EXPECT_EQ(text_from_clipboard,
            "https://dev-pages.bravesoftware.com/clean-urls/exempted/"
            "?brave_testing1=foo&brave_testing2=bar&brave_testing3=keep&&;b&d&"
            "e=&f=g&=end");
}

IN_PROC_BROWSER_TEST_F(BraveOmniboxViewViewsTest, CopyTextToClipboardByHotkey) {
  brave::URLSanitizerServiceFactory::GetForBrowserContext(browser()->profile())
      ->Initialize(R"([
    { "include": [ "*://*/*"], "params": ["utm_content"] }
  ])");
  const std::string test_text(
      "exempted/"
      "?brave_testing1=foo&brave_testing2=bar&brave_testing3=keep&&;b&"
      "d&utm_content=removethis&e=&f=g&=end");
  auto* textfield = static_cast<views::Textfield*>(omnibox_view());
  textfield->SetText(base::UTF8ToUTF16(test_text));

  omnibox_view()->SelectAll(true);

  textfield->AcceleratorPressed(
      ui::Accelerator(ui::VKEY_C, ui::EF_PLATFORM_ACCELERATOR));
  ui::Clipboard* clipboard = ui::Clipboard::GetForCurrentThread();
  std::string text_from_clipboard;
  clipboard->ReadAsciiText(ui::ClipboardBuffer::kCopyPaste,
                           /* data_dst = */ nullptr, &text_from_clipboard);
  EXPECT_EQ(text_from_clipboard, test_text);
}
