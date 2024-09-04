/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/search_engines/normal_window_search_engine_provider_service.h"

#include "base/functional/bind.h"
#include "brave/browser/search_engines/search_engine_provider_util.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "components/search_engines/search_engines_pref_names.h"
#include "components/search_engines/template_url_service.h"

NormalWindowSearchEngineProviderService::
    NormalWindowSearchEngineProviderService(Profile* profile)
    : profile_(profile) {
  brave::UpdateDefaultSearchSuggestionsPrefs(g_browser_process->local_state(),
                                             profile_->GetPrefs());
  private_search_provider_guid_.Init(
      prefs::kSyncedDefaultPrivateSearchProviderGUID, profile_->GetPrefs(),
      base::BindRepeating(
          &NormalWindowSearchEngineProviderService::OnPreferenceChanged,
          base::Unretained(this)));

  auto* service = TemplateURLServiceFactory::GetForProfile(profile_);
  if (service->loaded()) {
    PrepareInitialPrivateSearchProvider();
    return;
  }

  // Using Unretained safe with subscription_.
  template_url_service_subscription_ =
      service->RegisterOnLoadedCallback(base::BindOnce(
          &NormalWindowSearchEngineProviderService::OnTemplateURLServiceLoaded,
          base::Unretained(this)));
}

NormalWindowSearchEngineProviderService::
    ~NormalWindowSearchEngineProviderService() = default;

void NormalWindowSearchEngineProviderService::Shutdown() {
  template_url_service_subscription_ = {};
  private_search_provider_guid_.Destroy();
}

void NormalWindowSearchEngineProviderService::OnTemplateURLServiceLoaded() {
  template_url_service_subscription_ = {};
  PrepareInitialPrivateSearchProvider();
}

void NormalWindowSearchEngineProviderService::
    PrepareInitialPrivateSearchProvider() {
  brave::PrepareDefaultPrivateSearchProviderDataIfNeeded(profile_);
}

void NormalWindowSearchEngineProviderService::OnPreferenceChanged() {
  brave::UpdateDefaultPrivateSearchProviderData(profile_);
}
