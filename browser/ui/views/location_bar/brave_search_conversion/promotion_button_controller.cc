/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/location_bar/brave_search_conversion/promotion_button_controller.h"

#include "base/functional/bind.h"
#include "brave/browser/ui/views/location_bar/brave_search_conversion/promotion_button_view.h"
#include "brave/components/brave_search_conversion/features.h"
#include "brave/components/brave_search_conversion/pref_names.h"
#include "brave/components/brave_search_conversion/utils.h"
#include "brave/components/omnibox/browser/leo_provider.h"
#include "brave/components/omnibox/browser/promotion_utils.h"
#include "brave/components/search_engines/brave_prepopulated_engines.h"
#include "chrome/browser/image_fetcher/image_fetcher_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_key.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_navigator.h"
#include "chrome/browser/ui/browser_navigator_params.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/views/omnibox/omnibox_view_views.h"
#include "components/image_fetcher/core/image_fetcher.h"
#include "components/image_fetcher/core/image_fetcher_service.h"
#include "components/omnibox/browser/autocomplete_match.h"
#include "components/omnibox/browser/omnibox_edit_model.h"
#include "components/omnibox/browser/omnibox_view.h"
#include "components/search_engines/template_url_data_util.h"
#include "components/search_engines/template_url_service.h"
#include "ui/base/page_transition_types.h"

namespace {

constexpr char kImageFetcherUmaClientName[] = "SearchPromotionButtonFavicon";

constexpr net::NetworkTrafficAnnotationTag
    kSearchPromotionButtonTrafficAnnotation =
        net::DefineNetworkTrafficAnnotation("search_promotion", R"(
      semantics {
        sender: "PromotionButtonController"
        description:
          "Fetches favicon for current search provider"
        trigger:
          "When current search provider is changed"
        data: "URL of the favicon image to be fetched."
        destination: WEBSITE
      }
      policy {
        cookies_allowed: NO
        setting: "Disabled when the user dismissed."
      })");

}  // namespace

// static
bool PromotionButtonController::PromotionEnabled(PrefService* prefs) {
  if (!base::FeatureList::IsEnabled(
          brave_search_conversion::features::kOmniboxPromotionButton)) {
    return false;
  }

  if (prefs->GetBoolean(brave_search_conversion::prefs::kDismissed)) {
    return false;
  }

  return true;
}

PromotionButtonController::PromotionButtonController(
    PromotionButtonView* button,
    OmniboxViewViews* omnibox_view,
    Browser* browser)
    : button_(button),
      omnibox_view_(omnibox_view),
      prefs_(*browser->profile()->GetPrefs()),
      browser_(*browser) {
  CHECK(button_ && omnibox_view);
  button_->SetDismissedCallback(base::BindOnce(
      &PromotionButtonController::Dismissed, weak_factory_.GetWeakPtr()));
  button_->SetMakeDefaultCallback(base::BindOnce(
      &PromotionButtonController::SetDefaultAndLoadBraveSearchWithCurrentInput,
      weak_factory_.GetWeakPtr()));
  template_url_service_ =
      TemplateURLServiceFactory::GetForProfile(browser_->profile());
  is_brave_search_default_ = IsBraveSearchDefault();
  view_observation_.AddObservation(button_);
  view_observation_.AddObservation(omnibox_view_);
  UpdateButtonUI();
  template_url_service_observation_.Observe(template_url_service_);
}

PromotionButtonController::~PromotionButtonController() {
  button_ = nullptr;
  omnibox_view_ = nullptr;
}

void PromotionButtonController::Show(bool show) {
  if (!button_) {
    return;
  }

  button_->SetVisible(show);

  // For now, animation is triggered only once after changing search provider.
  // Showing animation whenever this button is visible is too much.
  // TODO(simonhong): adjust how frequently this animation is used.
  if (use_animation_ && show) {
    button_->AnimateExpand();
    use_animation_ = false;
  }
}

