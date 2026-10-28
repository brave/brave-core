/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/location_bar/brave_location_bar_view.h"

#include <memory>

#include "base/functional/callback_helpers.h"
#include "base/run_loop.h"
#include "base/scoped_observation.h"
#include "base/test/bind.h"
#include "base/test/scoped_feature_list.h"
#include "base/time/time.h"
#include "brave/browser/ui/views/location_bar/brave_search_conversion/promotion_button_controller.h"
#include "brave/browser/ui/views/location_bar/brave_search_conversion/promotion_button_view.h"
#include "brave/components/brave_search_conversion/features.h"
#include "brave/components/brave_search_conversion/pref_names.h"
#include "brave/components/search_engines/brave_prepopulated_engines.h"
#include "chrome/browser/favicon/favicon_service_factory.h"
#include "chrome/browser/history/history_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "chrome/browser/ui/omnibox/omnibox_controller.h"
#include "chrome/browser/ui/omnibox/omnibox_edit_model.h"
#include "chrome/browser/ui/omnibox/omnibox_view.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/view_ids.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/omnibox/omnibox_view_views.h"
#include "chrome/browser/ui/views/toolbar/toolbar_view.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/interactive_test_utils.h"
#include "chrome/test/base/search_test_utils.h"
#include "components/favicon/core/favicon_service.h"
#include "components/favicon_base/favicon_types.h"
#include "components/history/core/browser/history_service.h"
#include "components/history/core/browser/history_types.h"
#include "components/omnibox/browser/omnibox_client.h"
#include "components/search_engines/template_url.h"
#include "components/search_engines/template_url_data_util.h"
#include "components/search_engines/template_url_service.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/models/image_model.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/image/image_unittest_util.h"
#include "ui/views/view_observer.h"

class BraveLocationBarViewBrowserTest : public InProcessBrowserTest {
 public:
  BraveLocationBarViewBrowserTest() {
    // Disabled by default.
    EXPECT_FALSE(base::FeatureList::IsEnabled(
        brave_search_conversion::features::kOmniboxPromotionButton));

    features_.InitAndEnableFeature(
        brave_search_conversion::features::kOmniboxPromotionButton);
  }

  BraveToolbarView* toolbar() {
    auto* browser_view = BrowserView::GetBrowserViewForBrowser(browser());
    return static_cast<BraveToolbarView*>(browser_view->toolbar());
  }

  BraveLocationBarView* location_bar() {
    auto* browser_view = BrowserView::GetBrowserViewForBrowser(browser());
    return static_cast<BraveLocationBarView*>(
        browser_view->toolbar()->location_bar());
  }

  OmniboxViewViews* omnibox_view() { return location_bar()->omnibox_view(); }

  OmniboxController* controller() {
    return location_bar()->GetOmniboxController();
  }

  OmniboxEditModel* edit_model() {
    return location_bar()->GetOmniboxController()->edit_model();
  }

  views::View* promotion_button_view() {
    return location_bar()->GetSearchPromotionButton();
  }

  TemplateURLService* GetTemplateURLService() {
    return TemplateURLServiceFactory::GetForProfile(browser()->GetProfile());
  }

  content::WebContents* web_contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  // Stores a favicon for `icon_url` and warms the omnibox favicon cache so that
  // subsequent lookups for it resolve synchronously.
  void PrimeFavicon(const GURL& icon_url) {
    const GURL page_url("https://" + icon_url.host() + "/");
    HistoryServiceFactory::GetForProfile(browser()->GetProfile(),
                                         ServiceAccessType::EXPLICIT_ACCESS)
        ->AddPage(page_url, base::Time::Now(), history::SOURCE_BROWSED);
    FaviconServiceFactory::GetForProfile(browser()->GetProfile(),
                                         ServiceAccessType::EXPLICIT_ACCESS)
        ->SetFavicons(
            {page_url}, icon_url, favicon_base::IconType::kFavicon,
            gfx::Image::CreateFrom1xBitmap(gfx::test::CreateBitmap(16)));

    WaitUntil(base::BindLambdaForTesting([&]() {
      return !controller()
                  ->client()
                  ->GetFaviconForIconUrl(icon_url, base::DoNothing(),
                                         /*notify_on_empty=*/false)
                  .IsEmpty();
    }));
  }

