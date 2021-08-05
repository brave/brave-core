// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_NEW_TAB_PAGE_TOP_SITES_MESSAGE_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_NEW_TAB_PAGE_TOP_SITES_MESSAGE_HANDLER_H_

#include <map>
#include <memory>

#include "base/values.h"
#include "components/ntp_tiles/most_visited_sites.h"
#include "components/ntp_tiles/ntp_tile.h"
#include "content/public/browser/web_ui_message_handler.h"

class Profile;
namespace content {
class WebUIDataSource;
}

// Handles "top sites" related messages for new tab page
//
// This was handled by InstantService before - but that was removed:
// https://bugs.chromium.org/p/chromium/issues/detail?id=1084363
//
// Borrows some logic from:
// `chrome\browser\ui\webui\cr_components\most_visited\most_visited_handler.cc`
// `chrome\browser\ui\webui\new_tab_page\new_tab_page_handler.cc`
class TopSitesMessageHandler : public content::WebUIMessageHandler,
                               public ntp_tiles::MostVisitedSites::Observer {
 public:
  TopSitesMessageHandler(const TopSitesMessageHandler&) = delete;
  TopSitesMessageHandler& operator=(const TopSitesMessageHandler&) = delete;
  explicit TopSitesMessageHandler(Profile* profile);
  ~TopSitesMessageHandler() override;

 private:
  // WebUIMessageHandler:
  void RegisterMessages() override;

  // ntp_tiles::MostVisitedSites::Observer:
  void OnURLsAvailable(
      const std::map<ntp_tiles::SectionType, ntp_tiles::NTPTilesVector>&
          sections) override;
  void OnIconMadeAvailable(const GURL& site_url) override;

  int GetCustomLinksNum() const;
  bool IsCustomLinksEnabled() const;
  bool IsShortcutsVisible() const;

  // handlers
  void HandleUpdateMostVisitedInfo(const base::ListValue* args);
  void HandleDeleteMostVisitedTile(const base::ListValue* args);
  void HandleReorderMostVisitedTile(const base::ListValue* args);
  void HandleRestoreMostVisitedDefaults(const base::ListValue* args);
  void HandleUndoMostVisitedTileAction(const base::ListValue* args);
  void HandleSetMostVisitedSettings(const base::ListValue* args);
  void HandleEditTopSite(const base::ListValue* args);
  void HandleAddNewTopSite(const base::ListValue* args);

  Profile* profile_;
  std::unique_ptr<ntp_tiles::MostVisitedSites> most_visited_sites_;
  GURL last_blocklisted_;
  base::Value top_site_tiles_;
};

#endif  // BRAVE_BROWSER_UI_WEBUI_NEW_TAB_PAGE_TOP_SITES_MESSAGE_HANDLER_H_
