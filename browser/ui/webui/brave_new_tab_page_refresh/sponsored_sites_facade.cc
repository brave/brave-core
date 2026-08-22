// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/brave_new_tab_page_refresh/sponsored_sites_facade.h"

#include <utility>
#include <vector>

#include "base/containers/to_vector.h"
#include "base/functional/bind.h"
#include "base/task/bind_post_task.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/buildflags/buildflags.h"
#include "brave/components/brave_rewards/core/buildflags/buildflags.h"
#include "brave/components/ntp_background_images/browser/ntp_sponsored_sites_data.h"
#include "brave/components/ntp_background_images/common/pref_names.h"
#include "components/history/core/browser/history_service.h"
#include "components/history/core/browser/history_types.h"
#include "components/prefs/pref_service.h"
#include "url/gurl.h"

#if BUILDFLAG(ENABLE_BRAVE_ADS)
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"
#endif  // BUILDFLAG(ENABLE_BRAVE_ADS)

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

// The advertiser's hostname, with any leading "www." stripped so a visit to
// either the www or non-www variant counts as the same domain.
std::string AdvertiserDomain(const GURL& url) {
  std::string host(url.host());
  if (host.starts_with("www.")) {
    return host.substr(4);
  }
  return host;
}

// `HistoryService` matches hosts exactly, with no `www.` normalization, so both
// variants must be queried. `domain` must not have a `www.` prefix.
std::vector<std::string> HostVariants(const std::string& domain) {
  return {domain, "www." + domain};
}

}  // namespace

SponsoredSitesFacade::SponsoredSitesFacade(
    PrefService& pref_service,
    ntp_background_images::NTPBackgroundImagesService*
        background_images_service,
    history::HistoryService& history_service)
    : pref_service_(pref_service),
      background_images_service_(background_images_service),
      history_service_(history_service) {
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

  std::vector<mojom::SponsoredSitePtr> sites = ToMojom(sites_data->sites);
  for (auto& site : sites) {
    site->has_genuine_visit = genuine_visit_domains_.contains(
        AdvertiserDomain(GURL(site->target_url)));
  }
  QueryGenuineVisits(sites);

  std::move(callback).Run(std::move(sites));
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
#if BUILDFLAG(ENABLE_BRAVE_ADS)
  return pref_service_->GetBoolean(brave_ads::prefs::kSponsoredEnabled);
#else
  return false;
#endif  // BUILDFLAG(ENABLE_BRAVE_ADS)
}

bool SponsoredSitesFacade::IsNewTabPageAdsEnabled() const {
  return pref_service_->GetBoolean(
             ntp_background_images::prefs::kNewTabPageShowBackgroundImage) &&
         IsEnabled();
}

bool SponsoredSitesFacade::IsRewardsWalletConnected() const {
#if BUILDFLAG(ENABLE_BRAVE_REWARDS)
  return !pref_service_->GetString(brave_rewards::prefs::kExternalWalletType)
              .empty();
#else
  return false;
#endif  // BUILDFLAG(ENABLE_BRAVE_REWARDS)
}

void SponsoredSitesFacade::QueryGenuineVisits(
    const std::vector<mojom::SponsoredSitePtr>& sites) {
  history_task_tracker_.TryCancelAll();
  for (auto& site : sites) {
    if (site->has_genuine_visit) {
      continue;
    }
    GURL target_url(site->target_url);
    if (!target_url.is_valid()) {
      continue;
    }
    // Matches by host rather than exact URL, since the advertiser's own
    // landing page frequently isn't the page a visit was actually recorded
    // against (redirects, trailing slashes).
    const std::string domain = AdvertiserDomain(target_url);
    for (auto& host : HostVariants(domain)) {
      history_service_->GetLastVisitToHost(
          host, base::Time(), base::Time::Max(),
          history::VisitQuery404sPolicy::kInclude404s,
          base::BindOnce(&SponsoredSitesFacade::OnGenuineVisitQueried,
                         weak_factory_.GetWeakPtr(), domain),
          &history_task_tracker_);
    }
  }
}

void SponsoredSitesFacade::OnGenuineVisitQueried(
    const std::string& advertiser_domain,
    history::HistoryLastVisitResult result) {
  // Never downgrades: a false result from one host variant doesn't rule
  // out a confirming visit from the other, still-pending variant.
  if (!result.success || result.last_visit.is_null()) {
    return;
  }

  if (!genuine_visit_domains_.insert(advertiser_domain).second) {
    return;
  }

  if (updated_callback_) {
    updated_callback_.Run();
  }
}

}  // namespace brave_new_tab_page_refresh
