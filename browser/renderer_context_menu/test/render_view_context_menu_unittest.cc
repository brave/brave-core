/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/renderer_context_menu/render_view_context_menu.h"

#include <optional>
#include <string>
#include <vector>

#include "base/check.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/run_until.h"
#include "brave/app/brave_command_ids.h"
#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "brave/components/query_filter/browser/test_support/query_filter_test_helper.h"
#include "build/build_config.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/autocomplete/autocomplete_classifier_factory.h"
#include "chrome/browser/autocomplete/chrome_autocomplete_provider_client.h"
#include "chrome/browser/custom_handlers/protocol_handler_registry_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "chrome/browser/ui/browser_window/public/create_browser_window.h"
#include "chrome/test/base/search_test_utils.h"
#include "chrome/test/base/test_browser_window.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "components/custom_handlers/protocol_handler_registry.h"
#include "components/custom_handlers/test_protocol_handler_registry_delegate.h"
#include "components/search_engines/default_search_manager.h"
#include "components/search_engines/search_engines_pref_names.h"
#include "components/search_engines/template_url.h"
#include "components/search_engines/template_url_data.h"
#include "components/search_engines/template_url_service.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/browser/context_menu_params.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_delegate.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/clipboard/clipboard.h"
#include "ui/base/models/menu_model.h"

#if BUILDFLAG(ENABLE_AI_CHAT)
#include "brave/components/ai_chat/core/common/pref_names.h"
#endif

namespace {

content::ContextMenuParams CreateSelectedTextParams(
    const std::u16string& selected_text) {
  content::ContextMenuParams rv;
  rv.is_editable = false;
  rv.page_url = GURL("http://test.page/");
  rv.selection_text = selected_text;
  return rv;
}

content::ContextMenuParams CreateLinkParams(const GURL& selected_link) {
  content::ContextMenuParams rv;
  rv.is_editable = false;
  rv.unfiltered_link_url = selected_link;
  rv.page_url = GURL("http://test.page/");
  rv.link_url = selected_link;
  return rv;
}

std::unique_ptr<KeyedService> BuildProtocolHandlerRegistry(
    content::BrowserContext* context) {
  Profile* profile = Profile::FromBrowserContext(context);
  return std::make_unique<custom_handlers::ProtocolHandlerRegistry>(
      profile->GetPrefs(),
      std::make_unique<custom_handlers::TestProtocolHandlerRegistryDelegate>());
}

}  // namespace

class BraveRenderViewContextMenuMock : public RenderViewContextMenu {
 public:
  using RenderViewContextMenu::RenderViewContextMenu;

  void Show() override {}

  void SetBrowser(BrowserWindowInterface* browser) { browser_ = browser; }

  BrowserWindowInterface* GetBrowser() const override {
    if (browser_) {
      return browser_;
    }
    return RenderViewContextMenu::GetBrowser();
  }

 private:
  raw_ptr<BrowserWindowInterface> browser_ = nullptr;
};

class BraveRenderViewContextMenuTest : public testing::Test {
 protected:
  BraveRenderViewContextMenuTest() = default;
  content::WebContents* GetWebContents() { return web_contents_.get(); }

  // Returns a test context menu.
  std::unique_ptr<BraveRenderViewContextMenuMock> CreateContextMenu(
      content::WebContents* web_contents,
      content::ContextMenuParams params,
      bool is_pwa_browser = false) {
    ResetBrowser();
    auto menu = std::make_unique<BraveRenderViewContextMenuMock>(
        *web_contents->GetPrimaryMainFrame(), params,
        /*is_paste_enabled=*/false, /*is_paste_and_match_style_enabled=*/false);

    BrowserWindowCreateParams create_params(
        is_pwa_browser ? BrowserWindowInterface::Type::TYPE_APP
                       : BrowserWindowInterface::Type::TYPE_NORMAL,
        profile_.get(), true);
    auto browser_window = std::make_unique<TestBrowserWindow>();
    create_params.window = browser_window.release();
    browser_ =
        DeprecatedCreateOwnedBrowserWindowForTesting(std::move(create_params));
    menu->SetBrowser(browser_.get());

    menu->Init();
    return menu;
  }

  void ResetBrowser() { browser_.reset(); }

