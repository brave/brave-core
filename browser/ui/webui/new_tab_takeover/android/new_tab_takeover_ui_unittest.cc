/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/new_tab_takeover/android/new_tab_takeover_ui.h"

#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/test/test_future.h"
#include "brave/components/constants/url_constants.h"
#include "brave/components/new_tab_takeover/mojom/new_tab_takeover.mojom.h"
#include "brave/components/ntp_background_images/browser/ntp_background_images_service.h"
#include "brave/components/ntp_background_images/browser/ntp_sponsored_rich_media_ad_event_handler.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "chrome/test/base/testing_profile.h"
#include "components/search_engines/template_url.h"
#include "components/search_engines/template_url_service.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/test_web_contents_factory.h"
#include "content/public/test/test_web_ui.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

// The mojom reply callback contract requires that a callback always run
// exactly once, even if the operation it was waiting on never completes.
// This fixture exercises the two ways `QueryAutocomplete()` can resolve a
// callback without the underlying `AutocompleteController` finishing:
// superseding an in-flight query, and destroying the controller.
class NewTabTakeoverUITest : public testing::Test {
 public:
  NewTabTakeoverUITest()
      : ntp_background_images_service_(/*variations_service=*/nullptr,
                                       /*component_update_service=*/nullptr,
                                       /*pref_service=*/nullptr) {}

  void SetUp() override {
    testing::Test::SetUp();

    TemplateURLServiceFactory::GetInstance()->SetTestingFactoryAndUse(
        &profile_,
        base::BindRepeating(&TemplateURLServiceFactory::BuildInstanceFor));

    web_ui_ = std::make_unique<content::TestWebUI>();
    web_ui_->set_web_contents(
        web_contents_factory_.CreateWebContents(&profile_));

    new_tab_takeover_ui_ = std::make_unique<NewTabTakeoverUI>(
        web_ui_.get(), ntp_background_images_service_,
        std::make_unique<
            ntp_background_images::NTPSponsoredRichMediaAdEventHandler>(
            /*ads_service=*/nullptr));
  }

  // Returns the mojom interface, rather than the concrete type, because
  // `NewTabTakeoverUI` overrides these methods as private and only exposes
  // them through the interface it implements.
  new_tab_takeover::mojom::NewTabTakeover& new_tab_takeover_ui() {
    return *new_tab_takeover_ui_;
  }

  // Destroys the controller before test teardown, to exercise the
  // destructor's pending-callback contract.
  void DestroyNewTabTakeoverUI() { new_tab_takeover_ui_.reset(); }

  TemplateURLService* template_url_service() {
    return TemplateURLServiceFactory::GetForProfile(&profile_);
  }

 private:
  content::BrowserTaskEnvironment task_environment_;
  TestingProfile profile_;
  content::TestWebContentsFactory web_contents_factory_;
  ntp_background_images::NTPBackgroundImagesService
      ntp_background_images_service_;
  std::unique_ptr<content::TestWebUI> web_ui_;
  std::unique_ptr<NewTabTakeoverUI> new_tab_takeover_ui_;
};

}  // namespace

// A second `QueryAutocomplete()` call must resolve the still-pending first
// callback with an empty result rather than dropping it.
TEST_F(NewTabTakeoverUITest, SupersedingQueryResolvesPendingCallbackEmpty) {
  std::optional<std::vector<new_tab_takeover::mojom::AutocompleteMatchPtr>>
      first_result;
  new_tab_takeover_ui().QueryAutocomplete(
      "first", base::BindOnce(
                   [](std::optional<std::vector<
                          new_tab_takeover::mojom::AutocompleteMatchPtr>>* out,
                      std::vector<new_tab_takeover::mojom::AutocompleteMatchPtr>
                          matches) { *out = std::move(matches); },
                   &first_result));
  ASSERT_FALSE(first_result.has_value());

  new_tab_takeover_ui().QueryAutocomplete("second", base::DoNothing());

  ASSERT_TRUE(first_result.has_value());
  EXPECT_TRUE(first_result->empty());
}

// Destroying `NewTabTakeoverUI` with a query still in flight must still run
// the pending callback, per the mojom reply callback contract.
TEST_F(NewTabTakeoverUITest, DestructorResolvesPendingCallbackEmpty) {
  std::optional<std::vector<new_tab_takeover::mojom::AutocompleteMatchPtr>>
      result;
  new_tab_takeover_ui().QueryAutocomplete(
      "input", base::BindOnce(
                   [](std::optional<std::vector<
                          new_tab_takeover::mojom::AutocompleteMatchPtr>>* out,
                      std::vector<new_tab_takeover::mojom::AutocompleteMatchPtr>
                          matches) { *out = std::move(matches); },
                   &result));
  ASSERT_FALSE(result.has_value());

  DestroyNewTabTakeoverUI();

  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->empty());
}

// Brave Search is present in the user's search engine choice list: it should
// become the default and the callback should report success.
TEST_F(NewTabTakeoverUITest, SetDefaultSearchEngineAsBraveSearchSucceeds) {
  base::test::TestFuture<bool> success_future;
  new_tab_takeover_ui().SetDefaultSearchEngineAsBraveSearch(
      success_future.GetCallback());

  EXPECT_TRUE(success_future.Get());
  const TemplateURL* const default_search_provider =
      template_url_service()->GetDefaultSearchProvider();
  ASSERT_TRUE(default_search_provider);
  EXPECT_EQ(kBraveSearchHost, default_search_provider->url_ref().GetHost(
                                  template_url_service()->search_terms_data()));
}

// Brave Search is absent from the user's search engine choice list (e.g. not
// offered in their region): the callback should report failure rather than
// crashing or silently doing nothing.
TEST_F(NewTabTakeoverUITest,
       SetDefaultSearchEngineAsBraveSearchFailsWhenAbsent) {
  for (TemplateURL* const template_url :
       template_url_service()->GetTemplateURLs()) {
    if (template_url->url_ref().GetHost(
            template_url_service()->search_terms_data()) == kBraveSearchHost) {
      template_url_service()->Remove(template_url);
    }
  }

  base::test::TestFuture<bool> success_future;
  new_tab_takeover_ui().SetDefaultSearchEngineAsBraveSearch(
      success_future.GetCallback());

  EXPECT_FALSE(success_future.Get());
}
