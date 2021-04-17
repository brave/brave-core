/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/search_engines/active_window_search_provider_manager.h"

#include <memory>
#include <vector>

#include "base/values.h"
#include "brave/browser/profiles/profile_util.h"
#include "brave/browser/search_engines/search_engine_provider_util.h"
#include "brave/common/pref_names.h"
#include "brave/components/search_engines/brave_prepopulated_engines.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/location_bar/location_bar_view.h"
#include "chrome/browser/ui/views/omnibox/omnibox_view_views.h"
#include "components/prefs/pref_service.h"
#include "components/search_engines/template_url_data_util.h"
#include "components/search_engines/template_url_prepopulate_data.h"
#include "components/search_engines/template_url_service.h"

namespace {

std::unique_ptr<TemplateURLData> GetSearchEngineProviderForTor(
    PrefService* prefs) {
  std::unique_ptr<TemplateURLData> provider_data;

  int id = TemplateURLPrepopulateData::GetPrepopulatedDefaultSearch(prefs)
               ->prepopulate_id;
  switch (id) {
    case TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_QWANT:
    case TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_DUCKDUCKGO:
    case TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_DUCKDUCKGO_DE:
    case TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_DUCKDUCKGO_AU_NZ_IE:
      break;

    default:
      id = TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_DUCKDUCKGO;
      break;
  }
  provider_data = TemplateURLPrepopulateData::GetPrepopulatedEngine(prefs, id);

  DCHECK(provider_data);
  return provider_data;
}

void HandleTorWindowActivationStateChange(Profile* profile, bool active) {
  if (!active)
    return;

  auto provider_data = GetSearchEngineProviderForTor(profile->GetPrefs());
  TemplateURL provider_url(*provider_data);
  auto* tus = TemplateURLServiceFactory::GetForProfile(profile);
  tus->SetUserSelectedDefaultSearchProvider(&provider_url);
}

void ChangeToAlternativeSearchEngineProvider(Profile* profile) {
  std::unique_ptr<TemplateURLData> data =
      brave::GetDDGTemplateURLData(profile->GetPrefs());
  auto* tus = TemplateURLServiceFactory::GetForProfile(profile);
  TemplateURL url(*data);
  tus->SetUserSelectedDefaultSearchProvider(&url);
}

void ChangeToNormalWindowSearchEngineProvider(Profile* profile) {
  const base::Value* data_value =
      profile->GetOriginalProfile()->GetPrefs()->Get(
          kCachedNormalSearchProvider);
  if (data_value && data_value->is_dict()) {
    std::unique_ptr<TemplateURLData> data = TemplateURLDataFromDictionary(
        base::Value::AsDictionaryValue(*data_value));
    auto* tus = TemplateURLServiceFactory::GetForProfile(profile);
    TemplateURL url(*data);
    tus->SetUserSelectedDefaultSearchProvider(&url);
  }
}

void HandlePrivateWindowActivationStateChange(Profile* profile, bool active) {
  if (!active)
    return;

  brave::UseAlternativeSearchEngineProviderEnabled(profile)
      ? ChangeToAlternativeSearchEngineProvider(profile)
      : ChangeToNormalWindowSearchEngineProvider(profile);
}

void HandleNormalWindowActivationStateChange(Profile* profile, bool active) {
  auto* tus = TemplateURLServiceFactory::GetForProfile(profile);
  auto* tu = tus->GetDefaultSearchProvider();
  // Cache DSE. This cached DSE will be set when normal window is activated.
  if (!active) {
    std::unique_ptr<base::DictionaryValue> data_value =
        TemplateURLDataToDictionary(tu->data());
    profile->GetPrefs()->Set(kCachedNormalSearchProvider, data_value->Clone());
    return;
  }

  auto* preference =
      profile->GetPrefs()->FindPreference(kCachedNormalSearchProvider);
  if (preference->IsDefaultValue())
    return;

  const base::Value* data_value =
      profile->GetPrefs()->Get(kCachedNormalSearchProvider);
  // Apply cached DSE.
  if (data_value && data_value->is_dict()) {
    std::unique_ptr<TemplateURLData> data = TemplateURLDataFromDictionary(
        base::Value::AsDictionaryValue(*data_value));
    TemplateURL url(*data);
    tus->SetUserSelectedDefaultSearchProvider(&url);
  }
}

}  // namespace

ActiveWindowSearchProviderManager::ActiveWindowSearchProviderManager(
    Browser* browser,
    views::Widget* widget)
    : browser_(browser), profile_(browser->profile()) {
  observation_.Observe(widget);
  ObserveSearchEngineProviderPrefs();
}

ActiveWindowSearchProviderManager::~ActiveWindowSearchProviderManager() =
    default;

void ActiveWindowSearchProviderManager::ObserveSearchEngineProviderPrefs() {
  if (profile_->IsTor() || brave::IsGuestProfile(profile_)) {
    return;
  }

  if (profile_->IsIncognitoProfile()) {
    use_alternative_search_engine_provider_.Init(
        kUseAlternativeSearchEngineProvider,
        profile_->GetOriginalProfile()->GetPrefs(),
        base::Bind(&ActiveWindowSearchProviderManager::OnPreferenceChanged,
                   base::Unretained(this)));
  }
}

void ActiveWindowSearchProviderManager::OnPreferenceChanged() {
  auto* tus = TemplateURLServiceFactory::GetForProfile(profile_);
  if (!tus)
    return;

  auto* tu = tus->GetDefaultSearchProvider();
  if (tu && tu->type() == TemplateURL::NORMAL_CONTROLLED_BY_EXTENSION)
    return;

  HandlePrivateWindowActivationStateChange(profile_, true);
}

void ActiveWindowSearchProviderManager::OnWidgetActivationChanged(
    views::Widget* widget,
    bool active) {
  if (active) {
    // Set proper placeholder text whenever it's activated because inactive
    // window's omnibox placeholder could have invalid one.
    // See BraveOmniboxViewViews::OnTemplateURLServiceChanged() comment.
    auto* location_bar =
        BrowserView::GetBrowserViewForBrowser(browser_)->GetLocationBarView();
    location_bar->omnibox_view()->InstallPlaceholderText();
  }

  auto* tus = TemplateURLServiceFactory::GetForProfile(profile_);
  if (!tus)
    return;
  auto* tu = tus->GetDefaultSearchProvider();
  if (tu && tu->type() == TemplateURL::NORMAL_CONTROLLED_BY_EXTENSION)
    return;

  if (profile_->IsTor()) {
    HandleTorWindowActivationStateChange(profile_, active);
    return;
  }

  if (brave::IsGuestProfile(profile_)) {
    // Handled by SearchEngineProviderService.
    return;
  }

  if (profile_->IsIncognitoProfile()) {
    HandlePrivateWindowActivationStateChange(profile_, active);
    return;
  }

  HandleNormalWindowActivationStateChange(profile_, active);
}

void ActiveWindowSearchProviderManager::OnWidgetClosing(views::Widget* widget) {
  observation_.Reset();
}
