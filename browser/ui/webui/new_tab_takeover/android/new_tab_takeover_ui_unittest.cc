/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/new_tab_takeover/android/new_tab_takeover_ui.h"

#include <memory>
#include <vector>

#include "brave/components/new_tab_takeover/mojom/new_tab_takeover.mojom.h"
#include "brave/components/ntp_background_images/browser/ntp_background_images_service.h"
#include "brave/components/ntp_background_images/browser/ntp_sponsored_rich_media_ad_event_handler.h"
#include "brave/components/ntp_background_images/browser/test/fake_ntp_background_images_service.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
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

}  // namespace

class NewTabTakeoverUITest : public ChromeRenderViewHostTestHarness {
 public:
  void SetUp() override {
    ChromeRenderViewHostTestHarness::SetUp();

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

 private:
  std::unique_ptr<ntp_background_images::FakeNTPBackgroundImagesService>
      ntp_background_images_service_;
  content::TestWebUI web_ui_;
  std::unique_ptr<NewTabTakeoverUI> new_tab_takeover_ui_;
  mojo::Remote<new_tab_takeover::mojom::NewTabTakeover> new_tab_takeover_;
};

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
