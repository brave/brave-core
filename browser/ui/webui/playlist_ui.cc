/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/playlist_ui.h"

#include "brave/browser/playlist/playlist_service_factory.h"
#include "brave/browser/ui/webui/brave_webui_source.h"
#include "brave/common/webui_url_constants.h"
#include "brave/components/playlist/resources/grit/playlist_generated_map.h"
#include "components/grit/brave_components_resources.h"
#include "url/gurl.h"

namespace playlist {

// static
bool PlaylistUI::ShouldBlockPlaylistWebUI(
    content::BrowserContext* browser_context,
    const GURL& url) {
  if (url.host_piece() != kPlaylistHost)
    return false;

  return !PlaylistServiceFactory::IsPlaylistEnabled(browser_context);
}

PlaylistUI::PlaylistUI(content::WebUI* web_ui, const std::string& name)
    : WebUIController(web_ui) {
  CreateAndAddWebUIDataSource(web_ui, name, kPlaylistGenerated,
                              kPlaylistGeneratedSize,
                              IDR_PLAYLIST_HTML);
}

PlaylistUI::~PlaylistUI() = default;

}  // namespace playlist
