/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <list>
#include <memory>
#include <utility>
#include <vector>

#include "base/callback_list.h"
#include "base/functional/bind.h"
#include "base/test/gmock_callback_support.h"
#include "base/test/test_future.h"
#include "brave/browser/tor/tor_profile_manager.h"
#include "chrome/browser/image_fetcher/image_fetcher_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_key.h"
#include "chrome/browser/ui/autofill/mock_autofill_popup_controller.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/views/autofill/popup/popup_view_views.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/autofill/core/browser/filling/filling_product.h"
#include "components/autofill/core/browser/suggestions/suggestion.h"
#include "components/autofill/core/browser/suggestions/suggestion_type.h"
#include "components/autofill/core/common/aliases.h"
#include "components/image_fetcher/core/image_fetcher_service.h"
#include "components/image_fetcher/core/mock_image_fetcher.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/keyed_service/core/simple_factory_key.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/rect_f.h"
#include "ui/views/view_tracker.h"
#include "ui/views/widget/widget.h"
#include "url/gurl.h"

namespace autofill {

namespace {

// The site of the saved password, and the favicon its row would fetch.
constexpr char kPasswordSiteUrl[] = "https://example.test/";
constexpr char kFaviconUrl[] = "https://example.test/favicon.ico";

// Records every image a profile's `ImageFetcherService` is asked to fetch,
// without touching the network.
class RecordingImageFetcherService : public image_fetcher::ImageFetcherService {
 public:
  RecordingImageFetcherService() {
    ON_CALL(image_fetcher_, FetchImageAndData_)
        .WillByDefault([this](const GURL& image_url, auto&&...) {
          requested_urls_.push_back(image_url);
        });
  }
  ~RecordingImageFetcherService() override = default;

  const std::vector<GURL>& requested_urls() const { return requested_urls_; }

  // image_fetcher::ImageFetcherService:
  image_fetcher::ImageFetcher* GetImageFetcher(
      image_fetcher::ImageFetcherConfig config) override {
    return &image_fetcher_;
  }

 private:
  std::vector<GURL> requested_urls_;
  testing::NiceMock<image_fetcher::MockImageFetcher> image_fetcher_;
};

}  // namespace

// Password rows load their favicon through `PasswordFaviconLoaderImpl`, which
// fetches it with the browser-wide system network context or the original
// profile. In an off-the-record profile such a request would leave the
// profile's network partition, and in a Tor window it would bypass the Tor
// proxy. Every profile here gets an `ImageFetcherService` that records fetches,
// and a row asks it for its favicon while the row paints, so the fetches are
// known as soon as the popup has painted.
class PasswordFaviconOffTheRecordBrowserTest : public InProcessBrowserTest {
 public:
  PasswordFaviconOffTheRecordBrowserTest() {
    create_services_subscription_ =
        BrowserContextDependencyManager::GetInstance()
            ->RegisterCreateServicesCallbackForTesting(
                base::BindRepeating(&PasswordFaviconOffTheRecordBrowserTest::
                                        OnWillCreateBrowserContextServices,
                                    base::Unretained(this)));
  }
  ~PasswordFaviconOffTheRecordBrowserTest() override = default;

  void TearDownOnMainThread() override {
    for (auto& tracker : popups_) {
      views::View* popup = tracker.view();
      if (popup && popup->GetWidget()) {
        popup->GetWidget()->CloseNow();
      }
    }
    InProcessBrowserTest::TearDownOnMainThread();
  }

