// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/brave_new_tab_page_refresh/top_sites_facade.h"

#include <utility>

#include "base/check.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/ui/webui/new_tab_page/ntp_pref_names.h"
#include "components/ntp_tiles/constants.h"
#include "components/ntp_tiles/tile_type.h"
#include "components/prefs/pref_service.h"

namespace brave_new_tab_page_refresh {

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

TopSitesFacade::TopSitesFacade(
    std::unique_ptr<ntp_tiles::MostVisitedSites> most_visited_sites,
    PrefService& pref_service)
    : most_visited_sites_(std::move(most_visited_sites)),
      pref_service_(pref_service) {
  CHECK(most_visited_sites_);

  pref_change_registrar_.Init(&pref_service_.get());
  pref_change_registrar_.Add(ntp_prefs::kNtpShortcutsVisible,
                             base::BindRepeating(&TopSitesFacade::OnPrefChanged,
                                                 weak_factory_.GetWeakPtr()));
  pref_change_registrar_.Add(ntp_prefs::kNtpShortcutsType,
                             base::BindRepeating(&TopSitesFacade::OnPrefChanged,
                                                 weak_factory_.GetWeakPtr()));

  SyncMostVisitedSites();
  most_visited_sites_->AddMostVisitedURLsObserver(
      this, ntp_tiles::kMaxNumMostVisited);
}

TopSitesFacade::~TopSitesFacade() = default;

bool TopSitesFacade::GetTopSitesVisible() {
  return pref_service_->GetBoolean(ntp_prefs::kNtpShortcutsVisible);
}

void TopSitesFacade::SetTopSitesVisible(bool visible) {
  pref_service_->SetBoolean(ntp_prefs::kNtpShortcutsVisible, visible);
}

mojom::TopSitesListKind TopSitesFacade::GetListKind() {
  auto tile_type = static_cast<ntp_tiles::TileType>(
      pref_service_->GetInteger(ntp_prefs::kNtpShortcutsType));
  if (tile_type == ntp_tiles::TileType::kCustomLinks) {
    return mojom::TopSitesListKind::kCustom;
  }
  return mojom::TopSitesListKind::kMostVisited;
}

void TopSitesFacade::SetListKind(mojom::TopSitesListKind list_kind) {
  pref_service_->SetInteger(
      ntp_prefs::kNtpShortcutsType,
      static_cast<int>(list_kind == mojom::TopSitesListKind::kMostVisited
                           ? ntp_tiles::TileType::kTopSites
                           : ntp_tiles::TileType::kCustomLinks));
}

void TopSitesFacade::GetSites(GetSitesCallback callback) {
  std::vector<mojom::TopSitePtr> sites;
  sites.reserve(current_sites_.size());
  for (auto& site : current_sites_) {
    sites.push_back(site.Clone());
  }
  std::move(callback).Run(std::move(sites));
}

void TopSitesFacade::AddCustomSite(std::string_view url,
                                   std::string_view title) {
  if (GURL site_url(url); site_url.is_valid()) {
    most_visited_sites_->AddCustomLink(site_url, base::UTF8ToUTF16(title));
  }
}

void TopSitesFacade::UpdateCustomSite(std::string_view url,
                                      std::string_view new_url,
                                      std::string_view title) {
  GURL original_url(url);
  GURL updated_url(new_url);
  if (!original_url.is_valid() || !updated_url.is_valid()) {
    return;
  }

  // If we are not changing the URL, then `most_visited_sites_` will expect the
  // "new_url" parameter to be empty.
  if (url == new_url) {
    updated_url = GURL();
  }

  most_visited_sites_->UpdateCustomLink(original_url, updated_url,
                                        base::UTF8ToUTF16(title));
}

void TopSitesFacade::SetCustomSitePosition(std::string_view url,
                                           int32_t position) {
  if (GURL site_url(url); site_url.is_valid()) {
    most_visited_sites_->ReorderCustomLink(site_url, position);
  }
}

void TopSitesFacade::RemoveCustomSite(std::string_view url) {
  if (GURL site_url(url); site_url.is_valid()) {
    most_visited_sites_->DeleteCustomLink(site_url);
  }
}

void TopSitesFacade::UndoCustomSiteAction() {
  most_visited_sites_->UndoCustomLinkAction();
}

void TopSitesFacade::ExcludeMostVisitedSite(std::string_view url) {
  if (GURL site_url(url); site_url.is_valid()) {
    most_visited_sites_->AddOrRemoveBlockedUrl(site_url, true);
  }
}

void TopSitesFacade::IncludeMostVisitedTopSite(std::string_view url) {
  if (GURL site_url(url); site_url.is_valid()) {
    most_visited_sites_->AddOrRemoveBlockedUrl(site_url, false);
  }
}

void TopSitesFacade::SetSitesUpdatedCallback(
    base::RepeatingCallback<void()> callback) {
  sites_updated_callback_ = std::move(callback);
}

void TopSitesFacade::OnURLsAvailable(
    bool is_user_triggered,
    const std::map<ntp_tiles::SectionType, ntp_tiles::NTPTilesVector>&
        sections) {
  current_sites_ = TopSitesFromSections(sections);
  if (sites_updated_callback_) {
    sites_updated_callback_.Run();
  }
}

void TopSitesFacade::OnIconMadeAvailable(const GURL& site_url) {}

void TopSitesFacade::SyncMostVisitedSites() {
  most_visited_sites_->SetShortcutsVisible(GetTopSitesVisible());
  most_visited_sites_->EnableTileTypes(
      ntp_tiles::MostVisitedSites::EnableTileTypesOptions().with_custom_links(
          GetListKind() == mojom::TopSitesListKind::kCustom));
}

void TopSitesFacade::OnPrefChanged(const std::string& path) {
  SyncMostVisitedSites();
}

}  // namespace brave_new_tab_page_refresh
