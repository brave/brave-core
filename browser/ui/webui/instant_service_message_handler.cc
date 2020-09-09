// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/instant_service_message_handler.h"

#include <string>
#include <memory>
#include <utility>

#include "base/i18n/rtl.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/browser/profiles/profile_util.h"
#include "brave/browser/ui/webui/brave_new_tab_ui.h"
#include "brave/browser/ntp_background_images/view_counter_service_factory.h"
#include "brave/common/pref_names.h"
#include "brave/components/ntp_background_images/browser/features.h"
#include "brave/components/ntp_background_images/browser/ntp_background_images_data.h"
#include "brave/components/ntp_background_images/browser/view_counter_service.h"
#include "brave/components/ntp_background_images/common/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search/instant_service.h"
#include "content/public/browser/web_ui_data_source.h"

using ntp_background_images::features::kBraveNTPBrandedWallpaper;
using ntp_background_images::prefs::kNewTabPageShowBackgroundImage;
using ntp_background_images::prefs::kNewTabPageShowSponsoredImagesBackgroundImage;  // NOLINT
using ntp_background_images::prefs::kBrandedWallpaperNotificationDismissed;
using ntp_background_images::ViewCounterServiceFactory;

namespace {

bool ShouldExcludeFromTiles(const GURL& url) {
  return url.spec().find("https://chrome.google.com/webstore") == 0;
}

}  // namespace

// static
InstantServiceMessageHandler* InstantServiceMessageHandler::Create(
      content::WebUIDataSource* source, Profile* profile,
      InstantService* instant_service) {
  //
  // Initial Values
  // Should only contain data that is static
  //
  return new InstantServiceMessageHandler(profile, instant_service);
}

InstantServiceMessageHandler::InstantServiceMessageHandler(Profile* profile,
    InstantService* instant_service)
        : profile_(profile),
          instant_service_(instant_service) {
  instant_service_->AddObserver(this);
}

InstantServiceMessageHandler::~InstantServiceMessageHandler() {
  instant_service_->RemoveObserver(this);
}

void InstantServiceMessageHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback(
    "getMostVisitedInfo",
    base::BindRepeating(
      &InstantServiceMessageHandler::HandleGetMostVisitedInfo,
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
}

// InstantServiceObserver:
void InstantServiceMessageHandler::MostVisitedInfoChanged(
    const InstantMostVisitedInfo& info) {
  base::Value result(base::Value::Type::DICTIONARY);
  base::Value tiles(base::Value::Type::LIST);
  int tile_id = 1;

  auto* service = ViewCounterServiceFactory::GetForProfile(profile_);
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
    tile_value.SetIntKey(
        "source", static_cast<int32_t>(ntp_tiles::TileTitleSource::INFERRED));
    tiles.Append(std::move(tile_value));
  }

  // See chrome/common/search/instant_types.h for more info
  for (auto& tile : info.items) {
    if (ShouldExcludeFromTiles(tile.url))
      continue;

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
  top_site_tiles_ = std::move(result);

  // Notify listeners of this update (ex: new tab page)
  if (IsJavascriptAllowed()) {
    FireWebUIListener("most-visited-info-changed", top_site_tiles_);
  }
}

void InstantServiceMessageHandler::HandleGetMostVisitedInfo(
    const base::ListValue* args) {
  AllowJavascript();

  ResolveJavascriptCallback(
      args->GetList()[0],
      top_site_tiles_);
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
