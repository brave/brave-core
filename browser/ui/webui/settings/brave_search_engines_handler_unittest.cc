/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/settings/brave_search_engines_handler.h"

#include <memory>
#include <string>

#include "base/command_line.h"
#include "base/memory/raw_ptr.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "chrome/browser/regional_capabilities/regional_capabilities_service_factory.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "components/regional_capabilities/regional_capabilities_switches.h"
#include "components/search_engines/search_engines_pref_names.h"
#include "components/search_engines/template_url.h"
#include "components/search_engines/template_url_data.h"
#include "components/search_engines/template_url_service.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/test_web_contents_factory.h"
#include "content/public/test/test_web_ui.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace settings {

namespace {

// Adds an engine that appears in the "default" section of the table model.
TemplateURL* AddDefaultListEngine(TemplateURLService* service,
                                  const std::string& name,
                                  int prepopulate_id) {
  TemplateURLData data;
  data.SetShortName(base::UTF8ToUTF16(name));
  data.SetKeyword(base::UTF8ToUTF16(name));
  data.SetURL("https://" + name + "/search?q={searchTerms}");
  data.prepopulate_id = prepopulate_id;
  return service->Add(std::make_unique<TemplateURL>(data));
}

// Adds an engine like the one a user creates from the settings page: no
// prepopulate id, so it only shows up in the "default" section of the table
// model once it becomes the default search provider.
TemplateURL* AddUserDefinedEngine(TemplateURLService* service,
                                  const std::string& name) {
  TemplateURLData data;
  data.SetShortName(base::UTF8ToUTF16(name));
  data.SetKeyword(base::UTF8ToUTF16(name));
  data.SetURL("https://" + name + "/search?q={searchTerms}");
  data.is_active = TemplateURLData::ActiveStatus::kTrue;
  return service->Add(std::make_unique<TemplateURL>(data));
}

}  // namespace

class BraveSearchEnginesHandlerTest : public testing::Test {
 public:
  BraveSearchEnginesHandlerTest()
      : profile_manager_(TestingBrowserProcess::GetGlobal()) {}

  ~BraveSearchEnginesHandlerTest() override {
    handler_->set_web_ui(nullptr);
    handler_.reset();
  }

  void SetUp() override {
    testing::Test::SetUp();

    base::CommandLine::ForCurrentProcess()->AppendSwitchASCII(
        switches::kSearchEngineChoiceCountry, "US");

    ASSERT_TRUE(profile_manager_.SetUp());

    profile_ = profile_manager_.CreateTestingProfile("Profile 1");

    TemplateURLServiceFactory::GetInstance()->SetTestingFactoryAndUse(
        profile_,
        base::BindRepeating(&TemplateURLServiceFactory::BuildInstanceFor));

    template_url_service_ = TemplateURLServiceFactory::GetForProfile(profile_);

    // Add two engines that will appear in the "defaults" section of the table
    // model (prepopulate_id > 0 required by ShowInDefaultList). Use IDs high
    // enough to avoid collisions with real prepopulated engines (~1-300).
    //
    // We deliberately do NOT call SetUserSelectedDefaultSearchProvider here
    // because that can cause the service to recreate the TemplateURL object
    // internally, invalidating the raw pointer returned by Add().
    TemplateURL* a =
        AddDefaultListEngine(template_url_service_, "engine_a", 9001);
    TemplateURL* b =
        AddDefaultListEngine(template_url_service_, "engine_b", 9002);
    ASSERT_TRUE(a && b);

    // Save GUIDs immediately; the service owns the objects and ID-based
    // lookup later uses the map built from GetTemplateURLs().
    engine_a_guid_ = a->sync_guid();
    engine_b_guid_ = b->sync_guid();
    engine_a_id_ = a->id();
    engine_b_id_ = b->id();

    // engine_b is the private window default; engine_a is not.
    // The normal window default is whatever the service loaded from
    // prepopulated data - neither of the two engines we just added.
    profile_->GetPrefs()->SetString(
        prefs::kSyncedDefaultPrivateSearchProviderGUID, engine_b_guid_);

    auto* regional_capabilities = regional_capabilities::
        RegionalCapabilitiesServiceFactory::GetForProfile(profile_);

    web_ui_ = std::make_unique<content::TestWebUI>();
    web_ui_->set_web_contents(
        web_contents_factory_.CreateWebContents(profile_));

    handler_ = std::make_unique<BraveSearchEnginesHandler>(
        profile_, regional_capabilities);
    handler_->set_web_ui(web_ui_.get());
    handler_->AllowJavascript();
    handler_->RegisterMessages();
    web_ui_->ClearTrackedCalls();
  }

  content::TestWebUI* web_ui() { return web_ui_.get(); }
  Profile* profile() { return profile_; }
  TemplateURLService* template_url_service() { return template_url_service_; }
  base::DictValue CreateDictionaryForEngine(TemplateURL* template_url) {
    return handler_->CreateDictionaryForEngine(template_url);
  }

