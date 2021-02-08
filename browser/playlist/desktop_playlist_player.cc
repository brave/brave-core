/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/playlist/desktop_playlist_player.h"

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "brave/browser/playlist/playlist_service_factory.h"
#include "brave/components/playlist/playlist_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/scoped_tabbed_browser_displayer.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/page_navigator.h"
#include "net/base/filename_util.h"

namespace playlist {

DesktopPlaylistPlayer::DesktopPlaylistPlayer(content::BrowserContext* context)
    : context_(context) {}

DesktopPlaylistPlayer::~DesktopPlaylistPlayer() {}

void DesktopPlaylistPlayer::Play(const std::string& id) {
  auto* service =
      PlaylistServiceFactory::GetInstance()->GetForBrowserContext(context_);
  if (!service)
    return;

  auto html_file_path = service->GetPlaylistItemDirPath(id).Append(
      FILE_PATH_LITERAL("index.html"));
  GURL html_file_url = net::FilePathToFileURL(html_file_path);
  chrome::ScopedTabbedBrowserDisplayer browser_displayer(
      Profile::FromBrowserContext(context_));
  content::OpenURLParams open_url_params(
      html_file_url, content::Referrer(),
      WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui::PAGE_TRANSITION_AUTO_TOPLEVEL, false);
  browser_displayer.browser()->OpenURL(open_url_params);
}

}  // namespace playlist