  void SetUp() override {
    TestingProfile::Builder builder;
    builder.AddTestingFactory(
        TemplateURLServiceFactory::GetInstance(),
        base::BindRepeating(&TemplateURLServiceFactory::BuildInstanceFor));
    profile_ = builder.Build();
    web_contents_ = content::WebContents::Create(
        content::WebContents::CreateParams(profile_.get()));
    auto* service = TemplateURLServiceFactory::GetForProfile(profile_.get());
    EXPECT_TRUE(service);
    client_ =
        std::make_unique<ChromeAutocompleteProviderClient>(profile_.get());
    registry_ = std::make_unique<custom_handlers::ProtocolHandlerRegistry>(
        profile_.get()->GetPrefs(), nullptr);
    AutocompleteClassifierFactory::GetInstance()->SetTestingFactoryAndUse(
        profile_.get(),
        base::BindRepeating(&AutocompleteClassifierFactory::BuildInstanceFor));
    ProtocolHandlerRegistryFactory::GetInstance()->SetTestingFactory(
        profile_.get(), base::BindRepeating(&BuildProtocolHandlerRegistry));
  }

  void TearDown() override {
    registry_.reset();
    web_contents_.reset();
    client_.reset();
    ResetBrowser();
    profile_.reset();

    // We run into a DCHECK on Windows. The scenario is addressed explicitly
    // in Chromium's source for MessageWindow::WindowClass::~WindowClass().
    // See base/win/message_window.cc for more information.
    ui::Clipboard::DestroyClipboardForCurrentThread();
  }

  PrefService* GetPrefs() { return profile_->GetPrefs(); }

 private:
  content::BrowserTaskEnvironment browser_task_environment;
  query_filter::test::ScopedTestingQueryFilterRules query_filter_rules_;
  std::unique_ptr<TestingProfile> profile_;
  std::unique_ptr<custom_handlers::ProtocolHandlerRegistry> registry_;
  std::unique_ptr<BrowserWindowInterface> browser_;
  std::unique_ptr<ChromeAutocompleteProviderClient> client_;
  std::unique_ptr<content::WebContents> web_contents_;
};

TEST_F(BraveRenderViewContextMenuTest, MenuForPlainText) {
  content::ContextMenuParams params = CreateSelectedTextParams(u"plain text");
  auto context_menu = CreateContextMenu(GetWebContents(), params);
  EXPECT_TRUE(context_menu);
  std::optional<size_t> clean_link_index =
      context_menu->menu_model().GetIndexOfCommandId(IDC_COPY_CLEAN_LINK);
  EXPECT_FALSE(clean_link_index.has_value());
}

TEST_F(BraveRenderViewContextMenuTest, MenuForSelectedUrl) {
  content::ContextMenuParams params = CreateSelectedTextParams(u"brave.com");
  auto context_menu = CreateContextMenu(GetWebContents(), params);
  EXPECT_TRUE(context_menu);
  std::optional<size_t> clean_link_index =
      context_menu->menu_model().GetIndexOfCommandId(IDC_COPY_CLEAN_LINK);
  EXPECT_TRUE(clean_link_index.has_value());
  EXPECT_TRUE(context_menu->IsCommandIdEnabled(IDC_COPY_CLEAN_LINK));
}

TEST_F(BraveRenderViewContextMenuTest, MenuForLinkWithTrackingParams) {
  content::ContextMenuParams params =
      CreateLinkParams(GURL("https://brave.com/?fbclid=123&foo=bar"));
  auto context_menu = CreateContextMenu(GetWebContents(), params);
  EXPECT_TRUE(context_menu);
  std::optional<size_t> clean_link_index =
      context_menu->menu_model().GetIndexOfCommandId(IDC_COPY_CLEAN_LINK);
  EXPECT_TRUE(clean_link_index.has_value());
  EXPECT_TRUE(context_menu->IsCommandIdEnabled(IDC_COPY_CLEAN_LINK));
}

