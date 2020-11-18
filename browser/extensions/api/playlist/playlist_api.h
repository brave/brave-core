/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_EXTENSIONS_API_PLAYLIST_PLAYLIST_API_H_
#define BRAVE_BROWSER_EXTENSIONS_API_PLAYLIST_PLAYLIST_API_H_

#include "extensions/browser/extension_function.h"

#include "base/values.h"

namespace extensions {
namespace api {

class BravePlaylistGetAllPlaylistItemsFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("bravePlaylist.getAllPlaylistItems", UNKNOWN)

 protected:
  ~BravePlaylistGetAllPlaylistItemsFunction() override {}

  // ExtensionFunction:
  ResponseAction Run() override;
};

class BravePlaylistGetPlaylistItemFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("bravePlaylist.getPlaylistItem", UNKNOWN)

 protected:
  ~BravePlaylistGetPlaylistItemFunction() override {}

  // ExtensionFunction:
  ResponseAction Run() override;
};

class BravePlaylistRecoverPlaylistItemFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("bravePlaylist.recoverPlaylistItem", UNKNOWN)

 protected:
  ~BravePlaylistRecoverPlaylistItemFunction() override {}

  // ExtensionFunction:
  ResponseAction Run() override;
};

class BravePlaylistDeletePlaylistItemFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("bravePlaylist.deletePlaylistItem", UNKNOWN)

 protected:
  ~BravePlaylistDeletePlaylistItemFunction() override {}

  // ExtensionFunction:
  ResponseAction Run() override;
};

class BravePlaylistDeleteAllPlaylistItemsFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("bravePlaylist.deleteAllPlaylistItems", UNKNOWN)

 protected:
  ~BravePlaylistDeleteAllPlaylistItemsFunction() override {}

  // ExtensionFunction:
  ResponseAction Run() override;
};

// TODO(simonhong): Rename this api to CreatePlaylistItem.
class BravePlaylistRequestDownloadFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("bravePlaylist.requestDownload", UNKNOWN)

 protected:
  ~BravePlaylistRequestDownloadFunction() override {}

  // ExtensionFunction:
  ResponseAction Run() override;
};

class BravePlaylistPlayItemFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("bravePlaylist.playItem", UNKNOWN)

 protected:
  ~BravePlaylistPlayItemFunction() override {}

  // ExtensionFunction:
  ResponseAction Run() override;
};

}  // namespace api
}  // namespace extensions

#endif  // BRAVE_BROWSER_EXTENSIONS_API_PLAYLIST_PLAYLIST_API_H_