  void WaitUntil(base::RepeatingCallback<bool()> condition) {
    if (condition.Run()) {
      return;
    }

    base::RepeatingTimer scheduler;
    scheduler.Start(FROM_HERE, base::Milliseconds(100),
                    base::BindLambdaForTesting([this, &condition] {
                      if (condition.Run()) {
                        run_loop_->Quit();
                      }
                    }));
    Run();
  }

  void Run() {
    run_loop_ = std::make_unique<base::RunLoop>();
    run_loop_->Run();
  }

  base::test::ScopedFeatureList features_;
  std::unique_ptr<base::RunLoop> run_loop_;
};

IN_PROC_BROWSER_TEST_F(BraveLocationBarViewBrowserTest,
                       SearchConversionButtonTest) {
  search_test_utils::WaitForTemplateURLServiceToLoad(GetTemplateURLService());

  auto bing_search_data = TemplateURLDataFromPrepopulatedEngine(
      TemplateURLPrepopulateData::brave_bing);
  TemplateURL bing_template_url(*bing_search_data);

  auto brave_search_data = TemplateURLDataFromPrepopulatedEngine(
      TemplateURLPrepopulateData::brave_search);
  TemplateURL brave_template_url(*brave_search_data);

  // Set non brave search as a default provider and type any input.
  // Check promotion button is launched.
  GetTemplateURLService()->SetUserSelectedDefaultSearchProvider(
      &bing_template_url);
  location_bar()->FocusLocation(/*is_user_initiated=*/true,
                                /*clear_focus_if_failed=*/false);
  omnibox_view()->SetUserText(u"a");
  WaitUntil(base::BindLambdaForTesting(
      [&]() { return controller()->IsPopupOpen(); }));
  EXPECT_TRUE(promotion_button_view()->GetVisible());

  // Unfocus from the omnibox.
  // Omnibox popup is hidden and promotion button will be gone also.
  web_contents()->Focus();
  WaitUntil(base::BindLambdaForTesting(
      [&]() { return !controller()->IsPopupOpen(); }));
  EXPECT_FALSE(promotion_button_view()->GetVisible());

  // Set brave search as a default provider and type any input.
  GetTemplateURLService()->SetUserSelectedDefaultSearchProvider(
      &brave_template_url);

  // Check button is not shown with brave search.
  location_bar()->FocusLocation(/*is_user_initiated=*/true,
                                /*clear_focus_if_failed=*/false);
  omnibox_view()->SetUserText(u"a");
  WaitUntil(base::BindLambdaForTesting(
      [&]() { return controller()->IsPopupOpen(); }));
  EXPECT_FALSE(promotion_button_view()->GetVisible());

  location_bar()->Revert();
  WaitUntil(base::BindLambdaForTesting(
      [&]() { return !controller()->IsPopupOpen(); }));
  EXPECT_FALSE(promotion_button_view()->GetVisible());

  GetTemplateURLService()->SetUserSelectedDefaultSearchProvider(
      &bing_template_url);

  // Set dismissed and check button is not shown anymore.
  browser()->GetProfile()->GetPrefs()->SetBoolean(
      brave_search_conversion::prefs::kDismissed, true);
  omnibox_view()->SetUserText(u"a");
  WaitUntil(base::BindLambdaForTesting(
      [&]() { return controller()->IsPopupOpen(); }));
  EXPECT_FALSE(promotion_button_view()->GetVisible());

  location_bar()->Revert();
  WaitUntil(base::BindLambdaForTesting(
      [&]() { return !controller()->IsPopupOpen(); }));
  EXPECT_FALSE(promotion_button_view()->GetVisible());

  constexpr std::u16string search_term = u"a";

  // Unset dismissed and simulate promotion button click.
  browser()->GetProfile()->GetPrefs()->SetBoolean(
      brave_search_conversion::prefs::kDismissed, false);
  location_bar()->FocusLocation(/*is_user_initiated=*/true,
                                /*clear_focus_if_failed=*/false);
  omnibox_view()->SetUserText(search_term);
  WaitUntil(base::BindLambdaForTesting(
      [&]() { return controller()->IsPopupOpen(); }));
  EXPECT_TRUE(promotion_button_view()->GetVisible());

  // Check brave search is set as default provider and brave search is loaded in
  // active tab.
  location_bar()
      ->promotion_controller_->SetDefaultAndLoadBraveSearchWithCurrentInput();
  const TemplateURL* default_provider =
      GetTemplateURLService()->GetDefaultSearchProvider();
  const auto target_search_url = brave_template_url.GenerateSearchURL(
      GetTemplateURLService()->search_terms_data(), search_term);
  EXPECT_EQ(brave_template_url.prepopulate_id(),
            default_provider->prepopulate_id());
  content::WaitForLoadStop(web_contents());
  EXPECT_EQ(target_search_url, web_contents()->GetVisibleURL());

  // Check dismissed bit is set after user clicks button.
  EXPECT_TRUE(browser()->GetProfile()->GetPrefs()->GetBoolean(
      brave_search_conversion::prefs::kDismissed));
}

