/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/guest_window_search_engine_provider_controller.h"

#include "chrome/browser/profiles/profile.h"
#include "components/search_engines/template_url_service.h"

GuestWindowSearchEngineProviderController::
GuestWindowSearchEngineProviderController(Profile* profile)
    : SearchEngineProviderControllerBase(profile) {
  DCHECK_EQ(profile->GetProfileType(), Profile::GUEST_PROFILE);
}

GuestWindowSearchEngineProviderController::
~GuestWindowSearchEngineProviderController() {
}

void GuestWindowSearchEngineProviderController::OnTemplateURLServiceChanged() {
}