TEST_F(BraveRenderViewContextMenuTest, MenuForLink) {
  // A link which is already clean shouldn't get the "Copy clean link" item.
  content::ContextMenuParams params =
      CreateLinkParams(GURL("https://brave.com/?foo=bar"));
  auto context_menu = CreateContextMenu(GetWebContents(), params);
  EXPECT_TRUE(context_menu);
  std::optional<size_t> clean_link_index =
      context_menu->menu_model().GetIndexOfCommandId(IDC_COPY_CLEAN_LINK);
  EXPECT_FALSE(clean_link_index.has_value());

#if !BUILDFLAG(IS_ANDROID)
  // Split view item should be last in first section (right before first
  // separator).
  std::optional<size_t> split_view_index =
      context_menu->menu_model().GetIndexOfCommandId(
          IDC_CONTENT_CONTEXT_OPENLINKSPLITVIEW);
  ASSERT_TRUE(split_view_index.has_value())
      << "Open link in split view should be present for link menu on desktop";
  std::optional<size_t> first_separator_index;
  for (size_t i = 0; i < context_menu->menu_model().GetItemCount(); ++i) {
    if (context_menu->menu_model().GetTypeAt(i) ==
        ui::MenuModel::TYPE_SEPARATOR) {
      first_separator_index = i;
      break;
    }
  }
  ASSERT_TRUE(first_separator_index.has_value())
      << "Menu should have at least one separator";
  EXPECT_EQ(*split_view_index, *first_separator_index - 1)
      << "Open link in split view should be right before the first separator";
#endif
}

#if BUILDFLAG(ENABLE_AI_CHAT)
TEST_F(BraveRenderViewContextMenuTest, MenuForAIChat) {
  content::ContextMenuParams params = CreateSelectedTextParams(u"hello");

  for (auto enabled : {true, false}) {
    GetPrefs()->SetBoolean(ai_chat::prefs::kBraveAIChatContextMenuEnabled,
                           enabled);
    auto context_menu = CreateContextMenu(GetWebContents(), params);
    EXPECT_TRUE(context_menu);
    std::optional<size_t> ai_chat_index =
        context_menu->menu_model().GetIndexOfCommandId(
            IDC_AI_CHAT_CONTEXT_LEO_TOOLS);
    EXPECT_EQ(ai_chat_index.has_value(), enabled);
    EXPECT_EQ(context_menu->IsCommandIdEnabled(IDC_AI_CHAT_CONTEXT_LEO_TOOLS),
              enabled);
  }
}

TEST_F(BraveRenderViewContextMenuTest, MenuForAIChat_PWA) {
  content::ContextMenuParams params = CreateSelectedTextParams(u"hello");

  GetPrefs()->SetBoolean(ai_chat::prefs::kBraveAIChatContextMenuEnabled, true);
  auto context_menu = CreateContextMenu(GetWebContents(), params,
                                        /*is_pwa_browser=*/true);
  EXPECT_TRUE(context_menu);
  std::optional<size_t> ai_chat_index =
      context_menu->menu_model().GetIndexOfCommandId(
          IDC_AI_CHAT_CONTEXT_LEO_TOOLS);
  EXPECT_FALSE(ai_chat_index.has_value());
}
#endif

namespace {

// Records URLs that the context menu asks the web contents to open.
class OpenedUrlRecorder : public content::WebContentsDelegate {
 public:
  content::WebContents* OpenURLFromTab(
      content::WebContents* source,
      const content::OpenURLParams& params,
      base::OnceCallback<void(content::NavigationHandle&)>
          navigation_handle_callback) override {
    urls_.push_back(params.url);
    return nullptr;
  }

  const std::vector<GURL>& urls() const { return urls_; }

 private:
  std::vector<GURL> urls_;
};

}  // namespace

// Tests the "Search for" and "Go to" items for selected text, in regular and
// off-the-record profiles, by checking the URL they open.
class BraveRenderViewContextMenuSelectionTest : public testing::Test {
 protected:
  static constexpr char16_t kRegularName[] = u"Regular Search";
  static constexpr char16_t kOtrName[] = u"Private Search";
  static constexpr char kRegularSearchUrl[] =
      "https://regular.example/search?q={searchTerms}";
  static constexpr char kOtrSearchUrl[] =
      "https://private.example/find?q={searchTerms}";

