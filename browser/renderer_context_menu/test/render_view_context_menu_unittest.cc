/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/renderer_context_menu/render_view_context_menu.h"

#include <optional>

#include "brave/app/brave_command_ids.h"
#include "chrome/browser/autocomplete/autocomplete_classifier_factory.h"
#include "chrome/browser/autocomplete/chrome_autocomplete_provider_client.h"
#include "chrome/browser/custom_handlers/protocol_handler_registry_factory.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "chrome/test/base/scoped_testing_local_state.h"
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

class BraveRenderViewContextMenuMock : public BraveRenderViewContextMenu {
  using BraveRenderViewContextMenu::BraveRenderViewContextMenu;
  void Show() override {}
};

class BraveRenderViewContextMenuTest : public testing::Test {
 protected:
  BraveRenderViewContextMenuTest()
      : testing_local_state_(TestingBrowserProcess::GetGlobal()) {}
  content::WebContents* GetWebContents() { return web_contents_.get(); }

  // Returns a test context menu.
  std::unique_ptr<BraveRenderViewContextMenuMock> CreateContextMenu(
      content::WebContents* web_contents,
      content::ContextMenuParams params) {
    auto menu = std::make_unique<BraveRenderViewContextMenuMock>(
        *web_contents->GetPrimaryMainFrame(), params);
    menu->Init();
    return menu;
  }

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
  void TearDown() override { registry_.reset(); }

 private:
  content::BrowserTaskEnvironment browser_task_environment;
  std::unique_ptr<TestingProfile> profile_;
  std::unique_ptr<custom_handlers::ProtocolHandlerRegistry> registry_;
  ScopedTestingLocalState testing_local_state_;
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

TEST_F(BraveRenderViewContextMenuTest, MenuForLink) {
  content::ContextMenuParams params =
      CreateLinkParams(GURL("https://brave.com"));
  auto context_menu = CreateContextMenu(GetWebContents(), params);
  EXPECT_TRUE(context_menu);
  std::optional<size_t> clean_link_index =
      context_menu->menu_model().GetIndexOfCommandId(IDC_COPY_CLEAN_LINK);
  EXPECT_TRUE(clean_link_index.has_value());
  EXPECT_TRUE(context_menu->IsCommandIdEnabled(IDC_COPY_CLEAN_LINK));
}
