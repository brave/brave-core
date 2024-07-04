/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/omnibox/brave_omnibox_view_views.h"

#include "brave/browser/brave_browser_features.h"
#include "brave/browser/url_sanitizer/url_sanitizer_service_factory.h"
#include "brave/components/url_sanitizer/browser/url_sanitizer_service.h"
#include "build/build_config.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/location_bar/location_bar_view.h"
#include "chrome/browser/ui/views/omnibox/omnibox_view_views.h"
#include "chrome/browser/ui/views/toolbar/toolbar_view.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/search_test_utils.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/search_engines/template_url.h"
#include "components/search_engines/template_url_service.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "ui/base/clipboard/clipboard.h"
#include "ui/base/clipboard/scoped_clipboard_writer.h"
#include "ui/strings/grit/ui_strings.h"
#include "ui/views/controls/textfield/textfield.h"
#include "url/gurl.h"

namespace {

void SetClipboardText(ui::ClipboardBuffer buffer, const std::u16string& text) {
  ui::ScopedClipboardWriter(buffer).WriteText(text);
}

testing::AssertionResult VerifyTemplateURLServiceLoad(
    TemplateURLService* service) {
  if (service->loaded()) {
    return testing::AssertionSuccess();
  }
  search_test_utils::WaitForTemplateURLServiceToLoad(service);
  if (service->loaded()) {
    return testing::AssertionSuccess();
  }
  return testing::AssertionFailure() << "TemplateURLService isn't loaded";
}

}  // namespace

class BraveOmniboxViewViewsTest : public InProcessBrowserTest {
 public:
  LocationBarView* location_bar() {
    auto* browser_view = BrowserView::GetBrowserViewForBrowser(browser());
    return browser_view->toolbar()->location_bar();
  }
  OmniboxViewViews* omnibox_view() { return location_bar()->omnibox_view(); }

  void SetSanitizerRules(const std::string& matchers) {
    base::RunLoop loop;

    auto* url_sanitizer_service =
        brave::URLSanitizerServiceFactory::GetForBrowserContext(
            browser()->profile());
    url_sanitizer_service->SetInitializationCallbackForTesting(
        loop.QuitClosure());
    brave::URLSanitizerComponentInstaller::RawConfig config;
    config.matchers = matchers;
    auto* component_intaller =
        static_cast<brave::URLSanitizerComponentInstaller::Observer*>(
            url_sanitizer_service);
    component_intaller->OnConfigReady(config);

    loop.Run();
  }
};

class BraveOmniboxViewViewsEnabledFeatureTest
    : public BraveOmniboxViewViewsTest {
 public:
  BraveOmniboxViewViewsEnabledFeatureTest() {
    features_.InitWithFeatureState(features::kBraveCopyCleanLinkByDefault,
                                   true);
  }

 private:
  base::test::ScopedFeatureList features_;
};

class BraveOmniboxViewViewsDisabledFeatureTest
    : public BraveOmniboxViewViewsTest {
 public:
  BraveOmniboxViewViewsDisabledFeatureTest() {
    features_.InitWithFeatureState(features::kBraveCopyCleanLinkByDefault,
                                   false);
  }

 private:
  base::test::ScopedFeatureList features_;
};

