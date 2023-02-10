/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/search_engines/private_window_search_engine_provider_service_android.h"

#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "components/prefs/pref_service.h"
#include "components/search_engines/search_engines_pref_names.h"
#include "components/search_engines/template_url.h"
#include "components/search_engines/template_url_data.h"
#include "components/search_engines/template_url_data_util.h"

PrivateWindowSearchEngineProviderServiceAndroid::
    PrivateWindowSearchEngineProviderServiceAndroid(Profile* otr_profile)
    : otr_profile_(otr_profile) {
  DCHECK(otr_profile_->IsIncognitoProfile());
  observation_.Observe(TemplateURLServiceFactory::GetForProfile(otr_profile_));
}

PrivateWindowSearchEngineProviderServiceAndroid::
    ~PrivateWindowSearchEngineProviderServiceAndroid() = default;

void PrivateWindowSearchEngineProviderServiceAndroid::
    OnTemplateURLServiceChanged() {
  auto* service = TemplateURLServiceFactory::GetForProfile(otr_profile_);
  if (auto* url = service->GetDefaultSearchProvider()) {
    otr_profile_->GetOriginalProfile()->GetPrefs()->SetDict(
        prefs::kSyncedDefaultPrivateSearchProviderData,
        TemplateURLDataToDictionary(url->data()));
  }
}

void PrivateWindowSearchEngineProviderServiceAndroid::Shutdown() {
  observation_.Reset();
}
