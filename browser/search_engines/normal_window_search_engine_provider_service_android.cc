/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/search_engines/normal_window_search_engine_provider_service_android.h"

#include <memory>

#include "base/functional/bind.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "components/prefs/pref_service.h"
#include "components/search_engines/search_engines_pref_names.h"
#include "components/search_engines/template_url_data_util.h"
#include "components/search_engines/template_url_prepopulate_data.h"
#include "components/search_engines/template_url_service.h"

NormalWindowSearchEngineProviderServiceAndroid::
    NormalWindowSearchEngineProviderServiceAndroid(Profile* profile)
    : profile_(profile) {
  auto* service = TemplateURLServiceFactory::GetForProfile(profile_);
  if (service->loaded()) {
    PrepareInitialPrivateSearchProvider();
    return;
  }

  // Using Unretained safe with subscription_.
  template_url_service_subscription_ = service->RegisterOnLoadedCallback(
      base::BindOnce(&NormalWindowSearchEngineProviderServiceAndroid::
                         OnTemplateURLServiceLoaded,
                     base::Unretained(this)));
}

NormalWindowSearchEngineProviderServiceAndroid::
    ~NormalWindowSearchEngineProviderServiceAndroid() = default;

void NormalWindowSearchEngineProviderServiceAndroid::Shutdown() {
  template_url_service_subscription_ = {};
}

void NormalWindowSearchEngineProviderServiceAndroid::
    OnTemplateURLServiceLoaded() {
  template_url_service_subscription_ = {};
  PrepareInitialPrivateSearchProvider();
}

void NormalWindowSearchEngineProviderServiceAndroid::
    PrepareInitialPrivateSearchProvider() {
  auto* prefs = profile_->GetPrefs();
  auto* preference =
      prefs->FindPreference(prefs::kSyncedDefaultPrivateSearchProviderData);

  if (preference->IsDefaultValue()) {
    return;
  }

  auto private_url_data =
      TemplateURLDataFromDictionary(preference->GetValue()->GetDict());
  auto* service = TemplateURLServiceFactory::GetForProfile(profile_);
  if (!service->GetTemplateURLForGUID(private_url_data->sync_guid)) {
    // This means updated default provider list doesn't include previously used
    // default search provider. So, we should add it explicitely.
    private_url_data->id = kInvalidTemplateURLID;
    service->Add(std::make_unique<TemplateURL>(*private_url_data));
  }
}