IN_PROC_BROWSER_TEST_F(BraveOmniboxViewViewsTest, PasteAndSearchTest) {
  auto* brave_omnibox_view =
      static_cast<BraveOmniboxViewViews*>(omnibox_view());
  SetClipboardText(ui::ClipboardBuffer::kCopyPaste, u"Brave browser");
  EXPECT_TRUE(brave_omnibox_view->GetClipboardTextForPasteAndSearch());

  auto* service =
      TemplateURLServiceFactory::GetForProfile(browser()->profile());
  EXPECT_TRUE(VerifyTemplateURLServiceLoad(service));

  // Set custom search provider to normal profile.
  TemplateURLData test_data;
  test_data.SetShortName(u"test1");
  test_data.SetKeyword(u"test1.com");
  test_data.SetURL("https://test1.com/search?t={searchTerms}");
  std::unique_ptr<TemplateURL> test_url(new TemplateURL(test_data));
  service->SetUserSelectedDefaultSearchProvider(test_url.get());

  // Paste and search for normal window.
  brave_omnibox_view->ExecuteCommand(IDC_PASTE_AND_GO, ui::EF_NONE);
  TabStripModel* tab_strip = browser()->tab_strip_model();
  auto* active_web_contents = tab_strip->GetActiveWebContents();
  content::WaitForLoadStop(active_web_contents);

  // Check loaded url's host and search provider's url host are same in normal
  // window.
  EXPECT_EQ(active_web_contents->GetVisibleURL().host(),
            GURL(service->GetDefaultSearchProvider()->url()).host());

  // Create private window.
  Browser* private_browser = CreateIncognitoBrowser();
  auto* private_service =
      TemplateURLServiceFactory::GetForProfile(private_browser->profile());
  EXPECT_TRUE(VerifyTemplateURLServiceLoad(private_service));

  // Set custom search provider to private profile.
  TemplateURLData private_test_data;
  private_test_data.SetShortName(u"test2");
  private_test_data.SetKeyword(u"test2.com");
  private_test_data.SetURL("https://test2.com/search?t={searchTerms}");
  std::unique_ptr<TemplateURL> private_test_url(
      new TemplateURL(private_test_data));
  private_service->SetUserSelectedDefaultSearchProvider(private_test_url.get());

  auto* private_browser_view =
      BrowserView::GetBrowserViewForBrowser(private_browser);
  auto* private_brave_omnibox_view = static_cast<BraveOmniboxViewViews*>(
      private_browser_view->toolbar()->location_bar()->omnibox_view());

  SetClipboardText(ui::ClipboardBuffer::kCopyPaste, u"Brave browser");
  EXPECT_TRUE(private_brave_omnibox_view->GetClipboardTextForPasteAndSearch());

  // Paste and search for private window
  private_brave_omnibox_view->ExecuteCommand(IDC_PASTE_AND_GO, ui::EF_NONE);
  TabStripModel* private_tab_strip = private_browser->tab_strip_model();
  auto* private_active_web_contents = private_tab_strip->GetActiveWebContents();
  content::WaitForLoadStop(private_active_web_contents);

  // Check loaded url's host and search provider's url host are same in private
  // window.
  EXPECT_EQ(private_active_web_contents->GetVisibleURL().host(),
            GURL(private_service->GetDefaultSearchProvider()->url()).host());
}

// Load brave url and check copied url also has brave scheme.
IN_PROC_BROWSER_TEST_F(BraveOmniboxViewViewsTest,
                       CopyInternalURLToClipboardTest) {
  const std::string test_url("brave://version/");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), GURL(test_url)));

  omnibox_view()->SelectAll(true);
  omnibox_view()->ExecuteCommand(views::Textfield::kCopy, 0);
  ui::Clipboard* clipboard = ui::Clipboard::GetForCurrentThread();
  std::string text_from_clipboard;
  clipboard->ReadAsciiText(ui::ClipboardBuffer::kCopyPaste,
                           /* data_dst = */ nullptr, &text_from_clipboard);
  EXPECT_EQ(test_url, text_from_clipboard);

#if BUILDFLAG(IS_LINUX)
  clipboard->ReadAsciiText(ui::ClipboardBuffer::kSelection,
                           /* data_dst = */ nullptr, &text_from_clipboard);
  EXPECT_EQ(test_url, text_from_clipboard);
#endif
}