// The Brave logo already stands for Shields in the location bar, so Brave
// Search must keep the generic search icon rather than show its own favicon.
IN_PROC_BROWSER_TEST_F(BraveLocationBarViewBrowserTest,
                       BraveSearchKeepsSearchVectorIconTest) {
  search_test_utils::WaitForTemplateURLServiceToLoad(GetTemplateURLService());

  TemplateURL bing_template_url(*TemplateURLDataFromPrepopulatedEngine(
      TemplateURLPrepopulateData::brave_bing));
  TemplateURL brave_template_url(*TemplateURLDataFromPrepopulatedEngine(
      TemplateURLPrepopulateData::brave_search));

  PrimeFavicon(bing_template_url.favicon_url());
  PrimeFavicon(brave_template_url.favicon_url());

  // Other engines keep using their favicon.
  GetTemplateURLService()->SetUserSelectedDefaultSearchProvider(
      &bing_template_url);
  location_bar()->FocusLocation(/*is_user_initiated=*/true,
                                /*clear_focus_if_failed=*/false);
  omnibox_view()->SetUserText(u"a");
  WaitUntil(base::BindLambdaForTesting([&]() {
    return location_bar()->GetLocationIcon(base::DoNothing()).IsImage();
  }));

  location_bar()->Revert();
  WaitUntil(base::BindLambdaForTesting(
      [&]() { return !controller()->IsPopupOpen(); }));

  GetTemplateURLService()->SetUserSelectedDefaultSearchProvider(
      &brave_template_url);
  location_bar()->FocusLocation(/*is_user_initiated=*/true,
                                /*clear_focus_if_failed=*/false);
  omnibox_view()->SetUserText(u"a");
  WaitUntil(base::BindLambdaForTesting(
      [&]() { return controller()->IsPopupOpen(); }));
  EXPECT_TRUE(
      location_bar()->GetLocationIcon(base::DoNothing()).IsVectorIcon());
}

// After resizing the toolbar to a narrower width, every visible child of the
// location bar must remain within the location bar's horizontal bounds.
// This exercises the InvalidateLayout() fix in ResetLocationBarBounds() and
// the additive GetMinimumSize() formula.
IN_PROC_BROWSER_TEST_F(BraveLocationBarViewBrowserTest,
                       ChildViewsStayWithinBoundsOnNarrowResize) {
  auto toolbar_bounds = toolbar()->bounds();
  toolbar_bounds.Inset(gfx::Insets::VH(0, toolbar_bounds.width() / 3));
  toolbar()->SetBoundsRect(toolbar_bounds);

  for (views::View* child : location_bar()->children()) {
    // The FocusRing intentionally overflows its parent by 1px to draw the ring.
    if (!child->GetVisible() || (child->GetClassName() == "FocusRing")) {
      continue;
    }
    EXPECT_GE(child->bounds().x(), 0)
        << child->GetClassName() << " overflows left edge";
    EXPECT_LE(child->bounds().right(), location_bar()->width())
        << child->GetClassName() << " overflows right edge";
  }
}

class OmniboxViewObserver : public views::ViewObserver {
 public:
  OmniboxViewObserver() = default;
  ~OmniboxViewObserver() override = default;

  void OnViewBoundsChanged(views::View* observed_view) override {
    bounds_changed_count_++;
  }

  int bounds_changed_count_ = 0;
};

// Check location bar's layout is called only once when toolbar's size changed.
IN_PROC_BROWSER_TEST_F(BraveLocationBarViewBrowserTest,
                       OmniboxViewBoundsChangedTest) {
  OmniboxViewObserver observer;
  base::ScopedObservation<OmniboxViewViews, views::ViewObserver> observation(
      &observer);
  observation.Observe(omnibox_view());
  auto toolbar_bounds = toolbar()->bounds();
  toolbar_bounds.Inset(gfx::Insets::VH(0, 10));
  toolbar()->SetBoundsRect(toolbar_bounds);

  EXPECT_EQ(1, observer.bounds_changed_count_);
}
