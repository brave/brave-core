// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/new_tab_page/instant_service_message_handler.h"

#include <string>
#include <memory>
#include <utility>

#include "base/i18n/rtl.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/browser/ntp_background_images/view_counter_service_factory.h"
#include "brave/browser/profiles/profile_util.h"
#include "brave/browser/ui/webui/new_tab_page/brave_new_tab_ui.h"
#include "brave/browser/ui/webui/new_tab_page/brave_new_tab_ui_utils.h"
#include "brave/common/pref_names.h"
#include "brave/components/ntp_background_images/browser/ntp_background_images_data.h"
#include "brave/components/ntp_background_images/browser/view_counter_service.h"
#include "chrome/browser/ntp_tiles/chrome_most_visited_sites_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search/instant_service.h"
#include "chrome/browser/search/instant_service_factory.h"
#include "components/ntp_tiles/most_visited_sites.h"

using ntp_background_images::ViewCounterServiceFactory;

// NOTE: InstantService methods used here will eventually be moved to:
// chrome/browser/ui/webui/new_tab_page/new_tab_page_handler.h
//
// For more info, see:
// https://bugs.chromium.org/p/chromium/issues/detail?id=1084363

InstantServiceMessageHandler::InstantServiceMessageHandler(Profile* profile)
        : profile_(profile) {
  instant_service_ = InstantServiceFactory::GetForProfile(profile_);
  instant_service_->AddObserver(this);
}

InstantServiceMessageHandler::~InstantServiceMessageHandler() {
  instant_service_->RemoveObserver(this);
}

void InstantServiceMessageHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback(
    "updateMostVisitedInfo",
    base::BindRepeating(
      &InstantServiceMessageHandler::HandleUpdateMostVisitedInfo,
      base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
    "deleteMostVisitedTile",
    base::BindRepeating(
      &InstantServiceMessageHandler::HandleDeleteMostVisitedTile,
      base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
    "reorderMostVisitedTile",
    base::BindRepeating(
      &InstantServiceMessageHandler::HandleReorderMostVisitedTile,
      base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
    "restoreMostVisitedDefaults",
    base::BindRepeating(
      &InstantServiceMessageHandler::HandleRestoreMostVisitedDefaults,
      base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
    "undoMostVisitedTileAction",
    base::BindRepeating(
      &InstantServiceMessageHandler::HandleUndoMostVisitedTileAction,
      base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
    "setMostVisitedSettings",
    base::BindRepeating(
      &InstantServiceMessageHandler::HandleSetMostVisitedSettings,
      base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "addNewTopSite",
      base::BindRepeating(&InstantServiceMessageHandler::HandleAddNewTopSite,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "editTopSite",
      base::BindRepeating(&InstantServiceMessageHandler::HandleEditTopSite,
                          base::Unretained(this)));
}

// InstantServiceObserver:
void InstantServiceMessageHandler::MostVisitedInfoChanged(
    const InstantMostVisitedInfo& info) {
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
          "source", static_cast<int32_t>(ntp_tiles::TileTitleSource::INFERRED));
      tiles.Append(std::move(tile_value));
    }
  }

  // See chrome/common/search/instant_types.h for more info
  for (auto& tile : info.items) {
    base::Value tile_value(base::Value::Type::DICTIONARY);
    if (tile.title.empty()) {
      tile_value.SetStringKey("title", tile.url.spec());
      tile_value.SetIntKey("title_direction", base::i18n::LEFT_TO_RIGHT);
    } else {
      tile_value.SetStringKey("title", base::UTF16ToUTF8(tile.title));
      tile_value.SetIntKey("title_direction",
          base::i18n::GetFirstStrongCharacterDirection(tile.title));
    }
    tile_value.SetIntKey("id", tile_id++);
    tile_value.SetStringKey("url", tile.url.spec());
    tile_value.SetStringKey("favicon", tile.favicon.spec());
    tile_value.SetIntKey("source", static_cast<int32_t>(tile.title_source));
    tiles.Append(std::move(tile_value));
  }
  result.SetBoolKey("custom_links_enabled", !info.use_most_visited);
  result.SetKey("tiles", std::move(tiles));
  result.SetBoolKey("visible", info.is_visible);
  result.SetIntKey("custom_links_num", GetCustomLinksNum());
  top_site_tiles_ = std::move(result);

  // Notify listeners of this update (ex: new tab page)
  if (IsJavascriptAllowed()) {
    FireWebUIListener("most-visited-info-changed", top_site_tiles_);
  }
}

int InstantServiceMessageHandler::GetCustomLinksNum() const {
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

void InstantServiceMessageHandler::HandleUpdateMostVisitedInfo(
    const base::ListValue* args) {
  AllowJavascript();

  // OnNewTabPageOpened refreshes the most visited entries while
  // UpdateMostVisitedInfo triggers a call to MostVisitedInfoChanged.
  instant_service_->OnNewTabPageOpened();
  instant_service_->UpdateMostVisitedInfo();
}

void InstantServiceMessageHandler::HandleDeleteMostVisitedTile(
    const base::ListValue* args) {
  AllowJavascript();

  std::string url;
  if (!args->GetString(0, &url))
    return;

  GURL gurl(url);
  if (instant_service_->IsCustomLinksEnabled()) {
    instant_service_->DeleteCustomLink(gurl);
  } else {
    instant_service_->DeleteMostVisitedItem(gurl);
    last_blacklisted_ = gurl;
  }
}

void InstantServiceMessageHandler::HandleReorderMostVisitedTile(
    const base::ListValue* args) {
  AllowJavascript();

  std::string url;
  if (!args->GetString(0, &url))
    return;

  int new_pos;
  if (!args->GetInteger(1, &new_pos))
    return;

  GURL gurl(url);
  instant_service_->ReorderCustomLink(gurl, (uint8_t)new_pos);
}

void InstantServiceMessageHandler::HandleRestoreMostVisitedDefaults(
    const base::ListValue* args) {
  AllowJavascript();

  if (instant_service_->IsCustomLinksEnabled()) {
    instant_service_->ResetCustomLinks();
  } else {
    instant_service_->UndoAllMostVisitedDeletions();
  }
}

void InstantServiceMessageHandler::HandleUndoMostVisitedTileAction(
    const base::ListValue* args) {
  AllowJavascript();

  if (instant_service_->IsCustomLinksEnabled()) {
    instant_service_->UndoCustomLinkAction();
  } else if (last_blacklisted_.is_valid()) {
    instant_service_->UndoMostVisitedDeletion(last_blacklisted_);
    last_blacklisted_ = GURL();
  }
}

void InstantServiceMessageHandler::HandleSetMostVisitedSettings(
    const base::ListValue* args) {
  AllowJavascript();

  bool custom_links_enabled;
  if (!args->GetBoolean(0, &custom_links_enabled))
    return;

  bool visible;
  if (!args->GetBoolean(1, &visible))
    return;

  auto pair = instant_service_->GetCurrentShortcutSettings();
  // The first of the pair is true if most-visited tiles are being used.
  bool old_custom_links_enabled = !pair.first;
  bool old_visible = pair.second;
  // |ToggleMostVisitedOrCustomLinks()| always notifies observers. Since we only
  // want to notify once, we need to call |ToggleShortcutsVisibility()| with
  // false if we are also going to call |ToggleMostVisitedOrCustomLinks()|.
  bool toggleCustomLinksEnabled =
      old_custom_links_enabled != custom_links_enabled;
  if (old_visible != visible) {
    instant_service_->ToggleShortcutsVisibility(
        /* do_notify= */ !toggleCustomLinksEnabled);
  }
  if (toggleCustomLinksEnabled) {
    instant_service_->ToggleMostVisitedOrCustomLinks();
  }
}

void InstantServiceMessageHandler::HandleEditTopSite(
    const base::ListValue* args) {
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

  // when user modifies current top sites, change to favorite mode.
  auto pair = instant_service_->GetCurrentShortcutSettings();
  const bool custom_links_enabled = !pair.first;
  if (!custom_links_enabled) {
    instant_service_->ToggleMostVisitedOrCustomLinks();

    // When user tries to edit from frecency mode, we just try to add modified
    // item to favorites. If modified url is already existed in favorites,
    // nothing happened.
    instant_service_->AddCustomLink(new_url.empty() ? GURL(url) : GURL(new_url),
                                    title);
  } else {
    instant_service_->UpdateCustomLink(GURL(url), GURL(new_url), title);
  }
}

void InstantServiceMessageHandler::HandleAddNewTopSite(
    const base::ListValue* args) {
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

  // when user adds new top sites, change to favorite mode.
  auto pair = instant_service_->GetCurrentShortcutSettings();
  const bool custom_links_enabled = !pair.first;
  if (!custom_links_enabled)
    instant_service_->ToggleMostVisitedOrCustomLinks();

  instant_service_->AddCustomLink(GURL(url), title);
}
