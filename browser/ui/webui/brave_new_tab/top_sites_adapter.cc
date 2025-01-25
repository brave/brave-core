// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/brave_new_tab/top_sites_adapter.h"

#include <utility>

#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/ui/webui/new_tab_page/ntp_pref_names.h"
#include "components/ntp_tiles/constants.h"
#include "components/prefs/pref_service.h"

namespace brave_new_tab {

namespace {

std::vector<mojom::TopSitePtr> TopSitesFromSections(
    const std::map<ntp_tiles::SectionType, ntp_tiles::NTPTilesVector>&
        sections) {
  std::vector<mojom::TopSitePtr> top_sites;
  for (auto& tile : sections.at(ntp_tiles::SectionType::PERSONALIZED)) {
    auto site = mojom::TopSite::New();
    site->title = base::UTF16ToUTF8(tile.title);
    site->url = tile.url.spec();
    site->favicon = tile.favicon_url.spec();
    if (site->title.empty()) {
      site->title = site->url;
    }
    top_sites.push_back(std::move(site));
  }
  return top_sites;
}

}  // namespace

TopSitesAdapter::TopSitesAdapter(
    std::unique_ptr<ntp_tiles::MostVisitedSites> most_visited_sites,
    PrefService& pref_service)
    : most_visited_sites_(std::move(most_visited_sites)),
      pref_service_(pref_service) {
  CHECK(most_visited_sites_);
  most_visited_sites_->SetShortcutsVisible(GetTopSitesVisible());
  most_visited_sites_->EnableCustomLinks(GetListKind() ==
                                         mojom::TopSitesListKind::kCustom);
  most_visited_sites_->AddMostVisitedURLsObserver(
      this, ntp_tiles::kMaxNumMostVisited);
}

TopSitesAdapter::~TopSitesAdapter() = default;

bool TopSitesAdapter::GetTopSitesVisible() {
  return pref_service_->GetBoolean(ntp_prefs::kNtpShortcutsVisible);
}

void TopSitesAdapter::SetTopSitesVisible(bool visible) {
  pref_service_->SetBoolean(ntp_prefs::kNtpShortcutsVisible, visible);
  most_visited_sites_->SetShortcutsVisible(visible);
}

mojom::TopSitesListKind TopSitesAdapter::GetListKind() {
  bool use_most_visited =
      pref_service_->GetBoolean(ntp_prefs::kNtpUseMostVisitedTiles);
  return use_most_visited ? mojom::TopSitesListKind::kMostVisited
                          : mojom::TopSitesListKind::kCustom;
}

void TopSitesAdapter::SetListKind(mojom::TopSitesListKind list_kind) {
  bool use_most_visited = list_kind == mojom::TopSitesListKind::kMostVisited;
  pref_service_->SetBoolean(ntp_prefs::kNtpUseMostVisitedTiles,
                            use_most_visited);
  most_visited_sites_->EnableCustomLinks(!use_most_visited);
}

void TopSitesAdapter::GetSites(GetSitesCallback callback) {
  std::vector<mojom::TopSitePtr> sites;
  sites.reserve(current_sites_.size());
  for (auto& site : current_sites_) {
    sites.push_back(site.Clone());
  }
  std::move(callback).Run(std::move(sites));
}

void TopSitesAdapter::AddCustomSite(std::string_view url,
                                    std::string_view title) {
  most_visited_sites_->AddCustomLink(GURL(url), base::UTF8ToUTF16(title));
}

void TopSitesAdapter::UpdateCustomSite(std::string_view url,
                                       std::string_view new_url,
                                       std::string_view title) {
  GURL updated_url(new_url);

  // If we are not changing the URL, then `most_visited_sites_` will expect the
  // "new_url" parameter to be empty.
  if (url == new_url) {
    updated_url = GURL();
  }

  most_visited_sites_->UpdateCustomLink(GURL(url), updated_url,
                                        base::UTF8ToUTF16(title));
}

void TopSitesAdapter::SetCustomSitePosition(std::string_view url,
                                            int32_t position) {
  most_visited_sites_->ReorderCustomLink(GURL(url), position);
}

void TopSitesAdapter::RemoveCustomSite(std::string_view url) {
  most_visited_sites_->DeleteCustomLink(GURL(url));
}

void TopSitesAdapter::UndoCustomSiteAction() {
  most_visited_sites_->UndoCustomLinkAction();
}

void TopSitesAdapter::ExcludeMostVisitedSite(std::string_view url) {
  most_visited_sites_->AddOrRemoveBlockedUrl(GURL(url), true);
}

void TopSitesAdapter::IncludeMostVisitedTopSite(std::string_view url) {
  most_visited_sites_->AddOrRemoveBlockedUrl(GURL(url), false);
}

void TopSitesAdapter::SetSitesUpdatedCallback(
    base::RepeatingCallback<void()> callback) {
  sites_updated_callback_ = std::move(callback);
}

void TopSitesAdapter::OnURLsAvailable(
    const std::map<ntp_tiles::SectionType, ntp_tiles::NTPTilesVector>&
        sections) {
  current_sites_ = TopSitesFromSections(sections);
  if (sites_updated_callback_) {
    sites_updated_callback_.Run();
  }
}

void TopSitesAdapter::OnIconMadeAvailable(const GURL& site_url) {}

}  // namespace brave_new_tab
