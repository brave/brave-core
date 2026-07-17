// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/brave_new_tab_page_refresh/sponsored_sites_facade.h"

#include <algorithm>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "base/barrier_callback.h"
#include "base/containers/to_vector.h"
#include "base/strings/strcat.h"
#include "base/task/bind_post_task.h"
#include "brave/components/brave_rewards/core/buildflags/buildflags.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/ntp_background_images/browser/ntp_sponsored_sites_data.h"
#include "brave/components/ntp_background_images/common/pref_names.h"
#include "components/history/core/browser/history_service.h"
#include "components/history/core/browser/history_types.h"
#include "components/prefs/pref_service.h"
#include "url/gurl.h"

#if BUILDFLAG(ENABLE_BRAVE_REWARDS)
#include "brave/components/brave_rewards/core/pref_names.h"
#endif  // BUILDFLAG(ENABLE_BRAVE_REWARDS)

namespace brave_new_tab_page_refresh {

namespace {

GURL WithWwwDotPrefix(const GURL& url) {
  std::string_view host = url.host();
  if (host.empty() || host.starts_with("www.")) {
    return url;
  }

  const std::string www_host = base::StrCat({"www.", host});

  GURL::Replacements replacements;
  replacements.SetHostStr(www_host);
  return url.ReplaceComponents(replacements);
}

GURL WithoutWwwDotPrefix(const GURL& url) {
  std::string_view host = url.host();
  if (!host.starts_with("www.")) {
    return url;
  }

  host.remove_prefix(4);

  GURL::Replacements replacements;
  replacements.SetHostStr(host);
  return url.ReplaceComponents(replacements);
}

mojom::SponsoredSitePtr ToMojom(
    const ntp_background_images::NTPSponsoredSite& ntp_site) {
  mojom::SponsoredSitePtr mojom_site = mojom::SponsoredSite::New();
  mojom_site->relative_image_url_spec = ntp_site.relative_image_url_spec;
  mojom_site->title = ntp_site.title;
  mojom_site->ad_disclosure = ntp_site.ad_disclosure;
  mojom_site->target_url = ntp_site.target_url;
  return mojom_site;
}

std::vector<mojom::SponsoredSitePtr> ToMojom(
    const std::vector<ntp_background_images::NTPSponsoredSite>& ntp_sites) {
  return base::ToVector(ntp_sites,
                        [](const auto& ntp_site) { return ToMojom(ntp_site); });
}

void OnFilterSiteByHistory(
    mojom::SponsoredSitePtr mojom_site,
    base::RepeatingCallback<void(mojom::SponsoredSitePtr)> callback,
    std::vector<history::VisibleVisitCountToHostResult> host_results) {
  const bool has_visited = std::ranges::any_of(
      host_results,
      [](const history::VisibleVisitCountToHostResult& host_result) {
        return host_result.success && host_result.count > 0;
      });
  callback.Run(has_visited ? std::move(mojom_site) : nullptr);
}

// `HistoryService` treats www and non-www as distinct hosts, so both variants
// are queried and a visit to either counts as a visit.
void FilterSiteByHistory(
    mojom::SponsoredSitePtr mojom_site,
    history::HistoryService& history_service,
    base::CancelableTaskTracker& task_tracker,
    base::RepeatingCallback<void(mojom::SponsoredSitePtr)> callback) {
  const GURL target_url = mojom_site->target_url;
  base::RepeatingCallback<void(history::VisibleVisitCountToHostResult)>
      barrier_callback =
          base::BarrierCallback<history::VisibleVisitCountToHostResult>(
              2, base::BindOnce(&OnFilterSiteByHistory, std::move(mojom_site),
                                callback));
  history_service.GetVisibleVisitCountToHost(WithoutWwwDotPrefix(target_url),
                                             barrier_callback, &task_tracker);
  history_service.GetVisibleVisitCountToHost(WithWwwDotPrefix(target_url),
                                             barrier_callback, &task_tracker);
}

void OnFilterSites(
    base::OnceCallback<void(std::vector<mojom::SponsoredSitePtr>)> callback,
    std::vector<mojom::SponsoredSitePtr> mojom_sites) {
  std::erase_if(mojom_sites, [](const mojom::SponsoredSitePtr& mojom_site) {
    return !mojom_site;
  });
  std::move(callback).Run(std::move(mojom_sites));
}

void FilterSites(
    std::vector<mojom::SponsoredSitePtr> mojom_sites,
    history::HistoryService* history_service,
    base::CancelableTaskTracker& task_tracker,
    base::OnceCallback<void(std::vector<mojom::SponsoredSitePtr>)> callback) {
  if (!history_service) {
    std::move(callback).Run({});
    return;
  }

  base::RepeatingCallback<void(mojom::SponsoredSitePtr)> barrier_callback =
      base::BarrierCallback<mojom::SponsoredSitePtr>(
          mojom_sites.size(),
          base::BindOnce(&OnFilterSites, std::move(callback)));
  for (mojom::SponsoredSitePtr& mojom_site : mojom_sites) {
    FilterSiteByHistory(std::move(mojom_site), *history_service, task_tracker,
                        barrier_callback);
  }
}

}  // namespace

SponsoredSitesFacade::SponsoredSitesFacade(
    PrefService& pref_service,
    ntp_background_images::NTPBackgroundImagesService*
        background_images_service,
    history::HistoryService* history_service)
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

  // Copy before async work: a concurrent component update can invalidate the
  // pointer.
  std::vector<mojom::SponsoredSitePtr> mojom_sites = ToMojom(sites_data->sites);

  FilterSites(std::move(mojom_sites), history_service_, history_task_tracker_,
              std::move(callback));
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
