/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/search_engines/search_engine_provider_service.h"

#include <utility>
#include <vector>

#include "brave/browser/search_engines/search_engine_provider_util.h"
#include "brave/common/pref_names.h"
#include "brave/components/search_engines/brave_prepopulated_engines.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "components/prefs/pref_service.h"
#include "components/search_engines/prepopulated_engines.h"
#include "components/search_engines/template_url_data_util.h"
#include "components/search_engines/template_url_prepopulate_data.h"
#include "components/search_engines/template_url_service.h"
#include "extensions/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "extensions/browser/extension_prefs.h"
#endif

SearchEngineProviderService::SearchEngineProviderService(
    Profile* otr_profile)
    : otr_profile_(otr_profile),
      original_template_url_service_(
          TemplateURLServiceFactory::GetForProfile(
              otr_profile_->GetOriginalProfile())),
      otr_template_url_service_(
          TemplateURLServiceFactory::GetForProfile(otr_profile_)) {
  use_alternative_search_engine_provider_.Init(
      kUseAlternativeSearchEngineProvider,
      otr_profile_->GetOriginalProfile()->GetPrefs(),
      base::Bind(&SearchEngineProviderService::OnPreferenceChanged,
                 base::Unretained(this)));

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
        otr_profile->GetPrefs(), id);
    if (data)
      break;
  }

  // There should ALWAYS be one entry
  DCHECK(data);
  alternative_search_engine_url_.reset(new TemplateURL(*data));
}

SearchEngineProviderService::~SearchEngineProviderService() = default;

void SearchEngineProviderService::OnPreferenceChanged(
    const std::string& pref_name) {
  DCHECK(pref_name == kUseAlternativeSearchEngineProvider);
  DCHECK(!brave::IsRegionForQwant(otr_profile_));

  OnUseAlternativeSearchEngineProviderChanged();
}

bool
SearchEngineProviderService::UseAlternativeSearchEngineProvider() const {
  return use_alternative_search_engine_provider_.GetValue();
}

void SearchEngineProviderService::ChangeToAlternativeSearchEngineProvider() {
  otr_template_url_service_->SetUserSelectedDefaultSearchProvider(
      alternative_search_engine_url_.get());
}

void SearchEngineProviderService::ChangeToNormalWindowSearchEngineProvider() {
  TemplateURL normal_url(
      original_template_url_service_->GetDefaultSearchProvider()->data());
  otr_template_url_service_->SetUserSelectedDefaultSearchProvider(
      &normal_url);
}

void SearchEngineProviderService::UseExtensionSearchProvider() {
#if BUILDFLAG(ENABLE_EXTENSIONS)
  DCHECK(ShouldUseExtensionSearchProvider());

  const auto* extension_provider_url =
      original_template_url_service_->GetDefaultSearchProvider();
  DCHECK(extension_provider_url);
  auto data = extension_provider_url->data();
  data.id = kInvalidTemplateURLID;

  // Can't add same turl again to service.
  if (CouldAddExtensionTemplateURL(extension_provider_url)) {
    auto type = extension_provider_url->type();
    auto extension_id = extension_provider_url->GetExtensionId();
    extensions::ExtensionPrefs* prefs =
        extensions::ExtensionPrefs::Get(otr_profile_->GetOriginalProfile());
    auto time = prefs->GetInstallTime(extension_id);

    auto turl =
        std::make_unique<TemplateURL>(data, type, extension_id, time, true);

    otr_template_url_service_->Add(std::move(turl));
  }

  otr_profile_->GetPrefs()->Set(
      DefaultSearchManager::kDefaultSearchProviderDataPrefName,
      *TemplateURLDataToDictionary(data));
#endif
}

bool SearchEngineProviderService::ShouldUseExtensionSearchProvider() const {
  return original_template_url_service_->IsExtensionControlledDefaultSearch();
}

bool SearchEngineProviderService::CouldAddExtensionTemplateURL(
    const TemplateURL* url) {
  DCHECK(url);
  DCHECK_NE(TemplateURL::NORMAL, url->type());
  for (const auto* turl : otr_template_url_service_->GetTemplateURLs()) {
    DCHECK(turl);
    if (url->type() == turl->type() &&
        url->GetExtensionId() == turl->GetExtensionId())
      return false;
  }
  return true;
}
