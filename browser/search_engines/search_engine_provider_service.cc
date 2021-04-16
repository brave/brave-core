/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/search_engines/search_engine_provider_service.h"

#include <vector>

#include "brave/browser/search_engines/search_engine_provider_util.h"
#include "brave/common/pref_names.h"
#include "brave/components/search_engines/brave_prepopulated_engines.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "components/prefs/pref_service.h"
#include "components/search_engines/prepopulated_engines.h"
#include "components/search_engines/template_url_prepopulate_data.h"

SearchEngineProviderService::SearchEngineProviderService(Profile* profile)
    : profile_(profile),
      template_url_service_(
          TemplateURLServiceFactory::GetForProfile(profile_)) {
  use_alternative_search_engine_provider_.Init(
      kUseAlternativeSearchEngineProvider,
      profile_->GetOriginalProfile()->GetPrefs(),
      base::Bind(&SearchEngineProviderService::OnPreferenceChanged,
                 base::Unretained(this)));

  std::unique_ptr<TemplateURLData> data =
      brave::GetDDGTemplateURLData(profile->GetPrefs());
  alternative_search_engine_url_.reset(new TemplateURL(*data));
  observation_.Observe(template_url_service_);
}

SearchEngineProviderService::~SearchEngineProviderService() = default;

void SearchEngineProviderService::OnTemplateURLServiceChanged() {
  // When this change is came from pref changing, we don't need to control
  // prefs again.
  if (ignore_template_url_service_changing_)
    return;

  // The purpose of below code is toggling alternative prefs
  // when user changes from ddg to different search engine provider
  // (or vice versa) from settings ui.
  const bool is_ddg_is_set =
      template_url_service_->GetDefaultSearchProvider()->prepopulate_id() ==
      alternative_search_engine_url_->prepopulate_id();
  if (UseAlternativeSearchEngineProvider() || is_ddg_is_set)
    brave::ToggleUseAlternativeSearchEngineProvider(profile_);
}

void SearchEngineProviderService::OnPreferenceChanged(
    const std::string& pref_name) {
  DCHECK(pref_name == kUseAlternativeSearchEngineProvider);

  // When this call is from setting's change, we don't need to set provider
  // again.
  if (ignore_template_url_service_changing_)
    return;

  base::AutoReset<bool> reset(&ignore_template_url_service_changing_, true);
  UseAlternativeSearchEngineProvider()
      ? ChangeToAlternativeSearchEngineProvider()
      : ChangeToDefaultSearchEngineProvider();
}

bool
SearchEngineProviderService::UseAlternativeSearchEngineProvider() const {
  return use_alternative_search_engine_provider_.GetValue();
}

void SearchEngineProviderService::ChangeToAlternativeSearchEngineProvider() {
  template_url_service_->SetUserSelectedDefaultSearchProvider(
      alternative_search_engine_url_.get());
}

void SearchEngineProviderService::ChangeToDefaultSearchEngineProvider() {
  template_url_service_->SetUserSelectedDefaultSearchProvider(nullptr);
}
