/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_EXTENSIONS_API_PLAYLIST_PLAYLIST_API_H_
#define BRAVE_BROWSER_EXTENSIONS_API_PLAYLIST_PLAYLIST_API_H_

#include "extensions/browser/extension_function.h"

#include "base/values.h"

namespace extensions {
namespace api {

class PlaylistGetAllPlaylistItemsFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("playlist.getAllPlaylistItems", UNKNOWN)

 protected:
  ~PlaylistGetAllPlaylistItemsFunction() override {}

  // ExtensionFunction:
  ResponseAction Run() override;
};

class PlaylistGetPlaylistItemFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("playlist.getPlaylistItem", UNKNOWN)

 protected:
  ~PlaylistGetPlaylistItemFunction() override {}

  // ExtensionFunction:
  ResponseAction Run() override;
};

class PlaylistRecoverPlaylistItemFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("playlist.recoverPlaylistItem", UNKNOWN)

 protected:
  ~PlaylistRecoverPlaylistItemFunction() override {}

  // ExtensionFunction:
  ResponseAction Run() override;
};

class PlaylistDeletePlaylistItemFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("playlist.deletePlaylistItem", UNKNOWN)

 protected:
  ~PlaylistDeletePlaylistItemFunction() override {}

  // ExtensionFunction:
  ResponseAction Run() override;
};

class PlaylistDeleteAllPlaylistItemsFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("playlist.deleteAllPlaylistItems", UNKNOWN)

 protected:
  ~PlaylistDeleteAllPlaylistItemsFunction() override {}

  // ExtensionFunction:
  ResponseAction Run() override;
};

// TODO(simonhong): Rename this api to CreatePlaylistItem.
class PlaylistRequestDownloadFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("playlist.requestDownload", UNKNOWN)

 protected:
  ~PlaylistRequestDownloadFunction() override {}

  // ExtensionFunction:
  ResponseAction Run() override;
};

class PlaylistPlayItemFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("playlist.playItem", UNKNOWN)

 protected:
  ~PlaylistPlayItemFunction() override {}

  // ExtensionFunction:
  ResponseAction Run() override;
};

}  // namespace api
}  // namespace extensions

#endif  // BRAVE_BROWSER_EXTENSIONS_API_PLAYLIST_PLAYLIST_API_H_
