/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_playlists_ui.h"

#include "brave/browser/brave_browser_process_impl.h"
#include "brave/browser/ui/webui/brave_playlists_source.h"
#include "brave/common/pref_names.h"
#include "brave/common/webui_url_constants.h"
#include "brave/components/playlists/resources/grit/playlists_generated_map.h"
#include "chrome/browser/profiles/profile.h"
#include "components/grit/brave_components_resources.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/url_data_source.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/browser/web_ui_message_handler.h"

BravePlaylistsUI::BravePlaylistsUI(content::WebUI* web_ui,
                                   const std::string& name)
    : BasicUI(web_ui,
              name,
              kPlaylistsGenerated,
              kPlaylistsGeneratedSize,
              IDR_BRAVE_PLAYLISTS_HTML) {
  Profile* profile = Profile::FromWebUI(web_ui);
  // Set up the playlists URL data source for thumbnail images
  content::URLDataSource::Add(profile,
                              std::make_unique<BravePlaylistsSource>(profile));
}

BravePlaylistsUI::~BravePlaylistsUI() {}