 protected:
  std::string engine_a_guid_;
  std::string engine_b_guid_;
  TemplateURLID engine_a_id_ = kInvalidTemplateURLID;
  TemplateURLID engine_b_id_ = kInvalidTemplateURLID;

 private:
  content::BrowserTaskEnvironment task_environment_;
  TestingProfileManager profile_manager_;
  content::TestWebContentsFactory web_contents_factory_;

  raw_ptr<TemplateURLService> template_url_service_ = nullptr;
  raw_ptr<Profile> profile_ = nullptr;
  std::unique_ptr<content::TestWebUI> web_ui_;
  std::unique_ptr<BraveSearchEnginesHandler> handler_;
};

// Verify that getPrivateSearchEnginesList marks the engine whose sync_guid
// matches kSyncedDefaultPrivateSearchProviderGUID as default - not the engine
// that happens to be the normal window's default.
TEST_F(BraveSearchEnginesHandlerTest,
       PrivateListDefaultFlagMatchesPrivateGUID) {
  base::ListValue args;
  args.Append("callback_id");
  web_ui()->HandleReceivedMessage("getPrivateSearchEnginesList", args);

  ASSERT_EQ(1U, web_ui()->call_data().size());
  const auto& call_data = *web_ui()->call_data().back();
  // arg3 is the response from ResolveJavascriptCallback.
  const base::Value* result = call_data.arg3();
  ASSERT_TRUE(result && result->is_list());

  // engine_b is the private default; engine_a is not.
  bool found_a = false;
  bool found_b = false;
  for (const auto& entry : result->GetList()) {
    const base::DictValue& dict = entry.GetDict();
    const std::string* keyword = dict.FindString("keyword");
    if (!keyword) {
      continue;
    }

    std::optional<bool> is_default = dict.FindBool("default");
    ASSERT_TRUE(is_default.has_value());

    if (*keyword == "engine_a") {
      EXPECT_FALSE(*is_default) << "engine_a must not be the private default";
      found_a = true;
    } else if (*keyword == "engine_b") {
      EXPECT_TRUE(*is_default) << "engine_b must be the private default";
      found_b = true;
    }
  }

  EXPECT_TRUE(found_a) << "engine_a not found in private engines list";
  EXPECT_TRUE(found_b) << "engine_b not found in private engines list";
}

// Verify that setDefaultPrivateSearchEngine updates the pref when given a
// valid engine database ID.
TEST_F(BraveSearchEnginesHandlerTest, SetDefaultPrivateEngineByDatabaseId) {
  ASSERT_NE(engine_a_id_, kInvalidTemplateURLID);
  EXPECT_EQ(engine_b_guid_,
            profile()->GetPrefs()->GetString(
                prefs::kSyncedDefaultPrivateSearchProviderGUID));

  base::ListValue args;
  args.Append(
      base::StrCat({"db:", base::NumberToString(engine_a_id_.value())}));
  web_ui()->HandleReceivedMessage("setDefaultPrivateSearchEngine", args);

  EXPECT_EQ(engine_a_guid_,
            profile()->GetPrefs()->GetString(
                prefs::kSyncedDefaultPrivateSearchProviderGUID));
}

// A user-added engine gets the highest database ID of all engines, which is by
// definition greater than or equal to the number of rows in the table model.
// Verify that such an engine can still be picked as the private window default
// (i.e. that the ID isn't validated as if it were a table model index).
TEST_F(BraveSearchEnginesHandlerTest, SetDefaultPrivateEngineUserAddedEngine) {
  TemplateURL* user_engine =
      AddUserDefinedEngine(template_url_service(), "user_engine");
  ASSERT_TRUE(user_engine);
  const std::string user_engine_guid = user_engine->sync_guid();

  // Make it the normal window default, which is what puts a user-added engine
  // into the list offered for private windows.
  template_url_service()->SetUserSelectedDefaultSearchProvider(user_engine);

  // The service may recreate the TemplateURL, so look it up again by GUID.
  const TemplateURL* default_engine =
      template_url_service()->GetTemplateURLForGUID(user_engine_guid);
  ASSERT_TRUE(default_engine);

  base::ListValue args;
  args.Append(base::StrCat(
      {"db:", base::NumberToString(default_engine->id().value())}));
  web_ui()->HandleReceivedMessage("setDefaultPrivateSearchEngine", args);

  EXPECT_EQ(user_engine_guid,
            profile()->GetPrefs()->GetString(
                prefs::kSyncedDefaultPrivateSearchProviderGUID));
}

// Verify that passing an unknown database ID is silently ignored (no crash,
// no pref change).
TEST_F(BraveSearchEnginesHandlerTest, SetDefaultPrivateEngineUnknownIdIsNoop) {
  const std::string initial_guid = profile()->GetPrefs()->GetString(
      prefs::kSyncedDefaultPrivateSearchProviderGUID);

  base::ListValue args;
  args.Append("db:999999");
  web_ui()->HandleReceivedMessage("setDefaultPrivateSearchEngine", args);

  EXPECT_EQ(initial_guid, profile()->GetPrefs()->GetString(
                              prefs::kSyncedDefaultPrivateSearchProviderGUID));
}