 protected:
  // Shows a popup with a single password row for `kPasswordSiteUrl`, and waits
  // until the popup has painted.
  void ShowPasswordPopupAndWaitForPaint(
      BrowserWindowInterface* browser_window) {
    content::WebContents* web_contents =
        browser_window->GetTabStripModel()->GetActiveWebContents();
    ASSERT_TRUE(web_contents);

    auto& controller = controllers_.emplace_back();
    ON_CALL(controller, GetWebContents())
        .WillByDefault(testing::Return(web_contents));
    ON_CALL(controller, container_view())
        .WillByDefault(testing::Return(web_contents->GetNativeView()));
    ON_CALL(controller, GetMainFillingProduct())
        .WillByDefault(testing::Return(FillingProduct::kPassword));
    // Anchor the popup to a field inside this window's content area. The
    // windows open at different screen positions.
    const gfx::Rect content_bounds = web_contents->GetContainerBounds();
    controller.set_element_bounds(
        gfx::RectF(content_bounds.x() + 20, content_bounds.y() + 20, 200, 30));

    Suggestion suggestion(u"username", SuggestionType::kPasswordEntry);
    suggestion.icon = Suggestion::Icon::kGlobe;
    suggestion.custom_icon =
        Suggestion::FaviconDetails(/*domain_url=*/GURL(kPasswordSiteUrl));
    controller.set_suggestions({std::move(suggestion)});

    base::test::TestFuture<void> painted;
    EXPECT_CALL(controller, OnPopupPainted())
        .WillOnce(base::test::RunOnceClosure(painted.GetCallback()))
        .WillRepeatedly(testing::Return());

    auto* popup = new PopupViewViews(controller.GetWeakPtr());
    popups_.emplace_back(popup);
    ASSERT_TRUE(popup->Show(AutoselectFirstSuggestion(false)));
    ASSERT_TRUE(painted.Wait());
  }

  // Returns the favicons that password rows in `browser_window` fetched.
  std::vector<GURL> FetchedFavicons(BrowserWindowInterface* browser_window) {
    return static_cast<RecordingImageFetcherService*>(
               ImageFetcherServiceFactory::GetForKey(
                   browser_window->GetProfile()->GetProfileKey()))
        ->requested_urls();
  }

 private:
  void OnWillCreateBrowserContextServices(content::BrowserContext* context) {
    ImageFetcherServiceFactory::GetInstance()->SetTestingFactory(
        Profile::FromBrowserContext(context)->GetProfileKey(),
        base::BindRepeating(
            [](SimpleFactoryKey* key) -> std::unique_ptr<KeyedService> {
              return std::make_unique<RecordingImageFetcherService>();
            }));
  }

  base::CallbackListSubscription create_services_subscription_;
  std::list<testing::NiceMock<MockAutofillPopupController>> controllers_;
  std::list<views::ViewTracker> popups_;
};

IN_PROC_BROWSER_TEST_F(PasswordFaviconOffTheRecordBrowserTest,
                       TorWindowDoesNotFetchFavicon) {
  BrowserWindowInterface* tor_browser =
      TorProfileManager::SwitchToTorProfile(browser()->GetProfile());
  ASSERT_TRUE(tor_browser);
  ASSERT_TRUE(tor_browser->GetProfile()->IsTor());
  ASSERT_NO_FATAL_FAILURE(ShowPasswordPopupAndWaitForPaint(tor_browser));
  EXPECT_THAT(FetchedFavicons(tor_browser), testing::IsEmpty())
      << "A password row in the Tor window fetched its favicon outside Tor.";

  // Control: the same row in a regular window fetches its favicon, so the
  // popup above painted the row and loaded its icon.
  ASSERT_NO_FATAL_FAILURE(ShowPasswordPopupAndWaitForPaint(browser()));
  EXPECT_THAT(FetchedFavicons(browser()),
              testing::ElementsAre(GURL(kFaviconUrl)))
      << "A password row in the regular window did not fetch its favicon.";
}

IN_PROC_BROWSER_TEST_F(PasswordFaviconOffTheRecordBrowserTest,
                       PrivateWindowDoesNotFetchFavicon) {
  BrowserWindowInterface* private_browser = CreateIncognitoBrowser();
  ASSERT_TRUE(private_browser);
  ASSERT_TRUE(private_browser->GetProfile()->IsOffTheRecord());
  ASSERT_NO_FATAL_FAILURE(ShowPasswordPopupAndWaitForPaint(private_browser));
  EXPECT_THAT(FetchedFavicons(private_browser), testing::IsEmpty())
      << "A password row in the private window fetched its favicon outside "
         "the private profile's network context.";

  ASSERT_NO_FATAL_FAILURE(ShowPasswordPopupAndWaitForPaint(browser()));
  EXPECT_THAT(FetchedFavicons(browser()),
              testing::ElementsAre(GURL(kFaviconUrl)))
      << "A password row in the regular window did not fetch its favicon.";
}

}  // namespace autofill
