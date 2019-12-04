/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_EXTENSIONS_API_BRAVE_PLAYLISTS_API_H_
#define BRAVE_BROWSER_EXTENSIONS_API_BRAVE_PLAYLISTS_API_H_

#include "extensions/browser/extension_function.h"

#include "base/values.h"

namespace extensions {
namespace api {

class BravePlaylistsCreatePlaylistFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("bravePlaylists.createPlaylist", UNKNOWN)

 protected:
  ~BravePlaylistsCreatePlaylistFunction() override {}

  ResponseAction Run() override;
};

class BravePlaylistsIsInitializedFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("bravePlaylists.isInitialized", UNKNOWN)

 protected:
  ~BravePlaylistsIsInitializedFunction() override {}

  // ExtensionFunction:
  ResponseAction Run() override;
};

class BravePlaylistsInitializeFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("bravePlaylists.initialize", UNKNOWN)

 protected:
  ~BravePlaylistsInitializeFunction() override {}

  // ExtensionFunction:
  ResponseAction Run() override;
};

class BravePlaylistsGetAllPlaylistsFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("bravePlaylists.getAllPlaylists", UNKNOWN)

 protected:
  ~BravePlaylistsGetAllPlaylistsFunction() override {}

  // ExtensionFunction:
  ResponseAction Run() override;

 private:
  void OnGetAllPlaylists(base::Value playlists);
};

class BravePlaylistsGetPlaylistFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("bravePlaylists.getPlaylist", UNKNOWN)

 protected:
  ~BravePlaylistsGetPlaylistFunction() override {}

  // ExtensionFunction:
  ResponseAction Run() override;

 private:
  void OnGetPlaylist(base::Value playlist);
};

class BravePlaylistsRecoverPlaylistFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("bravePlaylists.recoverPlaylist", UNKNOWN)

 protected:
  ~BravePlaylistsRecoverPlaylistFunction() override {}

  // ExtensionFunction:
  ResponseAction Run() override;
};

class BravePlaylistsDeletePlaylistFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("bravePlaylists.deletePlaylist", UNKNOWN)

 protected:
  ~BravePlaylistsDeletePlaylistFunction() override {}

  // ExtensionFunction:
  ResponseAction Run() override;
};

class BravePlaylistsDeleteAllPlaylistsFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("bravePlaylists.deleteAllPlaylists", UNKNOWN)

 protected:
  ~BravePlaylistsDeleteAllPlaylistsFunction() override {}

  // ExtensionFunction:
  ResponseAction Run() override;

 private:
  void OnDeleteAllPlaylists(bool deleted);
};

class BravePlaylistsRequestDownloadFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("bravePlaylists.requestDownload", UNKNOWN)

 protected:
  ~BravePlaylistsRequestDownloadFunction() override {}

  // ExtensionFunction:
  ResponseAction Run() override;
};

class BravePlaylistsPlayFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("bravePlaylists.play", UNKNOWN)

 protected:
  ~BravePlaylistsPlayFunction() override {}

  // ExtensionFunction:
  ResponseAction Run() override;
};

}  // namespace api
}  // namespace extensions

#endif  // BRAVE_BROWSER_EXTENSIONS_API_BRAVE_PLAYLISTS_API_H_