  void SetUp() override {
    TestingProfile::Builder builder;
    builder.AddTestingFactory(
        TemplateURLServiceFactory::GetInstance(),
        base::BindRepeating(&TemplateURLServiceFactory::BuildInstanceFor));
    profile_ = builder.Build();
    // Brave gives OTR profiles their own TemplateURLService.
    TestingProfile::Builder otr_builder;
    otr_builder.AddTestingFactory(
        TemplateURLServiceFactory::GetInstance(),
        base::BindRepeating(&TemplateURLServiceFactory::BuildInstanceFor));
    otr_profile_ = otr_builder.BuildOffTheRecord(
        profile_.get(), Profile::OTRProfileID::CreateUniqueForTesting());
    ASSERT_TRUE(otr_profile_->IsOffTheRecord());

    // Upstream classifies with the regular profile's classifier for both
    // profile types (the factory redirects OTR to the original).
    AutocompleteClassifierFactory::GetInstance()->SetTestingFactoryAndUse(
        profile_.get(),
        base::BindRepeating(&AutocompleteClassifierFactory::BuildInstanceFor));
    ProtocolHandlerRegistryFactory::GetInstance()->SetTestingFactory(
        profile_.get(), base::BindRepeating(&BuildProtocolHandlerRegistry));
    ProtocolHandlerRegistryFactory::GetInstance()->SetTestingFactory(
        otr_profile_, base::BindRepeating(&BuildProtocolHandlerRegistry));

    SetDefaultSearchProvider(profile_.get(), kRegularName, "regular.example",
                             kRegularSearchUrl);
    SetDefaultSearchProvider(otr_profile_, kOtrName, "private.example",
                             kOtrSearchUrl);
  }

  void TearDown() override {
    menus_.clear();
    web_contentses_.clear();
    otr_profile_ = nullptr;
    profile_.reset();
    ui::Clipboard::DestroyClipboardForCurrentThread();
  }

  static void SetDefaultSearchProvider(Profile* profile,
                                       const std::u16string& name,
                                       const std::string& keyword,
                                       const std::string& url) {
    auto* service = TemplateURLServiceFactory::GetForProfile(profile);
    ASSERT_TRUE(service);
    search_test_utils::WaitForTemplateURLServiceToLoad(service);
    TemplateURLData data;
    data.SetShortName(name);
    data.SetKeyword(base::UTF8ToUTF16(keyword));
    data.SetURL(url);
    const TemplateURL* provider =
        service->Add(std::make_unique<TemplateURL>(data));
    ASSERT_TRUE(provider);
    service->SetUserSelectedDefaultSearchProvider(
        const_cast<TemplateURL*>(provider));
    ASSERT_EQ(service->GetDefaultSearchProvider(), provider);
  }

  Profile* regular() { return profile_.get(); }
  Profile* otr() { return otr_profile_; }

  BraveRenderViewContextMenuMock* CreateMenu(Profile* profile,
                                             const std::u16string& selection) {
    web_contentses_.push_back(content::WebContents::Create(
        content::WebContents::CreateParams(profile)));
    content::WebContents* web_contents = web_contentses_.back().get();
    recorders_.push_back(std::make_unique<OpenedUrlRecorder>());
    web_contents->SetDelegate(recorders_.back().get());

    content::ContextMenuParams params = CreateSelectedTextParams(selection);
    params.properties[prefs::kDefaultSearchProviderContextMenuAccessAllowed] =
        "";
    auto menu = std::make_unique<BraveRenderViewContextMenuMock>(
        *web_contents->GetPrimaryMainFrame(), params,
        /*is_paste_enabled=*/false, /*is_paste_and_match_style_enabled=*/false);

    menu->Init();
    menus_.push_back(std::move(menu));
    return menus_.back().get();
  }

  static bool HasCommand(BraveRenderViewContextMenuMock* menu, int command) {
    return menu->menu_model().GetIndexOfCommandId(command).has_value();
  }

  static std::u16string LabelOf(BraveRenderViewContextMenuMock* menu,
                                int command) {
    return menu->menu_model().GetLabelAt(
        *menu->menu_model().GetIndexOfCommandId(command));
  }

  // Executes `command` and returns the URLs the menu asked to open.
  std::vector<GURL> Execute(BraveRenderViewContextMenuMock* menu, int command) {
    menu->ExecuteCommand(command, /*event_flags=*/0);
    for (size_t i = 0; i < menus_.size(); ++i) {
      if (menus_[i].get() == menu) {
        EXPECT_TRUE(base::test::RunUntil(
            [&] { return !recorders_[i]->urls().empty(); }));
        return recorders_[i]->urls();
      }
    }
    NOTREACHED();
  }

 private:
  content::BrowserTaskEnvironment browser_task_environment_;
  std::unique_ptr<TestingProfile> profile_;
  raw_ptr<TestingProfile> otr_profile_ = nullptr;
  std::vector<std::unique_ptr<content::WebContents>> web_contentses_;
  std::vector<std::unique_ptr<OpenedUrlRecorder>> recorders_;
  std::vector<std::unique_ptr<BraveRenderViewContextMenuMock>> menus_;
};

