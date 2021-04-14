/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/brave_browser_frame.h"

#include "base/values.h"
#include "brave/browser/profiles/profile_util.h"
#include "brave/browser/search_engines/search_engine_provider_util.h"
#include "brave/common/pref_names.h"
#include "brave/components/search_engines/brave_prepopulated_engines.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "chrome/browser/themes/theme_service.h"
#include "chrome/browser/themes/theme_service_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "components/search_engines/template_url_data_util.h"
#include "components/search_engines/template_url_prepopulate_data.h"
#include "components/search_engines/template_url_service.h"

namespace {

std::unique_ptr<TemplateURLData> GetSearchEngineProviderForTor(
    PrefService* prefs) {
  std::unique_ptr<TemplateURLData> provider_data;

  int id = TemplateURLPrepopulateData::GetPrepopulatedDefaultSearch(
                       prefs)->prepopulate_id;
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
  provider_data =
      TemplateURLPrepopulateData::GetPrepopulatedEngine(prefs, id);

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
  // TODO(simonhong): De-duplicate this
  std::vector<TemplateURLPrepopulateData::BravePrepopulatedEngineID>
      alt_search_providers = {
          TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_DUCKDUCKGO,
          TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_DUCKDUCKGO_DE,
          TemplateURLPrepopulateData::
              PREPOPULATED_ENGINE_ID_DUCKDUCKGO_AU_NZ_IE
      };

  std::unique_ptr<TemplateURLData> data;
  for (const auto& id : alt_search_providers) {
    data = TemplateURLPrepopulateData::GetPrepopulatedEngine(
        profile->GetPrefs(), id);
    if (data)
      break;
  }

  // There should ALWAYS be one entry
  DCHECK(data);
  // alternative_search_engine_url.reset(new TemplateURL(*data));
  auto* tus = TemplateURLServiceFactory::GetForProfile(profile);
  tus->SetUserSelectedDefaultSearchProvider(new TemplateURL(*data));
}

void ChangeToNormalWindowSearchEngineProvider(Profile* profile) {
  const base::Value* data_value =
      profile->GetOriginalProfile()->GetPrefs()->Get(kCachedNormalSearchProvider);
  if (data_value && data_value->is_dict()) {
    std::unique_ptr<TemplateURLData> data =
        TemplateURLDataFromDictionary(base::Value::AsDictionaryValue(*data_value));
    auto* tus = TemplateURLServiceFactory::GetForProfile(profile);
    tus->SetUserSelectedDefaultSearchProvider(new TemplateURL(*data));
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
  // Cache DSE.
  if (!active) {
    std::unique_ptr<base::DictionaryValue> data_value =
        TemplateURLDataToDictionary(tu->data());
    profile->GetPrefs()->Set(kCachedNormalSearchProvider, data_value->Clone());
    return;
  }

  const base::Value* data_value =
      profile->GetPrefs()->Get(kCachedNormalSearchProvider);
  // Apply cached DSE.
  if (data_value && data_value->is_dict()) {
    std::unique_ptr<TemplateURLData> data =
        TemplateURLDataFromDictionary(base::Value::AsDictionaryValue(*data_value));
    tus->SetUserSelectedDefaultSearchProvider(new TemplateURL(*data));
  }
}

}  // namespace

BraveBrowserFrame::BraveBrowserFrame(BrowserView* browser_view)
    : BrowserFrame(browser_view),
      view_(browser_view) {
  ObserveWidget();
  ObserveSearchEngineProviderPrefs();
}

BraveBrowserFrame::~BraveBrowserFrame() = default;

// Tor/Guest profile should use DarkAura. If not, their native ui is affected by
// normal windows theme change.
const ui::NativeTheme* BraveBrowserFrame::GetNativeTheme() const {
  if ((view_->browser()->profile()->IsIncognitoProfile() ||
       view_->browser()->profile()->IsTor() ||
       brave::IsGuestProfile(view_->browser()->profile())) &&
      ThemeServiceFactory::GetForProfile(view_->browser()->profile())
          ->UsingDefaultTheme()) {
    return ui::NativeTheme::GetInstanceForDarkUI();
  }
  return views::Widget::GetNativeTheme();
}

void BraveBrowserFrame::ObserveWidget() {
  auto* profile = view_->browser()->profile();
  if (!brave::IsGuestProfile(profile)) {
    observation_.Observe(this);
  }
}

void BraveBrowserFrame::ObserveSearchEngineProviderPrefs() {
  auto* profile = view_->browser()->profile();
  if (profile->IsTor() || brave::IsGuestProfile(profile)) {
    return;
  }

  if (profile->IsIncognitoProfile()) {
    use_alternative_search_engine_provider_.Init(
        kUseAlternativeSearchEngineProvider,
        profile->GetOriginalProfile()->GetPrefs(),
        base::Bind(&BraveBrowserFrame::OnPreferenceChanged,
                   base::Unretained(this)));
  }
}

void BraveBrowserFrame::OnPreferenceChanged() {
  HandlePrivateWindowActivationStateChange(view_->browser()->profile(), true);
}

// TODO(simonhong): Create separate class for this.
void BraveBrowserFrame::OnWidgetActivationChanged(Widget* widget, bool active) {
  auto* profile = view_->browser()->profile();
  auto* tus = TemplateURLServiceFactory::GetForProfile(profile);
  if (!tus)
    return;

  auto* tu = tus->GetDefaultSearchProvider();
  if (tu && tu->type() == TemplateURL::NORMAL_CONTROLLED_BY_EXTENSION)
    return;

  if (profile->IsTor()) {
    HandleTorWindowActivationStateChange(profile, active);
    return;
  }

  if (brave::IsGuestProfile(profile)) {
    NOTREACHED();
    return;
  }

  if (profile->IsIncognitoProfile()) {
    HandlePrivateWindowActivationStateChange(profile, active);
    return;
  }

  HandleNormalWindowActivationStateChange(profile, active);
}
