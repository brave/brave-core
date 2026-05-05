/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/settings/brave_search_engines_handler.h"

#include <memory>
#include <string>

#include "base/command_line.h"
#include "base/memory/raw_ptr.h"
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
  args.Append(static_cast<int>(engine_a_id_));
  web_ui()->HandleReceivedMessage("setDefaultPrivateSearchEngine", args);

  EXPECT_EQ(engine_a_guid_,
            profile()->GetPrefs()->GetString(
                prefs::kSyncedDefaultPrivateSearchProviderGUID));
}

// Verify that passing an unknown database ID is silently ignored (no crash,
// no pref change).
TEST_F(BraveSearchEnginesHandlerTest, SetDefaultPrivateEngineUnknownIdIsNoop) {
  const std::string initial_guid = profile()->GetPrefs()->GetString(
      prefs::kSyncedDefaultPrivateSearchProviderGUID);

  base::ListValue args;
  args.Append(999999);
  web_ui()->HandleReceivedMessage("setDefaultPrivateSearchEngine", args);

  EXPECT_EQ(initial_guid, profile()->GetPrefs()->GetString(
                              prefs::kSyncedDefaultPrivateSearchProviderGUID));
}

}  // namespace settings
