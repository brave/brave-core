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
#include "brave/components/ntp_background_images/browser/test/fake_ntp_background_images_service.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "components/search_engines/template_url.h"
#include "components/search_engines/template_url_service.h"
#include "content/public/test/test_web_ui.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/geometry/rect_f.h"

namespace {

class FakeNewTabTakeoverPage final
    : public new_tab_takeover::mojom::NewTabTakeoverPage {
 public:
  FakeNewTabTakeoverPage() = default;

  FakeNewTabTakeoverPage(const FakeNewTabTakeoverPage&) = delete;
  FakeNewTabTakeoverPage& operator=(const FakeNewTabTakeoverPage&) = delete;

  ~FakeNewTabTakeoverPage() override = default;

  mojo::PendingRemote<new_tab_takeover::mojom::NewTabTakeoverPage>
  BindNewPipeAndPassRemote() {
    return receiver_.BindNewPipeAndPassRemote();
  }

  void FlushForTesting() { receiver_.FlushForTesting(); }

  const std::vector<gfx::RectF>& notified_safe_areas() const {
    return notified_safe_areas_;
  }

  // new_tab_takeover::mojom::NewTabTakeoverPage:
  void SetSafeArea(const gfx::RectF& safe_area) override {
    notified_safe_areas_.push_back(safe_area);
  }

 private:
  std::vector<gfx::RectF> notified_safe_areas_;

  mojo::Receiver<new_tab_takeover::mojom::NewTabTakeoverPage> receiver_{this};
};

// The mojom reply callback contract requires that a callback always run
// exactly once, even if the operation it was waiting on never completes.
// This fixture also exercises the two ways `QueryAutocomplete()` can resolve
// a callback without the underlying `AutocompleteController` finishing:
// superseding an in-flight query, and destroying the controller.
class NewTabTakeoverUITest : public ChromeRenderViewHostTestHarness {
 public:
  void SetUp() override {
    ChromeRenderViewHostTestHarness::SetUp();

    TemplateURLServiceFactory::GetInstance()->SetTestingFactoryAndUse(
        profile(),
        base::BindRepeating(&TemplateURLServiceFactory::BuildInstanceFor));

    ntp_background_images_service_ =
        std::make_unique<ntp_background_images::FakeNTPBackgroundImagesService>(
            /*variations_service=*/nullptr,
            /*component_update_service=*/nullptr, /*local_state=*/nullptr);

    web_ui_.set_web_contents(web_contents());
    new_tab_takeover_ui_ = std::make_unique<NewTabTakeoverUI>(
        &web_ui_, *ntp_background_images_service_,
        std::make_unique<
            ntp_background_images::NTPSponsoredRichMediaAdEventHandler>(
            /*ads_service=*/nullptr));
    new_tab_takeover_ui_->BindInterface(
        new_tab_takeover_.BindNewPipeAndPassReceiver());
  }

  void TearDown() override {
    new_tab_takeover_.reset();
    new_tab_takeover_ui_.reset();
    ntp_background_images_service_.reset();

    ChromeRenderViewHostTestHarness::TearDown();
  }

  void SetPage(FakeNewTabTakeoverPage& page) {
    new_tab_takeover_->SetPage(page.BindNewPipeAndPassRemote());
    new_tab_takeover_.FlushForTesting();
    page.FlushForTesting();
  }

  void SetSafeArea(const gfx::RectF& safe_area, FakeNewTabTakeoverPage* page) {
    new_tab_takeover_ui_->SetSafeArea(safe_area);
    new_tab_takeover_.FlushForTesting();
    if (page) {
      page->FlushForTesting();
    }
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
    return TemplateURLServiceFactory::GetForProfile(profile());
  }

 private:
  std::unique_ptr<ntp_background_images::FakeNTPBackgroundImagesService>
      ntp_background_images_service_;
  content::TestWebUI web_ui_;
  std::unique_ptr<NewTabTakeoverUI> new_tab_takeover_ui_;
  mojo::Remote<new_tab_takeover::mojom::NewTabTakeover> new_tab_takeover_;
};

}  // namespace

TEST_F(NewTabTakeoverUITest, AppliesSafeAreaMeasuredBeforePageBinds) {
  SetSafeArea(gfx::RectF(0, 42, 360, 200), /*page=*/nullptr);

  FakeNewTabTakeoverPage page;
  SetPage(page);

  EXPECT_THAT(page.notified_safe_areas(),
              ::testing::ElementsAre(gfx::RectF(0, 42, 360, 200)));
}

TEST_F(NewTabTakeoverUITest, NotifiesPageWhenSafeAreaChanges) {
  FakeNewTabTakeoverPage page;
  SetPage(page);

  SetSafeArea(gfx::RectF(0, 42, 360, 200), &page);
  SetSafeArea(gfx::RectF(0, 42, 360, 120), &page);

  EXPECT_THAT(page.notified_safe_areas(),
              ::testing::ElementsAre(gfx::RectF(0, 42, 360, 200),
                                     gfx::RectF(0, 42, 360, 120)));
}

TEST_F(NewTabTakeoverUITest, DoesNotNotifyPageWhenTheSafeAreaIsUnchanged) {
  FakeNewTabTakeoverPage page;
  SetPage(page);

  SetSafeArea(gfx::RectF(0, 42, 360, 200), &page);
  SetSafeArea(gfx::RectF(0, 42, 360, 200), &page);

  EXPECT_THAT(page.notified_safe_areas(),
              ::testing::ElementsAre(gfx::RectF(0, 42, 360, 200)));
}

TEST_F(NewTabTakeoverUITest, DoesNotNotifyPageWhenNothingWasMeasured) {
  FakeNewTabTakeoverPage page;
  SetPage(page);

  EXPECT_THAT(page.notified_safe_areas(), ::testing::IsEmpty());
}

TEST_F(NewTabTakeoverUITest, AppliesSafeAreaWhenAnotherPageBinds) {
  FakeNewTabTakeoverPage page;
  SetPage(page);
  SetSafeArea(gfx::RectF(0, 42, 360, 200), &page);

  FakeNewTabTakeoverPage another_page;
  SetPage(another_page);

  EXPECT_THAT(another_page.notified_safe_areas(),
              ::testing::ElementsAre(gfx::RectF(0, 42, 360, 200)));
}

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
