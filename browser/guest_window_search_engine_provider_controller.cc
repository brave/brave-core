/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/guest_window_search_engine_provider_controller.h"

#include "base/auto_reset.h"
#include "brave/browser/search_engine_provider_util.h"
#include "chrome/browser/profiles/profile.h"
#include "components/search_engines/template_url_service.h"

GuestWindowSearchEngineProviderController::
GuestWindowSearchEngineProviderController(Profile* profile)
    : SearchEngineProviderControllerBase(profile) {
  DCHECK_EQ(profile->GetProfileType(), Profile::GUEST_PROFILE);

  // Monitor otr(off the record) profile's search engine changing to tracking
  // user's default search engine provider.
  // OTR profile's service is used for that.
  otr_template_url_service_->AddObserver(this);
  ConfigureSearchEngineProvider();
}

GuestWindowSearchEngineProviderController::
~GuestWindowSearchEngineProviderController() {
  otr_template_url_service_->RemoveObserver(this);
}

void GuestWindowSearchEngineProviderController::OnTemplateURLServiceChanged() {
  if (ignore_template_url_service_changing_)
    return;

  // Prevent search engine changing from settings page for tor profile.
  // TODO(simonhong): Revisit when related ux is determined.
  if (otr_profile_->IsTorProfile()) {
    base::AutoReset<bool> reset(&ignore_template_url_service_changing_, true);
    ChangeToAlternativeSearchEngineProvider();
    return;
  }


  // The purpose of below code is turn off alternative prefs
  // when user changes to different search engine provider from settings.
  // However, this callback is also called during the TemplateURLService
  // initialization phase. Because of this, guest view always starts with
  // this prefs off state when browser restarted(persisted during the runtime).
  // Currently I don't know how to determine who is caller of this callback.
  // TODO(simonhong): Revisit here when brave's related ux is determined.
  if (UseAlternativeSearchEngineProvider())
    brave::ToggleUseAlternativeSearchEngineProvider(otr_profile_);
}

void
GuestWindowSearchEngineProviderController::ConfigureSearchEngineProvider() {
  base::AutoReset<bool> reset(&ignore_template_url_service_changing_, true);
  UseAlternativeSearchEngineProvider()
      ? ChangeToAlternativeSearchEngineProvider()
      : ChangeToNormalWindowSearchEngineProvider();
}
