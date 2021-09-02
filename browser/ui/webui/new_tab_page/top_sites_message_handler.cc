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
#include "brave/browser/ntp_background_images/view_counter_service_factory.h"
#include "brave/browser/profiles/profile_util.h"
#include "brave/browser/ui/webui/new_tab_page/brave_new_tab_ui.h"
#include "brave/browser/ui/webui/new_tab_page/brave_new_tab_ui_utils.h"
#include "brave/common/pref_names.h"
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

  base::Value result(base::Value::Type::DICTIONARY);
  base::Value tiles(base::Value::Type::LIST);
  int tile_id = 1;

  // Super Referral feature only present in regular tabs (not private tabs)
  auto* service = ViewCounterServiceFactory::GetForProfile(profile_);
  if (service) {
    for (auto& top_site : service->GetTopSitesVectorForWebUI()) {
      base::Value tile_value(base::Value::Type::DICTIONARY);
      if (top_site.name.empty()) {
        tile_value.SetStringKey("title", top_site.destination_url);
        tile_value.SetIntKey("title_direction", base::i18n::LEFT_TO_RIGHT);
      } else {
        tile_value.SetStringKey("title", top_site.name);
        tile_value.SetIntKey("title_direction",
                             base::i18n::GetFirstStrongCharacterDirection(
                                 base::UTF8ToUTF16(top_site.name)));
      }
      tile_value.SetIntKey("id", tile_id++);
      tile_value.SetStringKey("url", top_site.destination_url);
      tile_value.SetStringKey("favicon", top_site.image_path);
      tile_value.SetBoolKey("defaultSRTopSite", true);
      tile_value.SetIntKey(
          "source", static_cast<int32_t>(ntp_tiles::TileSource::ALLOWLIST));
      tile_value.SetIntKey(
          "title_source",
          static_cast<int32_t>(ntp_tiles::TileTitleSource::INFERRED));
      tiles.Append(std::move(tile_value));
    }
  }

  for (auto& tile : sections.at(ntp_tiles::SectionType::PERSONALIZED)) {
    base::Value tile_value(base::Value::Type::DICTIONARY);
    if (tile.title.empty()) {
      tile_value.SetStringKey("title", tile.url.spec());
      tile_value.SetIntKey("title_direction", base::i18n::LEFT_TO_RIGHT);
    } else {
      tile_value.SetStringKey("title", base::UTF16ToUTF8(tile.title));
      tile_value.SetIntKey(
          "title_direction",
          base::i18n::GetFirstStrongCharacterDirection(tile.title));
    }
    tile_value.SetIntKey("id", tile_id++);
    tile_value.SetStringKey("url", tile.url.spec());
    tile_value.SetStringKey("favicon", tile.favicon_url.spec());
    tile_value.SetIntKey("source", static_cast<int32_t>(tile.source));
    tile_value.SetIntKey("title_source",
                         static_cast<int32_t>(tile.title_source));
    tiles.Append(std::move(tile_value));
  }

  result.SetKey("tiles", std::move(tiles));
  result.SetBoolKey("custom_links_enabled",
                    most_visited_sites_->IsCustomLinksEnabled());
  result.SetBoolKey("visible", most_visited_sites_->IsShortcutsVisible());
  result.SetIntKey("custom_links_num", GetCustomLinksNum());
  top_site_tiles_ = std::move(result);

  // Notify listeners of this update (ex: new tab page)
  if (IsJavascriptAllowed()) {
    FireWebUIListener("most-visited-info-changed", top_site_tiles_);
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
    custom_links_num += service->GetTopSitesVectorForWebUI().size();
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
    const base::ListValue* args) {
  if (!most_visited_sites_)
    return;

  AllowJavascript();

  // same as `MostVisitedHandler::UpdateMostVisitedInfo`
  most_visited_sites_->RefreshTiles();
}

void TopSitesMessageHandler::HandleDeleteMostVisitedTile(
    const base::ListValue* args) {
  if (!most_visited_sites_)
    return;

  AllowJavascript();

  std::string url;
  if (!args->GetString(0, &url))
    return;

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
    const base::ListValue* args) {
  if (!most_visited_sites_)
    return;

  AllowJavascript();

  std::string url;
  if (!args->GetString(0, &url))
    return;

  int new_pos;
  if (!args->GetInteger(1, &new_pos))
    return;

  GURL gurl(url);

  // same as `MostVisitedHandler::ReorderMostVisitedTile`
  if (most_visited_sites_->IsCustomLinksEnabled()) {
    most_visited_sites_->ReorderCustomLink(gurl, new_pos);
  }
}

void TopSitesMessageHandler::HandleRestoreMostVisitedDefaults(
    const base::ListValue* args) {
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
    const base::ListValue* args) {
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
    const base::ListValue* args) {
  if (!most_visited_sites_)
    return;

  AllowJavascript();

  bool custom_links_enabled;
  if (!args->GetBoolean(0, &custom_links_enabled))
    return;

  bool visible;
  if (!args->GetBoolean(1, &visible))
    return;

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

void TopSitesMessageHandler::HandleEditTopSite(const base::ListValue* args) {
  if (!most_visited_sites_)
    return;

  AllowJavascript();

  std::string url;
  if (!args->GetString(0, &url))
    return;
  DCHECK(!url.empty());

  std::string new_url;
  if (!args->GetString(1, &new_url))
    return;

  std::string title;
  if (!args->GetString(2, &title))
    return;

  // |new_url| can be empty if user only want to change title.
  // Stop editing if we can't make |new_url| valid.
  if (!new_url.empty() && !GetValidURLStringForTopSite(&new_url))
    return;

  if (title.empty())
    title = new_url.empty() ? url : new_url;

  GURL gurl(url);
  GURL new_gurl(new_url);

  if (most_visited_sites_->IsCustomLinksEnabled()) {
    // similar to `MostVisitedHandler::UpdateMostVisitedTile`
    most_visited_sites_->UpdateCustomLink(
        gurl, new_gurl != gurl ? new_gurl : GURL(), base::UTF8ToUTF16(title));
  } else {
    // when user modifies current top sites, change to favorite mode.
    profile_->GetPrefs()->SetBoolean(ntp_prefs::kNtpUseMostVisitedTiles, false);
    most_visited_sites_->EnableCustomLinks(IsCustomLinksEnabled());

    // When user tries to edit from frecency mode, we just try to add modified
    // item to favorites. If modified url is already existed in favorites,
    // nothing happened.
    most_visited_sites_->AddCustomLink(
        new_url.empty() ? GURL(url) : GURL(new_url), base::UTF8ToUTF16(title));
  }
}

void TopSitesMessageHandler::HandleAddNewTopSite(const base::ListValue* args) {
  if (!most_visited_sites_)
    return;

  AllowJavascript();

  std::string url;
  if (!args->GetString(0, &url))
    return;
  DCHECK(!url.empty());

  std::string title;
  if (!args->GetString(1, &title))
    return;

  // Stop adding if we can't make |url| valid.
  if (!GetValidURLStringForTopSite(&url))
    return;

  // similar to `MostVisitedHandler::AddMostVisitedTile`
  if (most_visited_sites_->IsCustomLinksEnabled()) {
    most_visited_sites_->AddCustomLink(GURL(url), base::UTF8ToUTF16(title));
  }
}