// Verify that http(s) favicon URLs are routed through the Brave favicon proxy
// instead of being fetched directly from the search engine.
TEST_F(BraveSearchEnginesHandlerTest, IconURLIsProxied) {
  TemplateURLData data;
  data.SetShortName(u"engine_c");
  data.SetKeyword(u"engine_c");
  data.SetURL("https://engine-c.example/search?q={searchTerms}");
  data.favicon_url = GURL("https://cdn.engine-c.example/favicon.ico");
  TemplateURL* engine =
      template_url_service()->Add(std::make_unique<TemplateURL>(data));
  ASSERT_TRUE(engine);

  const base::DictValue dict = CreateDictionaryForEngine(engine);
  const std::string* icon_url = dict.FindString("iconURL");
  ASSERT_TRUE(icon_url);
  EXPECT_EQ(*icon_url,
            "https://favicons.proxy.brave.com/"
            "faviconV2?client=chrome&nfrp=2&check_seen=true&size=32&min_size=16"
            "&max_size=256&fallback_opts=TYPE,SIZE,URL&url=https%3A%2F%2F"
            "engine-c.example%2F");
}

// Verify that www.google.com is passed to the favicon proxy as google.com,
// since Google's favicon server has no icon for www.google.com.
TEST_F(BraveSearchEnginesHandlerTest, IconURLProxyStripsWwwForGoogle) {
  TemplateURLData google_data;
  google_data.SetShortName(u"engine_d");
  google_data.SetKeyword(u"engine_d");
  google_data.SetURL("https://www.google.com/search?q={searchTerms}");
  google_data.favicon_url = GURL("https://www.google.com/favicon.ico");
  TemplateURL* google =
      template_url_service()->Add(std::make_unique<TemplateURL>(google_data));
  ASSERT_TRUE(google);

  TemplateURLData other_data;
  other_data.SetShortName(u"engine_e");
  other_data.SetKeyword(u"engine_e");
  other_data.SetURL("https://www.engine-e.example/search?q={searchTerms}");
  other_data.favicon_url = GURL("https://www.engine-e.example/favicon.ico");
  TemplateURL* other =
      template_url_service()->Add(std::make_unique<TemplateURL>(other_data));
  ASSERT_TRUE(other);

  const base::DictValue google_dict = CreateDictionaryForEngine(google);
  const std::string* google_icon_url = google_dict.FindString("iconURL");
  ASSERT_TRUE(google_icon_url);
  EXPECT_TRUE(
      base::EndsWith(*google_icon_url, "&url=https%3A%2F%2Fgoogle.com%2F"))
      << *google_icon_url;

  // Other www. hosts are passed through unchanged.
  const base::DictValue other_dict = CreateDictionaryForEngine(other);
  const std::string* other_icon_url = other_dict.FindString("iconURL");
  ASSERT_TRUE(other_icon_url);
  EXPECT_TRUE(base::EndsWith(*other_icon_url,
                             "&url=https%3A%2F%2Fwww.engine-e.example%2F"))
      << *other_icon_url;
}

// Verify that non-http(s) and empty favicon URLs are not routed through the
// favicon proxy.
TEST_F(BraveSearchEnginesHandlerTest, NonHttpIconURLIsNotProxied) {
  constexpr char kDataFaviconURL[] = "data:image/png;base64,AAAA";
  TemplateURLData data_icon_data;
  data_icon_data.SetShortName(u"engine_f");
  data_icon_data.SetKeyword(u"engine_f");
  data_icon_data.SetURL("https://engine-f.example/search?q={searchTerms}");
  data_icon_data.favicon_url = GURL(kDataFaviconURL);
  TemplateURL* data_icon_engine = template_url_service()->Add(
      std::make_unique<TemplateURL>(data_icon_data));
  ASSERT_TRUE(data_icon_engine);

  TemplateURLData no_icon_data;
  no_icon_data.SetShortName(u"engine_g");
  no_icon_data.SetKeyword(u"engine_g");
  no_icon_data.SetURL("https://engine-g.example/search?q={searchTerms}");
  TemplateURL* no_icon_engine =
      template_url_service()->Add(std::make_unique<TemplateURL>(no_icon_data));
  ASSERT_TRUE(no_icon_engine);

  const base::DictValue data_icon_dict =
      CreateDictionaryForEngine(data_icon_engine);
  const std::string* data_icon_url = data_icon_dict.FindString("iconURL");
  ASSERT_TRUE(data_icon_url);
  EXPECT_EQ(*data_icon_url, kDataFaviconURL);

  const base::DictValue no_icon_dict =
      CreateDictionaryForEngine(no_icon_engine);
  EXPECT_FALSE(no_icon_dict.FindString("iconURL"));
}

}  // namespace settings
