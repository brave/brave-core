/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_LOCATION_BAR_BRAVE_SEARCH_CONVERSION_PROMOTION_BUTTON_CONTROLLER_H_
#define BRAVE_BROWSER_UI_VIEWS_LOCATION_BAR_BRAVE_SEARCH_CONVERSION_PROMOTION_BUTTON_CONTROLLER_H_

#include "base/gtest_prod_util.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "base/scoped_multi_source_observation.h"
#include "base/scoped_observation.h"
#include "components/search_engines/template_url_service_observer.h"
#include "ui/views/view_observer.h"

class Browser;
class OmniboxViewViews;
class PrefService;
class PromotionButtonView;
class TemplateURLService;

namespace gfx {
class Image;
}  // namespace gfx

namespace image_fetcher {
struct RequestMetadata;
}  // namespace image_fetcher

class PromotionButtonController : public views::ViewObserver,
                                  public TemplateURLServiceObserver {
 public:
  static bool PromotionEnabled(PrefService* prefs);

  PromotionButtonController(PromotionButtonView* button,
                            OmniboxViewViews* omnibox_view,
                            Browser* browser);
  ~PromotionButtonController() override;

  // Check with current input and autocomplete match.
  bool ShouldShowSearchPromotionButton();
  void Show(bool show);

 private:
  FRIEND_TEST_ALL_PREFIXES(BraveLocationBarViewBrowserTest,
                           SearchConversionButtonTest);

  // views::ViewObserver overrides:
  void OnViewIsDeleting(views::View* observed_view) override;

  // TemplateURLServiceObserver overrides:
  void OnTemplateURLServiceChanged() override;
  void OnTemplateURLServiceShuttingDown() override;

  bool IsBraveSearchDefault();
  void SetDefaultAndLoadBraveSearchWithCurrentInput();
  void Dismissed();
  void UpdateButtonUI();
  void OnGetFaviconImage(
      const gfx::Image& image,
      const image_fetcher::RequestMetadata& request_metadata);

  bool use_animation_ = false;
  bool is_brave_search_default_ = false;

  // |button_|, |omnibox_view_| and this are owned by
  // same parent(LocationBarView). Monitor their destroying as
  // this refers them and don't know exact destroying order.
  raw_ptr<PromotionButtonView> button_ = nullptr;
  raw_ptr<OmniboxViewViews> omnibox_view_ = nullptr;
  raw_ptr<TemplateURLService> template_url_service_ = nullptr;
  raw_ref<PrefService> prefs_;
  raw_ref<Browser> browser_;
  base::ScopedMultiSourceObservation<views::View, views::ViewObserver>
      view_observation_{this};
  base::ScopedObservation<TemplateURLService, TemplateURLServiceObserver>
      template_url_service_observation_{this};
  base::WeakPtrFactory<PromotionButtonController> weak_factory_{this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_LOCATION_BAR_BRAVE_SEARCH_CONVERSION_PROMOTION_BUTTON_CONTROLLER_H_
