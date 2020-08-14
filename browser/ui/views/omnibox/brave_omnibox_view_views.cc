/* Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/omnibox/brave_omnibox_view_views.h"

#include <utility>

#include "brave/browser/profiles/profile_util.h"
#include "brave/browser/tor/tor_profile_service.h"
#include "brave/browser/tor/tor_profile_service_factory.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/ui/views/location_bar/location_bar_view.h"
#include "ui/base/l10n/l10n_util.h"

BraveOmniboxViewViews::BraveOmniboxViewViews(
    OmniboxEditController* controller,
    std::unique_ptr<OmniboxClient> client,
    bool popup_window_mode,
    LocationBarView* location_bar,
    const gfx::FontList& font_list)
    : OmniboxViewViews(controller,
                       std::move(client),
                       popup_window_mode,
                       location_bar,
                       font_list) {
  if (brave::IsTorProfile(location_bar->profile())) {
    tor_profile_service_ =
        TorProfileServiceFactory::GetForProfile(location_bar->profile());
    DCHECK(tor_profile_service_);
    tor_profile_service_->AddObserver(this);
  }
}

BraveOmniboxViewViews::~BraveOmniboxViewViews() {
  if (tor_profile_service_)
    tor_profile_service_->RemoveObserver(this);
}

void BraveOmniboxViewViews::OnTorCircuitEstablished(bool result) {
  LOG(ERROR) << __func__;
  if (!result) return;
  InstallPlaceholderText();
  RevertAll();
  SetReadOnly(false);
}

void BraveOmniboxViewViews::OnTorInitializing(const std::string& percentage) {
  LOG(ERROR) << __func__ << percentage;
  SetPlaceholderText(
      l10n_util::GetStringUTF16(IDS_OMNIBOX_INITIALIZING_TOR) +
      base::ASCIIToUTF16(percentage + "%"));
  SetUserText(l10n_util::GetStringUTF16(IDS_OMNIBOX_INITIALIZING_TOR) +
              base::ASCIIToUTF16(percentage + "%"),
              false);
  SetReadOnly(true);
  SetFocus(false);
  TextChanged();
}
