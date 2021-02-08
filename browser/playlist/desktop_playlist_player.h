/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_PLAYLIST_DESKTOP_PLAYLIST_PLAYER_H_
#define BRAVE_BROWSER_PLAYLIST_DESKTOP_PLAYLIST_PLAYER_H_

#include <string>

namespace content {
class BrowserContext;
}  // namespace content

namespace playlist {

// Demo purpose player on desktop.
class DesktopPlaylistPlayer {
 public:
  explicit DesktopPlaylistPlayer(content::BrowserContext* context);
  ~DesktopPlaylistPlayer();

  DesktopPlaylistPlayer(const DesktopPlaylistPlayer&) = delete;
  DesktopPlaylistPlayer& operator=(const DesktopPlaylistPlayer&) = delete;

  void Play(const std::string& id);

 private:
  content::BrowserContext* context_;
};

}  // namespace playlist

#endif  // BRAVE_BROWSER_PLAYLIST_DESKTOP_PLAYLIST_PLAYER_H_
