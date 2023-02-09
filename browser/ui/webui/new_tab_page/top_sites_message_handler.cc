// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/new_tab_page/top_sites_message_handler.h"

#include <map>
#include <memory>
#include <string>
#include <utility>

#include "base/i18n/rtl.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/browser/ntp_background/view_counter_service_factory.h"
#include "brave/browser/profiles/profile_util.h"
#include "brave/browser/ui/webui/new_tab_page/brave_new_tab_ui.h"
#include "brave/browser/ui/webui/new_tab_page/brave_new_tab_ui_utils.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/ntp_background_images/browser/ntp_sponsored_images_data.h"
#include "brave/components/ntp_background_images/browser/view_counter_service.h"
#include "chrome/browser/ntp_tiles/chrome_most_visited_sites_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/webui/new_tab_page/ntp_pref_names.h"
#include "components/ntp_tiles/constants.h"
#include "components/ntp_tiles/most_visited_sites.h"
#include "components/prefs/pref_service.h"

using ntp_background_images::ViewCounterServiceFactory;

TopSitesMessageHandler::TopSitesMessageHandler(Profile* profile)
    : profile_(profile),
      most_visited_sites_(
          ChromeMostVisitedSitesFactory::NewForProfile(profile)) {
  // most_visited_sites_ can be nullptr if profile is OTR.
  if (most_visited_sites_) {
    most_visited_sites_->EnableCustomLinks(IsCustomLinksEnabled());
    most_visited_sites_->SetShortcutsVisible(IsShortcutsVisible());
    most_visited_sites_->AddMostVisitedURLsObserver(
        this, ntp_tiles::kMaxNumMostVisited);
  }
}

TopSitesMessageHandler::~TopSitesMessageHandler() = default;

void TopSitesMessageHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback(
      "updateMostVisitedInfo",
      base::BindRepeating(&TopSitesMessageHandler::HandleUpdateMostVisitedInfo,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "deleteMostVisitedTile",
      base::BindRepeating(&TopSitesMessageHandler::HandleDeleteMostVisitedTile,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "reorderMostVisitedTile",
      base::BindRepeating(&TopSitesMessageHandler::HandleReorderMostVisitedTile,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "restoreMostVisitedDefaults",
      base::BindRepeating(
          &TopSitesMessageHandler::HandleRestoreMostVisitedDefaults,
          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "undoMostVisitedTileAction",
      base::BindRepeating(
          &TopSitesMessageHandler::HandleUndoMostVisitedTileAction,
          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "setMostVisitedSettings",
      base::BindRepeating(&TopSitesMessageHandler::HandleSetMostVisitedSettings,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "addNewTopSite",
      base::BindRepeating(&TopSitesMessageHandler::HandleAddNewTopSite,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "editTopSite",
      base::BindRepeating(&TopSitesMessageHandler::HandleEditTopSite,
                          base::Unretained(this)));
}

// ntp_tiles::MostVisitedSites::Observer:
void TopSitesMessageHandler::OnURLsAvailable(
    const std::map<ntp_tiles::SectionType, ntp_tiles::NTPTilesVector>&
        sections) {
  if (!most_visited_sites_)
    return;

  base::Value::Dict result;
  base::Value::List tiles;
  int tile_id = 1;

  // Super Referral feature only present in regular tabs (not private tabs)
  auto* service = ViewCounterServiceFactory::GetForProfile(profile_);
  if (service) {
    for (auto& top_site : service->GetTopSitesData()) {
      base::Value::Dict tile_value;
      if (top_site.name.empty()) {
        tile_value.Set("title", top_site.destination_url);
        tile_value.Set("title_direction", base::i18n::LEFT_TO_RIGHT);
      } else {
        tile_value.Set("title", top_site.name);
        tile_value.Set("title_direction",
                       base::i18n::GetFirstStrongCharacterDirection(
                           base::UTF8ToUTF16(top_site.name)));
      }
      tile_value.Set("id", tile_id++);
      tile_value.Set("url", top_site.destination_url);
      tile_value.Set("favicon", top_site.image_path);
      tile_value.Set("defaultSRTopSite", true);
      tile_value.Set("source",
                     static_cast<int32_t>(ntp_tiles::TileSource::ALLOWLIST));
      tile_value.Set("title_source", static_cast<int32_t>(
                                         ntp_tiles::TileTitleSource::INFERRED));
      tiles.Append(std::move(tile_value));
    }
  }

  for (auto& tile : sections.at(ntp_tiles::SectionType::PERSONALIZED)) {
    base::Value::Dict tile_value;
    if (tile.title.empty()) {
      tile_value.Set("title", tile.url.spec());
      tile_value.Set("title_direction", base::i18n::LEFT_TO_RIGHT);
    } else {
      tile_value.Set("title", base::UTF16ToUTF8(tile.title));
      tile_value.Set("title_direction",
                     base::i18n::GetFirstStrongCharacterDirection(tile.title));
    }
    tile_value.Set("id", tile_id++);
    tile_value.Set("url", tile.url.spec());
    tile_value.Set("favicon", tile.favicon_url.spec());
    tile_value.Set("source", static_cast<int32_t>(tile.source));
    tile_value.Set("title_source", static_cast<int32_t>(tile.title_source));
    tiles.Append(std::move(tile_value));
  }

  result.Set("tiles", std::move(tiles));
  result.Set("custom_links_enabled",
             most_visited_sites_->IsCustomLinksEnabled());
  result.Set("visible", most_visited_sites_->IsShortcutsVisible());
  result.Set("custom_links_num", GetCustomLinksNum());

  // Notify listeners of this update (ex: new tab page)
  if (IsJavascriptAllowed()) {
    FireWebUIListener("most-visited-info-changed", result);
  }
}

void TopSitesMessageHandler::OnIconMadeAvailable(const GURL& site_url) {}

int TopSitesMessageHandler::GetCustomLinksNum() const {
  // Calculate the number of tiles that can be visible in favorites mode.
  int custom_links_num = 0;
  auto most_visited_sites =
      ChromeMostVisitedSitesFactory::NewForProfile(profile_);
  if (most_visited_sites) {
    custom_links_num += most_visited_sites->GetCustomLinkNum();
  }

  // In NTP SR mode, SR tiles are also shown in tiles.
  auto* service = ViewCounterServiceFactory::GetForProfile(profile_);
  if (service) {
    custom_links_num += service->GetTopSitesData().size();
  }

  return custom_links_num;
}

bool TopSitesMessageHandler::IsCustomLinksEnabled() const {
  return !profile_->GetPrefs()->GetBoolean(ntp_prefs::kNtpUseMostVisitedTiles);
}

bool TopSitesMessageHandler::IsShortcutsVisible() const {
  return profile_->GetPrefs()->GetBoolean(ntp_prefs::kNtpShortcutsVisible);
}

void TopSitesMessageHandler::HandleUpdateMostVisitedInfo(
    const base::Value::List& args) {
  if (!most_visited_sites_)
    return;

  AllowJavascript();

  // same as `MostVisitedHandler::UpdateMostVisitedInfo`
  most_visited_sites_->RefreshTiles();
}

void TopSitesMessageHandler::HandleDeleteMostVisitedTile(
    const base::Value::List& args) {
  if (!most_visited_sites_)
    return;

  AllowJavascript();

  if (!args[0].is_string())
    return;

  std::string url = args[0].GetString();
  GURL gurl(url);

  // same as `MostVisitedHandler::DeleteMostVisitedTile`
  if (most_visited_sites_->IsCustomLinksEnabled()) {
    most_visited_sites_->DeleteCustomLink(gurl);
  } else {
    most_visited_sites_->AddOrRemoveBlockedUrl(gurl, true);
    last_blocklisted_ = gurl;
  }
}

void TopSitesMessageHandler::HandleReorderMostVisitedTile(
    const base::Value::List& args) {
  if (!most_visited_sites_)
    return;

  AllowJavascript();

  if (!args[0].is_string() || !args[1].is_int())
    return;

  // same as `MostVisitedHandler::ReorderMostVisitedTile`
  if (most_visited_sites_->IsCustomLinksEnabled()) {
    std::string url = args[0].GetString();
    GURL gurl(url);
    int new_pos = args[1].GetInt();
    most_visited_sites_->ReorderCustomLink(gurl, new_pos);
  }
}

void TopSitesMessageHandler::HandleRestoreMostVisitedDefaults(
    const base::Value::List& args) {
  if (!most_visited_sites_)
    return;

  AllowJavascript();

  // same as `MostVisitedHandler::RestoreMostVisitedDefaults`
  if (most_visited_sites_->IsCustomLinksEnabled()) {
    most_visited_sites_->UninitializeCustomLinks();
  } else {
    most_visited_sites_->ClearBlockedUrls();
  }
}

void TopSitesMessageHandler::HandleUndoMostVisitedTileAction(
    const base::Value::List& args) {
  if (!most_visited_sites_)
    return;

  AllowJavascript();

  // same `MostVisitedHandler::UndoMostVisitedTileAction`
  if (most_visited_sites_->IsCustomLinksEnabled()) {
    most_visited_sites_->UndoCustomLinkAction();
  } else if (last_blocklisted_.is_valid()) {
    most_visited_sites_->AddOrRemoveBlockedUrl(last_blocklisted_, false);
    last_blocklisted_ = GURL();
  }
}

void TopSitesMessageHandler::HandleSetMostVisitedSettings(
    const base::Value::List& args) {
  if (!most_visited_sites_)
    return;

  AllowJavascript();

  if (!args[0].is_bool() || !args[1].is_bool())
    return;

  bool custom_links_enabled = args[0].GetBool();
  bool visible = args[1].GetBool();

  // similar to `NewTabPageHandler::SetMostVisitedSettings`
  bool old_visible = IsShortcutsVisible();
  if (old_visible != visible) {
    profile_->GetPrefs()->SetBoolean(ntp_prefs::kNtpShortcutsVisible, visible);
    most_visited_sites_->SetShortcutsVisible(IsShortcutsVisible());
  }

  bool old_custom_links_enabled = IsCustomLinksEnabled();
  if (old_custom_links_enabled != custom_links_enabled) {
    profile_->GetPrefs()->SetBoolean(ntp_prefs::kNtpUseMostVisitedTiles,
                                     !custom_links_enabled);
    most_visited_sites_->EnableCustomLinks(IsCustomLinksEnabled());
  }
}

void TopSitesMessageHandler::HandleEditTopSite(const base::Value::List& args) {
  if (!most_visited_sites_)
    return;

  AllowJavascript();

  if (!args[0].is_string() || !args[1].is_string() || !args[2].is_string())
    return;

  std::string url = args[0].GetString();
  DCHECK(!url.empty());

  std::string new_url = args[1].GetString();
  std::string title = args[2].GetString();

  // |new_url| can be empty if user only want to change title.
  // Stop editing if we can't make |new_url| valid.
  if (!new_url.empty() && !GetValidURLStringForTopSite(&new_url))
    return;

  if (title.empty())
    title = new_url.empty() ? url : new_url;

  // when user modifies current top sites, change to favorite mode.
  if (!most_visited_sites_->IsCustomLinksEnabled()) {
    profile_->GetPrefs()->SetBoolean(ntp_prefs::kNtpUseMostVisitedTiles, false);
    most_visited_sites_->EnableCustomLinks(IsCustomLinksEnabled());
  }

  GURL gurl(url);
  GURL new_gurl(new_url);
  std::u16string title16 = base::UTF8ToUTF16(title);

  const bool updated =
      most_visited_sites_->UpdateCustomLink(gurl, new_gurl, title16);
  if (!updated) {
    most_visited_sites_->AddCustomLink(new_url.empty() ? gurl : new_gurl,
                                       title16);
  }
}

void TopSitesMessageHandler::HandleAddNewTopSite(
    const base::Value::List& args) {
  if (!most_visited_sites_)
    return;

  AllowJavascript();

  if (!args[0].is_string() || !args[1].is_string())
    return;

  std::string url = args[0].GetString();
  DCHECK(!url.empty());

  std::string title = args[1].GetString();

  // Stop adding if we can't make |url| valid.
  if (!GetValidURLStringForTopSite(&url))
    return;

  // If the user tries to add a new site in top sites mode, change to favorite
  // mode.
  if (!most_visited_sites_->IsCustomLinksEnabled()) {
    profile_->GetPrefs()->SetBoolean(ntp_prefs::kNtpUseMostVisitedTiles, false);
    most_visited_sites_->EnableCustomLinks(IsCustomLinksEnabled());
  }

  most_visited_sites_->AddCustomLink(GURL(url), base::UTF8ToUTF16(title));
}