bool PromotionButtonController::ShouldShowSearchPromotionButton() {
  if (!button_ || !omnibox_view_) {
    return false;
  }

  if (is_brave_search_default_) {
    return false;
  }

  if (prefs_->GetBoolean(brave_search_conversion::prefs::kDismissed)) {
    return false;
  }

  // No popup means no suggestions for current input.
  // Promotion button will be shown for current search provider's
  // suggestion entries to make users search with brave search with that
  // suggestion.
  if (!omnibox_view_->model()->PopupIsOpen()) {
    return false;
  }

  // Only show promotion for search query. Not url.
  if (omnibox_view_->model()->CurrentTextIsURL()) {
    return false;
  }

  const AutocompleteMatch match = omnibox_view_->model()->CurrentMatch(nullptr);
  return !IsBraveSearchPromotionMatch(match) &&
         !LeoProvider::IsMatchFromLeoProvider(match) &&
         AutocompleteMatch::IsSearchType(match.type);
}

void PromotionButtonController::OnViewIsDeleting(views::View* observed_view) {
  // If any observed view is destroying, this controller will not do anything
  // after that.
  view_observation_.RemoveAllObservations();
  button_ = nullptr;
  omnibox_view_ = nullptr;
}

void PromotionButtonController::OnTemplateURLServiceChanged() {
  use_animation_ = true;
  is_brave_search_default_ = IsBraveSearchDefault();
  if (is_brave_search_default_) {
    return;
  }

  UpdateButtonUI();
}

void PromotionButtonController::OnTemplateURLServiceShuttingDown() {
  template_url_service_observation_.Reset();
}

void PromotionButtonController::SetDefaultAndLoadBraveSearchWithCurrentInput() {
  CHECK(omnibox_view_);

  // Set brave search as default.
  auto provider_data = TemplateURLDataFromPrepopulatedEngine(
      TemplateURLPrepopulateData::brave_search);
  TemplateURL template_url(*provider_data);
  template_url_service_->SetUserSelectedDefaultSearchProvider(&template_url);

  // Load brave search with current input.
  const auto url =
      template_url_service_->GenerateSearchURLForDefaultSearchProvider(
          omnibox_view_->GetText());

  NavigateParams params(&*browser_, url, ui::PAGE_TRANSITION_TYPED);
  params.disposition = WindowOpenDisposition::CURRENT_TAB;
  Navigate(&params);

  // Once user set brave as a default, we'll not show this button again.
  Dismissed();
}

void PromotionButtonController::Dismissed() {
  CHECK(button_);
  button_->SetVisible(false);
  button_->parent()->InvalidateLayout();
  brave_search_conversion::SetDismissed(&*prefs_);

  // After dismissed, we don't need to monitor search provider changes.
  template_url_service_observation_.Reset();
  view_observation_.RemoveAllObservations();

  button_ = nullptr;
  omnibox_view_ = nullptr;
}

void PromotionButtonController::UpdateButtonUI() {
  if (!template_url_service_->loaded()) {
    return;
  }

  const auto* template_url = template_url_service_->GetDefaultSearchProvider();
  auto* service = ImageFetcherServiceFactory::GetForKey(
      browser_->profile()->GetProfileKey());
  CHECK(service);
  auto* fetcher = service->GetImageFetcher(
      image_fetcher::ImageFetcherConfig::kDiskCacheOnly);
  image_fetcher::ImageFetcherParams params(
      kSearchPromotionButtonTrafficAnnotation, kImageFetcherUmaClientName);
  fetcher->FetchImage(
      template_url->favicon_url(),
      base::BindOnce(&PromotionButtonController::OnGetFaviconImage,
                     weak_factory_.GetWeakPtr()),
      params);
}

bool PromotionButtonController::IsBraveSearchDefault() {
  const auto* template_url = template_url_service_->GetDefaultSearchProvider();
  if (template_url->prepopulate_id() ==
          TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE ||
      template_url->prepopulate_id() ==
          TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE_TOR) {
    return true;
  }

  return false;
}

void PromotionButtonController::OnGetFaviconImage(
    const gfx::Image& image,
    const image_fetcher::RequestMetadata& request_metadata) {
  if (!image.IsEmpty()) {
    button_->UpdateTargetProviderImage(image);
  }
}
