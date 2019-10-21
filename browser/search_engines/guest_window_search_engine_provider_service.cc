/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/search_engines/guest_window_search_engine_provider_service.h"

#include "base/auto_reset.h"
#include "brave/browser/profiles/profile_util.h"
#include "brave/browser/search_engines/search_engine_provider_util.h"
#include "brave/components/search_engines/brave_prepopulated_engines.h"
#include "chrome/browser/profiles/profile.h"
#include "components/search_engines/template_url_prepopulate_data.h"
#include "components/search_engines/template_url_service.h"

// Guest windows starts with default search engine provider because it's guest.
GuestWindowSearchEngineProviderService::GuestWindowSearchEngineProviderService(
    Profile* otr_profile)
    : SearchEngineProviderService(otr_profile) {
  DCHECK(brave::IsGuestProfile(otr_profile));
  DCHECK(otr_profile->IsOffTheRecord());
  DCHECK(!brave::IsRegionForQwant(otr_profile));

  // Monitor otr(off the record) profile's search engine changing to tracking
  // user's default search engine provider.
  // OTR profile's service is used for that in guest window.
  otr_template_url_service_->AddObserver(this);
}

GuestWindowSearchEngineProviderService::
~GuestWindowSearchEngineProviderService() {
  otr_template_url_service_->RemoveObserver(this);
}

void GuestWindowSearchEngineProviderService::OnTemplateURLServiceChanged() {
  // When this change is came from pref changing, we don't need to control
  // prefs again.
  if (ignore_template_url_service_changing_)
    return;

  // The purpose of below code is toggling alternative prefs
  // when user changes from ddg to different search engine provider
  // (or vice versa) from settings ui.
  bool is_ddg_is_set = false;
  switch (otr_template_url_service_->GetDefaultSearchProvider()
              ->data()
              .prepopulate_id) {
    case TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_DUCKDUCKGO:
    case TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_DUCKDUCKGO_DE:
    case TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_DUCKDUCKGO_AU_NZ_IE:
      is_ddg_is_set = true;
  }

  if (UseAlternativeSearchEngineProvider() || is_ddg_is_set)
    brave::ToggleUseAlternativeSearchEngineProvider(otr_profile_);
}

void GuestWindowSearchEngineProviderService::
OnUseAlternativeSearchEngineProviderChanged() {
  // When this call is from setting's change, we don't need to set provider
  // again.
  if (ignore_template_url_service_changing_)
    return;

  base::AutoReset<bool> reset(&ignore_template_url_service_changing_, true);
  UseAlternativeSearchEngineProvider()
      ? ChangeToAlternativeSearchEngineProvider()
      : ChangeToNormalWindowSearchEngineProvider();
}