IN_PROC_BROWSER_TEST_F(BraveOmniboxViewViewsTest, CopyCleanURLToClipboardTest) {
  SetSanitizerRules(R"([
    { "include": [ "*://*/*"], "params": ["utm_content"] }
  ])");
  const std::string test_url(
      "https://dev-pages.bravesoftware.com/clean-urls/"
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
            "https://dev-pages.bravesoftware.com/clean-urls/"
            "?brave_testing1=foo&brave_testing2=bar&brave_testing3=keep&&;b&d&"
            "e=&f=g&=end");
}

IN_PROC_BROWSER_TEST_F(BraveOmniboxViewViewsTest, CopyURLToClipboardTest) {
  SetSanitizerRules(R"([
    { "include": [ "*://*/*"], "params": ["utm_content"] }
  ])");
  const std::string test_url(
      "https://dev-pages.bravesoftware.com/clean-urls/"
      "?brave_testing1=foo&brave_testing2=bar&brave_testing3=keep&"
      "d&utm_content=removethis&e=&f=g&=end");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), GURL(test_url)));

  omnibox_view()->SelectAll(true);
  omnibox_view()->ExecuteCommand(views::Textfield::kCopy, 0);
  ui::Clipboard* clipboard = ui::Clipboard::GetForCurrentThread();
  std::string text_from_clipboard;
  clipboard->ReadAsciiText(ui::ClipboardBuffer::kCopyPaste,
                           /* data_dst = */ nullptr, &text_from_clipboard);
  EXPECT_EQ(text_from_clipboard,
            "https://dev-pages.bravesoftware.com/clean-urls/"
            "?brave_testing1=foo&brave_testing2=bar&brave_testing3=keep&d&"
            "utm_content=removethis&e=&f=g&=end");
}

IN_PROC_BROWSER_TEST_F(BraveOmniboxViewViewsEnabledFeatureTest,
                       CopyCleanedURLToClipboardByHotkey) {
  SetSanitizerRules(R"([
    { "include": [ "*://*/*"], "params": ["utm_content"] }
  ])");
  const std::string test_url(
      "https://dev-pages.bravesoftware.com/clean-urls/"
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
            "https://dev-pages.bravesoftware.com/clean-urls/"
            "?brave_testing1=foo&brave_testing2=bar&brave_testing3=keep&&;b&d&"
            "e=&f=g&=end");
}

IN_PROC_BROWSER_TEST_F(BraveOmniboxViewViewsTest, DoNotSanitizeInternalURLS) {
  const std::string test_url("brave://settings/?utm_ad=1");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), GURL(test_url)));
  SetSanitizerRules(R"([
    { "include": [ "*://*/*"], "params": ["utm_ad"] }
  ])");
  base::RunLoop().RunUntilIdle();

  omnibox_view()->SelectAll(true);
  omnibox_view()->ExecuteCommand(views::Textfield::kCopy, 0);
  ui::Clipboard* clipboard = ui::Clipboard::GetForCurrentThread();
  std::string text_from_clipboard;
  clipboard->ReadAsciiText(ui::ClipboardBuffer::kCopyPaste,
                           /* data_dst = */ nullptr, &text_from_clipboard);
  EXPECT_EQ(text_from_clipboard, "brave://settings/?utm_ad=1");
}

IN_PROC_BROWSER_TEST_F(BraveOmniboxViewViewsDisabledFeatureTest,
                       CopyCleanedURLToClipboardByHotkey) {
  SetSanitizerRules(R"([
    { "include": [ "*://*/*"], "params": ["utm_content"] }
  ])");
  const std::string test_url(
      "https://dev-pages.bravesoftware.com/clean-urls/"
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
            "https://dev-pages.bravesoftware.com/clean-urls/"
            "?brave_testing1=foo&brave_testing2=bar&brave_testing3=keep&&;b&d&"
            "utm_content=removethis&e=&f=g&=end");
}

IN_PROC_BROWSER_TEST_F(BraveOmniboxViewViewsTest, CopyTextToClipboardByHotkey) {
  SetSanitizerRules(R"([
    { "include": [ "*://*/*"], "params": ["utm_content"] }
  ])");
  const std::string test_text(
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
