// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/brave_new_tab_page_refresh/sponsored_sites_facade.h"

#include <utility>
#include <vector>

#include "base/containers/to_vector.h"
#include "base/task/bind_post_task.h"
#include "brave/components/brave_rewards/core/buildflags/buildflags.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/ntp_background_images/browser/ntp_sponsored_sites_data.h"
#include "brave/components/ntp_background_images/common/pref_names.h"
#include "components/prefs/pref_service.h"

#if BUILDFLAG(ENABLE_BRAVE_REWARDS)
#include "brave/components/brave_rewards/core/pref_names.h"
#endif  // BUILDFLAG(ENABLE_BRAVE_REWARDS)

namespace brave_new_tab_page_refresh {

namespace {

mojom::SponsoredSitePtr ToMojom(
    const ntp_background_images::NTPSponsoredSite& ntp_site) {
  mojom::SponsoredSitePtr mojom_site = mojom::SponsoredSite::New();
  mojom_site->relative_image_url = ntp_site.relative_image_url_spec;
  mojom_site->title = ntp_site.title;
  mojom_site->ad_disclosure = ntp_site.ad_disclosure;
  mojom_site->target_url = ntp_site.target_url.spec();
  return mojom_site;
}

std::vector<mojom::SponsoredSitePtr> ToMojom(
    const std::vector<ntp_background_images::NTPSponsoredSite>& ntp_sites) {
  return base::ToVector(ntp_sites,
                        [](const auto& ntp_site) { return ToMojom(ntp_site); });
}

}  // namespace

SponsoredSitesFacade::SponsoredSitesFacade(
    PrefService& pref_service,
    ntp_background_images::NTPBackgroundImagesService*
        background_images_service)
    : pref_service_(pref_service),
      background_images_service_(background_images_service) {
  if (background_images_service_) {
    ntp_background_images_service_observation_.Observe(
        background_images_service_);
  }
}

SponsoredSitesFacade::~SponsoredSitesFacade() = default;

void SponsoredSitesFacade::GetSites(GetSitesCallback callback) {
  // Keep delivery consistently asynchronous across all branches below.
  callback = base::BindPostTaskToCurrentDefault(std::move(callback));

  if (!IsEligible() || !background_images_service_) {
    std::move(callback).Run({});
    return;
  }

  const ntp_background_images::NTPSponsoredSitesData* const sites_data =
      background_images_service_->GetSponsoredSitesData();
  if (!sites_data || sites_data->sites.empty()) {
    std::move(callback).Run({});
    return;
  }

  std::move(callback).Run(ToMojom(sites_data->sites));
}

void SponsoredSitesFacade::SetSitesUpdatedCallback(
    base::RepeatingClosure callback) {
  updated_callback_ = std::move(callback);
}

void SponsoredSitesFacade::OnSponsoredSitesDataDidUpdate() {
  if (updated_callback_) {
    updated_callback_.Run();
  }
}

bool SponsoredSitesFacade::IsEligible() const {
  return IsEnabled() && IsNewTabPageAdsEnabled() && !IsRewardsWalletConnected();
}

bool SponsoredSitesFacade::IsEnabled() const {
  return pref_service_->GetBoolean(kNewTabPageShowSponsoredSites);
}

bool SponsoredSitesFacade::IsNewTabPageAdsEnabled() const {
  return pref_service_->GetBoolean(
             ntp_background_images::prefs::kNewTabPageShowBackgroundImage) &&
         pref_service_->GetBoolean(
             ntp_background_images::prefs::
                 kNewTabPageShowSponsoredImagesBackgroundImage);
}

bool SponsoredSitesFacade::IsRewardsWalletConnected() const {
#if BUILDFLAG(ENABLE_BRAVE_REWARDS)
  return !pref_service_->GetString(brave_rewards::prefs::kExternalWalletType)
              .empty();
#else
  return false;
#endif  // BUILDFLAG(ENABLE_BRAVE_REWARDS)
}

}  // namespace brave_new_tab_page_refresh
