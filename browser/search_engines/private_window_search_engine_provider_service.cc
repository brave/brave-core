/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/search_engines/private_window_search_engine_provider_service.h"

#include "brave/browser/search_engines/search_engine_provider_util.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"
#include "components/search_engines/default_search_manager.h"
#include "components/search_engines/search_engines_pref_names.h"
#include "components/search_engines/template_url.h"

PrivateWindowSearchEngineProviderService::
    PrivateWindowSearchEngineProviderService(Profile* otr_profile)
    : PrivateWindowSearchEngineProviderServiceBase(otr_profile) {
  DCHECK(otr_profile->IsIncognitoProfile());
  private_search_provider_guid_.Init(
      prefs::kSyncedDefaultPrivateSearchProviderGUID,
      otr_profile_->GetOriginalProfile()->GetPrefs(),
      base::BindRepeating(
          &PrivateWindowSearchEngineProviderService::OnPreferenceChanged,
          base::Unretained(this)));
}

PrivateWindowSearchEngineProviderService::
    ~PrivateWindowSearchEngineProviderService() = default;

void PrivateWindowSearchEngineProviderService::
    UpdateExtensionPrefsAndProvider() {
  const bool use_extension_provider = ShouldUseExtensionSearchProvider();
  otr_profile_->GetPrefs()->SetBoolean(prefs::kDefaultSearchProviderByExtension,
                                       use_extension_provider);

  if (use_extension_provider) {
    UseExtensionSearchProvider();
  } else {
    ConfigurePrivateWindowSearchEngineProvider();
  }
}

void PrivateWindowSearchEngineProviderService::OnTemplateURLServiceChanged() {
  UpdateExtensionPrefsAndProvider();
}

void PrivateWindowSearchEngineProviderService::
    ConfigurePrivateWindowSearchEngineProvider() {
  if (auto* template_url =
          original_template_url_service_->GetTemplateURLForGUID(
              private_search_provider_guid_.GetValue())) {
    otr_template_url_service_->SetUserSelectedDefaultSearchProvider(
        template_url);
  }
}

void PrivateWindowSearchEngineProviderService::Initialize() {
  UpdateExtensionPrefsAndProvider();

  // Monitor normal profile's search engine changing because private window
  // should use that search engine provider when extension search provider is
  // used.
  observation_.Observe(original_template_url_service_);
}

void PrivateWindowSearchEngineProviderService::Shutdown() {
  PrivateWindowSearchEngineProviderServiceBase::Shutdown();
  observation_.Reset();
}

void PrivateWindowSearchEngineProviderService::OnPreferenceChanged(
    const std::string& pref_name) {
  // Don't update when extension's search provider is activated.
  // It has more higher priority than settings configuration.
  if (ShouldUseExtensionSearchProvider())
    return;

  ConfigurePrivateWindowSearchEngineProvider();
}