TEST_F(BraveRenderViewContextMenuSelectionTest, RegularPlainText) {
  auto* menu = CreateMenu(regular(), u"plain text");
  ASSERT_TRUE(HasCommand(menu, IDC_CONTENT_CONTEXT_SEARCHWEBFOR));
  EXPECT_FALSE(HasCommand(menu, IDC_CONTENT_CONTEXT_GOTOURL));
  EXPECT_NE(LabelOf(menu, IDC_CONTENT_CONTEXT_SEARCHWEBFOR)
                .find(std::u16string(kRegularName)),
            std::u16string::npos);
  EXPECT_THAT(Execute(menu, IDC_CONTENT_CONTEXT_SEARCHWEBFOR),
              testing::ElementsAre(GURL("https://regular.example/search?q="
                                        "plain+text")));
}

TEST_F(BraveRenderViewContextMenuSelectionTest, OtrPlainTextUsesOtrProvider) {
  auto* menu = CreateMenu(otr(), u"plain text");
  ASSERT_TRUE(HasCommand(menu, IDC_CONTENT_CONTEXT_SEARCHWEBFOR));
  EXPECT_FALSE(HasCommand(menu, IDC_CONTENT_CONTEXT_GOTOURL));
  EXPECT_NE(LabelOf(menu, IDC_CONTENT_CONTEXT_SEARCHWEBFOR)
                .find(std::u16string(kOtrName)),
            std::u16string::npos);
  EXPECT_THAT(
      Execute(menu, IDC_CONTENT_CONTEXT_SEARCHWEBFOR),
      testing::ElementsAre(GURL("https://private.example/find?q=plain+text")));
}

TEST_F(BraveRenderViewContextMenuSelectionTest, RegularUrlLike) {
  auto* menu = CreateMenu(regular(), u"example.com");
  ASSERT_TRUE(HasCommand(menu, IDC_CONTENT_CONTEXT_GOTOURL));
  EXPECT_FALSE(HasCommand(menu, IDC_CONTENT_CONTEXT_SEARCHWEBFOR));
  EXPECT_THAT(Execute(menu, IDC_CONTENT_CONTEXT_GOTOURL),
              testing::ElementsAre(GURL("http://example.com/")));
}

TEST_F(BraveRenderViewContextMenuSelectionTest, OtrUrlLike) {
  auto* menu = CreateMenu(otr(), u"example.com");
  ASSERT_TRUE(HasCommand(menu, IDC_CONTENT_CONTEXT_GOTOURL));
  EXPECT_FALSE(HasCommand(menu, IDC_CONTENT_CONTEXT_SEARCHWEBFOR));
  EXPECT_THAT(Execute(menu, IDC_CONTENT_CONTEXT_GOTOURL),
              testing::ElementsAre(GURL("http://example.com/")));
}

TEST_F(BraveRenderViewContextMenuSelectionTest, EmptyAndWhitespaceSelection) {
  for (Profile* profile : {regular(), otr()}) {
    for (const char16_t* selection : {u"", u"   ", u" \t\n "}) {
      auto* menu = CreateMenu(profile, selection);
      EXPECT_FALSE(HasCommand(menu, IDC_CONTENT_CONTEXT_SEARCHWEBFOR));
      EXPECT_FALSE(HasCommand(menu, IDC_CONTENT_CONTEXT_GOTOURL));
    }
  }
}

TEST_F(BraveRenderViewContextMenuSelectionTest, NoDefaultSearchProvider) {
  // The OTR profile shares the regular profile's prefs.
  base::DictValue disabled;
  disabled.Set(DefaultSearchManager::kDisabledByPolicy, true);
  static_cast<TestingProfile*>(regular())
      ->GetTestingPrefService()
      ->SetManagedPref(DefaultSearchManager::kDefaultSearchProviderDataPrefName,
                       std::move(disabled));

  for (Profile* profile : {regular(), otr()}) {
    ASSERT_FALSE(TemplateURLServiceFactory::GetForProfile(profile)
                     ->GetDefaultSearchProvider());
    auto* menu = CreateMenu(profile, u"plain text");
    EXPECT_FALSE(HasCommand(menu, IDC_CONTENT_CONTEXT_SEARCHWEBFOR));
    EXPECT_FALSE(HasCommand(menu, IDC_CONTENT_CONTEXT_GOTOURL));
  }
}
